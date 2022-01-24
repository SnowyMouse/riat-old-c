#ifndef RAT_IN_A_TUBE_H
#define RAT_IN_A_TUBE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/** Opaque instance pointer */
typedef struct RIAT_Instance RIAT_Instance;

/** Warning callback */
typedef void (*RIAT_InstanceWarnCallback)(RIAT_Instance *instance, const char *message, const char *file, size_t line, size_t column);

/** Optimization level */
typedef enum RIAT_OptimizationLevel {
    /**
     * No optimizations are done besides string data deduplication.
     * 
     * This will match tool.exe output and, with a smart decompiler, decompile to the same code.
     * */
    RIAT_OPTIMIZATION_PARANOID = 0,

    /**
     * Do not add a (begin) block to top-level script blocks if one already isn't present, preventing generational loss from tag extraction.
     * 
     * This will match tool.exe output (and decompile to the same code) provided there are no top-level begin blocks in the source code.
     */ 
    RIAT_OPTIMIZATION_PREVENT_GENERATIONAL_LOSS = 1,

    /**
     * Dedupe constants and function calls, and do not add unnecessary top-level begin blocks, saving nodes.
     * 
     * With the exception of *potentially* not adding a top-level begin block, this will be functionally equivalent to tool.exe output.
     */
    RIAT_OPTIMIZATION_DEDUPE_EXTRA = 2,

    /**
     * Evaluate constant arithmetic and perform more aggressive deduplication, saving more nodes.
     * 
     * Functionally, this will behave the same as tool.exe output, but it may be harder to debug with the console, and it will possibly not decompile to the same code.
     */
    RIAT_OPTIMIZATION_AGGRESSIVE = 3
} RIAT_OptimizationLevel;

