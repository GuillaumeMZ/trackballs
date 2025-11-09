/* general.cc
   Some quite general utility algorithms.

   Copyright (C) 2000  Mathias Broxvall

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "general.h"

#include <stdarg.h>

#include <chrono>
#include <cstdlib>

int low_memory;

double timeDilationFactor = 1.0;

void generalInit() {}

double frandom() { return (rand() % (1 << 30)) / ((double)(1 << 30)); }

int mymod(int v, int m) {
  int tmp = v % m;
  while (tmp < 0) tmp += m;
  return tmp;
}

double getTimeDifference(const std::chrono::time_point<std::chrono::system_clock>& from, const std::chrono::time_point<std::chrono::system_clock>& to) {
  return timeDilationFactor * std::chrono::duration<double>(to - from).count();
}

void error(const char *formatstr, ...) {
  char errmsg[256];
  va_list(args);
  snprintf(errmsg, 255, "[ERROR] %s\n", formatstr);
  errmsg[255] = '\0';
  va_start(args, formatstr);
  vfprintf(stderr, errmsg, args);
  exit(EXIT_FAILURE);
}
void warning(const char *formatstr, ...) {
  char errmsg[256];
  va_list(args);
  snprintf(errmsg, 255, "[WARNING] %s\n", formatstr);
  errmsg[255] = '\0';
  va_start(args, formatstr);
  vfprintf(stderr, errmsg, args);
  va_end(args);
}

float sRGBToLinear(float v) {
  if (v < 0.04045) {
    return v / 12.92f;
  } else {
    return std::pow(((v + 0.055f) / 1.055f), 2.4f);
  }
}
