#include "riat_internal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <assert.h>

#include "definition/definition.h"
#include "riat_strdup.h"

#define APPEND_NODE_ALLOCATION_ERROR SIZE_MAX

static size_t append_node_to_node_array(RIAT_NodeArrayContainer *container, const char *string_data, size_t file, size_t line, size_t column);

static RIAT_CompileResult read_block(
    RIAT_Instance *instance,
    RIAT_Token *tokens,
    size_t *ti,
    RIAT_NodeArrayContainer *nodes,
    RIAT_ScriptGlobalArrayContainer *current_script_info,
    size_t *root_node,
    bool is_script_block
);

static RIAT_CompileResult read_next_element(
    RIAT_Instance *instance,
    RIAT_Token *tokens,
    size_t *ti,
    RIAT_NodeArrayContainer *nodes,
    RIAT_ScriptGlobalArrayContainer *current_script_info,
    size_t *node
);

static RIAT_CompileResult resolve_type_of_block(RIAT_Instance *instance, RIAT_NodeArrayContainer *nodes, size_t node, RIAT_ValueType preferred_type, const RIAT_ScriptGlobalArrayContainer *script_globals);

/* Remove a single node */
static void remove_node(RIAT_NodeArrayContainer *nodes, size_t removing_node, const RIAT_ScriptGlobalArrayContainer *script_globals) {
    assert(nodes->nodes_count > removing_node);

    /* If we aren't just removing a node from the end (which is free), we could swap the topmost node */
    if(removing_node + 1 < nodes->nodes_count) {
        size_t index_of_swapping_node = nodes->nodes_count - 1;
        memcpy(&nodes->nodes[removing_node], &nodes->nodes[index_of_swapping_node], sizeof(nodes->nodes[removing_node]));

        /* Have all indices point to the new node */
        for(size_t n = 0; n < nodes->nodes_count - 1; n++) {
            RIAT_Node *node = &nodes->nodes[n];

            if(node->type == RIAT_VALUE_TYPE_UNPARSED) {
                continue;
            }

            if(node->next_node != SIZE_MAX && node->next_node == index_of_swapping_node) {
                node->next_node = removing_node;
            }

            if(!node->is_primitive && node->child_node == index_of_swapping_node) {
                node->child_node = removing_node;
            }
        }

        for(size_t s = 0; s < script_globals->script_count; s++) {
            RIAT_Script *script = &script_globals->scripts[s];
            if(script->first_node == index_of_swapping_node) {
                script->first_node = removing_node;
            }
        }

        for(size_t g = 0; g < script_globals->global_count; g++) {
            RIAT_Global *global = &script_globals->globals[g];
            if(global->first_node == index_of_swapping_node) {
                global->first_node = removing_node;
            }
        }
    }

    nodes->nodes_count--;
}

/* Delete all unparsed nodes */
static void remove_disabled_nodes(RIAT_NodeArrayContainer *nodes, const RIAT_ScriptGlobalArrayContainer *script_globals) {
    /* Nothing to remove...? */
    if(nodes->nodes_count == 0) {
        return;
    }

    size_t n = nodes->nodes_count - 1;
    while(true) {
        if(nodes->nodes[n].type == RIAT_VALUE_TYPE_UNPARSED) {
            remove_node(nodes, n, script_globals);
        }

        if(n == 0) {
            break;
        }
        n--;
    }
}

/* Recursively mark node and all of its descendents as unparsed */
static void recursively_disable_node(RIAT_NodeArrayContainer *nodes, size_t first_node) {
    assert(nodes->nodes_count > first_node);

    RIAT_Node *node = &nodes->nodes[first_node];

    /* Already disabled - we can stop here */
    if(node->type == RIAT_VALUE_TYPE_UNPARSED) {
        return;
    }

    /* Mark as unparsed (to be removed later) */
    node->type = RIAT_VALUE_TYPE_UNPARSED;

    /* Delete any following nodes */
    if(!node->is_primitive) {
        recursively_disable_node(nodes, node->child_node);
    }
    if(node->next_node != SIZE_MAX) {
        recursively_disable_node(nodes, node->next_node);
    }
}

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
        /* Passthrough is always OK */ \
        if(preferred_type == RIAT_VALUE_TYPE_PASSTHROUGH) { \
            preferred_type = actual_type; \
        } \
        else if( \
            /* Converting between real and int is OK */ \
            (preferred_type == RIAT_VALUE_TYPE_REAL && (actual_type == RIAT_VALUE_TYPE_LONG || actual_type == RIAT_VALUE_TYPE_SHORT)) || \
            (actual_type == RIAT_VALUE_TYPE_REAL && (preferred_type == RIAT_VALUE_TYPE_LONG || preferred_type == RIAT_VALUE_TYPE_SHORT)) || \
            \
            /* For whatever reason, demoting an int is OK even though it can possibly overflow/underflow, but promoting one is not...? */ \
            (preferred_type == RIAT_VALUE_TYPE_SHORT && actual_type == RIAT_VALUE_TYPE_LONG) || \
            (preferred_type == RIAT_VALUE_TYPE_BOOLEAN && (actual_type == RIAT_VALUE_TYPE_LONG || actual_type == RIAT_VALUE_TYPE_SHORT)) || \
            \
            /* Converting from an object to something else is OK */ \
            ((preferred_type == RIAT_VALUE_TYPE_OBJECT || preferred_type == RIAT_VALUE_TYPE_OBJECT_LIST) && (actual_type == RIAT_VALUE_TYPE_OBJECT || actual_type == RIAT_VALUE_TYPE_UNIT || actual_type == RIAT_VALUE_TYPE_WEAPON || actual_type == RIAT_VALUE_TYPE_SCENERY || actual_type == RIAT_VALUE_TYPE_VEHICLE || actual_type == RIAT_VALUE_TYPE_DEVICE)) \
        ) \
        { \
            actual_type = preferred_type; \
        } \
        else { \
            SYNTAX_ERROR_INSTANCE(instance, line, column, file); \
            snprintf(instance->last_compile_error.syntax_error_explanation, sizeof(instance->last_compile_error.syntax_error_explanation), "expected a %s, but %s cannot be converted into one", riat_value_type_to_string(preferred_type), riat_value_type_to_string(actual_type)); \
            return RIAT_COMPILE_SYNTAX_ERROR; \
        } \
    }

