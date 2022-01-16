#ifndef RIAT_INTERNAL_H
#define RIAT_INTERNAL_H

#include "../include/riat/riat.h"

typedef struct RIAT_Instance {
    /* Compiled files */
    struct {
        const char *file_names;
        size_t file_names_count;
        size_t file_names_capacity;
    } files;

    /* Last compile error */
    struct {
        RIAT_CompileResult result_int;

        char syntax_error_explanation[512];
        size_t syntax_error_line;
        size_t syntax_error_column;
    } last_compile_error;
} RIAT_Instance;

typedef struct RIAT_Token {
    char *token_string;
    size_t line;
    size_t column;
    size_t file;

    /* Left = +1. Right = -1. None = 0 */
    char parenthesis;
} RIAT_Token;

/**
 * Free the token array
 * 
 * @param tokens      pointer to first token to free
 * @param token_count number of tokens
 */
void RIAT_free_token_array(RIAT_Token *tokens, size_t token_count);

/**
 * Parse the script source data into tokens
 * 
 * @param instance             instance context this is applicable to
 * @param script_source_data   script source data to use
 * @param script_source_length length of script source length
 * @param file_name            name of the file (for error reporting)
 * @param tokens               (output) pointer to the pointer to be set to the token array
 * @param token_count          (output) number of tokens
 */
RIAT_CompileResult RIAT_tokenize(RIAT_Instance *instance, const char *script_source_data, size_t script_source_length, const char *file_name, RIAT_Token **tokens, size_t *token_count);

/**
 * Create a tree from the tokens
 * 
 * @param instance    instance context this is applicable to
 * @param tokens      pointer to the token array (success or failure, this will be automatically freed)
 * @param token_count number of tokens
 */
RIAT_CompileResult RIAT_tree(RIAT_Instance *instance, RIAT_Token *tokens, size_t token_count);

typedef struct RIAT_Global {
    char name[32];
} RIAT_Global;

typedef struct RIAT_Script {
    char name[32];
} RIAT_Script;

#endif
