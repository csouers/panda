#define OP_STATE 0x800
#define OTA_IN 0x1

static int elm327_tx_hook(CANPacket_t *to_send) {

  int tx = 1;
  int addr = GET_ADDR(to_send);
  int len = GET_LEN(to_send);

  bool little_msg = (addr == 0x0ef81218);
  if (little_msg) {
    if ((len != 2) && (addr == 0x0ef81218)){ // 2017 honda civic hatch keyfob replay
      tx = 0;
    }
  }
  //All ISO 15765-4, OTA, BCM IO Diagnostic, and OP system state messages must be 8 bytes long
  else {
    if (len != 8) {
      tx = 0;
    }

    //Check valid 29 bit send addresses for ISO 15765-4
    //Check valid 11 bit send addresses for ISO 15765-4
    if ((addr != 0x18DB33F1) && ((addr & 0x1FFF00FF) != 0x18DA00F1) &&
        ((addr & 0x1FFFFF00) != 0x700)) {
      tx = 0;
    }
  }
  return tx;
}

static int elm327_tx_lin_hook(int lin_num, uint8_t *data, int len) {
  int tx = 1;
  if (lin_num != 0) {
    tx = 0;  //Only operate on LIN 0, aka serial 2
  }
  if ((len < 5) || (len > 11)) {
    tx = 0;  //Valid KWP size
  }
  if (!(((data[0] & 0xF8U) == 0xC0U) && ((data[0] & 0x07U) != 0U) &&
        (data[1] == 0x33U) && (data[2] == 0xF1U))) {
    tx = 0;  //Bad msg
  }
  return tx;
}

// If current_board->has_obd and safety_param == 0, bus 1 is multiplexed to the OBD-II port
const safety_hooks elm327_hooks = {
  .init = nooutput_init,
  .rx = default_rx_hook,
  .tx = elm327_tx_hook,
  .tx_lin = elm327_tx_lin_hook,
  .fwd = default_fwd_hook,
};
