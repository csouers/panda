// board enforces
//   in-state
//      accel set/resume
//   out-state
//      cancel button
//      accel rising edge
//      brake rising edge
//      brake > 0mph
const CanMsg crossfire_N_TX_MSGS[] = {{0xE4, 0, 5}, {0x194, 0, 4}, {0x1FA, 0, 8}, {0x200, 0, 6}, {0x30C, 0, 8}, {0x33D, 0, 5}, {0x16F118F0, 0, 8}};
const CanMsg crossfire_BOSCH_TX_MSGS[] = {{0xE4, 0, 5}, {0xE5, 0, 8}, {0x296, 1, 4}, {0x33D, 0, 5}, {0x33DA, 0, 5}, {0x33DB, 0, 8}, {0x16F118F0, 0, 8}};  // Bosch
const CanMsg crossfire_BOSCH_LONG_TX_MSGS[] = {{0xE4, 1, 5}, {0x1DF, 1, 8}, {0x1EF, 1, 8}, {0x1FA, 1, 8}, {0x30C, 1, 8}, {0x33D, 1, 5}, {0x33DA, 1, 5}, {0x33DB, 1, 8}, {0x39F, 1, 8}, {0x18DAB0F1, 1, 8}, {0x16F118F0, 0, 8}};  // Bosch w/ gas and brakes

