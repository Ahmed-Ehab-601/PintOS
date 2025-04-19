#include "fixed-point.h"
#include <stdio.h>


int main() {
	fixed_t a = INT_TO_FP(5);
	fixed_t b = INT_TO_FP(2);
	fixed_t c = INT_TO_FP(-3);

	printf("a = %s\n", fixed_t_to_string(a));
	printf("b = %s\n", fixed_t_to_string(b));
	printf("c = %s\n", fixed_t_to_string(c));

	printf("\nAddition (a + b): %s\n", fixed_t_to_string(ADD_FP(a, b)));
	printf("Subtraction (a - c): %s\n", fixed_t_to_string(SUB_FP(a, c)));
	printf("Addition with int (a + 7): %s\n", fixed_t_to_string(ADD_MIX(a, 7)));
	printf("Multiplication (a * b): %s\n", fixed_t_to_string(MUL_FP(a, b)));
	printf("Division (a / b): %s\n", fixed_t_to_string(DIV_FP(a, b)));
	printf("Multiplication with int (b * 4): %s\n", fixed_t_to_string(MUL_MIX(b, 4)));
	printf("Division with int (a / 2): %s\n", fixed_t_to_string(DIV_MIX(a, 2)));

	return 0;
}
