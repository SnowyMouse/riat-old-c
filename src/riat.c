#include "riat_internal.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

RIAT_Instance *RIAT_instance_new(RIAT_CompileTarget compile_target) {
    RIAT_Instance *instance = calloc(sizeof(*instance), 1);
    if(!instance) {
        return NULL;
    }

    instance->compile_target = compile_target;
    return instance;
}

const char *RIAT_instance_get_last_compile_error(const RIAT_Instance *instance, size_t *line, size_t *column, const char **file) {
    switch(instance->last_compile_error.result_int) {
        case RIAT_COMPILE_OK:
            return NULL;
        case RIAT_COMPILE_SYNTAX_ERROR:
            *line = instance->last_compile_error.syntax_error_line;
            *column = instance->last_compile_error.syntax_error_column;
            *file = instance->last_compile_error.syntax_error_file_name;
            return instance->last_compile_error.syntax_error_explanation;
        case RIAT_COMPILE_ALLOCATION_ERROR:
            return "allocation error";
    }
}

#define COMPILE_RETURN_ERROR(what) \
    instance->last_compile_error.result_int = what; \
    return what;

RIAT_CompileResult RIAT_instance_load_script(RIAT_Instance *instance, const char *script_source_data, size_t script_source_length, const char *file_name) {
    /* Clear the error */
    memset(&instance->last_compile_error, 0, sizeof(instance->last_compile_error));

    RIAT_Token *tokens;
    size_t token_count;

    /* Tokenize it */
    RIAT_CompileResult result = RIAT_tokenize(instance, script_source_data, script_source_length, file_name, &tokens, &token_count);
    if(result != RIAT_COMPILE_OK) {
        strncpy(instance->last_compile_error.syntax_error_file_name, file_name, sizeof(instance->last_compile_error.syntax_error_file_name) - 1);
        COMPILE_RETURN_ERROR(result);
    }

    /* If that worked, we can append the file name now */
    char *file_name_copy = strdup(file_name);
    if(file_name_copy == NULL) {
        RIAT_token_free_array(tokens, token_count);
        COMPILE_RETURN_ERROR(RIAT_COMPILE_ALLOCATION_ERROR);
    }
    char **new_file_names_ptr = realloc(instance->files.file_names, sizeof(*new_file_names_ptr) * (instance->files.file_names_count + 1));
    if(new_file_names_ptr == NULL) {
        RIAT_token_free_array(tokens, token_count);
        free(file_name_copy);
        COMPILE_RETURN_ERROR(RIAT_COMPILE_ALLOCATION_ERROR);
    }
    new_file_names_ptr[instance->files.file_names_count] = file_name_copy;
    instance->files.file_names = new_file_names_ptr;

    /* If we don't have any tokens, there is no need to realloc */
    if(instance->tokens.token_count == 0) {
        instance->tokens.tokens = tokens;
        instance->tokens.token_count = token_count;
    }

    /* Otherwise, append the tokens */
    else {
        size_t new_token_count = token_count + instance->tokens.token_count;
        RIAT_Token *new_token_array = realloc(instance->tokens.tokens, sizeof(*instance->tokens.tokens) * new_token_count);

        /* Or don't lol */
        if(new_token_array == NULL) {
            RIAT_token_free_array(tokens, token_count);
            free(file_name_copy);
            COMPILE_RETURN_ERROR(RIAT_COMPILE_ALLOCATION_ERROR);
        }

        instance->tokens.tokens = new_token_array;
        memcpy(instance->tokens.tokens + instance->tokens.token_count, tokens, sizeof(*tokens) * token_count);
        instance->tokens.token_count = new_token_count;

        /* Free the local token array (no need to free the strings) */
        free(tokens);
    }

    /* Increment file name count */
    instance->files.file_names_count++;
    return RIAT_COMPILE_OK;
}

RIAT_CompileResult RIAT_instance_compile_scripts(RIAT_Instance *instance) {
    /* Clear the error */
    memset(&instance->last_compile_error, 0, sizeof(instance->last_compile_error));
    RIAT_CompileResult result = RIAT_tree(instance);
    if(result != RIAT_COMPILE_OK) {
        if(result == RIAT_COMPILE_SYNTAX_ERROR) {
            strncpy(instance->last_compile_error.syntax_error_file_name, instance->files.file_names[instance->last_compile_error.syntax_error_file], sizeof(instance->last_compile_error.syntax_error_file_name) - 1);
        }
        COMPILE_RETURN_ERROR(result);
    }

    /* Free it all */
    RIAT_token_free_array(instance->tokens.tokens, instance->tokens.token_count);
    instance->tokens.token_count = 0;
    instance->tokens.tokens = NULL;

    return RIAT_COMPILE_OK;
}

#undef COMPILE_TRY
#undef COMPILE_RETURN_ERROR

void RIAT_instance_delete(RIAT_Instance *instance) {
    if(instance == NULL) {
        return;
    }

    RIAT_token_free_array(instance->tokens.tokens, instance->tokens.token_count);
    RIAT_clear_node_array_container(&instance->nodes);
    free(instance);
}

void RIAT_clear_node_array_container(RIAT_ScriptNodeArrayContainer *container) {
    for(size_t i = 0; i < container->nodes_count; i++) {
        free(container->nodes[i].string_data);
    }
    free(container->nodes);
    container->nodes_capacity = 0;
    container->nodes_count = 0;
}

void RIAT_token_free_array(RIAT_Token *tokens, size_t token_count) {
    for(size_t i = 0; i < token_count; i++) {
        free(tokens[i].token_string);
    }
    free(tokens);
}
