#include <stdlib.h>
#include <string.h>

char *riat_strdup(const char *input) {
    /* Get the length */
    size_t len = strlen(input);

    /* Malloc a string with that length + 1 for the null terminator */
    char *str = malloc(len + 1);

    /* ...and die if it fails */
    if(str == NULL) {
        return NULL;
    }

    /* Copy it with memcpy since we already know the length */
    memcpy(str, input, len);

    /* Add the null terminator */
    str[len] = 0;

    /* Done! */
    return str;
}

char *riat_strndup(const char *input, size_t len) {
    /* We have the length, so malloc with that length + 1 for the null terminator */
    char *str = malloc(len + 1);

    /* ... and die if it fails, oh dear */
    if(str == NULL) {
        return NULL;
    }

    /* Copy it with strncpy since we do not know the string length */
    strncpy(str, input, len);

    /* Add a null terminator at the end (strncpy will null terminate everything before this if the string was shorter) */
    str[len] = 0;

    /* Done */
    return str;
}
