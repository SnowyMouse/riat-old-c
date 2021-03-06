import sys
import json
import copy

if len(sys.argv) != 3:
    print("Usage: {} <definition.json> <definition.c>".format(sys.argv[0]))
    sys.exit(1)

with open(sys.argv[1], "r") as f:
    definitions = json.load(f)

everything = copy.deepcopy(definitions["functions"])
everything.extend(definitions["globals"])
everything = sorted(everything, key=lambda d:d["name"])

output_defs = ""

for n in everything:
    is_function = n in definitions["functions"]

    def resolve_index(w,t):
        if t not in w:
            return "RIAT_BUILTIN_DEFINITION_INDEX_NOT_PRESENT"
        elif w[t] == None:
            return "RIAT_BUILTIN_DEFINITION_INDEX_UNKNOWN"
        else:
            return w[t]

    def format_value_type(v):
        return "RIAT_VALUE_TYPE_" + v.upper()

    def format_parameters(w):
        r = ",.parameters={"
        for p in w:
            r = r + "{{.type={type},.optional={optional},.many={many},.passthrough_last={passthrough_last},.allow_uppercase={allow_uppercase}}},".format(
                type=format_value_type(p["type"]),
                optional="true" if "optional" in p and p["optional"] else "false",
                many="true" if "many" in p and p["many"] else "false",
                passthrough_last="true" if "passthrough_last" in p and p["passthrough_last"] else "false",
                allow_uppercase="true" if "allow_uppercase" in p and p["allow_uppercase"] else "false"
            )
        return r + "}"

    output_defs += "{{.name=\"{name}\",.type={type},.value_type={value_type},.index_gbx_custom={index_gbx_custom},.index_gbx_demo={index_gbx_demo},.index_gbx_retail={index_gbx_retail},.index_mcc_cea={index_mcc_cea},.index_xbox={index_xbox},.parameter_count={parameter_count}{parameters}}},\n".format(
        name=n["name"],
        type="RIAT_BUILTIN_DEFINITION_TYPE_FUNCTION" if is_function else "RIAT_BUILTIN_DEFINITION_TYPE_GLOBAL",
        value_type=format_value_type(n["type"]),
        index_gbx_custom=resolve_index(n["engines"], "gbx-custom"),
        index_gbx_demo=resolve_index(n["engines"], "gbx-demo"),
        index_gbx_retail=resolve_index(n["engines"], "gbx-retail"),
        index_mcc_cea=resolve_index(n["engines"], "mcc-cea"),
        index_xbox=resolve_index(n["engines"], "xbox"),
        parameter_count=len(n["parameters"]) if "parameters" in n else 0,
        parameters=format_parameters(n["parameters"]) if "parameters" in n else ""
    )

output = '''// This file was auto-generated and should not be edited directly.

#include "definition.h"
#include <string.h>
#include <assert.h>

static const RIAT_BuiltinDefinition definitions[] = {
''' + output_defs + '''};
'''

with open(sys.argv[2], "w") as f:
    f.write(output)
