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

  // bus 0: Right radar
  // bus 1: body bus
  // bus 2: Left radar
  int bus_right = 0; // A01
  int bus_body = 1;
  int bus_left = 2; //A02

  if (bus_num == bus_right) {
    int addr = GET_ADDR(to_fwd);
    // fake the radar version number because I don't know how to buy the right parts. :|
    if (addr == 0x12F8BFA7) {
      to_fwd->RDHR = 0x0241414c;
    }
    can_send(to_fwd, bus_left, true);
    can_send(to_fwd, bus_body, true);
  }

  if (bus_num == bus_left) {
    can_send(to_fwd, bus_right, true);
    can_send(to_fwd, bus_body, true);
  }

  if (bus_num == bus_body) {
    can_send(to_fwd, bus_right, true);
    can_send(to_fwd, bus_left, true);
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
