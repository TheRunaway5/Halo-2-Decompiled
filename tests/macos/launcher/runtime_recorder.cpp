#include <nlohmann/json.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string_view>

int main(int argc, char** argv) {
    nlohmann::json arguments = nlohmann::json::array();
    for (int i = 1; i < argc; ++i) {
        if (std::string_view(argv[i]) == "--help") return 0;
        arguments.push_back(argv[i]);
    }
    const auto environment = [](const char* name) {
        const auto* value = std::getenv(name);
        return value ? nlohmann::json(value) : nlohmann::json(nullptr);
    };
    const nlohmann::json record{{"argv", arguments}, {"cwd", std::filesystem::current_path().string()},
        {"launcher", environment("H2_DISCORD_LAUNCHER")}, {"layout", environment("H2_INSTALLATION_LAYOUT")}};
    std::ofstream("runtime-call.json") << record.dump() << '\n';
}
