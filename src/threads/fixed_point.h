/* biblioteca aritmetica pra ponto fixo que o mlfq precisa
    pintOS não tem suporte a float dentro do kernel, então essa bib
    serve pra realizar os cálculos envolvendo */

#ifndef THREADS_FIXED_POINT_H
#define THREADS_FIXED_POINT_H

typedef int fixed_point; //alias

//17 bits pra parte inteira e 14 bits pra parte fracionária
#define F (1<<14) /*escala 17.14 -- para realizar operações de priority do advenced scheduler*/

//conversao de int pra ponto fixo
#define INT_FP(n)     ((n)*F)

//conversao de ponto fixo pra inteiro (truncado)
#define FP_INT(x)     ((x)/F)

//conversao de ponto fixo pra inteiro (arredondado)
#define FP_INT_ROUND(x)    ((x)>=0?((x)+F/2)/F:((x)-F/2)/F)

//OPERACOES ARITMETICAS
/* Adição e subtração */
#define FP_ADD(x, y)       ((x) + (y)) //soma dois fixed_point
#define FP_SUB(x, y)       ((x) - (y))
#define FP_ADD_INT(x, n)   ((x) + (n) * F) //soma um fixed_point com um inteiro
#define FP_SUB_INT(x, n)   ((x) - (n) * F)

/* Multiplicação e divisão */
#define FP_MUL(x, y)       ((fixed_point)(((int64_t)(x)) * (y) / F))
#define FP_DIV(x, y)       ((fixed_point)(((int64_t)(x)) * F / (y)))
#define FP_MUL_INT(x, n)   ((x) * (n))
#define FP_DIV_INT(x, n)   ((x) / (n))


#endif