typedef struct ScriptGlobalLookup {
    bool is_definition;
    RIAT_ValueType value_type;

    union {
        const RIAT_BuiltinDefinition *definition;
        const RIAT_Global *global_local;
        const RIAT_Script *script_local;
    };
} ScriptGlobalLookup;

static void lowercase_string(char *s) {
    while(*s) {
        *s = tolower(*s);
        s++;
    }
}

static void debug_verify_no_unparsed(const RIAT_NodeArrayContainer *nodes, size_t node_to_check) {
    #ifndef NDEBUG
    RIAT_Node *n = &nodes->nodes[node_to_check];
    assert(n->type != RIAT_VALUE_TYPE_UNPARSED);
    assert(n->type != RIAT_VALUE_TYPE_PASSTHROUGH);

    if(n->next_node != NEXT_NODE_NULL) {
        debug_verify_no_unparsed(nodes, n->next_node);
    }
    if(!n->is_primitive) {
        debug_verify_no_unparsed(nodes, n->child_node);
    }
    #endif
}

static bool get_global(ScriptGlobalLookup *output, const char *name, const RIAT_ScriptGlobalArrayContainer *script_globals, RIAT_CompileTarget target) {
    /* Search local */
    for(size_t g = 0; g < script_globals->global_count; g++) {
        const RIAT_Global *global = &script_globals->globals[g];
        if(riat_case_insensitive_strcmp(global->name, name) == 0) {
            output->is_definition = false;
            output->global_local = global;
            output->value_type = global->value_type;
            return true;
        }
    }

    /* Search target */
    const RIAT_BuiltinDefinition *definition = riat_builtin_definition_search(name, target, RIAT_BUILTIN_DEFINITION_TYPE_GLOBAL);
    if(definition != NULL) {
        output->definition = definition;
        output->is_definition = true;
        output->value_type = definition->value_type;
        return true;
    }

    return false;
}

static bool get_function(ScriptGlobalLookup *output, const char *name, const RIAT_ScriptGlobalArrayContainer *script_globals, RIAT_CompileTarget target) {
    /* Search local */
    for(size_t s = 0; s < script_globals->script_count; s++) {
        const RIAT_Script *script = &script_globals->scripts[s];
        if(riat_case_insensitive_strcmp(script->name, name) == 0) {
            output->is_definition = false;
            output->script_local = script;
            output->value_type = script->return_type;
            return true;
        }
    }

    /* Search target */
    const RIAT_BuiltinDefinition *definition = riat_builtin_definition_search(name, target, RIAT_BUILTIN_DEFINITION_TYPE_FUNCTION);
    if(definition != NULL) {
        output->definition = definition;
        output->is_definition = true;
        output->value_type = definition->value_type;
        return true;
    }

    return false;
}

static bool resolve_node_to_global(RIAT_NodeArrayContainer *nodes, size_t node, const RIAT_ScriptGlobalArrayContainer *script_globals, RIAT_CompileTarget target) {
    ScriptGlobalLookup lookup;

    if(get_global(&lookup, nodes->nodes[node].string_data, script_globals, target)) {
        lowercase_string(nodes->nodes[node].string_data);
        nodes->nodes[node].is_global = true;
        nodes->nodes[node].type = lookup.value_type;
        return true;
    }
    else {
        return false;
    }
}

static bool resolve_node_to_function_call(ScriptGlobalLookup *lookup, RIAT_NodeArrayContainer *nodes, size_t node, const RIAT_ScriptGlobalArrayContainer *script_globals, RIAT_CompileTarget target) {
    /* Is this even a function call? */
    RIAT_Node *calling_node = &nodes->nodes[node];
    if(calling_node->is_primitive) {
        return false;
    }
    assert(calling_node->child_node < nodes->nodes_count);

    /* Find it! */
    RIAT_Node *function_name_node = &nodes->nodes[calling_node->child_node];
    assert(function_name_node->is_primitive);
    if(get_function(lookup, function_name_node->string_data, script_globals, target)) {
        lowercase_string(function_name_node->string_data);

        /* Make sure these  fields are set properly */
        function_name_node->type = RIAT_VALUE_TYPE_FUNCTION_NAME;
        calling_node->type = lookup->value_type;
        calling_node->is_script_call = !lookup->is_definition;

        return true;
    }
    else {
        return false;
    }
}

