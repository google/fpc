/* Copyright 2016 Google Inc.
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

#ifndef __FPC__
#define __FPC__

#include <stdbool.h>

typedef __int128_t int128_t;

/* data structure for holding parameters and calculated values */
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

/* given an fpc_parameters struct with min, max, precision,
   calculate the other members */
bool fpc_calculate(struct fpc_parameters *param);

/* a simple expression evaluator
   supports (in order of precedence)
    - parenthesis: `(x)`
    - exponentiation: `x ^ y`
    - multiply: `x * y`, divide: `x / y`
    - addition: `x + y`, subtraction: `x - y`
    add variables with fpc_set_var()
*/
long double fpc_eval_expr(char *str);

/* set a single letter variable for use in fpc_eval_expr() expressions */
void fpc_set_var(char c, long double x);

/* get a variable */
long double *fpc_get_var(char c);

/* alternative to fpc_calculate that takes string expressions
   defines the following variables:
   - l = min
   - h = max
   - p = precision
*/
bool fpc_calculate_from_strings(char *min,
                                char *max,
                                char *precision,
                                struct fpc_parameters *param);


#endif
