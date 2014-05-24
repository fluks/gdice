#ifndef DICEEXPR_H
    #define DICEEXPR_H

/** @file
 *
 * @description A function for dice expressions. A dice expression consists of
 * dice rolls, possibly ignoring some of those rolls and constant modifiers.
 *
 * Grammar for dice expression.
 * s      ::= expr
 * expr   ::= INTEGER | ('-'|'+') expr | expr '-' expr | expr '+' expr |
              [INTEGER] ('d'|'D') INTEGER ignore
 * ignore ::= ('<' | '>' [INTEGER])*
 */

#include <stdint.h>
/** @enum parse_error de_parse() return values on error.
 */
enum parse_error {
    DE_MEMORY = 1,
	DE_INVALID_CHARACTER,
	DE_SYNTAX_ERROR,
    DE_NROLLS,              // Number of rolls is not positive.
    DE_DICE,                // Number of sides for a dice is not positive.
    DE_IGNORE,              // Number of ignores for a dice is too large.
    DE_OVERFLOW             // Integer overflow.
};

/** Parse dice expression.
 * Caller must call srand() once before using this function. Memory for
 * rolled_expression is allocated, caller should free it.
 * @param expr Dice expression, can't be NULL.
 * @param value Used to store evaluated value.
 * @param rolled_expr Used to store dice expression after rolling dices.
 * @return Zero on success, enum parse_error otherwise.
 */
enum parse_error
de_parse(const char *expr, int_least64_t *value, char **rolled_expression);

#endif
