#ifndef STR_H
    #define STR_H
#include <stddef.h>

/** A string library.
 * The data of the strings are handled as bytes, so the length of a string may not
 * be the same as the number of characters in it.
 */

/** String struct.
 * Will be null terminated at all times.
 */
typedef struct {
    // Data.
    char *str;
    // Length of current string without '\0'.
    size_t len;
    // Amount of memory allocated.
    size_t size;
} str;

/** Create new str on the heap.
 * @param chars An initial string to copy to str, can be NULL.
 * @return New str or NULL if can't allocate memory.
 */
str*
str_new(const char *chars);

/** Free str type and its data.
 * @param s Can't be NULL.
 * @return void
 */
void
str_free(str *s);

/** Set length to zero.
 * @param s Can't be NULL.
 * @return void
 */
void
str_erase(str *s);

/** Append a character.
 * @param s Can't be NULL.
 * @param c
 * @return Zero on success, ENOMEM on error.
 */
int
str_append_char(str *s, int c);

/** Append a string.
 * @param s Can't be NULL.
 * @param chars String to append, can't be NULL.
 * @return Zero on success, ENOMEM on error.
 */
int
str_append_chars(str *s, const char *chars);

/** Append a string using format string.
 * @param s Can't be NULL.
 * @param format Format string, can't be NULL.
 * @param ... Variables for format string.
 * @return Zero on success, -1 on failure.
 */
int
str_append_format(str *s, const char *format, ...);

/** Copy str's data to a string.
 * Memory for chars is allocated, so free it after use. chars will be
 * s->len + 1 of size. chars is nul terminated.
 * @param s Can't be NULL.
 * @param chars String to copy to, must point to NULL.
 * @return Zero on success, ENOMEM on error.
 */
int
str_copy_to_chars(str *s, char **chars);

#endif // STR_H
