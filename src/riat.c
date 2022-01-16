#include "riat_internal.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

RIAT_Instance *RIAT_instance_new() {
    RIAT_Instance *instance = calloc(sizeof(*instance), 1);
    if(!instance) {
        return NULL;
    }

    return instance;
}

const char *RIAT_instance_get_last_compile_error(const RIAT_Instance *instance, size_t *line, size_t *column) {
    switch(instance->last_compile_error.result_int) {
        case RIAT_COMPILE_OK:
            return NULL;
        case RIAT_COMPILE_SYNTAX_ERROR:
            *line = instance->last_compile_error.syntax_error_line;
            *column = instance->last_compile_error.syntax_error_column;
            return instance->last_compile_error.syntax_error_explanation;
        case RIAT_COMPILE_ALLOCATION_ERROR:
            return "allocation error";
    }
}

#define COMPILE_RETURN_ERROR(what) \
    instance->last_compile_error.result_int = what; \
    return what;

/* Do the thing. If it doesn't return OK, give up */
#define COMPILE_TRY(...) {\
    RIAT_CompileResult result = __VA_ARGS__; \
    if(result != RIAT_COMPILE_OK) { \
        COMPILE_RETURN_ERROR(result); \
    } \
}

RIAT_CompileResult RIAT_instance_compile_script(RIAT_Instance *instance, const char *script_source_data, size_t script_source_length, const char *file_name) {
    /* Clear the error */
    memset(&instance->last_compile_error, 0, sizeof(instance->last_compile_error));

    RIAT_Token *tokens;
    size_t token_count;
    COMPILE_TRY(RIAT_tokenize(instance, script_source_data, script_source_length, file_name, &tokens, &token_count));
    COMPILE_TRY(RIAT_tree(instance, tokens, token_count));

    COMPILE_RETURN_ERROR(RIAT_COMPILE_OK);
}

#undef COMPILE_TRY
#undef COMPILE_RETURN_ERROR

void RIAT_instance_delete(RIAT_Instance *instance) {
    if(instance == NULL) {
        return;
    }

    free(instance);
}

void RIAT_token_free_array(RIAT_Token *tokens, size_t token_count) {
    for(size_t i = 0; i < token_count; i++) {
        free(tokens[i].token_string);
    }
    free(tokens);
}
