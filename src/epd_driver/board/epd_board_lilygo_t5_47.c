#include "epd_board.h"

#include "../display_ops.h"
#include "../i2s_data_bus.h"
#include "../rmt_pulse.h"

#define CFG_DATA GPIO_NUM_23
#define CFG_CLK GPIO_NUM_18
#define CFG_STR GPIO_NUM_0
#define D7 GPIO_NUM_22
#define D6 GPIO_NUM_21
#define D5 GPIO_NUM_27
#define D4 GPIO_NUM_2
#define D3 GPIO_NUM_19
#define D2 GPIO_NUM_4
#define D1 GPIO_NUM_32
#define D0 GPIO_NUM_33

/* Control Lines */
#define CKV GPIO_NUM_25
#define STH GPIO_NUM_26

#define V4_LATCH_ENABLE GPIO_NUM_15

/* Edges */
#define CKH GPIO_NUM_5

typedef struct {
  bool ep_latch_enable : 1;
  bool power_disable : 1;
  bool pos_power_enable : 1;
  bool neg_power_enable : 1;
  bool ep_scan_direction : 1;
} epd_config_register_t;

static i2s_bus_config i2s_config = {
  .clock = CKH,
  .start_pulse = STH,
  .data_0 = D0,
  .data_1 = D1,
  .data_2 = D2,
  .data_3 = D3,
  .data_4 = D4,
  .data_5 = D5,
  .data_6 = D6,
  .data_7 = D7,
};

static void IRAM_ATTR push_cfg_bit(bool bit) {
  gpio_set_level(CFG_CLK, 0);
  gpio_set_level(CFG_DATA, bit);
  gpio_set_level(CFG_CLK, 1);
}

static epd_config_register_t config_reg;

static void epd_board_init(uint32_t epd_row_width) {
  /* Power Control Output/Off */
  PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[CFG_DATA], PIN_FUNC_GPIO);
  PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[CFG_CLK], PIN_FUNC_GPIO);
  PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[CFG_STR], PIN_FUNC_GPIO);
  gpio_set_direction(CFG_DATA, GPIO_MODE_OUTPUT);
  gpio_set_direction(CFG_CLK, GPIO_MODE_OUTPUT);
  gpio_set_direction(CFG_STR, GPIO_MODE_OUTPUT);
  fast_gpio_set_lo(CFG_STR);

  config_reg.ep_latch_enable = false;
  config_reg.power_disable = true;
  config_reg.pos_power_enable = false;
  config_reg.neg_power_enable = false;
  config_reg.ep_scan_direction = true;

  // Setup I2S
  // add an offset off dummy bytes to allow for enough timing headroom
  i2s_bus_init( &i2s_config, epd_row_width + 32 );

  rmt_pulse_init(CKV);
}

static void epd_board_set_ctrl(epd_ctrl_state_t *state) {
  fast_gpio_set_lo(CFG_STR);

  // push config bits in reverse order
  push_cfg_bit(state->ep_output_enable);
  push_cfg_bit(state->ep_mode);
  push_cfg_bit(config_reg.ep_scan_direction);
  push_cfg_bit(state->ep_stv);

  push_cfg_bit(config_reg.neg_power_enable);
  push_cfg_bit(config_reg.pos_power_enable);
  push_cfg_bit(config_reg.power_disable);
  push_cfg_bit(config_reg.ep_latch_enable);

  fast_gpio_set_hi(CFG_STR);
}

static void epd_board_poweron(epd_ctrl_state_t *state) {
  i2s_gpio_attach(&i2s_config);

  // This was re-purposed as power enable.
  config_reg.ep_scan_direction = true;

  // POWERON
  config_reg.power_disable = false;
  epd_board_set_ctrl(state);
  busy_delay(100 * 240);
  config_reg.neg_power_enable = true;
  epd_board_set_ctrl(state);
  busy_delay(500 * 240);
  config_reg.pos_power_enable = true;
  epd_board_set_ctrl(state);
  busy_delay(100 * 240);
  state->ep_stv = true;
  epd_board_set_ctrl(state);
  fast_gpio_set_hi(STH);
  // END POWERON
}

static void epd_board_poweroff(epd_ctrl_state_t *state) {
  // This was re-purposed as power enable.
  config_reg.ep_scan_direction = false;

  // POWEROFF
  config_reg.pos_power_enable = false;
  epd_board_set_ctrl(state);
  busy_delay(10 * 240);

  config_reg.neg_power_enable = false;
  config_reg.pos_power_enable = false;
  epd_board_set_ctrl(state);
  busy_delay(100 * 240);

  state->ep_stv = false;
  state->ep_output_enable = false;
  state->ep_mode = false;
  config_reg.power_disable = true;
  epd_board_set_ctrl(state);

  i2s_gpio_detach(&i2s_config);
  // END POWEROFF
}

static void epd_board_latch_row(epd_ctrl_state_t *state) {
  fast_gpio_set_hi(STH);
  config_reg.ep_latch_enable = true;
  epd_board_set_ctrl(state);

  config_reg.ep_latch_enable = false;
  epd_board_set_ctrl(state);
}

static void epd_board_end_frame(epd_ctrl_state_t *state) {
  state->ep_stv = false;
  epd_board_set_ctrl(state);
  pulse_ckv_us(1, 1, true);
  pulse_ckv_us(1, 1, true);
  pulse_ckv_us(1, 1, true);
  pulse_ckv_us(1, 1, true);
  pulse_ckv_us(1, 1, true);
  state->ep_mode = false;
  epd_board_set_ctrl(state);
  pulse_ckv_us(0, 10, true);
  state->ep_output_enable = false;
  epd_board_set_ctrl(state);
  pulse_ckv_us(1, 1, true);
  pulse_ckv_us(1, 1, true);
  pulse_ckv_us(1, 1, true);
}

const EpdBoardDefinition epd_board_lilygo_t5_47 = {
  .init = epd_board_init,
  .deinit = NULL,
  .set_ctrl = epd_board_set_ctrl,
  .poweron = epd_board_poweron,
  .poweroff = epd_board_poweroff,
  .latch_row = epd_board_latch_row,
  .end_frame = epd_board_end_frame,

  .temperature_init = NULL,
  .ambient_temperature = NULL,
};
