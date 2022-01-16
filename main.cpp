#include "include/riat/riat.hpp"
#include <vector>

int main(int argc, const char **argv) {
    if(argc != 2) {
        std::printf("Usage: %s <script>\n", argv[0]);
        std::exit(EXIT_FAILURE);
    }

    std::FILE *f = std::fopen(argv[1], "rb");
    if(!f) {
        std::printf("Can't open %s\n", argv[1]);
        std::exit(EXIT_FAILURE);
    }

    // Load it!
    std::fseek(f, 0, SEEK_END);
    std::vector<char> data(std::ftell(f));
    std::fseek(f, 0, SEEK_SET);
    std::fread(data.data(), data.size(), 1, f);
    std::fclose(f);

    try {
        RIAT::Instance instance;
        instance.compile_script(data.data(), data.size(), argv[1]);
    }
    catch(std::exception &e) {
        std::printf("%s Exception error: %s\n", typeid(e).name(), e.what());
    }
}
