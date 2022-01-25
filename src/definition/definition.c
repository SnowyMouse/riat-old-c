#include "definition_array.h"

const RIAT_BuiltinDefinition *riat_builtin_definition_search(const char *what, RIAT_CompileTarget compile_target, RIAT_BuiltinDefinitionType type) {
    /* Binary search */
    size_t end = sizeof(definitions) / sizeof(definitions[0]);
    size_t start = 0;
    assert(end > start);

    while(true) {
        size_t middle = (start + end) / 2;

        const RIAT_BuiltinDefinition *object = &definitions[middle];
        int comparison = strcmp(what, object->name);
        if(comparison == 0) {
            /* If the type isn't correct, skip it */
            if(type != RIAT_BUILTIN_DEFINITION_TYPE_ANY && object->type != type) {
                return NULL;
            }

            /* If the compile target isn't correct, skip it as well */
            switch(compile_target) {
                case RIAT_COMPILE_TARGET_ANY:
                    break;
                case RIAT_COMPILE_TARGET_XBOX:
                    if(object->index_xbox == RIAT_BUILTIN_DEFINITION_INDEX_NOT_PRESENT) {
                        return NULL;
                    }
                    break;
                case RIAT_COMPILE_TARGET_GEARBOX_RETAIL:
                    if(object->index_gbx_retail == RIAT_BUILTIN_DEFINITION_INDEX_NOT_PRESENT) {
                        return NULL;
                    }
                    break;
                case RIAT_COMPILE_TARGET_GEARBOX_CUSTOM_EDITION:
                    if(object->index_gbx_custom == RIAT_BUILTIN_DEFINITION_INDEX_NOT_PRESENT) {
                        return NULL;
                    }
                    break;
                case RIAT_COMPILE_TARGET_GEARBOX_DEMO:
                    if(object->index_gbx_demo == RIAT_BUILTIN_DEFINITION_INDEX_NOT_PRESENT) {
                        return NULL;
                    }
                    break;
                case RIAT_COMPILE_TARGET_MCC_CEA:
                    if(object->index_mcc_cea == RIAT_BUILTIN_DEFINITION_INDEX_NOT_PRESENT) {
                        return NULL;
                    }
                    break;
            }

            /* Otherwise return it */
            return object;
        }
        else if(comparison > 0) {
            start = middle + 1;
        }
        else {
            end = middle;
        }

        if(start == end) {
            return NULL;
        }
    }
}