/** Value type - value matches HCE's definitions */
typedef enum RIAT_ValueType {
    /** Invalid */
    RIAT_VALUE_TYPE_UNPARSED = 0,

    /** Invalid */
    RIAT_VALUE_TYPE_SPECIAL_FORM,

    /** Function name (value is in string data - used in the first node of a function or script call) */
    RIAT_VALUE_TYPE_FUNCTION_NAME,

    /** Passthrough - value type can be modified before compilation (invalid for script and global types) */
    RIAT_VALUE_TYPE_PASSTHROUGH,

    /** Void - no value returned */
    RIAT_VALUE_TYPE_VOID,

    /** Boolean - can be true or false */
    RIAT_VALUE_TYPE_BOOLEAN,

    /** Real - 32-bit float */
    RIAT_VALUE_TYPE_REAL,

    /** Short - 16-bit signed int between -32768 and 32767 */
    RIAT_VALUE_TYPE_SHORT,

    /** Long - 32-bit signed int between -2147483648 and 2147483647 */
    RIAT_VALUE_TYPE_LONG,

    /** String */
    RIAT_VALUE_TYPE_STRING,

    /* Script - value must be the script index (value is 16-bit) */
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

/** Script type - value matches HCE's definitions */
typedef enum RIAT_ScriptType {
    /** Script is executed once on map load */
    RIAT_SCRIPT_TYPE_STARTUP = 0,

    /** Script is not executed until the 'wake' command */
    RIAT_SCRIPT_TYPE_DORMANT,

    /** Script is executed repeatedly */
    RIAT_SCRIPT_TYPE_CONTINUOUS,

    /** Script can be executed at any time */
    RIAT_SCRIPT_TYPE_STATIC,

    /** Effectively the same as static except it will be replaced by a static script of the same type */
    RIAT_SCRIPT_TYPE_STUB
} RIAT_ScriptType;

/* Script node */
typedef struct RIAT_Node {
    /** String data (NULL if none) */
    char *string_data;

    /** Next node index (SIZE_MAX if none) */
    size_t next_node;

    /** Relevant file index (starts at 0) */
    size_t file;

    /** Relevant line index (starts at 1) */
    size_t line;

    /** Relevant column (starts at 1) */
    size_t column;

    /** Value type */
    RIAT_ValueType type;

    /** If true, the value is here. If this is false, it's a function call */
    bool is_primitive;

    /** If this is a global, then the name of the global is in the string data */
    bool is_global;

    /** If this is a script call, then it works like a function call but it's a script that is called instead */
    bool is_script_call;

    /** Index of the call */
    uint16_t call_index;

    /** Used if not primitive or if a primitive number/boolean type (and not a global!) */ 
    union {
        /** Child node (if not primitive), generally pointing to a function name */
        size_t child_node;

        /** 32-bit integer (if primitive long) */
        int32_t long_int;

        /** 16-bit integer (if primitive short or script) */
        int16_t short_int;

        /** 8-bit integer (if primitive boolean) */
        int8_t bool_int;

        /** 32-bit float (if primitive real) */
        float real;
    };
} RIAT_Node;

/** Script global */
typedef struct RIAT_Global {
    /** Name of global (null terminated) */
    char name[32];

    /** Index of first node */
    size_t first_node;

    /** Type of script */
    RIAT_ValueType value_type;

    /** Relevant file index (starts at 0) */
    size_t file;

    /** Relevant line index (starts at 1) */
    size_t line;

    /** Relevant column index (starts at 1) */
    size_t column;
} RIAT_Global;

typedef struct RIAT_Script {
    /** Name of global (null terminated) */
    char name[32];

    /** Index of first node */
    size_t first_node;

    /** Return type of script */
    RIAT_ValueType return_type;

    /** Type of script */
    RIAT_ScriptType script_type;

    /** Relevant file index (starts at 0) */
    size_t file;

    /** Relevant line index (starts at 1) */
    size_t line;

    /** Relevant column index (starts at 1) */
    size_t column;
} RIAT_Script;

typedef enum RIAT_CompileTarget {
    RIAT_COMPILE_TARGET_ANY,
    RIAT_COMPILE_TARGET_XBOX,
    RIAT_COMPILE_TARGET_GEARBOX_RETAIL,
    RIAT_COMPILE_TARGET_GEARBOX_DEMO,
    RIAT_COMPILE_TARGET_GEARBOX_CUSTOM_EDITION,
    RIAT_COMPILE_TARGET_MCC_CEA
} RIAT_CompileTarget;

/**
 * Make an instance
 * 
 * @param  target compile target
 * 
 * @return        new instance pointer or NULL on failure
 */
struct RIAT_Instance *riat_instance_new(RIAT_CompileTarget target);

/**
 * Clear the instance, freeing all memory associated with it. Passing NULL is a no-op.
 * 
 * @param instance
 */
void riat_instance_delete(RIAT_Instance *instance);

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
 * Load the script source data
 * 
 * @param instance             instance
 * @param script_source_data   source data to compile
 * @param script_source_length length of the source data in columns
 * @param file_name            source file name (can be anything)
 * 
 * @return                     result
 */
RIAT_CompileResult riat_instance_load_script_source(RIAT_Instance *instance, const char *script_source_data, size_t script_source_length, const char *file_name);

/**
 * Compile the loaded script(s)
 * 
 * @param instance             instance
 * 
 * @return                     result
 */
RIAT_CompileResult riat_instance_compile_scripts(RIAT_Instance *instance);

/**
 * Get the last compile error
 * 
 * @param instance  instance
 * @param line      set to the line where the error occurred (if applicable)
 * @param column    set to the column where the error occurred (if applicable)
 * @param file      set to the file where the error occurred (if applicable - will be invalid if any more riat_instance_* functions that return a RIAT_CompileResult are called on the instance)
 */
const char *riat_instance_get_last_compile_error(const RIAT_Instance *instance, size_t *line, size_t *column, const char **file);

/**
 * Get a pointer to the nodes
 * 
 * @param instance instance
 * @param count    number of nodes (output)
 * 
 * @return         pointer to node array (will be invalid if scripts are recompiled or the instance is deleted)
 */
const RIAT_Node *riat_instance_get_nodes(const RIAT_Instance *instance, size_t *count);

/**
 * Get a pointer to the scripts
 * 
 * @param instance instance
 * @param count    number of scripts (output)
 * 
 * @return         pointer to script array (will be invalid if scripts are recompiled or the instance is deleted)
 */
const RIAT_Script *riat_instance_get_scripts(const RIAT_Instance *instance, size_t *count);

/**
 * Get a pointer to the globals
 * 
 * @param instance instance
 * @param count    number of globals (output)
 * 
 * @return         pointer to global array (will be invalid if scripts are recompiled or the instance is deleted)
 */
const RIAT_Global *riat_instance_get_globals(const RIAT_Instance *instance, size_t *count);

/**
 * Set the user data
 * 
 * @param instance  instance
 * @param user_data user data to set
 */
void riat_instance_set_user_data(RIAT_Instance *instance, void *user_data);

/**
 * Get the user data
 * 
 * @param instance  instance
 * 
 * @return          user data
 */
void *riat_instance_get_user_data(const RIAT_Instance *instance);

/**
 * Set the warning callback
 * 
 * @param instance instance
 * @param callback callback
 */
void riat_instance_set_warn_callback(RIAT_Instance *instance, RIAT_InstanceWarnCallback callback);

/**
 * Set the optimization level
 * 
 * @param instance instance
 * @param level    optimization level
 */
void riat_instance_set_optimization_level(RIAT_Instance *instance, RIAT_OptimizationLevel level);

#ifdef __cplusplus
}
#endif

#endif
