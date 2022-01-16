#ifndef RAT_IN_A_TUBE_H
#define RAT_IN_A_TUBE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Opaque instance pointer */
typedef struct RIAT_Instance RIAT_Instance;

/** Value type */
typedef enum RIAT_ValueType {
    /* Generally does nothing */
    RIAT_VALUE_TYPE_UNPARSED,

    /* ??? */
    RIAT_VALUE_TYPE_SPECIAL_FORM,

    /* Function name (value is in string data - used in the first node of a function or script call) */
    RIAT_VALUE_TYPE_FUNCTION_NAME,

    /* Passthrough (???) */
    RIAT_VALUE_TYPE_PASSTHROUGH,

    /* Void - no value returned */
    RIAT_VALUE_TYPE_VOID,

    /* Boolean - not actually a boolean but a signed 8-bit value between -127 and 128. If not a primitive, see string data for where to get the value. */
    RIAT_VALUE_TYPE_BOOLEAN,

    /* Real - 32-bit float. If not a primitive, see string data for where to get the value. */
    RIAT_VALUE_TYPE_REAL,

    /* Short - 16-bit signed int between -32768 and 32767. If not a primitive, see string data for where to get the value. */
    RIAT_VALUE_TYPE_SHORT,

    /* Long - 32-bit signed int between -2147483648 and 2147483647. If not a primitive, see string data for where to get the value. */
    RIAT_VALUE_TYPE_LONG,

    /* Everything below here has the value in string data. If not a primitive, see string data for where to get the value. */
    RIAT_VALUE_TYPE_STRING,
    RIAT_VALUE_TYPE_SCRIPT,
    RIAT_VALUE_TYPE_TRIGGER_VOLUME,
    RIAT_VALUE_TYPE_CUTSCENE_FLAG,
    RIAT_VALUE_TYPE_CUTSCENE_CAMERA_POINT,
    RIAT_VALUE_TYPE_CUTSCENE_TITLE,
    RIAT_VALUE_TYPE_CUTSCENE_RECORDING,
    RIAT_VALUE_TYPE_DEVICE_GROUP,
    RIAT_VALUE_TYPE_AI,
    RIAT_VALUE_TYPE_AI_COMMAND_LIST,
    RIAT_VALUE_TYPE_STARTING_PROFILE,
    RIAT_VALUE_TYPE_CONVERSATION,
    RIAT_VALUE_TYPE_NAVPOINT,
    RIAT_VALUE_TYPE_HUD_MESSAGE,
    RIAT_VALUE_TYPE_OBJECT_LIST,
    RIAT_VALUE_TYPE_SOUND,
    RIAT_VALUE_TYPE_EFFECT,
    RIAT_VALUE_TYPE_DAMAGE,
    RIAT_VALUE_TYPE_LOOPING_SOUND,
    RIAT_VALUE_TYPE_ANIMATION_GRAPH,
    RIAT_VALUE_TYPE_ACTOR_VARIANT,
    RIAT_VALUE_TYPE_DAMAGE_EFFECT,
    RIAT_VALUE_TYPE_OBJECT_DEFINITION,
    RIAT_VALUE_TYPE_GAME_DIFFICULTY,
    RIAT_VALUE_TYPE_TEAM,
    RIAT_VALUE_TYPE_AI_DEFAULT_STATE,
    RIAT_VALUE_TYPE_ACTOR_TYPE,
    RIAT_VALUE_TYPE_HUD_CORNER,
    RIAT_VALUE_TYPE_OBJECT,
    RIAT_VALUE_TYPE_UNIT,
    RIAT_VALUE_TYPE_VEHICLE,
    RIAT_VALUE_TYPE_WEAPON,
    RIAT_VALUE_TYPE_DEVICE,
    RIAT_VALUE_TYPE_SCENERY,
    RIAT_VALUE_TYPE_OBJECT_NAME,
    RIAT_VALUE_TYPE_UNIT_NAME,
    RIAT_VALUE_TYPE_VEHICLE_NAME,
    RIAT_VALUE_TYPE_WEAPON_NAME,
    RIAT_VALUE_TYPE_DEVICE_NAME,
    RIAT_VALUE_TYPE_SCENERY_NAME
} RIAT_ValueType;

/**
 * Make an instance
 * 
 * @return new instance pointer or NULL on failure
 */
struct RIAT_Instance *RIAT_instance_new();

/**
 * Clear the instance, freeing all memory associated with it. Passing NULL is a no-op.
 * 
 * @param instance instance to free
 */
void RIAT_instance_delete(RIAT_Instance *instance);

/**
 * Compile result return values
 */
typedef enum RIAT_CompileResult {
    /** Successfully compiled */
    RIAT_COMPILE_OK = 0,

    /** Syntax error */
    RIAT_COMPILE_SYNTAX_ERROR,

    /** Failed to allocate */
    RIAT_COMPILE_ALLOCATION_ERROR
} RIAT_CompileResult;

/**
 * Compile the script
 * 
 * @param instance             instance to compile the script with
 * @param script_source_data   source data to compile
 * @param script_source_length length of the source data in columns
 * @param file_name            source file name (can be anything)
 * 
 * @return                     result
 */
RIAT_CompileResult RIAT_instance_compile_script(RIAT_Instance *instance, const char *script_source_data, size_t script_source_length, const char *file_name);

/**
 * Get the last compile error
 * 
 * @param instance  instance to check
 * @param line      set to the line where the error occurred (if applicable)
 * @param column    set to the column where the error occurred (if applicable)
 */
const char *RIAT_instance_get_last_compile_error(const RIAT_Instance *instance, size_t *line, size_t *column);

#ifdef __cplusplus
}
#endif

#endif
