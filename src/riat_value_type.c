#include "riat_internal.h"

#include <stdbool.h>
#include <string.h>

RIAT_ValueType RIAT_value_type_from_string(const char *type, bool *error) {
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

RIAT_ScriptType RIAT_script_type_from_string(const char *type, bool *error) {
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
