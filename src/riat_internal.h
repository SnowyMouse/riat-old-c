#ifndef RIAT_INTERNAL_H
#define RIAT_INTERNAL_H

#include "../include/riat/riat.h"
#include <stdbool.h>
#include <stdint.h>

#ifndef __has_builtin
    #define __has_builtin(x) 0
#endif

#if defined (_MSC_VER)
    #define UNREACHABLE() __assume(0)
#elif __has_builtin(__builtin_unreachable)
    #define UNREACHABLE() __builtin_unreachable()
#else
    #define UNREACHABLE() abort()
#endif

#define NEXT_NODE_NULL (SIZE_MAX)

/* Node array container */
typedef struct RIAT_NodeArrayContainer {
    /* Pointer to first node (or NULL) */
    RIAT_Node *nodes;

    /* Number of nodes in the array that are valid */
    size_t nodes_count;

    /* Number of nodes in the array that can fit */
    size_t nodes_capacity;
} RIAT_NodeArrayContainer;

/* Script and global array container */
typedef struct RIAT_ScriptGlobalArrayContainer {
    /* Number of globals */
    size_t global_count;

    /* Pointer to first global (or NULL) */
    RIAT_Global *globals;

    /* Number of scripts */
    size_t script_count;

    /* Pointer to first script (or NULL) */
    RIAT_Script *scripts;
} RIAT_ScriptGlobalArrayContainer;

typedef struct RIAT_Token {
    char *token_string;
    size_t line;
    size_t column;
    size_t file;

    /* Left = +1. Right = -1. None = 0 */
    char parenthesis;
} RIAT_Token;

typedef struct RIAT_Instance {
    /* Compile target */
    RIAT_CompileTarget compile_target;

    /* Optimization level */
    RIAT_OptimizationLevel optimization_level;

    /* User data */
    void *user_data;

    /* Warning callback */
    RIAT_InstanceWarnCallback warn_callback;

    /* Compiled files */
    struct {
        char **file_names;
        size_t file_names_count;
    } files;

    /* Last compile error */
    struct {
        RIAT_CompileResult result_int;

        char syntax_error_explanation[512];
        size_t syntax_error_line;
        size_t syntax_error_column;
        size_t syntax_error_file;
        char syntax_error_file_name[512];
    } last_compile_error;

    /* All current tokens */
    struct {
        RIAT_Token *tokens;
        size_t token_count;
    } tokens;

    /* All current nodes */
    struct {
        RIAT_NodeArrayContainer nodes;
        RIAT_ScriptGlobalArrayContainer script_globals;
    } last_compile_result;
} RIAT_Instance;

/**
 * Free the token array
 * 
 * @param tokens      pointer to first token to free
 * @param token_count number of tokens
 */
void riat_token_free_array(RIAT_Token *tokens, size_t token_count);

/**
 * Free resources used by the token
 * 
 * @param token token to free
 */
void riat_token_free(RIAT_Token *token);

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
RIAT_CompileResult riat_tokenize(RIAT_Instance *instance, const char *script_source_data, size_t script_source_length, const char *file_name, RIAT_Token **tokens, size_t *token_count);

/**
 * Create a tree from the tokens
 * 
 * @param instance    instance context this is applicable to
 */
RIAT_CompileResult riat_tree(RIAT_Instance *instance);

/**
 * Free the node array container
 * 
 * @param container container to free
 */
void riat_clear_node_array_container(RIAT_NodeArrayContainer *container);

/**
 * Free the script/global array container
 * 
 * @param container container to free
 */
void riat_clear_script_global_array_container(RIAT_ScriptGlobalArrayContainer *container);

/**
 * Free resources used by the node
 * 
 * @param node node to free
 */
void riat_node_free(RIAT_Node *node);

/**
 * Convert a string to a script type
 * 
 * @param type  string
 * @param error (output) pointer to error boolean which is set to true if error and false if not
 * 
 * @return      script type
 */
RIAT_ScriptType riat_script_type_from_string(const char *type, bool *error);

/**
 * Convert a string to a value type
 * 
 * @param type  string
 * @param error (output) pointer to error boolean which is set to true if error and false if not
 * 
 * @return      value type
 */
RIAT_ValueType riat_value_type_from_string(const char *type, bool *error);

/**
 * Convert a value type to its string representation.
 * 
 * @param type type
 * 
 * @return     string representation of type
 */
const char *riat_value_type_to_string(RIAT_ValueType type);

#endif