// Roughly calculated using the offsets in openpilot +5%:
// In openpilot: ((gas1_norm + gas2_norm)/2) > 15
// gas_norm1 = ((gain_dbc1*gas1) + offset_dbc)
// gas_norm2 = ((gain_dbc2*gas2) + offset_dbc)
// assuming that 2*(gain_dbc1*gas1) == (gain_dbc2*gas2)
// In this safety: ((gas1 + (gas2/2))/2) > THRESHOLD
const int crossfire_GAS_INTERCEPTOR_THRESHOLD = 344;
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

  // // TODO: add back crossfire Nidec once we handle it properly in openpilot
  // //const bool pcm_cruise = ((crossfire_hw == crossfire_BOSCH) && !crossfire_bosch_long) || ((crossfire_hw == crossfire_NIDEC) && !gas_interceptor_detected);
  // const bool pcm_cruise = ((crossfire_hw == crossfire_BOSCH) && !crossfire_bosch_long);

  if (valid) {
    int addr = GET_ADDR(to_push);
    int len = GET_LEN(to_push);
    // int bus = GET_BUS(to_push);

    // sample speed
    if (addr == 0x158) {
      // first 2 bytes
      vehicle_moving = GET_BYTE(to_push, 0) | GET_BYTE(to_push, 1);
    }

    // // check ACC main state
    // // 0x326 for all Bosch and some Nidec, 0x1A6 for some Nidec
    // if ((addr == 0x326) || (addr == 0x1A6)) {
    //   acc_main_on = GET_BIT(to_push, ((addr == 0x326) ? 28U : 47U));
    //   if (!acc_main_on) {
    //     controls_allowed = 0;
    //   }
    // }

    controls_allowed = 1;
    // find this!

    // // enter controls when PCM enters cruise state
    // if (pcm_cruise && (addr == 0x17C)) {
    //   const bool cruise_engaged = GET_BIT(to_push, 38U) != 0U;
    //   if (!cruise_engaged) {
    //     controls_allowed = 0;
    //   }
    //   if (cruise_engaged && !cruise_engaged_prev) {
    //     controls_allowed = 1;
    //   }
    //   cruise_engaged_prev = cruise_engaged;
    // }

    // // state machine to enter and exit controls for button enabling
    // // 0x1A6 for the ILX, 0x296 for the Civic Touring
    // if (!pcm_cruise && ((addr == 0x1A6) || (addr == 0x296))) {
    //   // check for button presses
    //   int button = (GET_BYTE(to_push, 0) & 0xE0U) >> 5;
    //   switch (button) {
    //     case 1:  // main
    //     case 2:  // cancel
    //       controls_allowed = 0;
    //       break;
    //     case 3:  // set
    //     case 4:  // resume
    //       if (acc_main_on) {
    //         controls_allowed = 1;
    //       }
    //       break;
    //     default:
    //       break; // any other button is irrelevant
    //   }
    // }

    // todo add this!
  //   if (addr == 0x17C) {
  //     // also if brake switch is 1 for two CAN frames, as brake pressed is delayed
  //     const bool brake_switch = GET_BIT(to_push, 32U) != 0U;
  //     brake_pressed = (GET_BIT(to_push, 53U) != 0U) || (brake_switch && crossfire_brake_switch_prev);
  //     crossfire_brake_switch_prev = brake_switch;
  //   }
  // }

    // length check because bosch hardware also uses this id (0x201 w/ len = 8)
    if ((addr == 0x201) && (len == 6)) {
      gas_interceptor_detected = 1;
      int gas_interceptor = crossfire_GET_INTERCEPTOR(to_push);
      gas_pressed = gas_interceptor > crossfire_GAS_INTERCEPTOR_THRESHOLD;
      gas_interceptor_prev = gas_interceptor;
    }

    if (!gas_interceptor_detected) {
      if (addr == 0x17C) {
        gas_pressed = GET_BYTE(to_push, 0) != 0U;
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

  // tx = msg_allowed(to_send, crossfire_N_TX_MSGS, sizeof(crossfire_N_TX_MSGS)/sizeof(crossfire_N_TX_MSGS[0]));


  // disallow actuator commands if gas or brake (with vehicle moving) are pressed
  // and the the latching controls_allowed flag is True
  int pedal_pressed = brake_pressed_prev && vehicle_moving;
  pedal_pressed = pedal_pressed || gas_pressed_prev;
  bool current_controls_allowed = controls_allowed && !(pedal_pressed);
  // int bus_pt = 0;

  // BRAKE: safety check (nidec)
  // if ((addr == 0x1FA) && (bus == bus_pt)) {
  //   crossfire_brake = (GET_BYTE(to_send, 0) << 2) + ((GET_BYTE(to_send, 1) >> 6) & 0x3U);
  //   if (!current_controls_allowed) {
  //     if (crossfire_brake != 0) {
  //       tx = 0;
  //     }
  //   }
  //   if (crossfire_brake > 255) {
  //     tx = 0;
  //   }
  //   if (crossfire_fwd_brake) {
  //     tx = 0;
  //   }
  // }

  // todo do this!
  // BRAKE/GAS: safety check (bosch)
  // if ((addr == 0x1DF) && (bus == bus_pt)) {
  //   int accel = (GET_BYTE(to_send, 3) << 3) | ((GET_BYTE(to_send, 4) >> 5) & 0x7U);
  //   accel = to_signed(accel, 11);
  //   if (!current_controls_allowed) {
  //     if (accel != 0) {
  //       tx = 0;
  //     }
  //   }
  //   if (accel < crossfire_BOSCH_ACCEL_MIN) {
  //     tx = 0;
  //   }
  //
  //   int gas = (GET_BYTE(to_send, 0) << 8) | GET_BYTE(to_send, 1);
  //   gas = to_signed(gas, 16);
  //   if (!current_controls_allowed) {
  //     if (gas != crossfire_BOSCH_NO_GAS_VALUE) {
  //       tx = 0;
  //     }
  //   }
  //   if (gas > crossfire_BOSCH_GAS_MAX) {
  //     tx = 0;
  //   }
  // }

  // GAS: safety check (interceptor)
  if (addr == 0x200) {
    if (!current_controls_allowed) {
      if (GET_BYTE(to_send, 0) || GET_BYTE(to_send, 1)) {
        tx = 0;
      }
    }
  }

  // // FORCE CANCEL: safety check only relevant when spamming the cancel button in Bosch HW
  // // ensuring that only the cancel button press is sent (VAL 2) when controls are off.
  // // This avoids unintended engagements while still allowing resume spam
  // if ((addr == 0x296) && !current_controls_allowed && (bus == bus_pt)) {
  //   if (((GET_BYTE(to_send, 0) >> 5) & 0x7U) != 2U) {
  //     tx = 0;
  //   }
  // }
  // TODO: gateway packet
  // if (addr == 0x800) {
  //
  // }

  // KWP over CAN. Allow only short turn signal request and cancel
  // TODO: move to gateway firmware
  // if (addr == 0x16F118F0){
  //
  //   bool signalCmd = ((GET_LEN(to_send) == 8U) && ((GET_BYTES_04(to_send) == 0x000F0A30U) || (GET_BYTES_04(to_send) == 0x000F0B30U)) && (GET_BYTES_48(to_send) == 0x0U));
  //   bool cancelCmd = ((GET_LEN(to_send) == 8U) && (GET_BYTES_04(to_send) == 0x00000020U) && (GET_BYTES_48(to_send) == 0x0U));
  //
  //   // always allow cancel
  //   if (!cancelCmd) {
  //     if (!current_controls_allowed) {
  //       tx = 0;
  //     }
  //     if (current_controls_allowed && !signalCmd){
  //       tx = 0;
  //     }
  //   }
  // }

  // Only tester present ("\x02\x3E\x80\x00\x00\x00\x00\x00") allowed on diagnostics address
  // if (addr == 0x18DAB0F1) {
  //   if ((GET_BYTES_04(to_send) != 0x00803E02U) || (GET_BYTES_48(to_send) != 0x0U)) {
  //     tx = 0;
  //   }
  // }

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
