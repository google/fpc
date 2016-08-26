#ifndef __FPC__
#define __FPC__

#include <stdbool.h>

typedef __int128_t int128_t;

struct fpc_parameters {
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

  const char *error;
};

bool fpc_calculate(struct fpc_parameters *param);
long double fpc_eval_expr(char **pstr);
bool fpc_calculate_from_strings(char *min,
                                char *max,
                                char *precision,
                                struct fpc_parameters *param);


#endif