static RIAT_CompileResult resolve_type_of_element(RIAT_Instance *instance, RIAT_NodeArrayContainer *nodes, size_t node, RIAT_ValueType preferred_type, const RIAT_ScriptGlobalArrayContainer *script_globals, bool allow_uppercase) {
    RIAT_Node *n = &nodes->nodes[node];
    bool should_lowercase = true;

    if(n->is_primitive) {
        /* Resolve the global? */
        if(resolve_node_to_global(nodes, node, script_globals, instance->compile_target)) {
            CONVERT_TYPE_OR_DIE(preferred_type, n->type, n->line, n->column, n->file);
            return RIAT_COMPILE_OK;
        }

        /* Otherwise, convert the string I guess! */
        switch(preferred_type) {
            case RIAT_VALUE_TYPE_VOID:
                RESOLVE_TYPE_OF_ELEMENT_FAIL("a void value was expected; got '%s' instead", n->string_data);
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
                    RESOLVE_TYPE_OF_ELEMENT_FAIL("expected a boolean value (i.e. 'false'/'true'/'0'/'1'/'off'/'on'); got '%s' instead", n->string_data);
                }
                free(n->string_data);
                n->string_data = NULL;
                break;
            case RIAT_VALUE_TYPE_REAL:
                {
                    char *end = NULL;
                    n->real = strtof(n->string_data, &end);
                    if(end == n->string_data) {
                        RESOLVE_TYPE_OF_ELEMENT_FAIL("a real value was expected; got '%s' instead", n->string_data);
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
                        RESOLVE_TYPE_OF_ELEMENT_FAIL("a short value was expected; got '%s' instead", n->string_data);
                    }
                    if(v < INT16_MIN || v > INT16_MAX) {
                        RESOLVE_TYPE_OF_ELEMENT_FAIL("a short value was expected; got '%s' (out of range) instead", n->string_data);
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
                        RESOLVE_TYPE_OF_ELEMENT_FAIL("a long value was expected; got '%s' instead", n->string_data);
                    }
                    if(v < INT32_MIN || v > INT32_MAX) {
                        RESOLVE_TYPE_OF_ELEMENT_FAIL("a long value was expected; got '%s' (out of range) instead", n->string_data);
                    }
                    n->long_int = (long)v;
                }
                free(n->string_data);
                n->string_data = NULL;
                break;

            case RIAT_VALUE_TYPE_SCRIPT:
                for(size_t s = 0; s < script_globals->script_count; s++) {
                    if(riat_case_insensitive_strcmp(n->string_data, script_globals->scripts[s].name) == 0) {
                        n->short_int = (int16_t)(s);
                        lowercase_string(n->string_data);
                        goto end;
                    }
                }
                snprintf(instance->last_compile_error.syntax_error_explanation, sizeof(instance->last_compile_error.syntax_error_explanation), "expected a script name; got '%s' instead", n->string_data);
                SYNTAX_ERROR_INSTANCE(instance, n->line, n->column, n->file);
                return RIAT_COMPILE_SYNTAX_ERROR;

            case RIAT_VALUE_TYPE_GAME_DIFFICULTY:
                if(riat_case_insensitive_strcmp(n->string_data, "easy") == 0) {
                    n->short_int = 0;
                }
                else if(riat_case_insensitive_strcmp(n->string_data, "normal") == 0) {
                    n->short_int = 1;
                }
                else if(riat_case_insensitive_strcmp(n->string_data, "hard") == 0) {
                    n->short_int = 2;
                }
                else if(riat_case_insensitive_strcmp(n->string_data, "impossible") == 0) {
                    n->short_int = 3;
                }
                else {
                    RESOLVE_TYPE_OF_ELEMENT_FAIL("expected a difficulty value (i.e. 'easy'/'normal'/'hard'/'impossible'); got '%s' instead", n->string_data);
                }
                break;

            case RIAT_VALUE_TYPE_TEAM:
                if(riat_case_insensitive_strcmp(n->string_data, "player") == 0) {
                    n->short_int = 1;
                }
                else if(riat_case_insensitive_strcmp(n->string_data, "human") == 0) {
                    n->short_int = 2;
                }
                else if(riat_case_insensitive_strcmp(n->string_data, "covenant") == 0) {
                    n->short_int = 3;
                }
                else if(riat_case_insensitive_strcmp(n->string_data, "flood") == 0) {
                    n->short_int = 4;
                }
                else if(riat_case_insensitive_strcmp(n->string_data, "sentinel") == 0) {
                    n->short_int = 5;
                }
                else if(riat_case_insensitive_strcmp(n->string_data, "unused6") == 0) {
                    n->short_int = 6;
                }
                else if(riat_case_insensitive_strcmp(n->string_data, "unused7") == 0) {
                    n->short_int = 7;
                }
                else if(riat_case_insensitive_strcmp(n->string_data, "unused8") == 0) {
                    n->short_int = 8;
                }
                else if(riat_case_insensitive_strcmp(n->string_data, "unused9") == 0) {
                    n->short_int = 9;
                }
                else {
                    RESOLVE_TYPE_OF_ELEMENT_FAIL("expected a team value (i.e. 'player'/'human'/'covenant'/'flood'/'unused6'/'unused7'/'unused8'/'unused9'); got '%s' instead", n->string_data);
                }
                break;

            case RIAT_VALUE_TYPE_STRING:
                should_lowercase = !allow_uppercase;
                break;

            case RIAT_VALUE_TYPE_PASSTHROUGH:
                RESOLVE_TYPE_OF_ELEMENT_FAIL("a passthrough type was expected; cannot determine type of '%s'", n->string_data);

            default:
                break;
        }
        end:
        n->type = preferred_type;

        if(should_lowercase && n->string_data != NULL) {
            lowercase_string(n->string_data);
        }
    }
    else {
        return resolve_type_of_block(instance, nodes, node, preferred_type, script_globals);
    }

    return RIAT_COMPILE_OK;
}

