#include "riat_internal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include "definition/definition.h"

#define APPEND_NODE_ALLOCATION_ERROR SIZE_MAX

typedef struct RIAT_ScriptGlobalContainer {
    size_t global_count;
    RIAT_Global *globals;
    size_t script_count;
    RIAT_Script *scripts;
} RIAT_ScriptGlobalContainer;

static size_t append_node_to_node_array(RIAT_ScriptNodeArrayContainer *container, const char *string_data);

static RIAT_CompileResult read_block(
    RIAT_Instance *instance,
    RIAT_Token *tokens,
    size_t *ti,
    RIAT_ScriptNodeArrayContainer *nodes,
    RIAT_ScriptGlobalContainer *current_script_info,
    size_t *root_node,
    bool is_script_block
);

static RIAT_CompileResult read_next_element(
    RIAT_Instance *instance,
    RIAT_Token *tokens,
    size_t *ti,
    RIAT_ScriptNodeArrayContainer *nodes,
    RIAT_ScriptGlobalContainer *current_script_info,
    size_t *node
);

static RIAT_CompileResult resolve_type_of_block(RIAT_Instance *instance, RIAT_ScriptNodeArrayContainer *nodes, size_t node, RIAT_ValueType preferred_type, const RIAT_ScriptGlobalContainer *script_globals);

#define SYNTAX_ERROR_INSTANCE(instance, line, column, file) \
    instance->last_compile_error.syntax_error_file = file; \
    instance->last_compile_error.syntax_error_line = line; \
    instance->last_compile_error.syntax_error_column = column; \
    instance->last_compile_error.result_int = RIAT_COMPILE_SYNTAX_ERROR; \

#define RESOLVE_TYPE_OF_ELEMENT_FAIL(...) \
    SYNTAX_ERROR_INSTANCE(instance, n->line, n->column, n->file); \
    snprintf(instance->last_compile_error.syntax_error_explanation, sizeof(instance->last_compile_error.syntax_error_explanation), __VA_ARGS__); \
    return RIAT_COMPILE_SYNTAX_ERROR;

#define CONVERT_TYPE_OR_DIE(preferred_type, actual_type, line, column, file) \
    if(preferred_type != actual_type) { \
        if( \
            (preferred_type == RIAT_VALUE_TYPE_REAL && (actual_type == RIAT_VALUE_TYPE_LONG || actual_type == RIAT_VALUE_TYPE_SHORT)) || \
            (actual_type == RIAT_VALUE_TYPE_REAL && (preferred_type == RIAT_VALUE_TYPE_LONG || preferred_type == RIAT_VALUE_TYPE_SHORT)) \
        ) { \
            actual_type = preferred_type; \
        } \
        else { \
            SYNTAX_ERROR_INSTANCE(instance, line, column, file); \
            snprintf(instance->last_compile_error.syntax_error_explanation, sizeof(instance->last_compile_error.syntax_error_explanation), "expected a %s, but %s cannot be converted into one", RIAT_value_type_to_string(preferred_type), RIAT_value_type_to_string(actual_type)); \
            return RIAT_COMPILE_SYNTAX_ERROR; \
        } \
    }

static const RIAT_ValueType *get_type_of_global(const char *name, const RIAT_ScriptGlobalContainer *script_globals) {
    /* If it's a global, try finding it, first! */
    for(size_t g = 0; g < script_globals->global_count; g++) {
        RIAT_Global *global = &script_globals->globals[g];
        if(strcmp(global->name, name) == 0) {
            return &global->value_type;
        }
    }
    const RIAT_BuiltinDefinition *definition = RIAT_builtin_definition_search(name);
    if(definition != NULL && definition->type == RIAT_BUILTIN_DEFINITION_TYPE_GLOBAL) {
        return &definition->value_type;
    }
    return NULL;
}

