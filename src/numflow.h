#ifndef NUMFLOW_H
#define NUMFLOW_H

/** @file
 * @brief Integer overflow checking for arithmetic operations.
 *
 * @description All macros take two operands as parameters for which an
 * operation is tried if it overflows or underflows. Addiotionally @a type
 * must be passed. It should be the constant, defined in limits.h or in
 * stdint.h, of the type of the operands, without the _MAX or _MIN suffix.
 * E.g. INT, USHRT, INT_LEAST64 etc.
 * The last parameter is of type enum flow_type. It will be set to zero
 * if operation is safe and non-zero otherwise.
 *
 * Example use:
 * @code
 * enum flow_type error;
 * int a = 1;
 * int b = 2;
 * NF_PLUS(a, b, INT, error);
 * if (error == 0)
 *     // Safe to add a and b.
 * else if (error == NF_UNDERFLOW)
 *     // a + b underflows.
 * else
 *     // a + b overflows.
 * @endcode
 */

#include <stdint.h>
#include <limits.h>

/** @enum flow_type
 * A type for @a error parameter for macros to check after call whether
 * operation overflows or underflows.
 */
enum flow_type {
    NF_UNDERFLOW = 1, NF_OVERFLOW
};

/** Check whether @a first + @a second overflows or underflows for signed
 * integers.
 * Order of @a first and @a second doesn't matter. If @a first and @a second
 * can be added safely @a error is zero, non-zero otherwise.
 * @param first
 * @param second
 * @param type
 * @param error 
 */
#define NF_PLUS(first, second, type, error) { \
    error = 0;                                \
    if ((first) > 0 && (second) > 0) {        \
        if (type##_MAX - (second) < (first))  \
            error = NF_OVERFLOW;              \
    }                                         \
    else if ((first) < 0 && (second) < 0) {   \
        if (type##_MIN - (second) > (first))  \
            error = NF_UNDERFLOW;             \
    }                                         \
}

/** Check whether @a first - @a second overflows or underflows for signed
 * integers.
 * Order of @a first and @a second matters. If @a first and @a second can be
 * subtracted safely @a error is zero, non-zero otherwise.
 * @param first
 * @param second
 * @param type
 * @param error 
 */
#define NF_MINUS(first, second, type, error) { \
    error = 0;                                 \
    if ((first) > 0 && (second) < 0) {         \
        if (type##_MAX + (second) < (first))   \
            error = NF_OVERFLOW;               \
    }                                          \
    else if ((first) < 0 && (second) > 0) {    \
        if (type##_MIN + (second) > (first))   \
            error = NF_UNDERFLOW;              \
    }                                          \
}

/** Check whether @a first * @a second overflows or underflows for signed
 * integers.
 * Order of @a first and @a second doesn't matter. If @a first and @a second can
 * be multiplied safely @a error is zero, non-zero otherwise.
 * @param first
 * @param second
 * @param type
 * @param error 
 */
#define NF_MULTIPLY(first, second, type, error) { \
    error = 0;                                    \
    if ((first) > 0 && (second) > 0) {            \
        if (type##_MAX / (second) < (first))      \
            error = NF_OVERFLOW;                  \
    }                                             \
    else if ((first) < 0 && (second) < 0) {       \
        if (type##_MAX / (second) > (first))      \
            error = NF_OVERFLOW;                  \
    }                                             \
    else if ((first) < 0 && (second) > 0) {       \
        if (type##_MIN / (second) > (first))      \
            error = NF_UNDERFLOW;                 \
    }                                             \
    else if ((first) > 0 && (second) < 0) {       \
        if (type##_MIN / (second) < (first))      \
            error = NF_UNDERFLOW;                 \
    }                                             \
}

/** Check whether @a first + @a second overflows for unsigned integers.
 * Order of @a first and @a second doesn't matter. If @a first and @a second
 * can be added safely @a error is zero, non-zero otherwise.
 * @param first
 * @param second
 * @param type
 * @param error 
 */
#define NF_UPLUS(first, second, type, error) { \
    error = 0;                                 \
    if (type##_MAX - (second) < (first))       \
        error = NF_OVERFLOW;                   \
}

/** Check whether @a first - @a second underflows for unsigned integers.
 * Order of @a first and @a second matters. If @a first and @a second can be
 * subtracted safely @a error is zero, non-zero otherwise.
 * @param first
 * @param second
 * @param type
 * @param error 
 */
#define NF_UMINUS(first, second, type, error) { \
    error = 0;                                  \
    if ((first) < (second))                     \
        error = NF_UNDERFLOW;                   \
}

/** Check whether @a first * @a second overflows for unsigned integers.
 * Order of @a first and @a second doesn't matter. If @a first and @a second
 * can be multiplied safely @a error is zero, non-zero otherwise.
 * @param first
 * @param second
 * @param type
 * @param error 
 */
#define NF_UMULTIPLY(first, second, type, error) { \
    error = 0;                                     \
    if ((second) > 0) {                            \
        if (type##_MAX / (second) < (first))       \
            error = NF_OVERFLOW;                   \
    }                                              \
}
#endif // NUMFLOW_H
