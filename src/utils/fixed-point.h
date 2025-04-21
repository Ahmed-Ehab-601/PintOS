#ifndef UTILS_FIXED_POINT_H
#define UTILS_FIXED_POINT_H

#include <stdint.h>
#include <stdio.h>

typedef int fixed_t;

// Number of fraction bits (q)
#define FP_SHIFT_AMOUNT 14
// f = 1 << q
#define F (1 << FP_SHIFT_AMOUNT)

// Convert integer n to fixed-point
#define INT_TO_FP(n) ((fixed_t)(n) * F)

// Convert fixed-point x to integer (rounding toward zero)
#define FP_TO_INT_ZERO(x) ((x) / F)

// Convert fixed-point x to integer (rounding to nearest)
#define FP_ROUND_TO_NEAREST_INT(x) ((x) >= 0 ? ((x) + F / 2) / F : ((x) - F / 2) / F)

// Add two fixed-point values
#define ADD_FP(x, y) ((x) + (y))

// Subtract fixed-point y from x
#define SUB_FP(x, y) ((x) - (y))

// Add fixed-point x and integer n
#define ADD_INT(x, n) ((x) + (n) * F)

// Subtract integer n from fixed-point x
#define SUB_INT(x, n) ((x) - (n) * F)

// Multiply two fixed-point values
#define MUL_FP(x, y) ((fixed_t)(((int64_t)(x)) * (y) / F))

// Multiply fixed-point x by integer n
#define MUL_INT(x, n) (x * n)

// Divide two fixed-point values
#define DIV_FP(x, y) ((fixed_t)(((int64_t)(x)) * F / (y)))

// Divide fixed-point x by integer n
#define DIV_INT(x, n) (x / n)


// char* fixed_t_to_string(fixed_t num) {
// 	static char buffer[35];
// 	int integer_part = FP_TO_INT_ZERO(num);
// 	int abs_num = num >= 0 ? num : -num;
// 	int fraction_part = (abs_num % F) * 100 / F;

// 	if (num < 0 && integer_part == 0)
// 		snprintf(buffer, sizeof(buffer), "-0.%02d", fraction_part);
// 	else
// 		snprintf(buffer, sizeof(buffer), "%d.%02d", integer_part, fraction_part);

// 	return buffer;
// }


#endif /* threads/fixed_point.h */