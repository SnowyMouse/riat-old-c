#include "riat_internal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

static RIAT_ValueType get_value_type(const char *type, bool *error);
static RIAT_CompileResult get_next_block(RIAT_Token *tokens, size_t *ti);
static RIAT_ScriptType get_script_type(const char *type, bool *error);

#define COMPILE_RETURN_ERROR(what, line, column, explanation_fmt, ...) \
    if(explanation_fmt) snprintf(instance->last_compile_error.syntax_error_explanation, sizeof(instance->last_compile_error.syntax_error_explanation), explanation_fmt, __VA_ARGS__); \
    instance->last_compile_error.syntax_error_line = line; \
    instance->last_compile_error.syntax_error_column = column; \
    result = what; \
    goto end;

RIAT_CompileResult RIAT_tree(RIAT_Instance *instance, RIAT_Token *tokens, size_t token_count) {
    RIAT_CompileResult result = RIAT_COMPILE_OK;

    size_t script_count = 0;
    size_t script_process_count = 0;
    RIAT_Script *scripts = NULL;

    size_t global_count = 0;
    size_t global_process_count = 0;
    RIAT_Global *globals = NULL;

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
                global_count++;
            }
            else if(strcmp(token_next->token_string, "script") == 0) {
                script_count++;
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
    if(script_count > 0) {
        if((scripts = calloc(sizeof(*scripts), script_count)) == NULL) {
            COMPILE_RETURN_ERROR(RIAT_COMPILE_ALLOCATION_ERROR, 0, 0, NULL, NULL);
        }
    }
    if(global_count > 0) {
        if((globals = calloc(sizeof(*globals), global_count)) == NULL) {
            COMPILE_RETURN_ERROR(RIAT_COMPILE_ALLOCATION_ERROR, 0, 0, NULL, NULL);
        }
    }

    /* Now process all the things */
    for(size_t ti = 0; ti < token_count; /* no incrementor necessary */) {
        /* We know for sure that the next three tokens are valid */
        assert(ti + 3 < token_count);

        /* Get 'em */
        RIAT_Token *block_open_token = &tokens[ti++];
        RIAT_Token *block_type_token = &tokens[ti++];
        
        /* Now we know for certain this is a left parenthesis */
        assert(block_open_token->parenthesis == 1);
        assert(block_type_token->parenthesis == 0);

        /* Global? */
        if(strcmp(block_type_token->token_string, "global") == 0) {
            /* We should not overflow here. If the above check is written correctly, this won't happen. */
            assert(global_count > global_process_count);
            RIAT_Global *relevant_global = &globals[global_process_count++];

            bool error;

            /* Get the type */
            RIAT_Token *global_type_token = &tokens[ti++];
            relevant_global->value_type = get_value_type(global_type_token->token_string, &error);
            if(global_type_token->parenthesis != 0 || error) {
                COMPILE_RETURN_ERROR(RIAT_COMPILE_SYNTAX_ERROR, global_type_token->line, global_type_token->column, "expected global type, got '%s'", global_type_token->token_string);
            }

            /* And the name */
            RIAT_Token *global_name_token = &tokens[ti++];
            if(global_name_token->parenthesis != 0) {
                COMPILE_RETURN_ERROR(RIAT_COMPILE_SYNTAX_ERROR, global_name_token->line, global_name_token->column, "expected global name, got '%s'", global_name_token->token_string);
            }
            strncpy(relevant_global->name, global_name_token->token_string, sizeof(relevant_global->name) - 1);

            /* TODO: Parse global */
            get_next_block(tokens, &ti);
        }

        else if(strcmp(block_type_token->token_string, "script") == 0) {
            /* We should not overflow here. If the above check is written correctly, this won't happen. */
            assert(script_count > script_process_count);
            RIAT_Script *relevant_script = &scripts[script_process_count++];

            bool error;

            /* Get the type */
            RIAT_Token *script_type_token = &tokens[ti++];
            relevant_script->script_type = get_script_type(script_type_token->token_string, &error);
            if(script_type_token->parenthesis != 0 || error) {
                COMPILE_RETURN_ERROR(RIAT_COMPILE_SYNTAX_ERROR, script_type_token->line, script_type_token->column, "expected script type, got '%s'", script_type_token->token_string);
            }

            /* If it's a static script, a type is expected */
            if(relevant_script->script_type == RIAT_SCRIPT_TYPE_STATIC || relevant_script->script_type == RIAT_SCRIPT_TYPE_STUB) {
                RIAT_Token *script_type_token = &tokens[ti++];
                relevant_script->return_type = get_value_type(script_type_token->token_string, &error);
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

            get_next_block(tokens, &ti);
        }
    }

    for(size_t g = 0; g < global_count; g++) {
        printf("DEBUG: Global #%zu = '%s'\n", g, globals[g].name);
    }

    for(size_t s = 0; s < script_count; s++) {
        printf("DEBUG: Script #%zu = '%s'\n", s, scripts[s].name);
    }

    printf("DEBUG: %zu global%s and %zu script%s\n", global_count, global_count == 1 ? "" : "s", script_count, script_count == 1 ? "" : "s");

    end:
    free(scripts);
    free(globals);
    RIAT_token_free_array(tokens, token_count);
    return result;
}

static RIAT_CompileResult get_next_block(RIAT_Token *tokens, size_t *ti) {
    long depth = 1;
    while(depth > 0) {
        depth += tokens[(*ti)++].parenthesis; /* stub for now */
    }

    return RIAT_COMPILE_OK;
}

static RIAT_ValueType get_value_type(const char *type, bool *error) {
    *error = false;
    if(strcmp(type, "void") == 0) {
        return RIAT_VALUE_TYPE_VOID;
    }
    else if(strcmp(type, "boolean") == 0) {
        return RIAT_VALUE_TYPE_BOOLEAN;
    }
    else if(strcmp(type, "real") == 0) {
        return RIAT_VALUE_TYPE_REAL;
    }
    else if(strcmp(type, "short") == 0) {
        return RIAT_VALUE_TYPE_SHORT;
    }
    else if(strcmp(type, "long") == 0) {
        return RIAT_VALUE_TYPE_LONG;
    }
    else if(strcmp(type, "string") == 0) {
        return RIAT_VALUE_TYPE_STRING;
    }
    else if(strcmp(type, "script") == 0) {
        return RIAT_VALUE_TYPE_SCRIPT;
    }
    else if(strcmp(type, "trigger_volume") == 0) {
        return RIAT_VALUE_TYPE_TRIGGER_VOLUME;
    }
    else if(strcmp(type, "cutscene_flag") == 0) {
        return RIAT_VALUE_TYPE_CUTSCENE_FLAG;
    }
    else if(strcmp(type, "cutscene_camera_point") == 0) {
        return RIAT_VALUE_TYPE_CUTSCENE_CAMERA_POINT;
    }
    else if(strcmp(type, "cutscene_title") == 0) {
        return RIAT_VALUE_TYPE_CUTSCENE_TITLE;
    }
    else if(strcmp(type, "cutscene_recording") == 0) {
        return RIAT_VALUE_TYPE_CUTSCENE_RECORDING;
    }
    else if(strcmp(type, "device_group") == 0) {
        return RIAT_VALUE_TYPE_DEVICE_GROUP;
    }
    else if(strcmp(type, "ai") == 0) {
        return RIAT_VALUE_TYPE_AI;
    }
    else if(strcmp(type, "ai_command_list") == 0) {
        return RIAT_VALUE_TYPE_AI_COMMAND_LIST;
    }
    else if(strcmp(type, "starting_profile") == 0) {
        return RIAT_VALUE_TYPE_STARTING_PROFILE;
    }
    else if(strcmp(type, "conversation") == 0) {
        return RIAT_VALUE_TYPE_CONVERSATION;
    }
    else if(strcmp(type, "navpoint") == 0) {
        return RIAT_VALUE_TYPE_NAVPOINT;
    }
    else if(strcmp(type, "hud_message") == 0) {
        return RIAT_VALUE_TYPE_HUD_MESSAGE;
    }
    else if(strcmp(type, "object_list") == 0) {
        return RIAT_VALUE_TYPE_OBJECT_LIST;
    }
    else if(strcmp(type, "sound") == 0) {
        return RIAT_VALUE_TYPE_SOUND;
    }
    else if(strcmp(type, "effect") == 0) {
        return RIAT_VALUE_TYPE_EFFECT;
    }
    else if(strcmp(type, "damage") == 0) {
        return RIAT_VALUE_TYPE_DAMAGE;
    }
    else if(strcmp(type, "looping_sound") == 0) {
        return RIAT_VALUE_TYPE_LOOPING_SOUND;
    }
    else if(strcmp(type, "animation_graph") == 0) {
        return RIAT_VALUE_TYPE_ANIMATION_GRAPH;
    }
    else if(strcmp(type, "actor_variant") == 0) {
        return RIAT_VALUE_TYPE_ACTOR_VARIANT;
    }
    else if(strcmp(type, "damage_effect") == 0) {
        return RIAT_VALUE_TYPE_DAMAGE_EFFECT;
    }
    else if(strcmp(type, "object_definition") == 0) {
        return RIAT_VALUE_TYPE_OBJECT_DEFINITION;
    }
    else if(strcmp(type, "game_difficulty") == 0) {
        return RIAT_VALUE_TYPE_GAME_DIFFICULTY;
    }
    else if(strcmp(type, "team") == 0) {
        return RIAT_VALUE_TYPE_TEAM;
    }
    else if(strcmp(type, "ai_default_state") == 0) {
        return RIAT_VALUE_TYPE_AI_DEFAULT_STATE;
    }
    else if(strcmp(type, "actor_type") == 0) {
        return RIAT_VALUE_TYPE_ACTOR_TYPE;
    }
    else if(strcmp(type, "hud_corner") == 0) {
        return RIAT_VALUE_TYPE_HUD_CORNER;
    }
    else if(strcmp(type, "object") == 0) {
        return RIAT_VALUE_TYPE_OBJECT;
    }
    else if(strcmp(type, "unit") == 0) {
        return RIAT_VALUE_TYPE_UNIT;
    }
    else if(strcmp(type, "vehicle") == 0) {
        return RIAT_VALUE_TYPE_VEHICLE;
    }
    else if(strcmp(type, "weapon") == 0) {
        return RIAT_VALUE_TYPE_WEAPON;
    }
    else if(strcmp(type, "device") == 0) {
        return RIAT_VALUE_TYPE_DEVICE;
    }
    else if(strcmp(type, "scenery") == 0) {
        return RIAT_VALUE_TYPE_SCENERY;
    }
    else if(strcmp(type, "object_name") == 0) {
        return RIAT_VALUE_TYPE_OBJECT_NAME;
    }
    else if(strcmp(type, "unit_name") == 0) {
        return RIAT_VALUE_TYPE_UNIT_NAME;
    }
    else if(strcmp(type, "vehicle_name") == 0) {
        return RIAT_VALUE_TYPE_VEHICLE_NAME;
    }
    else if(strcmp(type, "weapon_name") == 0) {
        return RIAT_VALUE_TYPE_WEAPON_NAME;
    }
    else if(strcmp(type, "device_name") == 0) {
        return RIAT_VALUE_TYPE_DEVICE_NAME;
    }
    else if(strcmp(type, "scenery_name") == 0) {
        return RIAT_VALUE_TYPE_SCENERY_NAME;
    }
    else {
        *error = true;
        return RIAT_VALUE_TYPE_UNPARSED;
    }
}

static RIAT_ScriptType get_script_type(const char *type, bool *error) {
    *error = false;
    if(strcmp(type, "static") == 0) {
        return RIAT_SCRIPT_TYPE_STATIC;
    }
    else if(strcmp(type, "startup") == 0) {
        return RIAT_SCRIPT_TYPE_STARTUP;
    }
    else if(strcmp(type, "continuous") == 0) {
        return RIAT_SCRIPT_TYPE_CONTINUOUS;
    }
    else if(strcmp(type, "stub") == 0) {
        return RIAT_SCRIPT_TYPE_STUB;
    }
    else if(strcmp(type, "dormant") == 0) {
        return RIAT_SCRIPT_TYPE_DORMANT;
    }
    else {
        *error = true;
        return RIAT_SCRIPT_TYPE_STATIC;
    }
}