static RIAT_CompileResult resolve_type_of_element(RIAT_Instance *instance, RIAT_ScriptNodeArrayContainer *nodes, size_t node, RIAT_ValueType preferred_type, const RIAT_ScriptGlobalContainer *script_globals) {
    RIAT_ScriptNode *n = &nodes->nodes[node];

    bool numeric_preferred = preferred_type == RIAT_VALUE_TYPE_REAL || preferred_type == RIAT_VALUE_TYPE_LONG || preferred_type == RIAT_VALUE_TYPE_SHORT;

    if(n->is_primitive) {
        const RIAT_ValueType *global_maybe = get_type_of_global(n->string_data, script_globals);
        if(global_maybe) {
            n->type = *global_maybe;
            CONVERT_TYPE_OR_DIE(preferred_type, n->type, n->line, n->column, n->file);
            return RIAT_COMPILE_OK;
        }

        /* Otherwise, convert the string I guess! */
        switch(preferred_type) {
            case RIAT_VALUE_TYPE_VOID:
                RESOLVE_TYPE_OF_ELEMENT_FAIL("a void type was expected; got '%s' instead", n->string_data);
            case RIAT_VALUE_TYPE_UNPARSED:
                abort();
            case RIAT_VALUE_TYPE_BOOLEAN:
                if(strcmp(n->string_data, "true") == 0 || strcmp(n->string_data, "on") == 0 || strcmp(n->string_data, "1") == 0) {
                    n->bool_int = 1;
                }
                else if(strcmp(n->string_data, "false") == 0 || strcmp(n->string_data, "off") == 0 || strcmp(n->string_data, "0") == 0) {
                    n->bool_int = 0;
                }
                else {
                    RESOLVE_TYPE_OF_ELEMENT_FAIL("a boolean type (i.e. 'false'/'true'/'0'/'1'/'off'/'on') was expected; got '%s' instead", n->string_data);
                }
                free(n->string_data);
                n->string_data = NULL;
                break;
            case RIAT_VALUE_TYPE_REAL:
                {
                    char *end = NULL;
                    n->real = strtof(n->string_data, &end);
                    if(end == n->string_data) {
                        RESOLVE_TYPE_OF_ELEMENT_FAIL("a real type was expected; got '%s' instead", n->string_data);
                    }
                }
                free(n->string_data);
                n->string_data = NULL;
                break;
            case RIAT_VALUE_TYPE_SHORT:
                {
                    char *end = NULL;
                    long long v = strtoll(n->string_data, &end, 10);
                    if(end == n->string_data) {
                        RESOLVE_TYPE_OF_ELEMENT_FAIL("a short type was expected; got '%s' instead", n->string_data);
                    }
                    if(v < INT16_MIN || v > INT16_MAX) {
                        RESOLVE_TYPE_OF_ELEMENT_FAIL("a short type was expected; got '%s' (out of range) instead", n->string_data);
                    }
                    n->short_int = (short)v;
                }
                free(n->string_data);
                n->string_data = NULL;
                break;
            case RIAT_VALUE_TYPE_LONG:
                {
                    char *end = NULL;
                    long long v = strtoll(n->string_data, &end, 10);
                    if(end == n->string_data) {
                        RESOLVE_TYPE_OF_ELEMENT_FAIL("a long type was expected; got '%s' instead", n->string_data);
                    }
                    if(v < INT32_MIN || v > INT32_MAX) {
                        RESOLVE_TYPE_OF_ELEMENT_FAIL("a long type was expected; got '%s' (out of range) instead", n->string_data);
                    }
                    n->long_int = (long)v;
                }
                free(n->string_data);
                n->string_data = NULL;
                break;
            default:
                break;
        }
        n->type = preferred_type;
    }
    else {
        return resolve_type_of_block(instance, nodes, node, preferred_type, script_globals);
    }

    return RIAT_COMPILE_OK;
}

