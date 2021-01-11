void can_send(CAN_FIFOMailBox_TypeDef *to_push, uint8_t bus_number, bool skip_tx_hook);

static int honda_body_test_rx_hook(CAN_FIFOMailBox_TypeDef *to_push) {
  int bus = GET_BUS(to_push);
  int addr = GET_ADDR(to_push);

  // verify that we can receive on all busses
  if ((addr == 0x05) && (bus == 0)){
    current_board->set_led(LED_GREEN, 1);
  }
  if ((addr == 0x06) && (bus == 1)){
    current_board->set_led(LED_BLUE, 1);
  }
  if ((addr == 0x07) && (bus == 2)){
    current_board->set_led(LED_RED, 1);
  }

  if (addr == 0x10){
    current_board->set_led(LED_RED, 0);
    current_board->set_led(LED_GREEN, 0);
    current_board->set_led(LED_BLUE, 0);
  }
  return true;
}

static int honda_body_test_tx_hook(CAN_FIFOMailBox_TypeDef *to_send) {
  UNUSED(to_send);
  return true;
}

static void honda_body_test_init(int16_t param) {
  UNUSED(param);
  controls_allowed = false;
}

static int honda_body_test_fwd_hook(int bus_num, CAN_FIFOMailBox_TypeDef *to_fwd) {
  UNUSED(bus_num);
  UNUSED(to_fwd);
  return -1;
}


const safety_hooks honda_body_test_hooks = {
  .init = honda_body_test_init,
  .rx = honda_body_test_rx_hook,
  .tx = honda_body_test_tx_hook,
  .tx_lin = nooutput_tx_lin_hook,
  .fwd = honda_body_test_fwd_hook,
};
