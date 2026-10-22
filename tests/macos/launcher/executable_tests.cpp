#include "halo2/macos/executable.hpp"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
    try {
        if (argc != 3) throw std::invalid_argument("Supply the actual executable and unrelated working directory");
        const auto expected = std::filesystem::canonical(argv[1]);
        std::filesystem::current_path(argv[2]);
        const auto actual = halo2::macos_executable_path();
        if (actual != expected || !actual.is_absolute())
            throw std::runtime_error("Darwin executable lookup depended on argv[0], its symlink or the working directory");
        std::cout << nlohmann::json{{"passed", true}, {"executable", actual.string()},
            {"working_directory", std::filesystem::current_path().string()}}.dump() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