static RIAT_CompileResult resolve_type_of_block(RIAT_Instance *instance, RIAT_ScriptNodeArrayContainer *nodes, size_t node, RIAT_ValueType preferred_type, const RIAT_ScriptGlobalContainer *script_globals) {
    RIAT_ScriptNode *n = &nodes->nodes[node];

    /* Duh. This needs to be a block. */
    assert(!n->is_primitive);

    /* First element needs to be a function name */
    RIAT_ScriptNode *function_name_node = &nodes->nodes[n->child_node];
    assert(function_name_node->is_primitive);
    function_name_node->type = RIAT_VALUE_TYPE_FUNCTION_NAME;
    const char *function_name = function_name_node->string_data;

    const RIAT_Script *script = NULL;
    const RIAT_BuiltinDefinition *definition = NULL;
    size_t max_arguments = 0;

    /* Erroring? */
    bool local_global_exists = false;
    bool definition_global_exists = false;

    const char *but_this_global_exists = "";

    /* Is this a script? */
    for(size_t s = 0; s < script_globals->script_count; s++) {
        const RIAT_Script *script_maybe = &script_globals->scripts[s];
        if(strcmp(function_name, script_maybe->name) == 0) {
            max_arguments = 0;
            script = script_maybe;
            n->type = script->return_type;
            goto iterate_elements;
        }
    }

    /* Is it a definition */
    definition = RIAT_builtin_definition_search(function_name);
    if(definition != NULL) {
        switch(definition->type) {
            case RIAT_BUILTIN_DEFINITION_TYPE_FUNCTION:
                max_arguments = definition->parameter_count;
                n->type = definition->value_type;
                goto iterate_elements;

            case RIAT_BUILTIN_DEFINITION_TYPE_GLOBAL:
                definition_global_exists = true;
                break;

            default:
                UNREACHABLE();
        }
    }

    /* Error handling */
    {
        /* Is this a global? */
        for(size_t g = 0; g < script_globals->global_count; g++) {
            const RIAT_Global *global = &script_globals->globals[g];
            if(strcmp(function_name, global->name) == 0) {
                local_global_exists = true;
                break;
            }
        }

        /* Add help for the user */
        if(local_global_exists) {
            but_this_global_exists = " (a local global by this name exists, but this was called like a function)";
        }
        else if(definition_global_exists) {
            but_this_global_exists = " (an engine global by this name exists, but this was called like a function)";
        }
        snprintf(instance->last_compile_error.syntax_error_explanation, sizeof(instance->last_compile_error.syntax_error_explanation), "no such function or script '%s' exists%s", function_name, but_this_global_exists);
        SYNTAX_ERROR_INSTANCE(instance, function_name_node->line, function_name_node->column, function_name_node->file);
        return RIAT_COMPILE_SYNTAX_ERROR;
    }

    /* Now let's keep going */
    iterate_elements:

    /* Are there no arguments yet some were given? */
    if(max_arguments == 0) {
        if(function_name_node->next_node != NEXT_NODE_NULL) {
            RIAT_ScriptNode *element_node = &nodes->nodes[function_name_node->next_node];
            snprintf(instance->last_compile_error.syntax_error_explanation, sizeof(instance->last_compile_error.syntax_error_explanation), "'%s' takes no parameters but a parameter was given", function_name);
            SYNTAX_ERROR_INSTANCE(instance, element_node->line, element_node->column, element_node->file);
            return RIAT_COMPILE_SYNTAX_ERROR;
        }
    }

    /* Otherwise, let's parse them */
    else {
        assert(script == NULL); /* scripts can't take arguments */

        size_t argument_index = 0;

        /* Special handling of the set function */
        if(strcmp(function_name, "set") == 0) {
            assert(max_arguments == 2); /* duh */

            /* Get all of the parameters */
            size_t parameter_elements[2] = {};
            size_t element = function_name_node->next_node;
            for(argument_index = 0; argument_index < 2; argument_index++) {
                /* Not enough arguments? */
                if(element == NEXT_NODE_NULL) {
                    break;
                }

                parameter_elements[argument_index] = element;
                element = nodes->nodes[parameter_elements[argument_index]].next_node;
            }

            if(argument_index == 2) {
                RIAT_ScriptNode *global_name_node = &nodes->nodes[parameter_elements[0]];
                const char *global_name = global_name_node->string_data;
                const RIAT_ValueType *global_maybe = get_type_of_global(global_name, script_globals);
                if(!global_maybe) {
                    snprintf(instance->last_compile_error.syntax_error_explanation, sizeof(instance->last_compile_error.syntax_error_explanation), "set takes a global, but '%s' was not found", global_name);
                    SYNTAX_ERROR_INSTANCE(instance, global_name_node->line, global_name_node->column, global_name_node->file);
                    return RIAT_COMPILE_SYNTAX_ERROR;
                }
                n->type = *global_maybe;
                
                /* Try it */
                RIAT_CompileResult result = resolve_type_of_element(instance, nodes, parameter_elements[1], n->type, script_globals);
                if(result != RIAT_COMPILE_OK) {
                    return result;
                }
            }
        }

        else {
            for(size_t element = function_name_node->next_node; element != NEXT_NODE_NULL; argument_index++) {
                RIAT_ScriptNode *element_node = &nodes->nodes[element];
                const RIAT_BuiltinFunctionParameter *parameter;

                /* Did we exceed the max number of arguments? If so, check if the last argument has 'many' set. */
                if(argument_index >= max_arguments) {
                    parameter = &definition->parameters[max_arguments - 1];
                    if(!parameter->many) {
                        snprintf(instance->last_compile_error.syntax_error_explanation, sizeof(instance->last_compile_error.syntax_error_explanation), "'%s' takes %zu parameter%s, but more were given", function_name, max_arguments, max_arguments == 1 ? "" : "s");
                        SYNTAX_ERROR_INSTANCE(instance, element_node->line, element_node->column, element_node->file);
                        return RIAT_COMPILE_SYNTAX_ERROR;
                    }
                }

                /* No, OK! */
                else {
                    parameter = &definition->parameters[argument_index];
                }

                RIAT_ValueType this_element_preferred_type = parameter->type;

                /* If the last element is passthrough, there is some special behavior here */
                if(parameter->type == RIAT_VALUE_TYPE_PASSTHROUGH) {
                    /* If it's not the last type and we only passthrough the last, then it's void */
                    if(parameter->passthrough_last && element_node->next_node != NEXT_NODE_NULL) {
                        this_element_preferred_type = RIAT_VALUE_TYPE_VOID;
                    }

                    /* Otherwise it's the preferred type */
                    else {
                        this_element_preferred_type = preferred_type;
                        n->type = this_element_preferred_type;
                    }
                }

                /* Punch it! */
                RIAT_CompileResult result = resolve_type_of_element(instance, nodes, element, this_element_preferred_type, script_globals);
                if(result != RIAT_COMPILE_OK) {
                    return result;
                }

                element = element_node->next_node;
            }
        }

        /* Not enough arguments? */
        if(argument_index < max_arguments) {
            if(!definition->parameters[argument_index].optional) {
                snprintf(instance->last_compile_error.syntax_error_explanation, sizeof(instance->last_compile_error.syntax_error_explanation), "'%s' takes %zu parameter%s, but only %zu were given", function_name, max_arguments, max_arguments == 1 ? "" : "s", argument_index);
                SYNTAX_ERROR_INSTANCE(instance, function_name_node->line, function_name_node->column, function_name_node->file);
                return RIAT_COMPILE_SYNTAX_ERROR;
            }
        }
    }

    /* If it's void, we don't need to care about the type */
    if(preferred_type == RIAT_VALUE_TYPE_VOID) {
        n->type = RIAT_VALUE_TYPE_VOID;
    }

    /* Otherwise, try converting it */
    else {
        CONVERT_TYPE_OR_DIE(preferred_type, n->type, n->line, n->column, n->file);
    }

    return RIAT_COMPILE_OK;
}

