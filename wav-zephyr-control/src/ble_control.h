#pragma once

#include <zephyr/kernel.h>

int ble_control_init(struct k_msgq *cmd_q);