static RIAT_CompileResult resolve_type_of_block(RIAT_Instance *instance, RIAT_NodeArrayContainer *nodes, size_t node, RIAT_ValueType preferred_type, const RIAT_ScriptGlobalArrayContainer *script_globals) {
    RIAT_Node *n = &nodes->nodes[node];

    /* Duh. This needs to be a block. */
    assert(!n->is_primitive);

    /* First element needs to be a function name */
    RIAT_Node *function_name_node = &nodes->nodes[n->child_node];
    assert(function_name_node->is_primitive);
    function_name_node->type = RIAT_VALUE_TYPE_FUNCTION_NAME;
    const char *function_name = function_name_node->string_data;

    size_t max_arguments = 0;

    /* Lookup the thing */
    ScriptGlobalLookup lookup;
    if(!resolve_node_to_function_call(&lookup, nodes, node, script_globals, instance->compile_target)) {
        const RIAT_BuiltinDefinition *def = riat_builtin_definition_search(function_name, RIAT_COMPILE_TARGET_ANY, RIAT_BUILTIN_DEFINITION_TYPE_FUNCTION);
        const char *suffix = "";

        if(def != NULL && def->type == RIAT_BUILTIN_DEFINITION_TYPE_FUNCTION) {
            suffix = " for the target engine (it is defined on another engine however)";
        }

        snprintf(instance->last_compile_error.syntax_error_explanation, sizeof(instance->last_compile_error.syntax_error_explanation), "no such function or script '%s' was defined%s", function_name, suffix);
        SYNTAX_ERROR_INSTANCE(instance, function_name_node->line, function_name_node->column, function_name_node->file);
        return RIAT_COMPILE_SYNTAX_ERROR;
    }

    if(lookup.is_definition) {
        max_arguments = lookup.definition->parameter_count;
    }

    /* Are there no arguments yet some were given? */
    if(max_arguments == 0) {
        if(function_name_node->next_node != NEXT_NODE_NULL) {
            RIAT_Node *element_node = &nodes->nodes[function_name_node->next_node];
            snprintf(instance->last_compile_error.syntax_error_explanation, sizeof(instance->last_compile_error.syntax_error_explanation), "'%s' takes no parameters but a parameter was given", function_name);
            SYNTAX_ERROR_INSTANCE(instance, element_node->line, element_node->column, element_node->file);
            return RIAT_COMPILE_SYNTAX_ERROR;
        }
    }

    /* Otherwise, let's parse them */
    else {
        assert(lookup.is_definition); /* scripts can't take arguments */

        size_t argument_index = 0;

        /* Special handling of these functions */
        if(strcmp(function_name, "set") == 0 || strcmp(function_name, "!=") == 0 || strcmp(function_name, "=") == 0) {
            assert(max_arguments == 2); /* duh */

            /* Get all of the parameters */
            size_t parameter_elements[2];
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
                if(strcmp(function_name, "set") == 0) {
                    RIAT_Node *global_name_node = &nodes->nodes[parameter_elements[0]];
                    if(!global_name_node->is_primitive) {
                        snprintf(instance->last_compile_error.syntax_error_explanation, sizeof(instance->last_compile_error.syntax_error_explanation), "set takes a global, but a function call was given instead");
                        SYNTAX_ERROR_INSTANCE(instance, global_name_node->line, global_name_node->column, global_name_node->file);
                        return RIAT_COMPILE_SYNTAX_ERROR;
                    }

                    /* Find it? */
                    const char *global_name = global_name_node->string_data;
                    if(!resolve_node_to_global(nodes, parameter_elements[0], script_globals, instance->compile_target)) {
                        const char *was_not_found = riat_builtin_definition_search(global_name, RIAT_COMPILE_TARGET_ANY, RIAT_BUILTIN_DEFINITION_TYPE_GLOBAL) == NULL ? "was not found" : "is not defined for the target engine (it is defined for another engine however)";
                        snprintf(instance->last_compile_error.syntax_error_explanation, sizeof(instance->last_compile_error.syntax_error_explanation), "set takes a global, but '%s' %s", global_name, was_not_found);
                        SYNTAX_ERROR_INSTANCE(instance, global_name_node->line, global_name_node->column, global_name_node->file);
                        return RIAT_COMPILE_SYNTAX_ERROR;
                    }
                    n->type = nodes->nodes[parameter_elements[0]].type;
                    
                    /* Try it */
                    RIAT_CompileResult result = resolve_type_of_element(instance, nodes, parameter_elements[1], n->type, script_globals, false);
                    if(result != RIAT_COMPILE_OK) {
                        return result;
                    }
                }
                else if(strcmp(function_name, "=") == 0 || strcmp(function_name, "!=") == 0) {
                    size_t e0 = parameter_elements[0], e1 = parameter_elements[1];
                    RIAT_Node *n0 = &nodes->nodes[e0], *n1 = &nodes->nodes[e1];

                    const RIAT_ValueType *g0 = NULL, *g1 = NULL;

                    if(n0->is_primitive && resolve_node_to_global(nodes, e0, script_globals, instance->compile_target)) {
                        g0 = &nodes->nodes[e0].type;
                    }
                    if(n1->is_primitive && resolve_node_to_global(nodes, e1, script_globals, instance->compile_target)) {
                        g1 = &nodes->nodes[e1].type;
                    }

                    /* If one is a global... */
                    if(g0 != NULL && g1 == NULL) {
                        RIAT_CompileResult result = resolve_type_of_element(instance, nodes, e1, n0->type, script_globals, false);
                        if(result != RIAT_COMPILE_OK) {
                            return result;
                        }
                    }

                    else if(g1 != NULL && g0 == NULL) {
                        RIAT_CompileResult result = resolve_type_of_element(instance, nodes, e0, n1->type, script_globals, false);
                        if(result != RIAT_COMPILE_OK) {
                            return result;
                        }
                    }

                    /* Both globals? */
                    else if(g1 != NULL && g0 != NULL) {
                        if(n0->type != n1->type) {
                            snprintf(instance->last_compile_error.syntax_error_explanation, sizeof(instance->last_compile_error.syntax_error_explanation), "%s requires both arguments to be the same type, but '%s' is a type %s and '%s' is a type %s",
                                                                                                                                                           function_name, 
                                                                                                                                                           n0->string_data,
                                                                                                                                                           riat_value_type_to_string(n0->type),
                                                                                                                                                           n1->string_data, 
                                                                                                                                                           riat_value_type_to_string(n1->type));
                            SYNTAX_ERROR_INSTANCE(instance, n0->line, n0->column, n0->file);
                        }

                        return RIAT_COMPILE_OK;
                    }

                    /* If they are both not globals, try to see if one is a block? Otherwise, test them as reals. */
                    else if(g0 == NULL && g1 == NULL) {
                        RIAT_ValueType test_type = RIAT_VALUE_TYPE_REAL;

                        if(!n0->is_primitive) {
                            ScriptGlobalLookup lookup;
                            if(get_function(&lookup, nodes->nodes[n0->child_node].string_data, script_globals, instance->compile_target)) {
                                test_type = lookup.value_type;
                            }
                        }
                        else if(!n1->is_primitive) {
                            ScriptGlobalLookup lookup;
                            if(get_function(&lookup, nodes->nodes[n1->child_node].string_data, script_globals, instance->compile_target)) {
                                test_type = lookup.value_type;
                            }
                        }

                        RIAT_CompileResult result;
                        result = resolve_type_of_element(instance, nodes, e0, test_type, script_globals, false);
                        if(result != RIAT_COMPILE_OK) {
                            return result;
                        }
                        result = resolve_type_of_element(instance, nodes, e1, test_type, script_globals, false);
                        if(result != RIAT_COMPILE_OK) {
                            return result;
                        }
                    }
                }
            }
        }

        /* Everything else... */
        else {
            bool return_type_is_passthrough = n->type == RIAT_VALUE_TYPE_PASSTHROUGH;
            n->type = preferred_type;

            for(size_t element = function_name_node->next_node; element != NEXT_NODE_NULL; argument_index++) {
                assert(element < nodes->nodes_count);

                RIAT_Node *element_node = &nodes->nodes[element];
                const RIAT_BuiltinFunctionParameter *parameter;

                /* Did we exceed the max number of arguments? If so, check if the last argument has 'many' set. */
                if(argument_index >= max_arguments) {
                    parameter = &lookup.definition->parameters[max_arguments - 1];
                    if(!parameter->many) {
                        snprintf(instance->last_compile_error.syntax_error_explanation, sizeof(instance->last_compile_error.syntax_error_explanation), "'%s' takes %zu parameter%s, but more were given", function_name, max_arguments, max_arguments == 1 ? "" : "s");
                        SYNTAX_ERROR_INSTANCE(instance, element_node->line, element_node->column, element_node->file);
                        return RIAT_COMPILE_SYNTAX_ERROR;
                    }
                }

                /* No? OK! */
                else {
                    parameter = &lookup.definition->parameters[argument_index];
                }

                RIAT_ValueType this_element_preferred_type = parameter->type;

                /* If the last element is passthrough and the type changes, then there is some special behavior here */
                if(this_element_preferred_type == RIAT_VALUE_TYPE_PASSTHROUGH && return_type_is_passthrough) {
                    /* If it's not the last type and we only passthrough the last, then it's void */
                    if(parameter->passthrough_last && element_node->next_node != NEXT_NODE_NULL) {
                        this_element_preferred_type = RIAT_VALUE_TYPE_VOID;
                    }

                    /* Otherwise, set to our return value */
                    else {
                        this_element_preferred_type = n->type;
                    }
                }

                /* Punch it! */
                RIAT_CompileResult result = resolve_type_of_element(instance, nodes, element, this_element_preferred_type, script_globals, parameter->allow_uppercase);
                if(result != RIAT_COMPILE_OK) {
                    return result;
                }

                if(return_type_is_passthrough && this_element_preferred_type == RIAT_VALUE_TYPE_PASSTHROUGH) {
                    n->type = nodes->nodes[element].type;
                }

                /* We should know the type of our element now */
                assert(nodes->nodes[element].type != RIAT_VALUE_TYPE_PASSTHROUGH);

                element = element_node->next_node;
            }

            /* We should know the type by now */
            assert(n->type != RIAT_VALUE_TYPE_PASSTHROUGH);
        }

        /* Not enough arguments? */
        if(argument_index < max_arguments) {
            if(!lookup.definition->parameters[argument_index].optional) {
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

#define COMPILE_RETURN_ERROR(what, file, line, column, explanation_fmt, ...) \
    if(explanation_fmt) snprintf(instance->last_compile_error.syntax_error_explanation, sizeof(instance->last_compile_error.syntax_error_explanation), explanation_fmt, __VA_ARGS__); \
    instance->last_compile_error.syntax_error_line = line; \
    instance->last_compile_error.syntax_error_file = file; \
    instance->last_compile_error.syntax_error_column = column; \
    result = what; \
    goto end;

RIAT_CompileResult riat_tree(RIAT_Instance *instance) {
    RIAT_CompileResult result = RIAT_COMPILE_OK;

    RIAT_Token *tokens = instance->tokens.tokens;
    size_t token_count = instance->tokens.token_count;

    RIAT_ScriptGlobalArrayContainer script_global_list;
    memset(&script_global_list, 0, sizeof(script_global_list));
    size_t script_process_count = 0;
    size_t global_process_count = 0;

    RIAT_NodeArrayContainer node_array;
    node_array.nodes = NULL;
    node_array.nodes_count = 0;
    node_array.nodes_capacity = 0;

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
                COMPILE_RETURN_ERROR(RIAT_COMPILE_SYNTAX_ERROR, token->file, token->line, token->column, "expected left parenthesis, got '%s'", token->token_string);
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
                COMPILE_RETURN_ERROR(RIAT_COMPILE_SYNTAX_ERROR, token->file, token_next->line, token_next->column, "expected 'global' or 'script', got %s", token_next->token_string);
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
            COMPILE_RETURN_ERROR(RIAT_COMPILE_ALLOCATION_ERROR, 0, 0, 0, "%i", 0);
        }
    }
    if(script_global_list.global_count > 0) {
        if((script_global_list.globals = calloc(sizeof(*script_global_list.globals), script_global_list.global_count)) == NULL) {
            COMPILE_RETURN_ERROR(RIAT_COMPILE_ALLOCATION_ERROR, 0, 0, 0, "%i", 0);
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
            relevant_global->value_type = riat_value_type_from_string(global_type_token->token_string, &error);
            if(global_type_token->parenthesis != 0 || error) {
                COMPILE_RETURN_ERROR(RIAT_COMPILE_SYNTAX_ERROR, global_type_token->file, global_type_token->line, global_type_token->column, "expected global type, got '%s'", global_type_token->token_string);
            }

            /* And the name */
            RIAT_Token *global_name_token = &tokens[ti++];
            if(global_name_token->parenthesis != 0) {
                COMPILE_RETURN_ERROR(RIAT_COMPILE_SYNTAX_ERROR, global_type_token->file, global_name_token->line, global_name_token->column, "expected global name, got '%s'", global_name_token->token_string);
            }
            if(strlen(global_name_token->token_string) > sizeof(relevant_global->name) - 1) {
                COMPILE_RETURN_ERROR(RIAT_COMPILE_SYNTAX_ERROR, global_type_token->file, global_name_token->line, global_name_token->column, "global name '%s' is too long", global_name_token->token_string);
            }
            strncpy(relevant_global->name, global_name_token->token_string, sizeof(relevant_global->name) - 1);
            lowercase_string(relevant_global->name);

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
            relevant_script->script_type = riat_script_type_from_string(script_type_token->token_string, &error);
            if(script_type_token->parenthesis != 0 || error) {
                COMPILE_RETURN_ERROR(RIAT_COMPILE_SYNTAX_ERROR, script_type_token->file, script_type_token->line, script_type_token->column, "expected script type, got '%s'", script_type_token->token_string);
            }

            /* If it's a static script, a type is expected */
            if(relevant_script->script_type == RIAT_SCRIPT_TYPE_STATIC || relevant_script->script_type == RIAT_SCRIPT_TYPE_STUB) {
                RIAT_Token *script_type_token = &tokens[ti++];
                relevant_script->return_type = riat_value_type_from_string(script_type_token->token_string, &error);
                if(script_type_token->parenthesis != 0 || error) {
                    COMPILE_RETURN_ERROR(RIAT_COMPILE_SYNTAX_ERROR, script_type_token->file, script_type_token->line, script_type_token->column, "expected script type, got '%s'", script_type_token->token_string);
                }
            }

            /* Otherwise, void */
            else {
                relevant_script->return_type = RIAT_VALUE_TYPE_VOID;
            }

            /* And the name */
            RIAT_Token *script_name_token = &tokens[ti++];
            if(script_name_token->parenthesis != 0) {
                COMPILE_RETURN_ERROR(RIAT_COMPILE_SYNTAX_ERROR, script_type_token->file, script_name_token->line, script_name_token->column, "expected script name, got '%s'", script_name_token->token_string);
            }
            if(strlen(script_name_token->token_string) > sizeof(relevant_script->name) - 1) {
                COMPILE_RETURN_ERROR(RIAT_COMPILE_SYNTAX_ERROR, script_name_token->file, script_name_token->line, script_name_token->column, "script name '%s' is too long", script_name_token->token_string);
            }
            strncpy(relevant_script->name, script_name_token->token_string, sizeof(relevant_script->name) - 1);
            lowercase_string(relevant_script->name);

            /* First get the root node */
            size_t root_node_index;
            result = read_block(instance, tokens, &ti, &node_array, &script_global_list, &root_node_index, true);
            if(result != RIAT_COMPILE_OK) {
                goto end;
            }

            bool add_begin_block = true;

            /* If we're optimizing, we shouldn't add a begin block if the root node already has a begin block (or if we're more aggressive about it, at all if possible?) */
            if(instance->optimization_level >= RIAT_OPTIMIZATION_PREVENT_GENERATIONAL_LOSS) {
                /* Get the first node in the block */
                RIAT_Node *root_node = &node_array.nodes[root_node_index];
                size_t child_node_index = root_node->child_node;
                RIAT_Node *child_node = &node_array.nodes[child_node_index];

                /* If the child node is a function call and has no "next_node", let's proceed */
                if(child_node->next_node == NEXT_NODE_NULL && !child_node->is_primitive) {
                    size_t function_name_index = child_node->child_node;
                    assert(node_array.nodes_count > function_name_index);
                    RIAT_Node *function_name = &node_array.nodes[function_name_index];

                    /* If optimizing, we can remove it if it's a begin block or, if optimization level is 2 or greater, always remove it */
                    if(strcmp(function_name->string_data, "begin") == 0 || instance->optimization_level >= RIAT_OPTIMIZATION_DEDUPE_EXTRA) {
                        add_begin_block = false;
                        root_node->type = RIAT_VALUE_TYPE_UNPARSED; /* marking it as unparsed to remove it later */
                        root_node_index = child_node_index;
                    }
                }
            }

            /* Implicitly add a begin block if we need to */
            if(add_begin_block) {
                size_t original_first_node_index = node_array.nodes[root_node_index].child_node;
                size_t begin_name = append_node_to_node_array(&node_array, "begin", block_type_token->file, block_type_token->line, block_type_token->column);
                if(begin_name == APPEND_NODE_ALLOCATION_ERROR) {
                    result = RIAT_COMPILE_ALLOCATION_ERROR;
                    goto end;
                }
                node_array.nodes[begin_name].is_primitive = true;
                node_array.nodes[begin_name].next_node = original_first_node_index;
                node_array.nodes[root_node_index].child_node = begin_name;
            }
            
            relevant_script->first_node = root_node_index;
            relevant_script->line = block_type_token->line;
            relevant_script->column = block_type_token->column;
            relevant_script->file = block_type_token->file;
        }
    }

    /* Resolve types for all nodes (auto-break if result is not OK) */
    for(size_t g = 0; g < script_global_list.global_count && result == RIAT_COMPILE_OK; g++) {
        RIAT_Global *global = &script_global_list.globals[g];
        result = resolve_type_of_element(instance, &node_array, global->first_node, global->value_type, &script_global_list, false);
    }

    for(size_t s = 0; s < script_global_list.script_count && result == RIAT_COMPILE_OK; s++) {
        RIAT_Script *script = &script_global_list.scripts[s];
        result = resolve_type_of_element(instance, &node_array, script->first_node, script->return_type, &script_global_list, false);
    }

    if(result != RIAT_COMPILE_OK) {
        goto end;
    }

    /* Verify no script or global tree has an unparsed node somewhere */
    #define VERIFY_NO_UNPARSED \
    if(node_array.nodes_count > 0) { \
        for(size_t g = 0; g < script_global_list.global_count && result == RIAT_COMPILE_OK; g++) { \
            RIAT_Global *global = &script_global_list.globals[g]; \
            debug_verify_no_unparsed(&node_array, global->first_node); \
        } \
        for(size_t s = 0; s < script_global_list.script_count && result == RIAT_COMPILE_OK; s++) { \
            RIAT_Script *script = &script_global_list.scripts[s]; \
            debug_verify_no_unparsed(&node_array, script->first_node); \
        } \
    }

    VERIFY_NO_UNPARSED

    /* Resolve stubbed functions */
    for(size_t s = 0; s < script_global_list.script_count; s++) {
        while(s < script_global_list.script_count) { /* Loop until we break or we resolved the last stub */
            bool should_remove = false;

            RIAT_Script *script = &script_global_list.scripts[s];
            if(script->script_type == RIAT_SCRIPT_TYPE_STUB) {
                for(size_t t = 0; t < script_global_list.script_count; t++) {
                    if(t == s) {
                        continue; /* don't compare the script against itself */
                    }
                    RIAT_Script *script_other = &script_global_list.scripts[t];

                    /* It's a match? */
                    if(strcmp(script_other->name, script->name) == 0) {
                        /* Make sure we can replace it */
                        if(script_other->script_type != RIAT_SCRIPT_TYPE_STATIC) {
                            COMPILE_RETURN_ERROR(RIAT_COMPILE_SYNTAX_ERROR, script_other->file, script_other->line, script_other->column, "cannot replace stub script '%s' with non-static script '%s'", script->name, script_other->name);
                        }
                        else if(script_other->return_type != script->return_type) {
                            COMPILE_RETURN_ERROR(RIAT_COMPILE_SYNTAX_ERROR, script_other->file, script_other->line, script_other->column, "cannot replace stub script '%s' (returns %s) with script '%s' (returns %s)", script->name, riat_value_type_to_string(script->return_type), script_other->name, riat_value_type_to_string(script_other->return_type));
                        }

                        /* Done! */
                        should_remove = true;
                        break;
                    }
                }
            }

            /* Delete the script? */
            if(should_remove) {
                recursively_disable_node(&node_array, script->first_node);

                script_global_list.script_count--;

                for(size_t t = s; t < script_global_list.script_count; t++) {
                    RIAT_Script *a = &script_global_list.scripts[t];
                    RIAT_Script *b = &script_global_list.scripts[t + 1];
                    memcpy(a, b, sizeof(*a));
                }

                /* Restart the loop iteration on s again */
                continue;
            }

            break;
        }
    }

    /* Remove deleted nodes */
    remove_disabled_nodes(&node_array, &script_global_list);
    VERIFY_NO_UNPARSED

    /* Set script call node indices */
    for(size_t n = 0; n < node_array.nodes_count; n++) {
        RIAT_Node *node = &node_array.nodes[n];
        if(node->is_script_call) {
            assert(node->child_node < node_array.nodes_count);
            RIAT_Node *function_name = &node_array.nodes[node->child_node];
            assert(function_name->string_data != NULL && function_name->type == RIAT_VALUE_TYPE_FUNCTION_NAME);

            bool found_node = false;
            for(size_t s = 0; s < script_global_list.script_count && !found_node; s++) {
                if(strcmp(function_name->string_data, script_global_list.scripts[s].name) == 0) {
                    node->call_index = s;
                    found_node = true;
                }
            }

            assert(found_node);
        }
    }

    /* Ensure scripts/globals do not have name collisions, since now that any removable stubs are gone, they should be unique */
    for(size_t s = 0; s < script_global_list.script_count; s++) {
        RIAT_Script *script = &script_global_list.scripts[s];
        for(size_t t = s + 1; t < script_global_list.script_count; t++) {
            RIAT_Script *script_other = &script_global_list.scripts[t];
            if(strcmp(script->name, script_other->name) == 0) {
                COMPILE_RETURN_ERROR(RIAT_COMPILE_SYNTAX_ERROR, script_other->file, script_other->line, script_other->column, "multiple scripts of the name '%s' exist", script->name);
            }
        }
    }
    for(size_t g = 0; g < script_global_list.global_count; g++) {
        RIAT_Global *global = &script_global_list.globals[g];
        for(size_t h = g + 1; h < script_global_list.global_count; h++) {
            RIAT_Global *global_other = &script_global_list.globals[h];
            if(strcmp(global->name, global_other->name) == 0) {
                COMPILE_RETURN_ERROR(RIAT_COMPILE_SYNTAX_ERROR, global_other->file, global_other->line, global_other->column, "multiple globals of the name '%s' exist", global->name);
            }
        }

        /* Check if a script by the same name exists. If so, it's fine but we should warn */
        for(size_t s = 0; s < script_global_list.script_count; s++) {
            RIAT_Script *script = &script_global_list.scripts[s];
            if(strcmp(script->name, global->name) == 0) {
                char message[256];
                snprintf(message, sizeof(message), "a global and script of the name '%s' already exists", global->name);

                bool is_script_that_is_first = false;

                /* Check if the first instance is a script or a global */
                if(global->file > script->file) {
                    is_script_that_is_first = true;
                }
                else if(global->file == script->file) {
                    if(global->line > script->line) {
                        is_script_that_is_first = true;
                    }
                    else if(global->line == script->line) {
                        is_script_that_is_first = global->column > script->column;
                    }
                }

                /* Show the next instance */
                size_t file, column, line;
                if(!is_script_that_is_first) {
                    file = script->file;
                    column = script->column;
                    line = script->line;
                }
                else {
                    file = global->file;
                    column = global->column;
                    line = global->line;
                }

                instance->warn_callback(instance, message, instance->files.file_names[file], line, column);
            }
        }
    }

    /* Clear the old results */
    riat_clear_node_array_container(&instance->last_compile_result.nodes);
    riat_clear_script_global_array_container(&instance->last_compile_result.script_globals);

    /* Copy the pointer/counts over */
    instance->last_compile_result.script_globals = script_global_list;
    instance->last_compile_result.nodes = node_array;

    /* Zero these out */
    memset(&node_array, 0, sizeof(node_array));
    memset(&script_global_list, 0, sizeof(script_global_list));

    end:

    /* Free our temporary results (if we have any) */
    riat_clear_script_global_array_container(&script_global_list);
    riat_clear_node_array_container(&node_array);

    return result;
}

static RIAT_CompileResult read_block(
    RIAT_Instance *instance,
    RIAT_Token *tokens,
    size_t *ti,
    RIAT_NodeArrayContainer *nodes,
    RIAT_ScriptGlobalArrayContainer *current_script_info,
    size_t *root_node,
    bool is_script_block
) {
    /* Do some checks on the first token */
    RIAT_Token *current_token = &tokens[*ti];

    /* First, is this a cond block? If so... this is going to be interesting */
    bool is_cond = !is_script_block && current_token->parenthesis == 0 && riat_case_insensitive_strcmp(current_token->token_string, "cond") == 0;

    /* Also, this should NOT be a left parenthesis unless it's the first thing in a script */
    if(!is_script_block && current_token->parenthesis == 1) {
        strncpy(instance->last_compile_error.syntax_error_explanation, "block starts with an expression (expected function name)", sizeof(instance->last_compile_error.syntax_error_explanation) - 1);
        SYNTAX_ERROR_INSTANCE(instance, current_token->line, current_token->column, current_token->file);
        return RIAT_COMPILE_SYNTAX_ERROR;
    }

    /* Also make sure it isn't empty */
    if(current_token->parenthesis == -1) {
        strncpy(instance->last_compile_error.syntax_error_explanation, "block is empty (unexpected left parenthesis)", sizeof(instance->last_compile_error.syntax_error_explanation) - 1);
        SYNTAX_ERROR_INSTANCE(instance, current_token->line, current_token->column, current_token->file);
        return RIAT_COMPILE_SYNTAX_ERROR;
    }

    /* Is this cond? If so, each thing needs to be turned into an if/else block */
    if(is_cond) {
        /* Require one block first */
        (*ti)++;
        RIAT_Token *current_token = &tokens[*ti];
        if(current_token->parenthesis == -1) {
            strncpy(instance->last_compile_error.syntax_error_explanation, "cond requires at least one block enclosed in parenthesis (<condition> <result...>)", sizeof(instance->last_compile_error.syntax_error_explanation) - 1);
            SYNTAX_ERROR_INSTANCE(instance, current_token->line, current_token->column, current_token->file);
            return RIAT_COMPILE_SYNTAX_ERROR;
        }

        bool root_node_set = false;
        size_t else_node_predecessor = APPEND_NODE_ALLOCATION_ERROR;

        while(current_token->parenthesis != -1) {
            /* Make sure it's a block */
            if(current_token->parenthesis != 1) {
                strncpy(instance->last_compile_error.syntax_error_explanation, "cond requires blocks enclosed in parenthesis (<condition> <result...>)", sizeof(instance->last_compile_error.syntax_error_explanation) - 1);
                SYNTAX_ERROR_INSTANCE(instance, current_token->line, current_token->column, current_token->file);
                return RIAT_COMPILE_SYNTAX_ERROR;
            }

            /* If so, increment 1 to look at the expression */
            (*ti)++;

            /* Get the condition */
            size_t if_expression_node_index;

            RIAT_CompileResult result = read_next_element(instance, tokens, ti, nodes, current_script_info, &if_expression_node_index);
            if(result != RIAT_COMPILE_OK) {
                return result;
            }

            if(tokens[*ti].parenthesis == -1) {
                strncpy(instance->last_compile_error.syntax_error_explanation, "cond requires a return value after the condition (<condition> <result...>)", sizeof(instance->last_compile_error.syntax_error_explanation) - 1);
                SYNTAX_ERROR_INSTANCE(instance, current_token->line, current_token->column, current_token->file);
                return RIAT_COMPILE_SYNTAX_ERROR;
            }

            /* And now the result - add a "begin" block here with the contents being everything in this block */
            size_t then_expression_node_index;
            result = read_block(instance, tokens, ti, nodes, current_script_info, &then_expression_node_index, true);
            if(result != RIAT_COMPILE_OK) {
                return result;
            }

            /* Make our begin function name now. */
            size_t begin_function_name_index = append_node_to_node_array(nodes, "begin", current_token->file, current_token->line, current_token->column);
            if(begin_function_name_index == APPEND_NODE_ALLOCATION_ERROR) {
                return RIAT_COMPILE_ALLOCATION_ERROR;
            }
            nodes->nodes[begin_function_name_index].is_primitive = true;

            /* We already have the function call from the then_expression_node_index, so we can slot the 'begin' block at the start here */
            size_t previous_child = nodes->nodes[then_expression_node_index].child_node;
            nodes->nodes[then_expression_node_index].child_node = begin_function_name_index;
            nodes->nodes[begin_function_name_index].next_node = previous_child;

            /* Make this an if statement */
            size_t if_fn_name_call_index = append_node_to_node_array(nodes, "if", current_token->file, current_token->line, current_token->column);
            if(if_fn_name_call_index == APPEND_NODE_ALLOCATION_ERROR) {
                return RIAT_COMPILE_ALLOCATION_ERROR;
            }
            nodes->nodes[if_fn_name_call_index].is_primitive = true;

            size_t if_fn_call_index = append_node_to_node_array(nodes, NULL, current_token->file, current_token->line, current_token->column);
            if(if_fn_call_index == APPEND_NODE_ALLOCATION_ERROR) {
                return RIAT_COMPILE_ALLOCATION_ERROR;
            }

            /* Associate that function name with the call */
            nodes->nodes[if_fn_call_index].child_node = if_fn_name_call_index;

            /* And then put the 'result' expression after the condition */
            nodes->nodes[if_fn_name_call_index].next_node = if_expression_node_index;
            nodes->nodes[if_expression_node_index].next_node = then_expression_node_index;

            /* If we don't have a root note set, that if statement looks like a good one */
            if(!root_node_set) {
                *root_node = if_fn_call_index;
                root_node_set = true;
            }

            /* Otherwise, set the next node of our last if block (the "else") to the new if block we just made */
            else {
                /* add the 'else' node if we have another node here */
                nodes->nodes[else_node_predecessor].next_node = if_fn_call_index;
            }

            else_node_predecessor = then_expression_node_index;

            current_token = &tokens[*ti];
        }
    }

    else {
        /* Push the root node */
        if((*root_node = append_node_to_node_array(nodes, NULL, current_token->file, current_token->line, current_token->column)) == APPEND_NODE_ALLOCATION_ERROR) {
            return RIAT_COMPILE_ALLOCATION_ERROR;
        }

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
    }

    /* Increment the token incrementor one more time */
    (*ti)++;

    return RIAT_COMPILE_OK;
}

static RIAT_CompileResult read_next_element(
    RIAT_Instance *instance,
    RIAT_Token *tokens,
    size_t *ti,
    RIAT_NodeArrayContainer *nodes,
    RIAT_ScriptGlobalArrayContainer *current_script_info,
    size_t *node
) {
    RIAT_Token *first_token = &tokens[*ti];
    (*ti)++;

    /* This should NOT be a right parenthesis */
    assert(first_token->parenthesis != -1);

    if(first_token->parenthesis == 0) {
        if((*node = append_node_to_node_array(nodes, first_token->token_string, first_token->file, first_token->line, first_token->column)) == APPEND_NODE_ALLOCATION_ERROR) {
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
        
    RIAT_Node *element_node = &nodes->nodes[*node];
    element_node->is_primitive = (first_token->parenthesis == 0);
    return RIAT_COMPILE_OK;
}

static size_t append_node_to_node_array(RIAT_NodeArrayContainer *container, const char *string_data, size_t file, size_t line, size_t column) {
    /* Capacity met? If so, reallocate with double size (or set to 16 if size is 0) */
    if(container->nodes_capacity == container->nodes_count) {
        size_t new_capacity = container->nodes_capacity > 0 ? container->nodes_capacity * 2 : 16;
        RIAT_Node *new_array = realloc(container->nodes, sizeof(*container->nodes) * new_capacity);

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
    RIAT_Node *next_node = &container->nodes[next_node_index];
    memset(next_node, 0, sizeof(*next_node));
    next_node->next_node = NEXT_NODE_NULL;
    next_node->string_data = string_data != NULL ? riat_strdup(string_data) : NULL;
    next_node->file = file;
    next_node->line = line;
    next_node->column = column;
    return next_node_index;
}