#define COMPILE_RETURN_ERROR(what, line, column, explanation_fmt, ...) \
    if(explanation_fmt) snprintf(instance->last_compile_error.syntax_error_explanation, sizeof(instance->last_compile_error.syntax_error_explanation), explanation_fmt, __VA_ARGS__); \
    instance->last_compile_error.syntax_error_line = line; \
    instance->last_compile_error.syntax_error_file = file; \
    instance->last_compile_error.syntax_error_column = column; \
    result = what; \
    goto end;

RIAT_CompileResult RIAT_tree(RIAT_Instance *instance, RIAT_Token *tokens, size_t token_count) {
    RIAT_CompileResult result = RIAT_COMPILE_OK;

    RIAT_ScriptGlobalContainer script_global_list = {};
    size_t script_process_count = 0;
    size_t global_process_count = 0;

    RIAT_ScriptNodeArrayContainer node_array = {};
    const RIAT_BuiltinDefinition *begin_definition = RIAT_builtin_definition_search("begin");

    assert(begin_definition != NULL);

    size_t node_count = 0;
    size_t file = instance->files.file_names_count - 1;

    /* Figure out how many scripts/globals we have. This saves allocations and verifies that all top-level nodes are scripts or globals. */
    long depth = 0;
    for(size_t ti = 0; ti < token_count; ti++) {
        RIAT_Token *token = &tokens[ti];
        int delta = token->parenthesis;
        if(depth == 0) {
            /* Make sure delta is 0 or 1. This should be true since we checked when tokenizing. */
            assert(delta == 0 || delta == 1);

            /* Is this wrong? */
            if(delta != 1) {
                COMPILE_RETURN_ERROR(RIAT_COMPILE_SYNTAX_ERROR, token->line, token->column, "expected left parenthesis, got '%s'", token->token_string);
            }

            /* If delta is 1, then ti + 1 should be valid since tokenizing checked if all left parenthesis had a matching right parenthesis. */
            assert(ti + 1 < token_count);

            /* Increment the count... or error */
            RIAT_Token *token_next = &tokens[ti + 1];
            if(strcmp(token_next->token_string, "global") == 0) {
                script_global_list.global_count++;
            }
            else if(strcmp(token_next->token_string, "script") == 0) {
                script_global_list.script_count++;
            }
            else {
                COMPILE_RETURN_ERROR(RIAT_COMPILE_SYNTAX_ERROR, token_next->line, token_next->column, "expected 'global' or 'script', got %s", token_next->token_string);
            }

            /* This should not have anything related to parenthesis here */
            assert(token_next->parenthesis == 0);

            /* We can skip this token, too */
            ti++;
        }
        depth += delta;
    }
    assert(depth == 0);

    /* Allocate that many now */
    if(script_global_list.script_count > 0) {
        if((script_global_list.scripts = calloc(sizeof(*script_global_list.scripts), script_global_list.script_count)) == NULL) {
            COMPILE_RETURN_ERROR(RIAT_COMPILE_ALLOCATION_ERROR, 0, 0, NULL, NULL);
        }
    }
    if(script_global_list.global_count > 0) {
        if((script_global_list.globals = calloc(sizeof(*script_global_list.globals), script_global_list.global_count)) == NULL) {
            COMPILE_RETURN_ERROR(RIAT_COMPILE_ALLOCATION_ERROR, 0, 0, NULL, NULL);
        }
    }

    /* Now process all the things */
    for(size_t ti = 0; ti < token_count; /* no incrementor necessary */) {
        /* We know for sure that the next three tokens are valid */
        assert(ti + 2 < token_count);

        /* Get 'em */
        RIAT_Token *block_open_token = &tokens[ti++];
        RIAT_Token *block_type_token = &tokens[ti++];
        
        /* Now we know for certain this is a left parenthesis */
        assert(block_open_token->parenthesis == 1);
        assert(block_type_token->parenthesis == 0);

        /* Global? */
        if(strcmp(block_type_token->token_string, "global") == 0) {
            /* We should not overflow here. If the above check is written correctly, this won't happen. */
            assert(script_global_list.global_count > global_process_count);
            RIAT_Global *relevant_global = &script_global_list.globals[global_process_count++];

            bool error;

            /* Get the type */
            RIAT_Token *global_type_token = &tokens[ti++];
            relevant_global->value_type = RIAT_value_type_from_string(global_type_token->token_string, &error);
            if(global_type_token->parenthesis != 0 || error) {
                COMPILE_RETURN_ERROR(RIAT_COMPILE_SYNTAX_ERROR, global_type_token->line, global_type_token->column, "expected global type, got '%s'", global_type_token->token_string);
            }

            /* And the name */
            RIAT_Token *global_name_token = &tokens[ti++];
            if(global_name_token->parenthesis != 0) {
                COMPILE_RETURN_ERROR(RIAT_COMPILE_SYNTAX_ERROR, global_name_token->line, global_name_token->column, "expected global name, got '%s'", global_name_token->token_string);
            }
            strncpy(relevant_global->name, global_name_token->token_string, sizeof(relevant_global->name) - 1);

            size_t init_node;
            result = read_next_element(instance, tokens, &ti, &node_array, &script_global_list, &init_node);
            if(result != RIAT_COMPILE_OK) {
                goto end;
            }

            relevant_global->first_node = init_node;
            relevant_global->line = block_type_token->line;
            relevant_global->column = block_type_token->column;
            relevant_global->file = block_type_token->file;

            ti++;
        }

        else if(strcmp(block_type_token->token_string, "script") == 0) {
            /* We should not overflow here. If the above check is written correctly, this won't happen. */
            assert(script_global_list.script_count > script_process_count);
            RIAT_Script *relevant_script = &script_global_list.scripts[script_process_count++];

            bool error;

            /* Get the type */
            RIAT_Token *script_type_token = &tokens[ti++];
            relevant_script->script_type = RIAT_script_type_from_string(script_type_token->token_string, &error);
            if(script_type_token->parenthesis != 0 || error) {
                COMPILE_RETURN_ERROR(RIAT_COMPILE_SYNTAX_ERROR, script_type_token->line, script_type_token->column, "expected script type, got '%s'", script_type_token->token_string);
            }

            /* If it's a static script, a type is expected */
            if(relevant_script->script_type == RIAT_SCRIPT_TYPE_STATIC || relevant_script->script_type == RIAT_SCRIPT_TYPE_STUB) {
                RIAT_Token *script_type_token = &tokens[ti++];
                relevant_script->return_type = RIAT_value_type_from_string(script_type_token->token_string, &error);
                if(script_type_token->parenthesis != 0 || error) {
                    COMPILE_RETURN_ERROR(RIAT_COMPILE_SYNTAX_ERROR, script_type_token->line, script_type_token->column, "expected script type, got '%s'", script_type_token->token_string);
                }
            }

            /* Otherwise, void */
            else {
                 relevant_script->return_type = RIAT_VALUE_TYPE_VOID;
            }

            /* And the name */
            RIAT_Token *script_name_token = &tokens[ti++];
            if(script_name_token->parenthesis != 0) {
                COMPILE_RETURN_ERROR(RIAT_COMPILE_SYNTAX_ERROR, script_name_token->line, script_name_token->column, "expected script name, got '%s'", script_name_token->token_string);
            }
            strncpy(relevant_script->name, script_name_token->token_string, sizeof(relevant_script->name) - 1);

            size_t root_node;
            result = read_block(instance, tokens, &ti, &node_array, &script_global_list, &root_node, true);
            if(result != RIAT_COMPILE_OK) {
                goto end;
            }

            /* Implicitly add a begin block */
            size_t original_first_node_index = node_array.nodes[root_node].child_node;
            size_t begin_name = append_node_to_node_array(&node_array, "begin");
            if(begin_name == APPEND_NODE_ALLOCATION_ERROR) {
                result = RIAT_COMPILE_ALLOCATION_ERROR;
                goto end;
            }
            node_array.nodes[begin_name].is_primitive = true;
            node_array.nodes[begin_name].next_node = original_first_node_index;
            node_array.nodes[root_node].child_node = begin_name;
            relevant_script->first_node = root_node;
            relevant_script->line = block_type_token->line;
            relevant_script->column = block_type_token->column;
            relevant_script->file = block_type_token->file;
        }
    }

    printf("DEBUG: %zu global%s, %zu script%s, and %zu node%s\n", script_global_list.global_count, script_global_list.global_count == 1 ? "" : "s", script_global_list.script_count, script_global_list.script_count == 1 ? "" : "s", node_array.nodes_count, node_array.nodes_count == 1 ? "" : "s");

    /* Resolve types for all nodes (auto-break if result is not OK) */
    for(size_t g = 0; g < script_global_list.global_count && result == RIAT_COMPILE_OK; g++) {
        RIAT_Global *global = &script_global_list.globals[g];
        printf("DEBUG: Global #%zu = '%s'\n", g, global->name);

        result = resolve_type_of_element(instance, &node_array, global->first_node, global->value_type, &script_global_list);
    }

    for(size_t s = 0; s < script_global_list.script_count && result == RIAT_COMPILE_OK; s++) {
        RIAT_Script *script = &script_global_list.scripts[s];
        printf("DEBUG: Script #%zu = '%s'\n", s, script->name);
        
        result = resolve_type_of_element(instance, &node_array, script->first_node, script->return_type, &script_global_list);
    }

    if(result != RIAT_COMPILE_OK) {
        goto end;
    }

    /* Merge the nodes */
    size_t old_node_count = instance->nodes.nodes_count;
    size_t new_node_count = instance->nodes.nodes_count + node_array.nodes_capacity;
    RIAT_ScriptNode *instance_new_nodes = realloc(instance->nodes.nodes, sizeof(*instance->nodes.nodes) * new_node_count);
    if(instance_new_nodes == NULL) {
        result = RIAT_COMPILE_ALLOCATION_ERROR;
        goto end;
    }
    instance->nodes.nodes = instance_new_nodes;
    instance->nodes.nodes_count = new_node_count;
    instance->nodes.nodes_capacity = new_node_count;
    for(size_t n = 0; n < node_array.nodes_count; n++) {
        RIAT_ScriptNode *node = &node_array.nodes[n];

        if(!node->is_primitive) {
            node->child_node += new_node_count;
        }

        if(node->next_node != NEXT_NODE_NULL) {
            node->next_node += new_node_count;
        }
    }
    memcpy(instance->nodes.nodes + old_node_count, node_array.nodes, node_array.nodes_count);

    /* Set the node count to 0 since the temporary nodes are no longer valid due to being moved */
    node_array.nodes_count = 0;

    end:
    free(script_global_list.scripts);
    free(script_global_list.globals);
    RIAT_clear_node_array_container(&node_array);
    RIAT_token_free_array(tokens, token_count);
    return result;
}

