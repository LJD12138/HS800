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
    FLOW_GLOBAL_VARIABLE_UCA_ERR_CODE_VALUE = 4
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

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/