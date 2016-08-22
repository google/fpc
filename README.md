# Fixed-point Calculator

This program takes a range [min, max] and precision and calculates constants and information to be used in code performing calculations in this range using fixed-point integer math.

# Try it!

    $ git clone ssh://git@stash.nestlabs.com:7999/user/fpc.git
    $ make
    $ ./fpc 30 1800 0.1
    min: 30.00 (30.00)
    max: 1800.00 (1800.00)
    precision: 0.06 (0.10)
    code density: 62.5%
    offset: 0
    code range: [480, 28800]
    machine bit width: 16
    fractional bits: 4
    integer bits: 11
    use signed: no
    machine integer type: uint16_t
    
    #include <math.h>
    #include <stdint.h>
    // can lose precision, for display only
    double convert_to_double(uint16_t x) {
      if(x >= UINT16_C(480) &&
         x <= UINT16_C(28800)) {
        return fmax(30.00, fmin(1800.00, round((ldexp(x, -4) * 10.00) * 0.10)));
      } else {
        // invalid input
        return NAN;
      }
    }