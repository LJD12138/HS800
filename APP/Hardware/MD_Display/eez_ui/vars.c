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


char uca_err_code_value[5] = { 0 };

const char *get_var_uca_err_code_value() {
    return uca_err_code_value;
}

void set_var_uca_err_code_value(const char *value) {
    strncpy(uca_err_code_value, value, sizeof(uca_err_code_value) / sizeof(char));
    uca_err_code_value[sizeof(uca_err_code_value) / sizeof(char) - 1] = 0;
}

char uca_update_countdown[100] = { 0 };

const char *get_var_uca_update_countdown() {
    return uca_update_countdown;
}

void set_var_uca_update_countdown(const char *value) {
    strncpy(uca_update_countdown, value, sizeof(uca_update_countdown) / sizeof(char));
    uca_update_countdown[sizeof(uca_update_countdown) / sizeof(char) - 1] = 0;
}

int32_t uca_update_state;

int32_t get_var_uca_update_state() {
    return uca_update_state;
}

void set_var_uca_update_state(int32_t value) {
    uca_update_state = value;
}

char uca_update_msg[100] = { 0 };

const char *get_var_uca_update_msg() {
    return uca_update_msg;
}

void set_var_uca_update_msg(const char *value) {
    strncpy(uca_update_msg, value, sizeof(uca_update_msg) / sizeof(char));
    uca_update_msg[sizeof(uca_update_msg) / sizeof(char) - 1] = 0;
}


