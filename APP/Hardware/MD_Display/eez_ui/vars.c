#include "vars.h"
#include <string.h>

char uca_bat_soc_value[5] = { 0 };

const char *get_var_uca_bat_soc_value() {
    return uca_bat_soc_value;
}

void set_var_uca_bat_soc_value(const char *value) {
    strncpy(uca_bat_soc_value, value, sizeof(uca_bat_soc_value) / sizeof(char));
    uca_bat_soc_value[sizeof(uca_bat_soc_value) / sizeof(char) - 1] = 0;
}




char uca_out_pwr_value[5] = { 0 };

const char *get_var_uca_out_pwr_value() {
    return uca_out_pwr_value;
}

void set_var_uca_out_pwr_value(const char *value) {
    strncpy(uca_out_pwr_value, value, sizeof(uca_out_pwr_value) / sizeof(char));
    uca_out_pwr_value[sizeof(uca_out_pwr_value) / sizeof(char) - 1] = 0;
}




char uca_in_pwr_value[5] = { 0 };

const char *get_var_uca_in_pwr_value() {
    return uca_in_pwr_value;
}

void set_var_uca_in_pwr_value(const char *value) {
    strncpy(uca_in_pwr_value, value, sizeof(uca_in_pwr_value) / sizeof(char));
    uca_in_pwr_value[sizeof(uca_in_pwr_value) / sizeof(char) - 1] = 0;
}




char uca_remaining_usage_time[10] = { 0 };

const char *get_var_uca_remaining_usage_time() {
    return uca_remaining_usage_time;
}

void set_var_uca_remaining_usage_time(const char *value) {
    strncpy(uca_remaining_usage_time, value, sizeof(uca_remaining_usage_time) / sizeof(char));
    uca_remaining_usage_time[sizeof(uca_remaining_usage_time) / sizeof(char) - 1] = 0;
}
