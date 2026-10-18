#include "halo2/cache.hpp"
#include "halo2/full_scoreboard_assets.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>

int main(int argc, char** argv) try {
    using namespace halo2;
    if (argc != 3 || !std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2])) return 77;
    Cache donor(argv[1]), original(argv[2]);
    if (!donor.is_mcc() || original.is_mcc() || original.is_vista())
        throw FormatError("Supply matching MCC and original Xbox maps");
    const auto converted = donor.load_full_scoreboard_assets();
    const auto expected = original.load_full_scoreboard_assets();
    if (!converted.scenario_present || converted.palette.empty() || converted.team_palette.empty())
        throw FormatError("Actual scoreboard metadata was not imported");
    if (std::any_of(expected.team_names.begin(), expected.team_names.end(), [](const auto& value) { return value.empty(); }) ||
        std::any_of(converted.team_names.begin(), converted.team_names.end(), [](const auto& value) { return value.empty(); }) ||
        converted.name_label.empty() || converted.wins_label.empty() || converted.score_label.empty() ||
        converted.name_label != expected.name_label || converted.wins_label != expected.wins_label ||
        converted.score_label != expected.score_label || converted.team_names != expected.team_names) {
        std::cerr << "MCC labels: " << converted.name_label << " / " << converted.wins_label << " / " << converted.score_label
                  << "; Xbox labels: " << expected.name_label << " / " << expected.wins_label << " / " << expected.score_label << '\n';
        for(unsigned n=0;n<converted.team_names.size();++n)
            std::cerr << n << ": MCC=" << converted.team_names[n] << "; Xbox=" << expected.team_names[n] << '\n';
        throw FormatError("MCC scoreboard locale labels differ from the paired original map");
    }
    std::cout << "PASS actual MCC shared locale offsets: three scoreboard labels and nine team names match original Xbox\n";
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
