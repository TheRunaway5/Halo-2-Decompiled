#include "halo2/original_data.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
void require(bool condition, const char *message) {
  if (!condition) throw std::runtime_error(message);
}
}
int main(int argc, char **argv) {
  try {
    require(argc == 3, "Supply verified base audio operand fixture and owned disc executable");
    for (int i = 1; i < argc; ++i)
      if (!std::filesystem::exists(argv[i])) return 77;
    const auto image = halo2::OriginalData::load_xbe(argv[2]);
    require(image->image == halo2::OriginalData::Image::Base, "Expected original base-disc image");
    require(bool(image->sound_driver_parameter_constants), "Base-disc sound driver operands unavailable");
    require(bool(image->sound_create_constants), "Base-disc sound distance operands unavailable");
    const auto &d = *image->sound_driver_parameter_constants;
    const auto &c = *image->sound_create_constants;
    const std::array driver{
        d.source_pitch_to_driver_pitch_bits, d.driver_pitch_min_bits, d.driver_pitch_max_bits,
        d.gain_normalized_to_driver_volume_bits, d.minimum_source_gain_bits,
        d.parameter_change_threshold_bits, d.driver_pitch_change_threshold_bits,
        d.driver_pitch_octave_scale_bits, d.relative_rate_ln2_bits, d.mono_channel_count_bits,
        d.stereo_channel_count_bits, d.routing_db_ln10_bits, d.routing_db_scale_bits,
        d.routing_linear_db_scale_bits, d.normalized_parameter_range_epsilon_bits,
        d.silent_source_gain_bits};
    const std::array create{
        c.default_extra_gain_bits, c.minimum_gain_bits, c.maximum_float_bits,
        c.rng_scale_bits, c.unsigned_conversion_correction_bits, c.one_bits,
        c.upper_gain_bits, c.db_exponent_scale_bits, c.db_exponent_log_bits,
        c.linear_db_scale_bits, c.fade_epsilon_bits};
    std::ifstream input(argv[1]);
    unsigned groups = 0, operands = 0;
    for (std::string line; std::getline(input, line);) {
      if (line.empty() || line[0] == '#') continue;
      std::istringstream row(line);
      char group;
      require(bool(row >> group) && group == (groups == 0 ? 'D' : 'C') && groups < 2,
              "Incomplete original operand groups");
      const std::span<const std::uint32_t> actual = group == 'D'
          ? std::span<const std::uint32_t>(driver) : std::span<const std::uint32_t>(create);
      for (auto value : actual) {
        std::string expected;
        require(bool(row >> expected) && value == std::stoul(expected, nullptr, 16),
                "Base-disc audio word differs from verified original operand");
        ++operands;
      }
      std::string extra;
      require(!(row >> extra), "Unexpected original audio operand");
      ++groups;
    }
    require(groups == 2 && operands == 27, "Incomplete base-disc audio coverage");
    std::cout << "baseDiscAudioOperands=27 immutableBytes=108\n";
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
