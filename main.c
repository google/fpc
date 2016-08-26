#include <string.h>
#include <stdio.h>
#include <math.h>
#include <inttypes.h>

#include "fpc.h"

#define max(x, y) ((y) > (x) ? (y) : (x))

static
void convert_to_double(struct fpc_parameters *param, FILE *f) {
#define printf(...) fprintf(f, __VA_ARGS__)
  int128_t
    lb = param->lower_bound - param->offset,
    ub = param->upper_bound - param->offset,
    min_int = param->use_signed ? -(1 << (param->fixed_encoding_width - 1)) : 0,
    max_int = (1 << (param->fixed_encoding_width - (param->use_signed ? 1 : 0))) - 1;
  printf("#include <math.h>\n"
         "#include <stdint.h>\n"
         "// can lose precision, for display only\n"
         "double convert_to_double(%s%d_t x) {\n",
         param->use_signed ? "int" : "uint", param->fixed_encoding_width);

  // Check bounds
  if(lb != min_int || ub != max_int) {
    printf("  if(");
    if(lb != min_int) {
      printf("x < %s%d_C(%lld)",
             param->use_signed ? "INT" : "UINT",
             param->fixed_encoding_width,
             (long long int)lb);
      if(ub != max_int) printf(" &&\n     ");
    }
    if(ub != max_int) {
      printf("x > %s%d_C(%lld)",
             param->use_signed ? "INT" : "UINT",
             param->fixed_encoding_width,
             (long long int)ub);
    }
    printf(") {\n"
      "    return NAN;\n"
      "  }\n");
  }

  printf("  return ");
  if(param->precision != 1.0l) {
    printf("round(");
  }
  if(param->offset) printf("(");
  if(param->fractional_bits) {
    printf("ldexp(x, %d)", -param->fractional_bits);
  } else {
    printf("x");
  }
  if(param->offset) {
    printf(" + %.19Lg)", ldexpl(param->offset, -param->fractional_bits));
  }
  if(param->precision != 1.0L) {
    printf(" * %.19Lg) * %.19Lg",
           1.0L / param->precision,
           param->precision);
  }
  printf(";\n");

  printf("}\n");
#undef printf
}

static
void print_params(struct fpc_parameters *param) {
  printf("[PARAMETERS]\n");
  printf("  min: %.19Lg (%.19Lg requested)\n",
         ldexpl(param->lower_bound, -param->fractional_bits),
         param->min);
  printf("  max: %.19Lg (%.19Lg requested)\n",
         ldexpl(param->upper_bound, -param->fractional_bits),
         param->max);
  long double actual_precision = ldexpl(1.0L, -param->fractional_bits);
  printf("  precision: %.19Lg (%.19Lg requested)\n",
         actual_precision,
         param->precision);
  printf("\n[CODE]\n");
  printf("  code density: %.1Lf%%\n", 100L * actual_precision / param->precision);
  if(param->large_offset) {
    printf("  offset: about %.19Lg\n", (long double)param->offset);
  } else {
    printf("  offset: %" PRId64 "\n", (int64_t)param->offset);
  }
  printf("  code range: [%" PRId64 ", %" PRId64 "]\n",
         (int64_t)(param->lower_bound - param->offset),
         (int64_t)(param->upper_bound - param->offset));
  printf("\n[ENCODING]\n");
  printf("  machine bit width: %d (%d used)\n", param->fixed_encoding_width, param->integer_bits + param->fractional_bits);
  printf("    fractional bits: %d\n", param->fractional_bits);
  printf("    integer bits: %d\n", param->integer_bits);
  printf("  use signed: %s\n", param->use_signed ? "yes" : "no");
  printf("  machine integer type: %s%d_t\n", param->use_signed ? "int" : "uint", param->fixed_encoding_width);
  printf("  Q notation: Q%c%d.%d\n", param->use_signed ? 's' : 'u', param->fixed_encoding_width - param->fractional_bits - (param->use_signed ? 1 : 0), param->fractional_bits);
  printf("\n[CONVERSION]\n");
  convert_to_double(param, stdout);
}

static
void gen_converter(struct fpc_parameters *param) {
  FILE *f = fopen("convert.c", "w");
  fprintf(f,
          "#include <stdio.h>\n"
          "#include <stdlib.h>\n\n");
  convert_to_double(param, f);
  fprintf(f,
          "\n"
          "int main(int argc, char **argv) {\n"
          "  int i;\n"
          "  argv++; argc--; // skip first arg\n"
          "  for(i = 0; i < argc; i++) {\n"
          "    long int x = strtol(argv[i], NULL, 10);\n"
          "    printf(\"%%ld -> %%.15g\\n\", x, convert_to_double(x));\n"
          "  }\n"
          "  return 0;\n"
          "}\n");
  fclose(f);
}

int main(int argc, char **argv) {
  struct fpc_parameters param;
  memset(&param, 0, sizeof(param));
  bool gen = false;

  if(argc == 2) {
    // simple expression evaluator
    printf("%.19Lg\n", fpc_eval_expr(argv[1]));
    return 0;
  }

  if(argc <= 3) {
    printf("fpc [min] [max] [precision]\n");
    return -1;
  }

  if(strcmp(argv[1], "-g") == 0) {
    gen = true;
    argv++;
  }

  if(fpc_calculate_from_strings(argv[1], argv[2], argv[3], &param)) {
    print_params(&param);
    if(gen) gen_converter(&param);
    return 0;
  } else {
    fprintf(stderr, "ERROR: %s\n", param.error);
    return -1;
  }
}
