%{
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <assert.h>
#include <inttypes.h>
#include "str.h"
#include "diceexpr.h"
#include "numflow.h"

int yylex();
void yyerror(const char *s);
// Set dice expression as input for lexer. Can't be NULL.
void set_scan_string(const char *expr);
// Free lexer's buffer.
void delete_buffer();
static enum parse_error roll(int_least64_t nrolls,
                             int_least64_t dice,
                             int_least64_t small,
                             int_least64_t large,
                             int_least64_t *sum);
static int sort_ascending(const void *a, const void *b);

// Number of smallest and largest rolls to ignore.
static int_least64_t ignore_small, ignore_large;
// Dice expression after dices are rolled.
static str *rolled_expr;
// Evaluated result of a dice expression.
static int_least64_t result;
// Parser error.
static enum parse_error parse_error;
%}

%code requires { #define YYSTYPE int_least64_t }

%token INTEGER
%token INVALID_CHARACTER OVERFLOW

%left '+' '-'
%nonassoc 'd'
%right '<' '>'

%nonassoc IGNORE_EMPTY
%nonassoc UMINUS UPLUS

%%

parse:
    expr { result = $1; }
    ;

expr:
    INVALID_CHARACTER {
        parse_error = DE_INVALID_CHARACTER;
        YYERROR;
    }

    | OVERFLOW {
        parse_error = DE_OVERFLOW;
        YYERROR;
    }

    | INTEGER {
        if (str_append_format(rolled_expr, "%" PRIdLEAST64, $1) != 0) {
            parse_error = DE_MEMORY;
            YYERROR;
        }
        $$ = $1;
    }

    | '-' {
        if (str_append_char(rolled_expr, '-')) {
            parse_error = DE_MEMORY;
            YYERROR;
        }
    } expr %prec UMINUS  {
        enum flow_type overflow;
        NF_MINUS(0, $3, INT_LEAST64, overflow);
        if (overflow != 0) {
            parse_error = DE_OVERFLOW;
            YYERROR;
        }
        $$ = -$3;
    }

    | '+' {
        if (str_append_char(rolled_expr, '+')) {
            parse_error = DE_MEMORY;
            YYERROR;
        }
    } expr %prec UPLUS { $$ = $3; }

    | expr '-' {
        if (str_append_char(rolled_expr, '-')) {
            parse_error = DE_MEMORY;
            YYERROR;
        }
    } expr {
        enum flow_type overflow;
        NF_MINUS($1, $4, INT_LEAST64, overflow);
        if (overflow != 0) {
            parse_error = DE_OVERFLOW;
            YYERROR;
        }
        $$ = $1 - $4;
    }

    | expr '+' {
        if (str_append_char(rolled_expr, '+')) {
            parse_error = DE_MEMORY;
            YYERROR;
        }
    } expr {
        enum flow_type overflow;
        NF_PLUS($1, $4, INT_LEAST64, overflow);
        if (overflow != 0) {
            parse_error = DE_OVERFLOW;
            YYERROR;
        }
        $$ = $1 + $4;
    }

    | maybe_int 'd' INTEGER ignore_list {
        int_least64_t sum;
        enum parse_error e =
            roll($1, $3, ignore_small, ignore_large, &sum);
        if (e != 0) {
            parse_error = e;
            YYERROR;
        }
        $$ = sum;
        ignore_small = 0;
        ignore_large = 0;
    }
    ;

maybe_int:
    INTEGER { $$ = $1; }
    /* If there's no number before 'd', roll the dice one time. */
    |       { $$ = 1; }
    ;

ignore_list:
    ignore
    | ignore_list ignore
    | %prec IGNORE_EMPTY
    ;

ignore:
    '<' {
        enum flow_type overflow;
        NF_PLUS(ignore_small, 1, INT_LEAST64, overflow);
        if (overflow != 0) {
            parse_error = DE_OVERFLOW;
            YYERROR;
        }
        ignore_small++;
    }

    | '>' {
        enum flow_type overflow;
        NF_PLUS(ignore_large, 1, INT_LEAST64, overflow);
        if (overflow != 0) {
            parse_error = DE_OVERFLOW;
            YYERROR;
        }
        ignore_large++;
    }

    | '<' INTEGER {
        enum flow_type overflow;
        NF_PLUS(ignore_small, $2, INT_LEAST64, overflow);
        if (overflow != 0) {
            parse_error = DE_OVERFLOW;
            YYERROR;
        }
        ignore_small += $2;
    }

    | '>' INTEGER {
        enum flow_type overflow;
        NF_PLUS(ignore_large, $2, INT_LEAST64, overflow);
        if (overflow != 0) {
            parse_error = DE_OVERFLOW;
            YYERROR;
        }
        ignore_large += $2;
    }
    ;

%%

enum parse_error
de_parse(const char *expr, int_least64_t *value, char **rolled_expression) {
    assert(expr != NULL);
    assert(*rolled_expression == NULL);

    if ((rolled_expr = str_new(NULL)) == NULL)
        return DE_MEMORY;

    enum parse_error retval = 0;

    set_scan_string(expr);
    int parse_retval = yyparse();
    // Any other error than bison's memory error.
    if (parse_retval == 1) {
        // If parse_error is set, then it's some other error than syntax error.
        retval = parse_error == 0 ? DE_SYNTAX_ERROR : parse_error;
        goto end;
    }
    else if (parse_retval == 2) {
        retval = DE_MEMORY;
        goto end;
    }
    *value = result;
    if (str_copy_to_chars(rolled_expr, rolled_expression) != 0)
        retval = DE_MEMORY;

    end:
        delete_buffer();
        yylex_destroy();
        str_free(rolled_expr);
        // Initialize all file globals for the next call.
        rolled_expr = NULL;
        ignore_small = 0;
        ignore_large = 0;
        parse_error = 0;
        result = 0;

    return retval;
}

/* Roll a dice.
 * Arguments must satisfy: ignore_small + ignore_large < nrolls.
 * @param nrolls Number of rolls for a dice. Must be > 0.
 * @param dice Number of sides in a dice. Must be > 0.
 * @param small Ignore this many smallest rolls.
 * @param large Ignore this many largest rolls.
 * @param dice_sum Sum of dices rolled.
 * @return Zero on success, enum parse_error otherwise.
 */
static enum parse_error
roll(int_least64_t nrolls,
     int_least64_t dice,
     int_least64_t small,
     int_least64_t large,
     int_least64_t *dice_sum) {
    if (nrolls <= 0)
        return DE_NROLLS;
    if (dice <= 0)
        return DE_DICE;
    if (small + large >= nrolls)
        return DE_IGNORE;

    int_least64_t *rolls = malloc(nrolls * sizeof(*rolls));
    if (rolls == NULL)
        return DE_MEMORY;

    for (int_least64_t i = 0; i < nrolls; i++)
        rolls[i] = (int_least64_t) (rand() / (double) RAND_MAX * dice + 1);

    qsort(rolls, nrolls, sizeof(int_least64_t), sort_ascending);

    int retval = 0;
    if (str_append_char(rolled_expr, '(') != 0) {
        retval = DE_MEMORY;
        goto free;
    }

    int_least64_t sum = 0;
    int_least64_t nth_included_roll = 0;
    for (int_least64_t i = small; i < nrolls - large; i++, nth_included_roll++) {
        enum flow_type overflow;
        NF_PLUS(sum, rolls[i], INT_LEAST64, overflow);
        if (overflow != 0)
            return DE_OVERFLOW;
        sum += rolls[i];

        const char *format_with_plus_or_not =
            nth_included_roll > 0 && nth_included_roll < nrolls - large ?
                "+%" PRIdLEAST64 : "%" PRIdLEAST64;
        if (str_append_format(rolled_expr, format_with_plus_or_not, rolls[i])
            != 0) {
            retval = DE_MEMORY;
            goto free;
        }
    }

    if (str_append_char(rolled_expr, ')') != 0) {
        retval = DE_MEMORY;
        goto free;
    }

    *dice_sum = sum;

    free:
        free(rolls);

    return retval;
}

static int
sort_ascending(const void *a, const void *b) {
    const int_least64_t *x = a;
    const int_least64_t *y = b;

    if (*x < *y)  return -1;
    if (*x == *y) return 0;
    else          return 1;
}

// Empty, because on syntax error we don't want to print anything.
void
yyerror(const char *s) { }
