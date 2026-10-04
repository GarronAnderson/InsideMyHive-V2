#ifndef TELEMETRY_H
#define TELEMETRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdio.h>


/* Serial print for flight sensor values */
HAL_StatusTypeDef lora_tx_telemetry(FlightSensorData *sensordata);
HAL_StatusTypeDef lora_rx_command(void);
HAL_StatusTypeDef lora_tx_continuity(void);
void pyro_fire_drogue_ground(void);
void pyro_fire_main_ground(void);

#ifdef __cplusplus
}
#endif

#endif