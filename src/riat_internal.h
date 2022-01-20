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

/* Script node */
typedef struct RIAT_ScriptNode {
    /* String data (NULL if none) */
    char *string_data;

    /* Next node index? */
    size_t next_node;

    /* Value type */
    RIAT_ValueType type;

    /* If this is false, it's a function call */
    bool is_primitive;

    /* If this is a global */
    bool is_global;

    /* Relevant file */
    size_t file;

    /* Relevant line */
    size_t line;

    /* Relevant column */
    size_t column;

    /* Used if not primitive or if a primitive number/boolean type (and not a global!) */ 
    union {
        /* Child node (if not primitive) */
        size_t child_node;

        /* 32-bit integer (if primitive long) */
        int32_t long_int;

        /* 16-bit integer (if primitive short) */
        int16_t short_int;

        /* 8-bit integer (if primitive boolean) */
        int8_t bool_int;

        /* 32-bit float (if primitive real) */
        float real;
    };
} RIAT_ScriptNode;

/* Node array container */
typedef struct RIAT_ScriptNodeArrayContainer {
    /* Pointer to first node (or NULL) */
    RIAT_ScriptNode *nodes;

    /* Number of nodes in the array that are valid */
    size_t nodes_count;

    /* Number of nodes in the array that can fit */
    size_t nodes_capacity;
} RIAT_ScriptNodeArrayContainer;

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
    RIAT_ScriptNodeArrayContainer nodes;
} RIAT_Instance;

/**
 * Free the token array
 * 
 * @param tokens      pointer to first token to free
 * @param token_count number of tokens
 */
void RIAT_token_free_array(RIAT_Token *tokens, size_t token_count);

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
 */
RIAT_CompileResult RIAT_tree(RIAT_Instance *instance);

typedef struct RIAT_Global {
    char name[32];
    size_t first_node;
    RIAT_ValueType value_type;

    size_t line, column, file;
} RIAT_Global;

typedef struct RIAT_Script {
    char name[32];
    size_t first_node;
    RIAT_ValueType return_type;
    RIAT_ScriptType script_type;

    bool is_function_call;
    size_t function_call_first_element;

    size_t line, column, file;
} RIAT_Script;

/**
 * Free the node array container
 * 
 * @param container container to free
 */
void RIAT_clear_node_array_container(RIAT_ScriptNodeArrayContainer *container);

/**
 * Convert a string to a script type
 * 
 * @param type  string
 * @param error (output) pointer to error boolean which is set to true if error and false if not
 * 
 * @return      script type
 */
RIAT_ScriptType RIAT_script_type_from_string(const char *type, bool *error);

/**
 * Convert a string to a value type
 * 
 * @param type  string
 * @param error (output) pointer to error boolean which is set to true if error and false if not
 * 
 * @return      value type
 */
RIAT_ValueType RIAT_value_type_from_string(const char *type, bool *error);

/**
 * Convert a value type to its string representation.
 * 
 * @param type type
 * 
 * @return     string representation of type
 */
const char *RIAT_value_type_to_string(RIAT_ValueType type);

#endif
