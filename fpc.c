// Dustin DeWeese (deweese@nestlabs.com)
// 2016

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <inttypes.h>

typedef __int128_t int128_t;

struct parameters {
  /* these are the inputs */
  long double
    min,
    max,
    precision;

  /* these can use more than fixed_encoding_width since there can be a _really_ large offset */
  int128_t
    lower_bound,
    upper_bound,
    offset;

  int
    fixed_encoding_width,
    fractional_bits;

  bool
    use_signed,
    large_offset;
};

int ceil_log2l(long double x) {
  int exp;
  long double n = frexpl(x, &exp);
  return n == 0.5L ? exp - 1 : exp;
}

int floor_log2l(long double x) {
  int exp;
  frexpl(x, &exp);
  return exp - 1;
}

bool calculate(struct parameters *param) {
  if(param->max < param->min + param->precision) {
    printf("ERROR: max < min + precision\n");
    return false;
  }
  if(param->precision <= 0.0L) {
    printf("ERROR: negative precision\n");
    return false;
  }
  param->fractional_bits = -floor_log2l(param->precision);
  param->lower_bound = floorl(ldexpl(param->min, param->fractional_bits));
  param->upper_bound = floorl(ldexpl(param->max, param->fractional_bits));
  param->fixed_encoding_width = ceil_log2l(param->upper_bound - param->lower_bound);
  if(param->fixed_encoding_width > 64) {
    printf("ERROR: fixed_encoding_width > 64\n");
    return false;
  }
  param->offset = param->lower_bound;
  param->large_offset = false;
  if(param->offset > (((int128_t)1) << 63) - 1 &&
     param->offset < (((int128_t)-1) << 63)) {
    printf("WARNING: really large offset\n");
    param->large_offset = true;
  }
  param->use_signed = false;
  if(param->min < 0.0L) {
    if(param->upper_bound <= (((int128_t)1) << (param->fixed_encoding_width - 1)) - 1 &&
       param->lower_bound >= (((int128_t)-1) << (param->fixed_encoding_width - 1))) {
      param->offset = 0;
      param->use_signed = true;
    }
  } else {
    if(param->upper_bound <= (((int128_t)1) << param->fixed_encoding_width) - 1) {
      param->offset = 0;
    }
  }
  return true;
}

void print_params(struct parameters *param) {
  printf("min = %Lf\n", param->min);
  printf("max = %Lf\n", param->max);
  printf("precision = %Lf\n", param->precision);
  printf("actual precision = %Lf\n", ldexpl(1.0L, -param->fractional_bits));
  printf("lower_bound - offset = %" PRId64 "\n", (int64_t)(param->lower_bound - param->offset));
  printf("upper_bound - offset = %" PRId64 "\n", (int64_t)(param->upper_bound - param->offset));
  if(param->large_offset) {
    printf("offset ~= %Le\n", (long double)param->offset);
  } else {
    printf("offset = %" PRId64 "\n", (int64_t)param->offset);
  }
  printf("fixed_encoding_width = %d\n", param->fixed_encoding_width);
  printf("fractional_bits = %d\n", param->fractional_bits);
  printf("integer_bits = %d\n", param->fixed_encoding_width - param->fractional_bits);
  printf("use_signed = %s\n", param->use_signed ? "yes" : "no");
}

int main(int argc, char **argv) {
  struct parameters param;
  if(argc <= 3) {
    printf("fpc [min] [max] [precision]\n");
    return -1;
  }
  param.min = strtold(argv[1], NULL);
  param.max = strtold(argv[2], NULL);
  param.precision = strtold(argv[3], NULL);

  if(calculate(&param)) {
    print_params(&param);
  } else {
    return -1;
  }
}
