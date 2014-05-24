%option noyywrap nounput noinput

%{
#include <assert.h>
#include <inttypes.h>
#include <errno.h>
#include "de.tab.h"
int read_int();
%}

%%

[0-9]           return read_int() != 0 ? OVERFLOW : INTEGER; 
[1-9][0-9]+     return read_int() != 0 ? OVERFLOW : INTEGER; 
[-+d<>]         return *yytext;
D               return 'd';
[ \t\n]         ;
.               { return INVALID_CHARACTER;}

%%

void
set_scan_string(const char *expr) {
    assert(expr != NULL);

    yy_scan_string(expr); 
}

void
delete_buffer() {
    yy_delete_buffer(YY_CURRENT_BUFFER);
}

int
read_int() {
    errno = 0;
    yylval = strtoimax(yytext, NULL, 10);
    return errno == ERANGE ? 1 : 0;
}
