void can_send(CAN_FIFOMailBox_TypeDef *to_push, uint8_t bus_number, bool skip_tx_hook);

void stop_test(CAN_FIFOMailBox_TypeDef *to_push){
  uint8_t bus_num = 1;
  uint32_t msg_addr = 0x16F118F0;
  uint8_t msg_len = 8U;

  CAN_FIFOMailBox_TypeDef to_go;
  // move the id 3 bits left and then add binary 101 for extended=true, rtr=false, txrequest=true
  to_go.RIR = (msg_addr << 3) | 5U;
  to_go.RDTR = (to_push->RDTR & 0xFFFFFFF0) | msg_len;
  to_go.RDLR = 0x00000020;
  to_go.RDHR = 0x0;

  can_send(&to_go, bus_num, true);
}

void stop_test(CAN_FIFOMailBox_TypeDef *to_push);

static int honda_body_rx_hook(CAN_FIFOMailBox_TypeDef *to_push) {
  UNUSED(to_push);
  return true;
}

static int honda_body_tx_hook(CAN_FIFOMailBox_TypeDef *to_send) {
  UNUSED(to_send);
  return true;
}

static void honda_body_init(int16_t param) {
  UNUSED(param);
  controls_allowed = false;
}

static int honda_body_fwd_hook(int bus_num, CAN_FIFOMailBox_TypeDef *to_fwd) {
  int bus_fwd = -1;

  // bus 0: powertrain bus (eps, adas control, etc)
  // bus 1: body bus
  // bus 2: obd2 bus
  int bus_powertrain = 0;
  int bus_body = 1;

  if (bus_num == bus_powertrain) {
      int addr = GET_ADDR(to_fwd);
      // TODO: add byte level safety checks for each message type
      int security_msg = (addr == 0xef81218);
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
