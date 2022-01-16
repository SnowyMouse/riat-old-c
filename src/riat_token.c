#include "riat_internal.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct TokenInfo {
    bool is_error;
    size_t line, column, length;
    const char *error_reason;
} TokenInfo;

static const char *find_next_token(const char *script_source_data, size_t source_data_length, size_t *current_line, size_t *current_column, TokenInfo *info);

#define COMPILE_RETURN_ERROR(what, explanation_fmt, ...) \
    if(explanation_fmt) snprintf(instance->last_compile_error.syntax_error_explanation, sizeof(instance->last_compile_error.syntax_error_explanation), explanation_fmt, __VA_ARGS__); \
    instance->last_compile_error.syntax_error_line = line; \
    instance->last_compile_error.syntax_error_column = column; \
    return what;

RIAT_CompileResult RIAT_tokenize(RIAT_Instance *instance, const char *script_source_data, size_t script_source_length, const char *file_name, RIAT_Token **tokens, size_t *token_count) {
    const char *tokenizer_data = script_source_data;
    size_t tokenizer_length = script_source_length;
    size_t line = 1, column = 1;

    /* Allocate tokens here */
    *token_count = 0;
    size_t token_capacity = 64;
    *tokens = malloc(sizeof(RIAT_Token) * token_capacity);

    /* Nope lol */
    if(tokens == NULL) {
        instance->last_compile_error.result_int = RIAT_COMPILE_ALLOCATION_ERROR;
        COMPILE_RETURN_ERROR(RIAT_COMPILE_ALLOCATION_ERROR, NULL, NULL);
    }

    /* Go through each token */
    while(tokenizer_length > 0) {
        TokenInfo info;
        const char *next_token = find_next_token(tokenizer_data, tokenizer_length, &line, &column, &info);

        /* Error? */
        if(info.is_error) {
            /* Clean up tokens */
            RIAT_token_free_array(*tokens, *token_count);

            /* Report the error */
            line = info.line;
            column = info.column;
            COMPILE_RETURN_ERROR(RIAT_COMPILE_SYNTAX_ERROR, "token error: %s", info.error_reason);
        }

        /* Last token gotten? */
        if(next_token == NULL) {
            break;
        }

        /* Allocate space for the token (as a string) so we can use it later */
        char *token_str = malloc(info.length + 1);
        if(!token_str) {
            /* Clean up tokens */
            RIAT_token_free_array(*tokens, *token_count);

            /* Report the error */
            COMPILE_RETURN_ERROR(RIAT_COMPILE_ALLOCATION_ERROR, NULL, NULL);
        }

        /* If we hit the max capacity, double it */
        if(*token_count == token_capacity) {
            token_capacity *= 2;
            RIAT_Token *tokens_new = realloc(*tokens, sizeof(**tokens) * token_capacity);

            /* Did that fail? Oops! */
            if(tokens_new == NULL) {
                RIAT_token_free_array(*tokens, *token_count);
                instance->last_compile_error.result_int = RIAT_COMPILE_ALLOCATION_ERROR;
                COMPILE_RETURN_ERROR(RIAT_COMPILE_ALLOCATION_ERROR, NULL, NULL);
            }

            /* Done! */
            *tokens = tokens_new;
        }
        
        /* Get our new token */
        RIAT_Token *this_token = *tokens + *token_count;
        this_token->file = instance->files.file_names_count;
        this_token->column = info.column;
        this_token->line = info.line;
        this_token->token_string = token_str;
        token_str[info.length] = 0; /* null terminate */

        /* Is it a quoted string? If so, copy only the inner portion */
        if(*next_token == '"') {
            memcpy(token_str, next_token + 1, info.length - 2);

            token_str[info.length - 2] = 0; /* null terminate these as well! */
            token_str[info.length - 1] = 0;
        }

        /* Otherwise, copy it as-is */
        else {
            memcpy(token_str, next_token, info.length);

            /* Unquoted parenthesis? */
            if(*next_token == '(') {
                this_token->parenthesis = +1;
            }
            else if(*next_token == ')') {
                this_token->parenthesis = -1;
            }
            else {
                this_token->parenthesis = 0;
            }
        }

        (*token_count)++;

        /* Advance to the end of this token */
        size_t travel_length = next_token - tokenizer_data + info.length;
        tokenizer_data += travel_length;
        tokenizer_length -= travel_length;
    }

    /* Make sure left parenthesis count = right parenthesis count and that depth never goes negative */
    long depth = 0;
    size_t d1 = 0;
    for(size_t i = 0; i < *token_count; i++) {
        char delta = (*tokens)[i].parenthesis;

        if(delta == 0) {
            continue;
        }

        depth += delta;

        /* Note the first left parenthesis so we can report this later if we find it hasn't been closed */
        if(depth == 1 && delta == 1) {
            d1 = i;
        }

        /* Did we go negative?? */
        if(depth < 0) {
            /* Clean up tokens */
            RIAT_token_free_array(*tokens, *token_count);

            /* Report the error */
            line = (*tokens)[i].line;
            column = (*tokens)[i].column;
            COMPILE_RETURN_ERROR(RIAT_COMPILE_SYNTAX_ERROR, "token error: right parenthesis without matching left", NULL);
        }
    }
    if(depth > 0) {
        line = (*tokens)[d1].line;
        column = (*tokens)[d1].column;
        COMPILE_RETURN_ERROR(RIAT_COMPILE_SYNTAX_ERROR, "token error: left parenthesis without matching right", NULL);
    }

    return RIAT_COMPILE_OK;
}

