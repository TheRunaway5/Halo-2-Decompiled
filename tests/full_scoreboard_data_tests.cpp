#include "halo2/original_data.hpp"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>

int main(int argc, char **argv) {
  try {
    if (argc < 3) throw std::runtime_error("Supply frozen scoreboard report and update executable(s)");
    for (int i=1; i<argc; ++i)
      if (!std::filesystem::exists(argv[i])) return 77;
    nlohmann::json report;
    std::ifstream(argv[1]) >> report;
    if (report.at("source_xbe_sha256") !=
        "21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" ||
        !report.at("instruction_patches").empty())
      throw std::runtime_error("Wrong original scoreboard evidence");
    const auto &words=report.at("source_data_words");
    for (int image=2; image<argc; ++image) {
      const auto data=halo2::OriginalData::load_xbe(argv[image]);
      if (!data->full_scoreboard_constants)
        throw std::runtime_error("Unverified image supplied scoreboard constants");
      const auto &c=*data->full_scoreboard_constants;
      const std::array<std::pair<const char *,std::uint32_t>,12> actual{{
          {"0046a130",c.counter_seconds}, {"0046a120",c.fade_scale},
          {"0046a0cc",c.stagger_fraction}, {"0046a11c",c.stagger_scale},
          {"0046a0d0",c.one}, {"0046a1d8",c.header_rgb},
          {"0046a130",c.background_alpha}, {"00456918",c.text_scale},
          {"0046a22c",c.text_bias}, {"0046a240",c.dim_scale},
          {"00445b20",c.scroll_axis_scale}, {"0046a81c",c.scroll_pixels}}};
      const auto compare=[&](const char *address, std::uint64_t value, unsigned size) {
        const auto hex=words.at(address).at("bytes_hex").get<std::string>();
        if (hex.size()!=size*2) throw std::runtime_error("Truncated original constant word");
        for (unsigned byte=0; byte<size; ++byte)
          if (((value>>(byte*8))&255) != std::stoul(hex.substr(byte*2,2),nullptr,16))
            throw std::runtime_error(std::string("Imported scoreboard word differs: ")+address);
      };
      for (const auto &[address,value]:actual) compare(address,value,4);
      compare("0046a378",c.fade_exponent,8);
    }
    std::cout << "Actual selected executable imports all 12 scoreboard words and the exact 64-bit exponent\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
