// board enforces
//   in-state
//      accel set/resume
//   out-state
//      cancel button
//      accel rising edge
//      brake rising edge
//      brake > 0mph
const CanMsg crossfire_TX_MSGS[] = {{0x202, 0, 5}, {0x201, 0, 4}, {0x1FA, 0, 8}, {0x202, 0, 6}, {0x30C, 0, 8}, {0x33D, 0, 5}, {0x16F118F0, 0, 8}};

// Roughly calculated using the offsets in openpilot +5%:
// In openpilot: ((gas1_norm + gas2_norm)/2) > 15
// gas_norm1 = ((gain_dbc1*gas1) + offset_dbc)
// gas_norm2 = ((gain_dbc2*gas2) + offset_dbc)
// assuming that 2*(gain_dbc1*gas1) == (gain_dbc2*gas2)
// In this safety: ((gas1 + (gas2/2))/2) > THRESHOLD
const int crossfire_GAS_INTERCEPTOR_THRESHOLD = 142;
#define crossfire_GET_INTERCEPTOR(msg) (((GET_BYTE((msg), 0) << 8) + GET_BYTE((msg), 1) + ((GET_BYTE((msg), 2) << 8) + GET_BYTE((msg), 3)) / 2U ) / 2U) // avg between 2 tracks

// Nidec has the powertrain bus on bus 0
AddrCheckStruct crossfire_addr_checks[] = {
  // {.msg = {{0x1A6, 0, 8, .check_checksum = true, .max_counter = 3U, .expected_timestep = 40000U},
  //          {0x296, 0, 4, .check_checksum = true, .max_counter = 3U, .expected_timestep = 40000U}, { 0 }}},
  // {.msg = {{0x158, 0, 8, .check_checksum = true, .max_counter = 3U, .expected_timestep = 10000U}, { 0 }, { 0 }}},
  // {.msg = {{0x17C, 0, 8, .check_checksum = true, .max_counter = 3U, .expected_timestep = 10000U}, { 0 }, { 0 }}},
  // {.msg = {{0x326, 0, 8, .check_checksum = true, .max_counter = 3U, .expected_timestep = 100000U}, { 0 }, { 0 }}},
};
#define crossfire_addr_checks_LEN (sizeof(crossfire_addr_checks) / sizeof(crossfire_addr_checks[0]))

int crossfire_brake = 0;
bool crossfire_brake_switch_prev = false;
addr_checks crossfire_rx_checks = {crossfire_addr_checks, crossfire_addr_checks_LEN};


// static uint8_t crossfire_get_checksum(CANPacket_t *to_push) {
//   int checksum_byte = GET_LEN(to_push) - 1U;
//   return (uint8_t)(GET_BYTE(to_push, checksum_byte)) & 0xFU;
// }
//
// static uint8_t crossfire_compute_checksum(CANPacket_t *to_push) {
//   int len = GET_LEN(to_push);
//   uint8_t checksum = 0U;
//   unsigned int addr = GET_ADDR(to_push);
//   while (addr > 0U) {
//     checksum += (addr & 0xFU); addr >>= 4;
//   }
//   for (int j = 0; j < len; j++) {
//     uint8_t byte = GET_BYTE(to_push, j);
//     checksum += (byte & 0xFU) + (byte >> 4U);
//     if (j == (len - 1)) {
//       checksum -= (byte & 0xFU);  // remove checksum in message
//     }
//   }
//   return (8U - checksum) & 0xFU;
// }
//
// static uint8_t crossfire_get_counter(CANPacket_t *to_push) {
//   int counter_byte = GET_LEN(to_push) - 1U;
//   return ((uint8_t)(GET_BYTE(to_push, counter_byte)) >> 4U) & 0x3U;
// }

static int crossfire_rx_hook(CANPacket_t *to_push) {

  // bool valid = addr_safety_check(to_push, &crossfire_rx_checks,
  //                                crossfire_get_checksum, crossfire_compute_checksum, crossfire_get_counter);
  bool valid = true;

  if (valid) {
    int addr = GET_ADDR(to_push);
    //int len = GET_LEN(to_push);
    // int bus = GET_BUS(to_push);

    // cruise available (exceed min speed)
    if (addr == 0x200) {
      acc_main_on = true;//GET_BIT(to_push, 4);
      if (!acc_main_on) {
         controls_allowed = 0;
       }

     }

     // enter controls when PCM exits cruise state
     if (addr == 0x210) {
       const bool cruise_engaged = GET_BIT(to_push, 36U) != 0U;
       if (cruise_engaged) {
         controls_allowed = 0;
       }
       if (!cruise_engaged && cruise_engaged_prev){
         controls_allowed = 1;
       }
       cruise_engaged_prev = cruise_engaged;
     }

    if (addr == 0x300) {
      // also if brake switch is 1 for two CAN frames, as brake pressed is delayed
      const bool brake_switch = GET_BIT(to_push, 10U) != 0U;
      brake_pressed = brake_switch;
      crossfire_brake_switch_prev = brake_switch;
      if (brake_switch){
        controls_allowed = 0;
      }
    }

    if (addr == 0x201) {
      gas_interceptor_detected = 1;
      int gas_interceptor = crossfire_GET_INTERCEPTOR(to_push);
      gas_pressed = gas_interceptor > crossfire_GAS_INTERCEPTOR_THRESHOLD;
      gas_interceptor_prev = gas_interceptor;
      if (gas_pressed){
        controls_allowed = 0;
      }
    }
    // generic_rx_checks(stock_ecu_detected);

  }
  return valid;
}

// all commands: gas, brake and steering
// if controls_allowed and no pedals pressed
//     allow all commands up to limit
// else
//     block all commands that produce actuation

static int crossfire_tx_hook(CANPacket_t *to_send) {
  int tx = 1;
  int addr = GET_ADDR(to_send);
  // int bus = GET_BUS(to_send);
  // tx = msg_allowed(to_send, crossfire_TX_MSGS, sizeof(crossfire_TX_MSGS)/sizeof(crossfire_TX_MSGS[0]));


  // disallow actuator commands if gas or brake (with vehicle moving) are pressed
  // and the the latching controls_allowed flag is True
  int pedal_pressed = brake_pressed_prev || gas_pressed_prev;
  bool current_controls_allowed = controls_allowed && !(pedal_pressed);

  // GAS: safety check (interceptor)
  if (addr == 0x202) {
    if (!current_controls_allowed) {
      if (GET_BYTE(to_send, 0) || GET_BYTE(to_send, 1)) {
        tx = 0;
      }
    }
  }

  // 1 allows the message through
  return tx;
}

static const addr_checks* crossfire_init(int16_t param) {
  UNUSED(param);
  controls_allowed = false;
  // relay_malfunction_reset();
  gas_interceptor_detected = 0;
  // crossfire_rx_checks = (addr_checks){crossfire_addr_checks, crossfire_addr_checks_LEN};
  return &crossfire_rx_checks;
}

const safety_hooks crossfire_hooks = {
  .init = crossfire_init,
  .rx = crossfire_rx_hook,
  .tx = crossfire_tx_hook,
  .tx_lin = nooutput_tx_lin_hook,
  .fwd = default_fwd_hook,
};
