#include <stdint.h>

typedef int float_type;

#define FLOAT_FACTOR 0b100000000000000 //2^14

#define FLOAT_FROM_INT(N) ((float_type)(N * FLOAT_FACTOR))

#define FLOAT_TO_INT_ROUND_ZERO(X) (X/FLOAT_FACTOR)

#define FLOAT_TO_INT_ROUND_NEAREST(X) ( X >= 0 ? ((X + (FLOAT_FACTOR/2))/FLOAT_FACTOR) : ((X - (FLOAT_FACTOR/2))/FLOAT_FACTOR) )

#define FLOAT_ADD_FF(X, Y) (X + Y)

#define FLOAT_SUB_FF(X, Y) (X - Y)

#define FLOAT_ADD_FI(X, N) (X + N*FLOAT_FACTOR)

#define FLOAT_SUB_FI(X, N) (X - N*FLOAT_FACTOR)

#define FLOAT_MULT_FF(X, Y) ((float_type)(((int64_t) X) * Y / FLOAT_FACTOR))

#define FLOAT_DIV_FF(X, Y) ((float_type)(((int64_t) X) * FLOAT_FACTOR / Y))

#define FLOAT_MULT_FI(X, N) (X * N)

#define FLOAT_DIV_FI(X, N) (X / N)