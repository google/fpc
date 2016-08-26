// Dustin DeWeese (deweese@nestlabs.com)
// 2016

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "fpc.h"

static
int floor_log2l(long double x) {
  int exp;
  frexpl(x, &exp);
  return exp - 1;
}

static
unsigned int int_log2(unsigned int x) {
  return x <= 1 ? 0 : (8 * sizeof(x)) - __builtin_clz(x - 1);
}

static
int clz128(int128_t x) {
  unsigned int y;
  unsigned int shift = sizeof(y) * 8;
  int i, n = sizeof(x) / sizeof(y);
  for(i = 0; i < n; i++) {
    y = x >> (shift * (n - 1 - i));
    if(y) return __builtin_clz(y) + shift * i;
  }
  return shift * n;
}

static
unsigned int int128_log2(int128_t x) {
  return x <= 1 ? 0 : 128 - clz128(x - 1);
}

bool fpc_calculate(struct fpc_parameters *param) {
  if(param->max < param->min + param->precision) {
    param->error = "max < min + precision";
    return false;
  }
  if(param->precision <= 0.0L) {
    param->error = "zero or negative precision";
    return false;
  }
  param->fractional_bits = -floor_log2l(param->precision);
  param->lower_bound = ceill(ldexpl(param->min - param->precision / 2, param->fractional_bits));
  param->upper_bound = floorl(ldexpl(param->max + param->precision / 2, param->fractional_bits));

  param->fixed_encoding_width = int128_log2(param->upper_bound - param->lower_bound + 1);
  param->integer_bits = param->fixed_encoding_width - param->fractional_bits;

  if(param->fixed_encoding_width > 64) {
    param->error = "fixed_encoding_width > 64";
    return false;
  }
  if(param->fixed_encoding_width < 8) { // could disable this lower bound to support packed formats
    param->fixed_encoding_width = 8;
  } else {
    param->fixed_encoding_width = 1 << int_log2(param->fixed_encoding_width);
  }
  param->offset = param->lower_bound;
  param->large_offset = false;
  if(param->offset > (((int128_t)1) << 63) - 1 ||
     param->offset < -(((int128_t)1) << 63)) {
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

static const char *ops = "+a-a*b/b^c)";

static
const char *get_op(char *cp, char **cpp) {
  char c = *cp;
  const char *op = ops;
  while(*op) {
    if(*op == c) {
      if(cpp) *cpp = cp + 1;
      return op;
    }
    op += 2;
  }
  // return default (+)
  return ops;
}

static char vars[16];
static long double values[sizeof(vars)];
static unsigned int n_vars = 0;

void fpc_set_var(char c, long double x) {
  if(n_vars < sizeof(vars)) {
    int n = n_vars++;
    vars[n] = c;
    values[n] = x;
  }
}

static
long double parse_num(char **pstr) {
  char *p = *pstr;
  while(*p == ' ') p++;
  if(*p == '(') {
    *pstr = p + 1;
    return fpc_eval_expr(pstr);
  } else {
    char *v = strchr(vars, *p);
    if(v) {
      *pstr = p + 1;
      return values[v - vars];
    } else {
      long double x = strtold(*pstr, pstr);
      if(*pstr == p) {
        *pstr = p + 1;
        x = NAN;
      }
      return x;
    }
  }
}

static
long double _eval_expr(char **pstr, long double x, char prec) {
  char *p = *pstr;
  const char *next_op;
  do {
    const char *op = get_op(p, &p);
    long double y = parse_num(&p);
    while(*p == ' ') p++;
    next_op = get_op(p, NULL);
    if(next_op[1] > op[1]) y = _eval_expr(&p, y, op[1]);
    switch(*op) {
    case '+': x += y; break;
    case '-': x -= y; break;
    case '*': x *= y; break;
    case '/': x /= y; break;
    case '^': x = powl(x, y); break;
    case ')': break;
    default:
      x = NAN;
      goto end;
    }
  } while(*p && next_op[1] > prec);
end:
  *pstr = p;
  return x;
}

long double fpc_eval_expr(char **pstr) {
  long double x = _eval_expr(pstr, 0.0L, 0);
  if(**pstr == ')') (*pstr)++;
  return x;
}

bool fpc_calculate_from_strings(char *min,
                                char *max,
                                char *precision,
                                struct fpc_parameters *param) {
  param->precision = fpc_eval_expr(&precision);
  fpc_set_var('p', param->precision);
  char *try = min;
  param->min = fpc_eval_expr(&try);
  fpc_set_var('l', param->min);
  param->max = fpc_eval_expr(&max);
  fpc_set_var('h', param->max);
  // evaluate again for dependency on 'h'
  if(isnan(param->min)) param->min = fpc_eval_expr(&min);
  return fpc_calculate(param);
}
