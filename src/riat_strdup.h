#ifndef RIAT_STRDUP_H
#define RIAT_STRDUP_H

/* 
 * Note that strdup() and strndup() will be standard in C23 (and hopefully will be supported on Windows before C3x)
 * 
 * Replace with these functions with those when that happens.
 */

#include <stddef.h>

/**
 * Allocate and create a copy of the input string. This uses malloc(), so the resulting string should be freed with free() when done.
 * 
 * @param input input string
 * 
 * @return      copy of the string or NULL on failure (i.e. allocation fail)
 */
char *riat_strdup(const char *input);

/**
 * Allocate and create a copy of the input substring. This uses malloc(), so the resulting string should be freed with free() when done.
 * 
 * @param input input string
 * 
 * @return      copy of the string or NULL on failure (i.e. allocation fail)
 */
char *riat_strndup(const char *input, size_t len);



/**
 * Do a case-insensitive string comparison
 * 
 * @param a string a
 * @param b string b
 * 
 * @return  comparison (0 if equal, >0 if a is greater, <0 if a is less)
 */
int riat_case_insensitive_strcmp(const char *a, const char *b);

#endif
