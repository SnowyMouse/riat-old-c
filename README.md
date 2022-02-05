# Rat In a Tube

RIAT is a free script compiler for Halo: Combat Evolved scripts.

To compile, you will need:
- CMake 3.14 or newer
- Python 3 or newer
- C11 compiler

## Example usage

RIAT comes with both a C and C++ header:
* The C header allows you to use RIAT with more programming languages, but error
  handling and cleanup have to be triggered by the caller.
* You can use the C++ header to take advantage of C++'s stronger type safety,
  automatic cleanup (via destructors), and exceptions. There are no exported C++
  functions, as all C++ functions in the header are static and call C functions.

```cpp
#include <riat/riat.hpp>
#include <filesystem>
#include <cstdio>
#include <string>

bool compile_scripts_cpp(const std::string &my_script_data, const std::filesystem::path &path) {
    // Instantiate our instance
    RIAT::Instance instance;

    // Load HSC data
    instance.load_script_source(my_script_data.c_str(), my_script_data.size(), path.filename().string().c_str());

    // Compile scripts
    try {
        instance.compile_scripts();
    }
    catch(RIAT::Exception &e) {
        std::fprintf(stderr, "Error: %s\n", e.what());
        return false;
    }

    // Final results
    std::printf("Scripts: %zu\n", instance.get_scripts().size());
    std::printf("Globals: %zu\n", instance.get_globals().size());
    std::printf("Nodes: %zu\n", instance.get_nodes().size());

    // NOTE: Cleanup is handled by RIAT::Instance's destructor, thus it will clean up when the function returns.
    return true;
}
```
```cpp
#include <riat/riat.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

bool compile_scripts_c(const char *my_script_data, size_t my_script_data_length, const char *filename) {
    // Store our results here
    bool result = false;
    const RIAT_Node *nodes;
    const RIAT_Global *globals;
    const RIAT_Script *scripts;
    size_t node_count, global_count, script_count;

    // Instantiate our instance
    RIAT_Instance *instance = riat_instance_new();
    if(instance == NULL) {
        goto cleanup;
    }

    // Load HSC data
    if(riat_instance_load_script_source(instance, my_script_data, my_script_data_length, filename) != RIAT_COMPILE_OK) {
        goto cleanup;
    }

    // Compile
    if(riat_instance_compile_scripts(instance) != RIAT_COMPILE_OK) {
        goto cleanup;
    }

    // Final results
    nodes = riat_instance_get_nodes(instance, &node_count);
    globals = riat_instance_get_globals(instance, &global_count);
    scripts = riat_instance_get_scripts(instance, &script_count);

    printf("Scripts: %zu\n", script_count);
    printf("Globals: %zu\n", global_count);
    printf("Nodes: %zu\n", node_count);

    cleanup:

    // Check if result is false. If so, check if we have an instance.
    if(!result && instance != NULL) {
        fprintf(stderr, "Error: %s\n", riat_instance_get_last_compile_error(instance, NULL, NULL, NULL));
    }

    // NOTE: Cleanup is required unless riat_instance_new returned NULL. If instance is NULL, this function will do nothing.
    riat_instance_delete(instance);

    return result;
}
```
