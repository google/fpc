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
    fractional_bits,
    integer_bits,
    fixed_encoding_width;

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

unsigned int int_log2(unsigned int x) {
  return x <= 1 ? 0 : (8 * sizeof(x)) - __builtin_clz(x - 1);
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
  param->integer_bits = ceil_log2l(param->max - param->min + param->precision);
  param->fixed_encoding_width = param->fractional_bits + param->integer_bits;
  param->lower_bound = floorl(ldexpl(param->min, param->fractional_bits));
  param->upper_bound = ceill(ldexpl(param->max, param->fractional_bits));
  if(param->fixed_encoding_width > 64) {
    printf("ERROR: fixed_encoding_width > 64\n");
    return false;
  }
  if(param->fixed_encoding_width < 8) { // could disable this lower bound to support packed formats
    param->fixed_encoding_width = 8;
  } else {
    param->fixed_encoding_width = 1 << int_log2(param->fixed_encoding_width);
  }
  param->offset = param->lower_bound;
  param->large_offset = false;
  if(param->offset > (((int128_t)1) << 63) - 1 &&
     param->offset < -(((int128_t)1) << 63)) {
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

#define max(x, y) ((y) > (x) ? (y) : (x))

void convert_to_double(struct parameters *param) {
  int128_t
    lb = param->lower_bound - param->offset,
    ub = param->upper_bound - param->offset;
  printf("#include <math.h>\n"
         "#include <stdint.h>\n"
         "// can lose precision, for display only\n"
         "double convert_to_double(%s%d_t x) {\n",
         param->use_signed ? "int" : "uint", param->fixed_encoding_width);
  if(param->use_signed || lb) {
    printf("  if(x >= %1$s%2$d_C(%3$lld) &&\n"
           "     x <= %1$s%2$d_C(%4$lld)) {\n",
           param->use_signed ? "INT" : "UINT",
           param->fixed_encoding_width,
           (long long int)lb,
           (long long int)ub);
  } else {
    printf("  if(x <= %1$s%2$d_C(%3$lld)) {\n",
           param->use_signed ? "INT" : "UINT",
           param->fixed_encoding_width,
           (long long int)ub);
  }
  if(param->offset) {
    if(param->precision == 1.0L) {
      printf("    return ldexp(x, %d) + %Lf;\n",
             -param->fractional_bits,
             ldexpl(param->offset, -param->fractional_bits));
    } else {
      printf("    return fmax(%Lg, fmin(%Lg, round((ldexp(x, %d) + %Lg) * %Lg) * %Lg));\n",
             param->min,
             param->max,
             -param->fractional_bits,
             ldexpl(param->offset, -param->fractional_bits),
             1.0L / param->precision,
             param->precision);
    }
  } else {
    if(param->precision == 1.0L) {
      printf("    return ldexp(x, %d);\n",
             -param->fractional_bits);
    } else {
      printf("    return fmax(%Lg, fmin(%Lg, round((ldexp(x, %d) * %Lg) * %Lg)));\n",
             param->min,
             param->max,
             -param->fractional_bits,
             1.0L / param->precision,
             param->precision);
    }
  }
  printf("  } else {\n"
         "    // invalid input\n"
         "    return NAN;\n"
         "  }\n"
         "}\n");
}

void print_params(struct parameters *param) {
  printf("min: %Lg (%Lg)\n",
         ldexpl(param->lower_bound, -param->fractional_bits),
         param->min);
  printf("max: %Lg (%Lg)\n",
         ldexpl(param->upper_bound, -param->fractional_bits),
         param->max);
  long double actual_precision = ldexpl(1.0L, -param->fractional_bits);
  printf("precision: %Lg (%Lg)\n",
         actual_precision,
         param->precision);
  printf("code density: %.1Lf%%\n", 100L * actual_precision / param->precision);
  if(param->large_offset) {
    printf("offset: about %Lg\n", (long double)param->offset);
  } else {
    printf("offset: %" PRId64 "\n", (int64_t)param->offset);
  }
  printf("code range: [%" PRId64 ", %" PRId64 "]\n",
         (int64_t)(param->lower_bound - param->offset),
         (int64_t)(param->upper_bound - param->offset));
  printf("machine bit width: %d\n", param->fixed_encoding_width);
  printf("fractional bits: %d\n", param->fractional_bits);
  printf("integer bits: %d\n", param->integer_bits);
  printf("use signed: %s\n", param->use_signed ? "yes" : "no");
  printf("machine integer type: %s%d_t\n", param->use_signed ? "int" : "uint", param->fixed_encoding_width);
  printf("\n");
  convert_to_double(param);
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
    return 0;
  } else {
    return -1;
  }
}
