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
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "esp_log.h"

#define GPS_TIME_SENTENCE_TYPE_GPGGA (((uint32_t)(1)) << 0)
#define GPS_TIME_SENTENCE_TYPE_GPGLL (((uint32_t)(1)) << 1)
#define GPS_TIME_SENTENCE_TYPE_GPGSA (((uint32_t)(1)) << 2)
#define GPS_TIME_SENTENCE_TYPE_GPGSV (((uint32_t)(1)) << 3)
#define GPS_TIME_SENTENCE_TYPE_GPRMC (((uint32_t)(1)) << 4)
#define GPS_TIME_SENTENCE_TYPE_GPVTG (((uint32_t)(1)) << 5)
#define GPS_TIME_SENTENCE_TYPE_OTHER ((uint32_t)(0))

#define GPS_TIME_NONE ((uint32_t)(0))
#define GPS_TIME_DATE_IS_CURRENT (((uint32_t)(1)) << 0)
#define GPS_TIME_TIME_IS_CURRENT (((uint32_t)(1)) << 1)
#define GPS_TIME_HAS_FIX (((uint32_t)(1)) << 2)
#define GPS_TIME_READY                                                         \
  (GPS_TIME_HAS_FIX | GPS_TIME_TIME_IS_CURRENT | GPS_TIME_DATE_IS_CURRENT)

#define GPS_TIME_MAX_SENTENCE_LENGTH 80
#define GPS_TIME_UPDATE_INTERVAL 60

typedef struct {
  uint32_t year;
  uint32_t month;
  uint32_t day;
  uint32_t hour;
  uint32_t minute;
  uint32_t second;

  uint32_t status;
  uint32_t lag;
} gps_time_t;

// Dynamic allocation of gps_time_t structs
void gps_time_init(gps_time_t **);

// Static fill of gps_time_t structs
void gps_time_fill(gps_time_t *);

// Read a NMEA line
void gps_time_nmea_read(gps_time_t *, const char *);

// Dynamic free of gps_time_t structs
void gps_time_free(gps_time_t *);
