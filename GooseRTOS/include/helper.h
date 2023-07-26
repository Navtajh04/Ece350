#ifndef RTX_APP_SRC_HELPER_INCLUDE_
#define RTX_APP_SRC_HELPER_INCLUDE_

/**
 * @brief efficiently computes the log base 2 (position of the highest bit set) of a number rounding down
 * 
 * @param num number that the function will take the log base 2 of
 * 
 * @return unsigned int - the log base 2 of num
*/
unsigned int log_two_floor(unsigned int num);

/**
 * @brief efficiently computes the log base 2 (position of the highest bit set) of a number rounding up
 * 
 * @param num number that the function will take the log base 2 of
 * 
 * @return unsigned int - the log base 2 of num
*/
unsigned int log_two_ceil(unsigned int num);
#endif /* RTX_APP_SRC_HELPER_INCLUDE_ */
