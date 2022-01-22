#include "riat_internal.h"

#include <stdbool.h>
#include <string.h>

RIAT_ValueType riat_value_type_from_string(const char *type, bool *error) {
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

const char *riat_value_type_to_string(RIAT_ValueType type) {
    switch(type) {
        case RIAT_VALUE_TYPE_VOID:
            return "void";
        case RIAT_VALUE_TYPE_UNPARSED:
            return "unparsed";
        case RIAT_VALUE_TYPE_SPECIAL_FORM:
            return "special_form";
        case RIAT_VALUE_TYPE_FUNCTION_NAME:
            return "function_name";
        case RIAT_VALUE_TYPE_PASSTHROUGH:
            return "passthrough";
        case RIAT_VALUE_TYPE_BOOLEAN:
            return "boolean";
        case RIAT_VALUE_TYPE_REAL:
            return "real";
        case RIAT_VALUE_TYPE_SHORT:
            return "short";
        case RIAT_VALUE_TYPE_LONG:
            return "long";
        case RIAT_VALUE_TYPE_STRING:
            return "string";
        case RIAT_VALUE_TYPE_SCRIPT:
            return "script";
        case RIAT_VALUE_TYPE_TRIGGER_VOLUME:
            return "trigger_volume";
        case RIAT_VALUE_TYPE_CUTSCENE_FLAG:
            return "cutscene_flag";
        case RIAT_VALUE_TYPE_CUTSCENE_CAMERA_POINT:
            return "cutscene_camera_point";
        case RIAT_VALUE_TYPE_CUTSCENE_TITLE:
            return "cutscene_title";
        case RIAT_VALUE_TYPE_CUTSCENE_RECORDING:
            return "cutscene_recording";
        case RIAT_VALUE_TYPE_DEVICE_GROUP:
            return "device_group";
        case RIAT_VALUE_TYPE_AI:
            return "ai";
        case RIAT_VALUE_TYPE_AI_COMMAND_LIST:
            return "ai_command_list";
        case RIAT_VALUE_TYPE_STARTING_PROFILE:
            return "starting_profile";
        case RIAT_VALUE_TYPE_CONVERSATION:
            return "conversation";
        case RIAT_VALUE_TYPE_NAVPOINT:
            return "navpoint";
        case RIAT_VALUE_TYPE_HUD_MESSAGE:
            return "hud_message";
        case RIAT_VALUE_TYPE_OBJECT_LIST:
            return "object_list";
        case RIAT_VALUE_TYPE_SOUND:
            return "sound";
        case RIAT_VALUE_TYPE_EFFECT:
            return "effect";
        case RIAT_VALUE_TYPE_DAMAGE:
            return "damage";
        case RIAT_VALUE_TYPE_LOOPING_SOUND:
            return "looping_sound";
        case RIAT_VALUE_TYPE_ANIMATION_GRAPH:
            return "animation_graph";
        case RIAT_VALUE_TYPE_ACTOR_VARIANT:
            return "actor_variant";
        case RIAT_VALUE_TYPE_DAMAGE_EFFECT:
            return "damage_effect";
        case RIAT_VALUE_TYPE_OBJECT_DEFINITION:
            return "object_definition";
        case RIAT_VALUE_TYPE_GAME_DIFFICULTY:
            return "game_difficulty";
        case RIAT_VALUE_TYPE_TEAM:
            return "team";
        case RIAT_VALUE_TYPE_AI_DEFAULT_STATE:
            return "ai_default_state";
        case RIAT_VALUE_TYPE_ACTOR_TYPE:
            return "actor_type";
        case RIAT_VALUE_TYPE_HUD_CORNER:
            return "hud_corner";
        case RIAT_VALUE_TYPE_OBJECT:
            return "object";
        case RIAT_VALUE_TYPE_UNIT:
            return "unit";
        case RIAT_VALUE_TYPE_VEHICLE:
            return "vehicle";
        case RIAT_VALUE_TYPE_WEAPON:
            return "weapon";
        case RIAT_VALUE_TYPE_DEVICE:
            return "device";
        case RIAT_VALUE_TYPE_SCENERY:
            return "scenery";
        case RIAT_VALUE_TYPE_OBJECT_NAME:
            return "object_name";
        case RIAT_VALUE_TYPE_UNIT_NAME:
            return "unit_name";
        case RIAT_VALUE_TYPE_VEHICLE_NAME:
            return "vehicle_name";
        case RIAT_VALUE_TYPE_WEAPON_NAME:
            return "weapon_name";
        case RIAT_VALUE_TYPE_DEVICE_NAME:
            return "device_name";
        case RIAT_VALUE_TYPE_SCENERY_NAME:
            return "scenery_name";
    }
}

RIAT_ScriptType riat_script_type_from_string(const char *type, bool *error) {
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
