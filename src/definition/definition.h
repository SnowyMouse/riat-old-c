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
    bool passthrough_last;
} RIAT_BuiltinFunctionParameter;

typedef struct RIAT_BuiltinDefinition {
    char name[64];
    RIAT_BuiltinDefinitionType type;
    RIAT_ValueType value_type;
    
    /* If 65535, then it's not present in the target engine. If 65534, then it's present but the index is unknown. */
    uint16_t index_gbx_custom;
    uint16_t index_gbx_demo;
    uint16_t index_gbx_retail;
    uint16_t index_mcc_cea;
    uint16_t index_xbox;

    uint16_t parameter_count;
    RIAT_BuiltinFunctionParameter parameters[6];
} RIAT_BuiltinDefinition;

RIAT_BuiltinDefinition *RIAT_builtin_definition_search(const char *what);

#endif
