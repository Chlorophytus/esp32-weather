/*
 * Copyright (c) 2023-2026 Roland Metivier <metivier.roland@chlorophyt.us>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#pragma once
#include "driver/i2c_master.h"
#include "driver/i2c_types.h"
#include <inttypes.h>
#include <stdlib.h>

#define I2CMUX_BUF_PRESSURE_SIZE 3
#define I2CMUX_DIG_PRESSURE_SIZE 9
#define I2CMUX_BUF_TEMPERATURE_SIZE 3
#define I2CMUX_DIG_TEMPERATURE_SIZE 3

typedef struct {
  i2c_master_bus_handle_t bus_handle;
  
  i2c_master_dev_handle_t bme280_handle;

  uint8_t bme280_buf_pressure[I2CMUX_BUF_PRESSURE_SIZE];
  uint32_t bme280_pressure;
  uint8_t bme280_buf_dig_P[I2CMUX_DIG_PRESSURE_SIZE * 2];
  uint16_t bme280_pressure_coeff_1;
  int16_t bme280_pressure_coeffs[I2CMUX_DIG_PRESSURE_SIZE - 1];

  uint8_t bme280_buf_temperature[I2CMUX_BUF_TEMPERATURE_SIZE];
  int32_t bme280_temperature;
  uint8_t bme280_buf_dig_T[I2CMUX_DIG_TEMPERATURE_SIZE * 2];
  uint16_t bme280_temperature_coeff_1;
  int16_t bme280_temperature_coeffs[I2CMUX_DIG_TEMPERATURE_SIZE - 1];
} iic_mux_t;

// Dynamic allocation of iic_mux_t structs
void iic_mux_init(iic_mux_t **);

// Static fill of iic_mux_t structs
void iic_mux_fill(iic_mux_t *);

// Starts the I2C multiplex
void iic_mux_start(iic_mux_t *);

// Sense...
void iic_mux_refresh(iic_mux_t *);

// Dynamic free of iic_mux_t structs
void iic_mux_free(iic_mux_t *);