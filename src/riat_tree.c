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

#define COMPILE_RETURN_ERROR(what, line, column, explanation_fmt, ...) \
    if(explanation_fmt) snprintf(instance->last_compile_error.syntax_error_explanation, sizeof(instance->last_compile_error.syntax_error_explanation), explanation_fmt, __VA_ARGS__); \
    instance->last_compile_error.syntax_error_line = line; \
    instance->last_compile_error.syntax_error_column = column; \
    result = what; \
    goto end;

RIAT_CompileResult RIAT_tree(RIAT_Instance *instance, RIAT_Token *tokens, size_t token_count) {
    RIAT_CompileResult result = RIAT_COMPILE_OK;

    RIAT_ScriptGlobalContainer script_global_list = {};
    size_t script_process_count = 0;
    size_t global_process_count = 0;

    RIAT_ScriptNodeArrayContainer node_array = {};
    RIAT_BuiltinDefinition *begin_definition = RIAT_builtin_definition_search("begin");

    assert(begin_definition != NULL);

    size_t node_count = 0;

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

            relevant_script->first_node = root_node;
        }
    }

    for(size_t g = 0; g < script_global_list.global_count; g++) {
        printf("DEBUG: Global #%zu = '%s'\n", g, script_global_list.globals[g].name);
    }

    for(size_t s = 0; s < script_global_list.script_count; s++) {
        printf("DEBUG: Script #%zu = '%s'\n", s, script_global_list.scripts[s].name);
    }

    printf("DEBUG: %zu global%s, %zu script%s, and %zu node%s\n", script_global_list.global_count, script_global_list.global_count == 1 ? "" : "s", script_global_list.script_count, script_global_list.script_count == 1 ? "" : "s", node_array.nodes_count, node_array.nodes_count == 1 ? "" : "s");

    /* TODO: Resolve types for all nodes */

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
            nodes->nodes[*root_node].is_primitive = true;
            nodes->nodes[last_node].child_node = new_node;
        }

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
