/* Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <stdint.h>

#define RADIX 10
#define LOG2_RATIO 1292913986 // 2^32 * (log 2 / log RADIX)
#define MAX_EXP 19
#define SENTINEL (INT_MAX / RADIX)

int main(int argc, char **argv) {
  char buf[64] = {0};
  if(argc >= 2) {
    int exponent, mantissa;
    char *arg = argv[1];
    int len = strlen(arg);
    int ret = read_float(arg, len, &exponent, &mantissa);
    printf("read_float(\"%s\", %d, %d, %d) = %d\n", arg, len, exponent, mantissa, ret);
    int fractional_bits = 16;
    int fixed = float_to_fixed(exponent, mantissa, fractional_bits);
    printf("float_to_fixed(%d, %d, %d) = %d\n", exponent, mantissa, fractional_bits, fixed);
    mantissa = fixed_to_float(fractional_bits, fixed, &exponent);
    printf("fixed_to_float(%d, %d, %d) = %d\n", fractional_bits, fixed, exponent, mantissa);
    ret = show_float(buf, sizeof buf - 1, exponent, mantissa);
    printf("show_float(\"%s\", %d, %d, %d) = %d\n", buf, (int)sizeof buf - 1, exponent, mantissa, ret);
    fixed = read_fixed(arg, len, fractional_bits);
    printf("read_fixed(\"%s\", %d, %d) = %d\n", arg, len, fractional_bits, fixed);
    ret = show_fixed(buf, sizeof buf - 1, fractional_bits, fixed);
    printf("show_fixed(\"%s\", %d, %d, %d) = %d\n", buf, (int)sizeof buf - 1, fractional_bits, fixed, ret);
  }
  return 0;
}

char num_to_digit(unsigned int num) {
  if(num < 10) {
    return '0' + num;
  } else return '?';
}

int digit_to_num(char digit) {
  if(digit < '0') {
    return -1;
  } else if(digit <= '9') {
    return digit - '0';
  } else {
    return -1;
  }
}

void reverse(char *buf, size_t size) {
  char
    *left = buf,
    *right = buf + size - 1,
    tmp;

  while(left < right) {
    tmp = *left;
    *left = *right;
    *right = tmp;
    left++;
    right--;
  }
}

int read_float(char *buf, size_t size, int *exponent_out, int *mantissa_out) {
  int mantissa = 0;
  int exponent = 0;
  bool neg = false;
  char *cursor = buf;
  char *end = buf + size;

  // sign
  if(cursor < end && *cursor == '-') {
    neg = true;
    cursor++;
  }

  // whole part
  while(cursor < end) {
    int digit = digit_to_num(*cursor);
    if(digit < 0) break;
    if(mantissa >= SENTINEL) return -EOVERFLOW;
    mantissa *= RADIX;
    mantissa += digit;
    cursor++;
  }

  // fractional part
  if(cursor < end && *cursor == '.') {
    cursor++;
    while(cursor < end) {
      int digit = digit_to_num(*cursor);
      if(digit < 0) break;
      if(mantissa >= SENTINEL) return -EOVERFLOW;
      mantissa *= RADIX;
      mantissa += digit;
      cursor++;
      exponent--;
    }
  }
  *exponent_out = exponent;
  *mantissa_out = neg ? -mantissa : mantissa;
  return cursor - buf;
}

int show_float(char *buf, size_t size, int exponent, int mantissa) {
  int n = mantissa < 0 ? -mantissa : mantissa;
  char *cursor = buf;
  char *end = buf + size;
  if(exponent >= 0) exponent = 1;

  // remove trailing zeroes
  while(exponent < 0 && n % RADIX == 0) {
    n /= RADIX;
    exponent++;
  }

  // fractional part
  if(exponent < 0) {

    while(exponent < 0) {
      if(cursor >= end) return -EOVERFLOW;
      *cursor++ = num_to_digit(n % RADIX);
      n /= RADIX;
      exponent++;
    }
  
    if(cursor >= end) return -EOVERFLOW;
    *cursor++ = '.';
  }

  // whole part
  do {
    if(cursor >= end) return -EOVERFLOW;
    *cursor++ = num_to_digit(n % RADIX);
    n /= RADIX;
    exponent++;
  } while(n);

  // sign
  if(mantissa < 0) {
    if(cursor >= end) return -EOVERFLOW;
    *cursor++ = '-';
  }

  // flip it
  int written = cursor - buf;
  reverse(buf, written);

  // add a null terminator
  if(cursor >= end) return -EOVERFLOW;
  *cursor = 0;

  return written;
}

#define SQUARED(X) ((unsigned long)(X) * (X))
#define RADIX2 SQUARED(RADIX)
#define RADIX4 SQUARED(RADIX2)
#define RADIX8 SQUARED(RADIX4)
#define RADIX16 SQUARED(RADIX8)
uint64_t exp_radix(unsigned int n) {
  int64_t x = 1;
  if(n > MAX_EXP) return 0;
  if(n >= 16) {
    x *= RADIX16;
    n -= 16;
  }
  if(n >= 8) {
    x *= RADIX8;
    n -= 8;
  }
  if(n >= 4) {
    x *= RADIX4;
    n -= 4;
  }
  if(n >= 2) {
    x *= RADIX2;
    n -= 2;
  }
  if(n >= 1) {
    x *= RADIX;
    n -= 1;
  }
  return x;
}

int32_t float_to_fixed(int exponent, int32_t mantissa, unsigned int fractional_bits) {
  int64_t fixed = (int64_t)mantissa << fractional_bits;
  if(exponent < 0) {
    int64_t divisor = exp_radix(-exponent);
    fixed += divisor - 1;
    fixed /= divisor;
  } else {
    fixed *= exp_radix(exponent);
  }
  return fixed;
}

int fixed_to_float(unsigned int fractional_bits, int32_t fixed, int *exponent_out) {
  int exponent = (((uint64_t)fractional_bits * LOG2_RATIO) + (1 << 31)) >> 32;
  int floating = fixed * exp_radix(exponent) >> fractional_bits;

  // trim zeroes
  while(floating % RADIX == 0 && exponent > 0) {
    exponent--;
    floating /= RADIX;
  }

  *exponent_out = -exponent;
  return floating;
}

int read_fixed(char *buf, size_t size, unsigned int fractional_bits) {
  int ret;
  int mantissa, exponent;

  ret = read_float(buf, size, &mantissa, &exponent);

  if(ret < 0)
    return 0;

  return float_to_fixed(mantissa, exponent, fractional_bits);
}

int show_fixed(char *buf, size_t size, unsigned int fractional_bits, int fixed) {
  int ret;
  int mantissa, exponent;

  mantissa = fixed_to_float(fractional_bits, fixed, &exponent);
  return show_float(buf, size, exponent, mantissa);
}