#undef COMPILE_RETURN_ERROR

/* Find the next token in a script */
const char *find_next_token(const char *script_source_data, size_t source_data_length, size_t *current_line, size_t *current_column, TokenInfo *info) {
    const char *searching_word_start = NULL;

    while(source_data_length > 0) {
        if(*script_source_data == 0) {
            info->line = *current_line;
            info->column = *current_column;
            info->is_error = true;
            info->error_reason = "unexpected null terminator";
            return NULL;
        }

        /* Whitespace? */
        if(*script_source_data == ' ' || *script_source_data == '\t' || *script_source_data == '\r') {
            if(searching_word_start) {
                break;
            }
            goto next_char_bump_counter;
        }

        /* Newline? */
        if(*script_source_data == '\n') {
            if(searching_word_start) {
                break;
            }

            (*current_line)++;
            *current_column = 1;
            goto next_char;
        }

        /* Single columns */
        if(*script_source_data == '(' || *script_source_data == ')') {
            if(searching_word_start) {
                break;
            }
            info->line = *current_line;
            info->column = *current_column;
            info->length = 1;
            info->is_error = false;
            (*current_column)++;
            return script_source_data;
        }

        /* Comments */
        if(*script_source_data == ';') {
            if(searching_word_start) {
                break;
            }

            while(*script_source_data != '\n' && source_data_length > 0) {
                source_data_length--;
                script_source_data++;
                (*current_column)++;
            }

            continue;
        }

        /* Quotes */
        if(*script_source_data == '"') {
            if(searching_word_start) {
                break;
            }

            const char *start = script_source_data;
            info->line = *current_line;
            info->column = *current_column;

            do {
                script_source_data++;
                source_data_length--;

                /* Handle newline here */
                if(*script_source_data == '\n') {
                    (*current_line)++;
                    *current_column = 0;
                }
                else {
                    (*current_column)++;

                    /* We did it! */
                    if(*script_source_data == '"') {
                        info->is_error = false;
                        info->length = (script_source_data - start) + 1;
                        (*current_column)++;
                        return start;
                    }
                }

            } while(*script_source_data != '"' && source_data_length > 0);

            if(source_data_length == 0) {
                info->is_error = true;
                info->error_reason = "unterminated string";
                return NULL;
            }
        }

        /* Everything else */
        if(!searching_word_start) {
            searching_word_start = script_source_data;
            info->line = *current_line;
            info->column = *current_column;
        }

        next_char_bump_counter:
        (*current_column)++;

        next_char:
        source_data_length--;
        script_source_data++;
    }

    /* Found the word */
    info->is_error = false;
    if(searching_word_start == NULL) {
        return NULL;
    }
    info->length = script_source_data - searching_word_start;
    return searching_word_start;
}
