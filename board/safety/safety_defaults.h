void can_send(CAN_FIFOMailBox_TypeDef *to_push, uint8_t bus_number, bool skip_tx_hook);

// refactor all of this down after it works
uint8_t bus_fcan = 1U;
uint8_t idx_1df = 0U;
uint8_t idx_1ef = 0U;
uint8_t idx_1fa = 0U;
uint8_t idx_30c = 0U;
uint8_t idx_39f = 0U;

void send_1df(uint32_t RIR, uint32_t RDTR) {
  CAN_FIFOMailBox_TypeDef to_send;
  uint32_t addr_mask = 0x001FFFFF;
  uint8_t msg_len = 8;
  int msg_addr = 0x1df;
  to_send.RIR = (msg_addr << 21) + (addr_mask & (RIR | 1));
  to_send.RDTR = (RDTR & 0xFFFFFFF0) | msg_len;
  to_send.RDLR = 0x0008d08a;
  to_send.RDHR = 0x0 + (idx_1df << 28);
  idx_1df++;
  if (idx_1df >= 4U){
    idx_1df = 0U;
  }
  can_send(&to_send, bus_fcan, true);
}

void send_1ef(uint32_t RIR, uint32_t RDTR) {
  CAN_FIFOMailBox_TypeDef to_send;
  uint32_t addr_mask = 0x001FFFFF;
  uint8_t msg_len = 8;
  int msg_addr = 0x1ef;
  to_send.RIR = (msg_addr << 21) + (addr_mask & (RIR | 1));
  to_send.RDTR = (RDTR & 0xFFFFFFF0) | msg_len;
  to_send.RDLR = 0x7500FF03;
  to_send.RDHR = 0x0 + (idx_1ef << 28);
  idx_1ef++;
  if (idx_1ef >= 4U){
    idx_1ef = 0U;
  }
  can_send(&to_send, bus_fcan, true);
}

void send_1fa(uint32_t RIR, uint32_t RDTR) {
  CAN_FIFOMailBox_TypeDef to_send;
  uint32_t addr_mask = 0x001FFFFF;
  uint8_t msg_len = 8;
  int msg_addr = 0x1fa;
  to_send.RIR = (msg_addr << 21) + (addr_mask & (RIR | 1));
  to_send.RDTR = (RDTR & 0xFFFFFFF0) | msg_len;
  to_send.RDLR = 0x0;
  to_send.RDHR = 0x0 + (idx_1fa << 28);
  idx_1fa++;
  if (idx_1fa >= 4U){
    idx_1fa = 0U;
  }
  can_send(&to_send, bus_fcan, true);
}

void send_30c(uint32_t RIR, uint32_t RDTR) {
  CAN_FIFOMailBox_TypeDef to_send;
  uint32_t addr_mask = 0x001FFFFF;
  uint8_t msg_len = 8;
  int msg_addr = 0x30c;
  to_send.RIR = (msg_addr << 21) + (addr_mask & (RIR | 1));
  to_send.RDTR = (RDTR & 0xFFFFFFF0) | msg_len;
  to_send.RDLR = 0xFF000000;
  to_send.RDHR = 0x00c0c019 + (idx_30c << 28);
  idx_30c++;
  if (idx_30c >= 4U){
    idx_30c = 0U;
  }
  can_send(&to_send, bus_fcan, true);
}

void send_39f(uint32_t RIR, uint32_t RDTR) {
  CAN_FIFOMailBox_TypeDef to_send;
  uint32_t addr_mask = 0x001FFFFF;
  uint8_t msg_len = 8;
  int msg_addr = 0x39f;
  to_send.RIR = (msg_addr << 21) + (addr_mask & (RIR | 1));
  to_send.RDTR = (RDTR & 0xFFFFFFF0) | msg_len;
  to_send.RDLR = 0x00003000;
  to_send.RDHR = 0x0 + (idx_39f << 28);
  idx_39f++;
  if (idx_39f>= 4U){
    idx_39f = 0U;
  }
  can_send(&to_send, bus_fcan, true);
}

int default_rx_hook(CAN_FIFOMailBox_TypeDef *to_push) {
  int addr = GET_ADDR(to_push);
  int bus = GET_BUS(to_push);

  if (bus == 1){
    if (addr == 0x1a4){
      send_1df(to_push->RIR,to_push->RDTR);
      send_1ef(to_push->RIR,to_push->RDTR);
    }
    if (addr == 0x309){
      send_1fa(to_push->RIR,to_push->RDTR);
      send_30c(to_push->RIR,to_push->RDTR);
      send_39f(to_push->RIR,to_push->RDTR);
    }
  }
  return true;
}

// *** no output safety mode ***

static void nooutput_init(int16_t param) {
  UNUSED(param);
  controls_allowed = false;
  relay_malfunction_reset();
}

static int nooutput_tx_hook(CAN_FIFOMailBox_TypeDef *to_send) {
  UNUSED(to_send);
  return false;
}

static int nooutput_tx_lin_hook(int lin_num, uint8_t *data, int len) {
  UNUSED(lin_num);
  UNUSED(data);
  UNUSED(len);
  return false;
}

static int default_fwd_hook(int bus_num, CAN_FIFOMailBox_TypeDef *to_fwd) {
  UNUSED(bus_num);
  UNUSED(to_fwd);
  return -1;
}

const safety_hooks nooutput_hooks = {
  .init = nooutput_init,
  .rx = default_rx_hook,
  .tx = nooutput_tx_hook,
  .tx_lin = nooutput_tx_lin_hook,
  .fwd = default_fwd_hook,
};

// *** all output safety mode ***

static void alloutput_init(int16_t param) {
  UNUSED(param);
  controls_allowed = true;
  relay_malfunction_reset();
}

static int alloutput_tx_hook(CAN_FIFOMailBox_TypeDef *to_send) {
  UNUSED(to_send);
  return true;
}

static int alloutput_tx_lin_hook(int lin_num, uint8_t *data, int len) {
  UNUSED(lin_num);
  UNUSED(data);
  UNUSED(len);
  return true;
}

const safety_hooks alloutput_hooks = {
  .init = alloutput_init,
  .rx = default_rx_hook,
  .tx = alloutput_tx_hook,
  .tx_lin = alloutput_tx_lin_hook,
  .fwd = default_fwd_hook,
};