static RIAT_CompileResult read_block(
    RIAT_Instance *instance,
    RIAT_Token *tokens,
    size_t *ti,
    RIAT_ScriptNodeArrayContainer *nodes,
    RIAT_ScriptGlobalContainer *current_script_info,
    size_t *root_node,
    bool is_script_block
) {
    /* Do some checks on the first token */
    RIAT_Token *current_token = &tokens[*ti];

    /* This should NOT be a left parenthesis unless it's the first thing in a script*/
    assert(is_script_block || current_token->parenthesis != 1);

    /* Also make sure it isn't empty */
    if(current_token->parenthesis == -1) {
        strncpy(instance->last_compile_error.syntax_error_explanation, "block is empty (unexpected left parenthesis)", sizeof(instance->last_compile_error.syntax_error_explanation) - 1);
        instance->last_compile_error.syntax_error_line = current_token->line;
        instance->last_compile_error.syntax_error_file = current_token->file;
        instance->last_compile_error.syntax_error_column = current_token->column;
        return RIAT_COMPILE_SYNTAX_ERROR;
    }

    /* Push the root node */
    if((*root_node = append_node_to_node_array(nodes, NULL)) == APPEND_NODE_ALLOCATION_ERROR) {
        return RIAT_COMPILE_ALLOCATION_ERROR;
    }
    nodes->nodes[*root_node].is_primitive = false;

    /* Initialize last_node to something that can't possibly be true */
    size_t last_node = APPEND_NODE_ALLOCATION_ERROR;

    while(current_token->parenthesis != -1) {
        /* Get the node index first */
        size_t new_node;
        RIAT_CompileResult element_result = read_next_element(instance, tokens, ti, nodes, current_script_info, &new_node);
        if(element_result != RIAT_COMPILE_OK) {
            return element_result;
        }

        /* If it's the first node, have our node point to it then */
        if(last_node == APPEND_NODE_ALLOCATION_ERROR) {
            nodes->nodes[*root_node].child_node = new_node;
        }

        /* Otherwise, set our next node to it */
        else {
            nodes->nodes[last_node].next_node = new_node;
        }

        last_node = new_node;
        current_token = &tokens[*ti];
    }

    /* Increment the token incrementor one more time */
    (*ti)++;

    return RIAT_COMPILE_OK;
}

