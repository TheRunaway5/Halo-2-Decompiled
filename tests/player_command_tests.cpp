#include "halo2/player_command.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace {
void check(bool value, const std::string &context) {
  if (!value) throw std::runtime_error("Original player command differs: " + context);
}
std::vector<std::byte> bytes(const std::string &text) {
  check(text.size() % 2 == 0, "hex extent");
  std::vector<std::byte> out;
  for (std::size_t i = 0; i < text.size(); i += 2)
    out.push_back(std::byte(std::stoul(text.substr(i, 2), nullptr, 16)));
  return out;
}
template<std::size_t N> std::array<std::byte, N> buffer(const std::string &text) {
  const auto raw = bytes(text);
  check(raw.size() == N, "literal source buffer extent");
  std::array<std::byte, N> out;
  std::copy(raw.begin(), raw.end(), out.begin());
  return out;
}
std::uint32_t hexword(const std::string &s) {
  return std::uint32_t(std::stoul(s,nullptr,16));
}
void producer_fixtures(const char *path) {
  std::ifstream file(path);
  unsigned produced=0, expected_count=0;
  for (std::string line; std::getline(file,line);) {
    std::istringstream stream(line);
    std::vector<std::string> r;
    for (std::string s; stream>>s;) r.push_back(s);
    if (r.empty()) continue;
    if (r[0]=="N") { expected_count=unsigned(std::stoul(r.at(1))); continue; }
    check(r[0]=="P" && r.size()==15,"source producer schema");
    const auto command=buffer<0x5c>(r[2]);
    const auto forward=buffer<12>(r[9]);
    const auto read=[&](unsigned at) {
      std::uint32_t v=0;
      for (unsigned i=0;i<4;++i) v|=std::uint32_t(std::to_integer<unsigned>(forward[at+i]))<<(i*8);
      return std::bit_cast<float>(v);
    };
    const halo2::SourceCommandCurrentUnit current{
      hexword(r[3]),std::uint8_t(hexword(r[4])),hexword(r[5]),
      std::uint8_t(hexword(r[6])),bool(std::stoul(r[7])),
      {read(0),read(4),read(8)},hexword(r[10]),
      std::bit_cast<std::int16_t>(std::uint16_t(hexword(r[11]))),std::nullopt};
    const auto out=halo2::produce_source_player_control(command,current,hexword(r[8]));
    check(bool(out),r[1]+" measured available branch");
    const auto expected=buffer<0x7c>(r[13]);
    if (out->packet!=expected) {
      for (unsigned i=0;i<0x7c;++i) if (out->packet[i]!=expected[i]) {
        std::ostringstream message;
        message<<r[1]<<" complete control byte+"<<std::hex<<i
               <<" got "<<std::to_integer<unsigned>(out->packet[i])
               <<" expected "<<std::to_integer<unsigned>(expected[i]);
        check(false,message.str());
      }
    }
    check(out->target_unit_flags134==hexword(r[12]),r[1]+" preceding target+134 store");
    check(unsigned(out->movement_basis_admitted)==std::stoul(r[14]),r[1]+" original 151A20 AL");
    ++produced;
  }
  check(produced==expected_count && produced>=15,"complete independent producer fixture");
  // The unmeasured seated transform must not silently emit on-foot output.
  std::array<std::byte,0x5c> neutral;
  halo2::initialize_source_neutral_command(neutral);
  halo2::SourceCommandCurrentUnit seated{0,0,1,1,false,{1.f,0.f,0.f},1,0,std::nullopt};
  check(!halo2::produce_source_player_control(neutral,seated,0),"missing seat definition unavailable");
  seated.parent_seat_flags=0;
  check(!halo2::produce_source_player_control(neutral,seated,0),"positive CCC60 unavailable");
  seated.parent_seat_flags=0x10;
  check(bool(halo2::produce_source_player_control(neutral,seated,0)),"original seat-bit16 skips transform");
  std::cout<<produced<<" complete original command/control packets, admission and +134 stores matched\n";
}
} // namespace

int main(int argc, char **argv) {
  try {
    if (argc != 2 && argc != 3) throw std::runtime_error("Supply command fixture and optional producer fixture");
    if (!std::filesystem::exists(argv[1])) return 77;
    if (argc==3 && !std::filesystem::exists(argv[2])) return 77;
    std::ifstream file(argv[1]);
    unsigned initialized = 0, delivered = 0, accepted = 0;
    for (std::string line; std::getline(file, line);) {
      std::istringstream stream(line);
      std::vector<std::string> r;
      for (std::string s; stream >> s;) r.push_back(s);
      if (r.empty()) continue;
      const auto context = "case " + r.at(1);
      if (r[0] == "I") {
        auto command = buffer<0x5c>(r.at(2));
        const auto expected = buffer<0x5c>(r.at(3));
        halo2::initialize_source_neutral_command(command);
        check(command == expected, context + " complete 14E780 buffer");
        ++initialized;
      } else if (r[0] == "D") {
        const auto record = buffer<0x88>(r.at(2));
        auto output = buffer<0x5c>(r.at(3));
        const auto expected = buffer<0x5c>(r.at(4));
        const auto tick = std::uint32_t(std::stoul(r.at(5), nullptr, 16));
        const auto dt = std::bit_cast<float>(std::uint32_t(std::stoul(r.at(6), nullptr, 16)));
        const auto expiration = std::bit_cast<float>(std::uint32_t(std::stoul(r.at(7), nullptr, 16)));
        const auto expected_al = std::stoul(r.at(8));
        const auto result = halo2::deliver_source_cached_command(record, output, tick, dt, expiration);
        check(unsigned(result) == expected_al, context + " defined original AL");
        check(output == expected, context + " complete delivery/unchanged rejected buffer");
        check(record == buffer<0x88>(r.at(9)), context + " complete unchanged original record");
        if (result) {
          check(halo2::source_player_command_valid(output), context + " executed original validator");
          ++accepted;
        }
        ++delivered;
      } else throw std::runtime_error("Unknown source player command fixture row");
    }
    check(initialized == 2 && delivered == 2 && accepted == 1,
          "complete independently measured two-case suite");
    std::cout << initialized << " original neutral buffers and " << delivered
              << " cached deliveries matched; " << accepted << " original admission\n";
    if (argc==3) producer_fixtures(argv[2]);
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
