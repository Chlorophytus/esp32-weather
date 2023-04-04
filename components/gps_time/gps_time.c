/*
 * Copyright (c) 2023 Roland Metivier <metivier.roland@chlorophyt.us>
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
#include "gps_time.h"

static const char *TAG = "gps_time";

void gps_time_init(gps_time_t **pointer) {
  *pointer = malloc(sizeof(gps_time_t));
  gps_time_fill(*pointer);
}

void gps_time_fill(gps_time_t *pointer) {
  pointer->status = 0;
  pointer->lag = 0;
}

void gps_time_nmea_read(gps_time_t *pointer, const char *sentence) {
  if (sentence[0] != '$' || !CONFIG_GPS_TIME_SYNC) {
    return;
  }
  uint32_t end = 0;
  uint32_t copy_point = 0;
  char current_sentence[GPS_TIME_MAX_SENTENCE_LENGTH];
  // Prepare the copied sentence to be strtok'd, which is destructive to the
  // original sentence data.
  for (uint32_t i = 0; i < GPS_TIME_MAX_SENTENCE_LENGTH; i++) {
    if (end != 0) {
      // Encountered null, or a line break
      break;
    }
    // Switch based on NMEA sentence point.
    switch (sentence[i]) {
    case '\0':
    case '\r':
    case '\n': {
      current_sentence[copy_point] = '\0';
      end = 1;
      break;
    }
    case '$':
    case '!': {
      continue;
    }
    default: {
      current_sentence[copy_point] = sentence[i];
      copy_point++;
      break;
    }
    }
  }
  // Get the NMEA sentence's tokens...
  char *state;
  char *nmea_token = strtok_r(current_sentence, ",", &state);
  uint32_t sentence_type = GPS_TIME_SENTENCE_TYPE_OTHER;
  uint32_t token_count = 0;
  while (nmea_token != NULL) {
    switch (token_count) {
    case 0: {
      if (strlen(nmea_token) == 5) {
        if (strcmp(nmea_token + 2, "GGA") == 0) {
          sentence_type = GPS_TIME_SENTENCE_TYPE_GPGGA;
        } else if (strcmp(nmea_token + 2, "GLL") == 0) {
          sentence_type = GPS_TIME_SENTENCE_TYPE_GPGLL;
        } else if (strcmp(nmea_token + 2, "GSA") == 0) {
          sentence_type = GPS_TIME_SENTENCE_TYPE_GPGSA;
        } else if (strcmp(nmea_token + 2, "GSV") == 0) {
          sentence_type = GPS_TIME_SENTENCE_TYPE_GPGSV;
        } else if (strcmp(nmea_token + 2, "RMC") == 0) {
          sentence_type = GPS_TIME_SENTENCE_TYPE_GPRMC;
        } else if (strcmp(nmea_token + 2, "VTG") == 0) {
          sentence_type = GPS_TIME_SENTENCE_TYPE_GPVTG;
        }
      }
      break;
    }
    case 1: {
      if (sentence_type == GPS_TIME_SENTENCE_TYPE_GPGGA ||
          sentence_type == GPS_TIME_SENTENCE_TYPE_GPRMC) {
        sscanf(nmea_token, "%02" PRIu32 "%02" PRIu32 "%02" PRIu32,
               &(pointer->hour), &(pointer->minute), &(pointer->second));
        // printf("Got time code: %s\n", nmea_token);
      }
      break;
    }
    case 4: {
      if (sentence_type == GPS_TIME_SENTENCE_TYPE_GPGGA) {
        uint32_t fix = 0;
        int num = sscanf(nmea_token, "%" PRIu32, &fix);
        if (num == 1 && fix > 0) {
          pointer->status |= (GPS_TIME_HAS_FIX | GPS_TIME_TIME_IS_CURRENT);
          // printf("Got fix\n");
        } else {
          pointer->status &= ~GPS_TIME_HAS_FIX;
          // printf("No fix, date and time are no longer valid\n");
        }
      }
      break;
    }
    case 8: {
      if (sentence_type == GPS_TIME_SENTENCE_TYPE_GPRMC) {
        sscanf(nmea_token, "%02" PRIu32 "%02" PRIu32 "%02" PRIu32,
               &(pointer->day), &(pointer->month), &(pointer->year));
        pointer->status |= GPS_TIME_DATE_IS_CURRENT;
        // printf("Got date code: %s\n", nmea_token);
      }
      break;
    }
    default: {
      break;
    }
    }

    nmea_token = strtok_r(NULL, ",", &state);
    token_count++;
  }

  if (((pointer->status & GPS_TIME_READY) == GPS_TIME_READY)) {
    pointer->status &= ~(GPS_TIME_DATE_IS_CURRENT | GPS_TIME_TIME_IS_CURRENT);
    if (pointer->lag == 0) {
      struct tm time;

      time.tm_year = pointer->year + 100; // Y2K much?
      time.tm_mon = pointer->month - 1;
      time.tm_mday = pointer->day;

      time.tm_hour = pointer->hour;
      time.tm_min = pointer->minute;
      time.tm_sec = pointer->second;

      time_t secs = mktime(&time);
      const struct timeval tval = {.tv_sec = secs, .tv_usec = 0};

      char buf[128];
      strftime(buf, 127, "%c", &time);
      ESP_LOGI(TAG, "Time of day will be set: %s", buf);

      settimeofday(&tval, NULL);
      pointer->lag = CONFIG_GPS_TIME_LAG;
    }
    (pointer->lag)--;
  }
}

void gps_time_free(gps_time_t *pointer) { free(pointer); }
