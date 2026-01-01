/*
 * Copyright (c) 2023-2025 Roland Metivier <metivier.roland@chlorophyt.us>
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
#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#define WIRELESS_STATUS_STARTED (((uint32_t)(1)) << 0)
#define WIRELESS_STATUS_BINARY_FETCHED_OTA (((uint32_t)(1)) << 1)
#define WIRELESS_STATUS_CONNECTED (((uint32_t)(1)) << 2)
#define WIRELESS_STATUS_NONE ((uint32_t)(0))

#define WIRELESS_CONNECTED_BIT BIT0
#define WIRELESS_FAIL_BIT BIT1

typedef struct {
  wifi_config_t config;
  uint32_t status;
  EventGroupHandle_t events;
  uint32_t tries_left;
} wireless_t;

// Dynamic allocation of wireless_t structs
void wireless_init(wireless_t **);

// Static fill of wireless_t structs
void wireless_fill(wireless_t *);

// Starts the Wi-Fi connection
void wireless_start(wireless_t *);

// Dynamic free of wireless_t structs
void wireless_free(wireless_t *);
