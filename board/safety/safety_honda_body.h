bool powerActive = false;
bool powerActive_last = false;
bool lfDoorUnlocked = false;
bool lfDoorUnlocked_last = false;

static int honda_body_rx_hook(CAN_FIFOMailBox_TypeDef *to_push) {
  int addr = GET_ADDR(to_push);

  // power + lighting status
  if (addr == 0x12F81018){
    powerActive = ((GET_BYTE(to_push, 0) & 0x20) || (GET_BYTE(to_push, 0) & 0x08));

    if (powerActive && !powerActive_last){
      // send the stop test frame if power is now on
      uint8_t bus_num = 0U;
      uint32_t msg_addr = 0x16F118F0;
      uint8_t msg_len = 8U;

      CAN_FIFOMailBox_TypeDef to_go;
      uint32_t addr_mask = 0x001FFFFF;
      to_go.RIR = (msg_addr << 21) + (addr_mask & (to_push->RIR | 1));
      to_go.RDTR = (to_push->RDTR & 0xFFFFFFF0) | msg_len;
      to_go.RDLR = 0x00000020;
      to_go.RDHR = 0x0;

      void can_send(CAN_FIFOMailBox_TypeDef *to_push, uint8_t bus_number, bool skip_tx_hook);
      can_send(&to_go, bus_num, true);
    }
  }

  // door lock status
  if (addr == 0x12F83130){
    lfDoorUnlocked = GET_BYTE(to_push, 0) & 0x08;
    // if the door is unlocked now and it wasn't before
    if (lfDoorUnlocked && !powerActive && (lfDoorUnlocked != lfDoorUnlocked_last)){
      // send the frame for DRL High
      // parking lights+drl low RDLR is 000F2530
      uint8_t bus_num = 0U;
      uint32_t msg_addr = 0x16F118F0;
      uint8_t msg_len = 8U;

      CAN_FIFOMailBox_TypeDef to_go;
      uint32_t addr_mask = 0x001FFFFF;
      to_go.RIR = (msg_addr << 21) + (addr_mask & (to_push->RIR | 1));
      to_go.RDTR = (to_push->RDTR & 0xFFFFFFF0) | msg_len;
      to_go.RDLR = 0x000F3630;
      to_go.RDHR = 0x0;

      void can_send(CAN_FIFOMailBox_TypeDef *to_push, uint8_t bus_number, bool skip_tx_hook);
      can_send(&to_go, bus_num, true);
    }
  }
  // be ready for next time around
  lfDoorUnlocked_last = lfDoorUnlocked;
  powerActive_last = powerActive;


  // build the package


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


// static int honda_body_fwd_hook(int bus_num, CAN_FIFOMailBox_TypeDef *to_fwd) {
//   int bus_fwd = -1;
//   // forward bus 1 to bus 0
//   return bus_fwd;
// }


const safety_hooks honda_body_hooks = {
  .init = honda_body_init,
  .rx = honda_body_rx_hook,
  .tx = honda_body_tx_hook,
  .tx_lin = nooutput_tx_lin_hook,
  .fwd = default_fwd_hook,
};
