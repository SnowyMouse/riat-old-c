#ifndef RIAT__DEFINITION__DEFINITION_H
#define RIAT__DEFINITION__DEFINITION_H

#include <stdint.h>
#include <stdbool.h>

#include "../../include/riat/riat.h"

typedef enum RIAT_BuiltinDefinitionType {
    RIAT_BUILTIN_DEFINITION_TYPE_FUNCTION,
    RIAT_BUILTIN_DEFINITION_TYPE_GLOBAL
} RIAT_BuiltinDefinitionType;

typedef struct RIAT_BuiltinFunctionParameter {
    RIAT_ValueType type;
    bool optional;
    bool many;
    bool passthrough_last;
} RIAT_BuiltinFunctionParameter;

typedef struct RIAT_BuiltinDefinition {
    char name[64];
    RIAT_BuiltinDefinitionType type;
    RIAT_ValueType value_type;
    
    /* Check RIAT_BuiltinDefinitionIndexError */
    uint16_t index_gbx_custom;
    uint16_t index_gbx_demo;
    uint16_t index_gbx_retail;
    uint16_t index_mcc_cea;
    uint16_t index_xbox;

    uint16_t parameter_count;
    RIAT_BuiltinFunctionParameter parameters[6];
} RIAT_BuiltinDefinition;

enum RIAT_BuiltinDefinitionIndexError {
    /* Not present in target engine */
    RIAT_BUILTIN_DEFINITION_INDEX_NOT_PRESENT = 65535,

    /* Present in target engine but unknown */
    RIAT_BUILTIN_DEFINITION_INDEX_UNKNOWN = 65534
};

const RIAT_BuiltinDefinition *RIAT_builtin_definition_search(const char *what);

#endif
