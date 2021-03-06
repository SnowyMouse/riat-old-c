#include <vector>
#include "../include/riat/riat.hpp"

static void warn(RIAT_Instance *instance, const char *message, const char *file, std::size_t line, std::size_t column) {
    std::fprintf(stderr, "%s:%zu:%zu: warning: %s\n", file, line, column, message);
}

int main(int argc, const char **argv) {
    // If we don't supply a script, show a "usage" message
    if(argc == 1) {
        std::printf("Usage: %s <script1.hsc> [script2.hsc ...]\n", argv[0]);
        std::exit(EXIT_FAILURE);
    }

    // Declare our instance here
    RIAT::Instance instance;

    instance.set_warn_callback(warn);

    try {
        // Go through each script file, read it from memory, and load it
        for(int i = 1; i < argc; i++) {
            // Open the file
            std::FILE *f = std::fopen(argv[i], "rb");
            if(!f) {
                std::printf("Can't open %s\n", argv[i]);
                std::exit(EXIT_FAILURE);
            }

            // Now read it
            std::fseek(f, 0, SEEK_END);
            std::vector<char> data(std::ftell(f));
            std::fseek(f, 0, SEEK_SET);
            std::fread(data.data(), data.size(), 1, f);
            std::fclose(f);

            // And now load it
            instance.load_script_source(data.data(), data.size(), argv[i]);
        }

        // Now compile the scripts
        instance.compile_scripts();
    }

    // If an error occurs, we can catch it here
    catch(std::exception &e) {
        std::fprintf(stderr, "%s\n", e.what());
        return EXIT_FAILURE;
    }

    // Show some metadata here
    std::printf("Scripts: %zu\n", instance.get_scripts().size());
    std::printf("Globals: %zu\n", instance.get_globals().size());
    std::printf("Nodes: %zu\n", instance.get_nodes().size());

    return EXIT_SUCCESS;
}