static RIAT_CompileResult read_next_element(
    RIAT_Instance *instance,
    RIAT_Token *tokens,
    size_t *ti,
    RIAT_ScriptNodeArrayContainer *nodes,
    RIAT_ScriptGlobalContainer *current_script_info,
    size_t *node
) {
    RIAT_Token *first_token = &tokens[*ti];
    (*ti)++;

    /* This should NOT be a right parenthesis */
    assert(first_token->parenthesis != -1);

    if(first_token->parenthesis == 0) {
        if((*node = append_node_to_node_array(nodes, first_token->token_string)) == APPEND_NODE_ALLOCATION_ERROR) {
            return RIAT_COMPILE_ALLOCATION_ERROR;
        }
    }
    else if(first_token->parenthesis == 1) {
        RIAT_CompileResult result = read_block(instance, tokens, ti, nodes, current_script_info, node, false);
        if(result != RIAT_COMPILE_OK) {
            return result;
        }
    }
    else {
        UNREACHABLE();
    }
        
    RIAT_ScriptNode *element_node = &nodes->nodes[*node];
    element_node->column = first_token->column;
    element_node->line = first_token->line;
    element_node->file = first_token->file;
    element_node->is_primitive = (first_token->parenthesis == 0);
    return RIAT_COMPILE_OK;
}

static size_t append_node_to_node_array(RIAT_ScriptNodeArrayContainer *container, const char *string_data) {
    /* Capacity met? If so, reallocate with double size (or set to 16 if size is 0) */
    if(container->nodes_capacity == container->nodes_count) {
        size_t new_capacity = container->nodes_capacity > 0 ? container->nodes_capacity * 2 : 16;
        RIAT_ScriptNode *new_array = realloc(container->nodes, sizeof(*container->nodes) * new_capacity);

        /* Allocation failure = return APPEND_NODE_ALLOCATION_ERROR */
        if(new_array == NULL) {
            return APPEND_NODE_ALLOCATION_ERROR;
        }

        /* Otherwise, set the pointer to our new array and update the capacity */
        container->nodes = new_array;
        container->nodes_capacity = new_capacity;
    }

    /* Set the string data then */
    size_t next_node_index = container->nodes_count;
    container->nodes_count++;
    RIAT_ScriptNode *next_node = &container->nodes[next_node_index];
    memset(next_node, 0, sizeof(*next_node));
    next_node->next_node = NEXT_NODE_NULL;
    next_node->string_data = string_data != NULL ? strdup(string_data) : NULL;
    return next_node_index;
}
