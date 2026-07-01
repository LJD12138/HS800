#ifndef EEZ_LVGL_UI_VARS_H
#define EEZ_LVGL_UI_VARS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// enum declarations

// Flow global variables

enum FlowGlobalVariables {
    FLOW_GLOBAL_VARIABLE_UCA_REMAINING_USAGE_TIME = 0,
    FLOW_GLOBAL_VARIABLE_UCA_OUT_PWR_VALUE = 1,
    FLOW_GLOBAL_VARIABLE_UCA_IN_PWR_VALUE = 2,
    FLOW_GLOBAL_VARIABLE_UCA_BAT_SOC_VALUE = 3,
    FLOW_GLOBAL_VARIABLE_UCA_ERR_CODE_VALUE = 4,
    FLOW_GLOBAL_VARIABLE_UCA_UPDATE_PROGRESS = 5,
    FLOW_GLOBAL_VARIABLE_UCA_UPDATE_COUNTDOWN = 6,
    FLOW_GLOBAL_VARIABLE_UCA_UPDATE_STATE = 7,
    FLOW_GLOBAL_VARIABLE_UCA_UPDATE_MSG = 8,
    FLOW_GLOBAL_VARIABLE_UCA_UPDATE_OBJ = 9,
    FLOW_GLOBAL_VARIABLE_UCA_UPDATE_CHANNEL = 10,
    FLOW_GLOBAL_VARIABLE_UCA_UPDATE_PROTO = 11,
    FLOW_GLOBAL_VARIABLE_UCA_UPDATE_FRAME = 12,
    FLOW_GLOBAL_VARIABLE_UCA_UPDATE_TIMEOUT = 13,
    FLOW_GLOBAL_VARIABLE_UCA_UPDATE_ERR_INFO = 14
};

// Native global variables

extern const char *get_var_uca_remaining_usage_time();
extern void set_var_uca_remaining_usage_time(const char *value);
extern const char *get_var_uca_out_pwr_value();
extern void set_var_uca_out_pwr_value(const char *value);
extern const char *get_var_uca_in_pwr_value();
extern void set_var_uca_in_pwr_value(const char *value);
extern const char *get_var_uca_bat_soc_value();
extern void set_var_uca_bat_soc_value(const char *value);
extern const char *get_var_uca_err_code_value();
extern void set_var_uca_err_code_value(const char *value);
extern const char *get_var_uca_update_progress();
extern void set_var_uca_update_progress(const char *value);
extern const char *get_var_uca_update_countdown();
extern void set_var_uca_update_countdown(const char *value);
extern int32_t get_var_uca_update_state();
extern void set_var_uca_update_state(int32_t value);
extern const char *get_var_uca_update_msg();
extern void set_var_uca_update_msg(const char *value);
extern const char *get_var_uca_update_obj();
extern void set_var_uca_update_obj(const char *value);
extern const char *get_var_uca_update_channel();
extern void set_var_uca_update_channel(const char *value);
extern const char *get_var_uca_update_proto();
extern void set_var_uca_update_proto(const char *value);
extern const char *get_var_uca_update_frame();
extern void set_var_uca_update_frame(const char *value);
extern const char *get_var_uca_update_timeout();
extern void set_var_uca_update_timeout(const char *value);
extern const char *get_var_uca_update_err_info();
extern void set_var_uca_update_err_info(const char *value);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/