bool powerActive = false;
bool powerActive_last = false;
bool lfDoorUnlocked = false;
bool lfDoorUnlocked_last = false;
bool lfDoorLocked = false;
bool lfDoorLocked_last = false;

uint32_t last_can1 = false;

void can_send(CAN_FIFOMailBox_TypeDef *to_push, uint8_t bus_number, bool skip_tx_hook);

void stop_test(CAN_FIFOMailBox_TypeDef *to_push){
  uint8_t bus_num = 1;
  uint32_t msg_addr = 0x16F118F0;
  uint8_t msg_len = 8U;

  CAN_FIFOMailBox_TypeDef to_go;
  // move the id 3 bits left and then add binary 101 for extended=true, rtr=false, txrequest=true
  to_go.RIR = ((msg_addr << 3) | 0x5);
  to_go.RDTR = (to_push->RDTR & 0xFFFFFFF0) | msg_len;
  to_go.RDLR = 0x00000020;
  to_go.RDHR = 0x0;

  can_send(&to_go, bus_num, true);
}

void stop_test(CAN_FIFOMailBox_TypeDef *to_push);

static int honda_body_rx_hook(CAN_FIFOMailBox_TypeDef *to_push) {
  int addr = GET_ADDR(to_push);
  int bus = GET_BUS(to_push);

  if (bus == 1) {
    // power + lighting status
    if (addr == 0x12F81018){
      powerActive = ((GET_BYTE(to_push, 0) & 0x20) || (GET_BYTE(to_push, 0) & 0x08));

      if (powerActive && !powerActive_last){
        // send the stop test frame if power is now on
        stop_test(to_push);
      }
    }

    // door lock status
    if (addr == 0x12F83130) {
      lfDoorUnlocked = GET_BYTE(to_push, 0) & 0x08;
      lfDoorLocked = GET_BYTE(to_push, 0) & 0x04;

      if (lfDoorUnlocked && !powerActive && (lfDoorUnlocked != lfDoorUnlocked_last)){
        // send the frame for DRL High
        // parking lights+drl low RDLR is 000F2530
        uint8_t bus_num = 1;
        uint32_t msg_addr = 0x16F118F0;
        uint8_t msg_len = 8U;

        CAN_FIFOMailBox_TypeDef to_go;
        // move the id 3 bits left and then add binary 101 for extended=true, rtr=false, txrequest=true
        to_go.RIR = ((msg_addr << 3) | 0x5);
        to_go.RDTR = (to_push->RDTR & 0xFFFFFFF0) | msg_len;
        to_go.RDLR = 0x000F2530;
        to_go.RDHR = 0x0;

        can_send(&to_go, bus_num, true);
      }
      if (lfDoorLocked && !lfDoorLocked_last){
        // send the stop frame if the door is new locked
        stop_test(to_push);
      }
      // be ready for next time around
      lfDoorUnlocked_last = lfDoorUnlocked;
      lfDoorLocked_last = lfDoorLocked;
      powerActive_last = powerActive;
    }
  }
  return 1;
}

static int honda_body_tx_hook(CAN_FIFOMailBox_TypeDef *to_send) {
  UNUSED(to_send);
  return true;
  //
  // int tx = 1;
  //
  // // 1 allows the message through
  // return tx;
}

static void honda_body_init(int16_t param) {
  UNUSED(param);
  controls_allowed = false;
}

static int honda_body_fwd_hook(int bus_num, CAN_FIFOMailBox_TypeDef *to_fwd) {
  int bus_fwd = -1;
  int bus_powertrain = 0;  // powertrain bus (eps, adas control, etc)
  int bus_body = 1;  // body bus

  if (bus_num == bus_powertrain) {
      int addr = GET_ADDR(to_fwd);
      // TODO add safety checks for each message type
      int security_msg = (addr == 0x0ef81218);
      int ioc_msg = (addr == 0x16f118f0);

      if (security_msg || ioc_msg) {
        bus_fwd = bus_body;
      }
  }

  if (bus_num == bus_body) {
    bus_fwd = bus_powertrain;
  }

  return bus_fwd;
}


const safety_hooks honda_body_hooks = {
  .init = honda_body_init,
  .rx = honda_body_rx_hook,
  .tx = honda_body_tx_hook,
  .tx_lin = nooutput_tx_lin_hook,
  .fwd = honda_body_fwd_hook,
};
