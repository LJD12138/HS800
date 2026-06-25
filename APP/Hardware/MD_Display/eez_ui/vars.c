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

char uca_update_progress[100] = { 0 };

const char *get_var_uca_update_progress() {
    return uca_update_progress;
}

void set_var_uca_update_progress(const char *value) {
    strncpy(uca_update_progress, value, sizeof(uca_update_progress) / sizeof(char));
    uca_update_progress[sizeof(uca_update_progress) / sizeof(char) - 1] = 0;
}

char uca_update_obj[100] = { 0 };

const char *get_var_uca_update_obj() {
    return uca_update_obj;
}

void set_var_uca_update_obj(const char *value) {
    strncpy(uca_update_obj, value, sizeof(uca_update_obj) / sizeof(char));
    uca_update_obj[sizeof(uca_update_obj) / sizeof(char) - 1] = 0;
}

char uca_update_channel[100] = { 0 };

const char *get_var_uca_update_channel() {
    return uca_update_channel;
}

void set_var_uca_update_channel(const char *value) {
    strncpy(uca_update_channel, value, sizeof(uca_update_channel) / sizeof(char));
    uca_update_channel[sizeof(uca_update_channel) / sizeof(char) - 1] = 0;
}

char uca_update_proto[100] = { 0 };

const char *get_var_uca_update_proto() {
    return uca_update_proto;
}

void set_var_uca_update_proto(const char *value) {
    strncpy(uca_update_proto, value, sizeof(uca_update_proto) / sizeof(char));
    uca_update_proto[sizeof(uca_update_proto) / sizeof(char) - 1] = 0;
}

char uca_update_frame[100] = { 0 };

const char *get_var_uca_update_frame() {
    return uca_update_frame;
}

void set_var_uca_update_frame(const char *value) {
    strncpy(uca_update_frame, value, sizeof(uca_update_frame) / sizeof(char));
    uca_update_frame[sizeof(uca_update_frame) / sizeof(char) - 1] = 0;
}

char uca_update_timeout[100] = { 0 };

const char *get_var_uca_update_timeout() {
    return uca_update_timeout;
}

void set_var_uca_update_timeout(const char *value) {
    strncpy(uca_update_timeout, value, sizeof(uca_update_timeout) / sizeof(char));
    uca_update_timeout[sizeof(uca_update_timeout) / sizeof(char) - 1] = 0;
}


