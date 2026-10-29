typedef int float_type;

#define FLOAT_SHIFT_AMOUNT 15

#define FLOAT_CONST(A) ((float_type)(A << FLOAT_SHIFT_AMOUNT))

#define FLOAT_ADD(A,B) (A + B)

#define FLOAT_ADD_MIX(A,B) (A + (B << FLOAT_SHIFT_AMOUNT))

#define FLOAT_SUB(A,B) (A - B)

#define FLOAT_SUB_MIX(A,B) (A - (B << FLOAT_SHIFT_AMOUNT))

#define FLOAT_MULT_MIX(A,B) (A * B)

#define FLOAT_DIV_MIX(A,B) (A / B)

#define FLOAT_MULT(A,B) ((float_type)(((int64_t) A) * B >> FLOAT_SHIFT_AMOUNT))

#define FLOAT_DIV(A,B) ((float_type)((((int64_t) A) << FLOAT_SHIFT_AMOUNT) / B))

#define FLOAT_INT_PART(A) (A >> FLOAT_SHIFT_AMOUNT)

#define FLOAT_ROUND(A) (A >= 0 ? ((A + (1 << (FLOAT_SHIFT_AMOUNT - 1))) >> FLOAT_SHIFT_AMOUNT) \
        : ((A - (1 << (FLOAT_SHIFT_AMOUNT - 1))) >> FLOAT_SHIFT_AMOUNT))

