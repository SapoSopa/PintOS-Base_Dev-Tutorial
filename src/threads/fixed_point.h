#ifndef THREADS_FIXED_POINT_H
#define THREADS_FIXED_POINT_H

#include <stdint.h>

/* Nosso Fator de Ponto Fixo (16384) */
#define F (1 << 14)

/* 
 * Usamos funções static inline em vez de macros para permitir o uso 
 * de estruturas de controle (if/else) e garantir verificação de tipos.
 */

/* Converte um inteiro normal para ponto fixo */
static inline int int_to_fp(int n) {
    return n * F;
}

/* Converte ponto fixo para inteiro (trunca em direção a zero) */
static inline int fp_to_int_zero(int x) {
    return x / F;
}

/* Converte ponto fixo para inteiro (arredonda para o mais próximo) */
static inline int fp_to_int_near(int x) {
    if (x >= 0) {
        return (x + (F / 2)) / F;
    } else {
        return (x - (F / 2)) / F;
    }
}

/* Soma de dois números em ponto fixo */
static inline int add_fp(int x, int y) {
    return x + y;
}

/* Subtração de dois números em ponto fixo */
static inline int sub_fp(int x, int y) {
    return x - y;
}

/* Soma de um ponto fixo (x) com um inteiro normal (n) */
static inline int add_mix(int x, int n) {
    return x + (n * F);
}

/* Subtração de um ponto fixo (x) por um inteiro normal (n) */
static inline int sub_mix(int x, int n) {
    return x - (n * F);
}

/* Multiplicação de dois números em ponto fixo (Exige int64_t para evitar overflow) */
static inline int mult_fp(int x, int y) {
    return (int) ((((int64_t) x) * y) / F);
}

/* Multiplicação de um ponto fixo (x) por um inteiro normal (n) */
static inline int mult_mix(int x, int n) {
    return x * n;
}

/* Divisão de dois números em ponto fixo (Exige int64_t para evitar overflow) */
static inline int div_fp(int x, int y) {
    return (int) ((((int64_t) x) * F) / y);
}

/* Divisão de um ponto fixo (x) por um inteiro normal (n) */
static inline int div_mix(int x, int n) {
    return x / n;
}

#endif /* threads/fixed_point.h */