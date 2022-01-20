#include "include/riat/riat.hpp"
#include <vector>

int main(int argc, const char **argv) {
    if(argc == 1) {
        std::printf("Usage: %s <script>\n", argv[0]);
        std::exit(EXIT_FAILURE);
    }

    try {
        RIAT::Instance instance;

        for(int i = 1; i < argc; i++) {

            std::FILE *f = std::fopen(argv[i], "rb");
            if(!f) {
                std::printf("Can't open %s\n", argv[i]);
                std::exit(EXIT_FAILURE);
            }

            // Load it!
            std::fseek(f, 0, SEEK_END);
            std::vector<char> data(std::ftell(f));
            std::fseek(f, 0, SEEK_SET);
            std::fread(data.data(), data.size(), 1, f);
            std::fclose(f);
            instance.load_script(data.data(), data.size(), argv[i]);
        }

        instance.compile_scripts();
    }
    catch(std::exception &e) {
        std::printf("%s Exception error: %s\n", typeid(e).name(), e.what());
    }
}
