#include "halo2/frontend_typewriter.hpp"
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace halo2;
namespace {
void check(bool condition, const std::string &message) {
  if (!condition)
    throw std::runtime_error(message);
}
std::u16string utf16(const std::string &hex) {
  if (hex == "-")
    return {};
  check(hex.size() % 4 == 0, "invalid original UTF16 payload");
  std::u16string result;
  for (std::size_t i = 0; i < hex.size(); i += 4)
    result.push_back(static_cast<char16_t>(
        std::stoul(hex.substr(i, 2), nullptr, 16) |
        (std::stoul(hex.substr(i + 2, 2), nullptr, 16) << 8)));
  return result;
}
void state_equal(const FrontendTypewriterState &actual,
                 const FrontendTypewriterState &expected,
                 const std::string &label) {
  check(actual.cursor == expected.cursor && actual.length == expected.length &&
            actual.last_clock == expected.last_clock,
        label + ": state differs from unchanged original text buffer");
}
} // namespace

int main(int argc, char **argv) {
  try {
    check(argc == 2, "usage: frontend_typewriter_tests original.tsv");
    std::ifstream input(argv[1]);
    check(bool(input), "missing original typewriter fixture");
    unsigned draws = 0, updates = 0, begins = 0, constructors = 0;
    for (std::string row; std::getline(input, row);) {
      std::istringstream in(row);
      char operation;
      in >> operation;
      if (operation == 'C') {
        FrontendTypewriterState want;
        in >> want.cursor >> want.length >> want.last_clock;
        state_equal({}, want, "constructor");
        ++constructors;
      } else {
        std::string label;
        in >> label;
        if (operation == 'D') {
          int interval;
          std::string glyph, supplied, displayed;
          std::uint32_t clock;
          bool nonzero;
          FrontendTypewriterState state, want;
          in >> interval >> glyph >> clock >> nonzero >> state.cursor >>
              state.length >> state.last_clock >> supplied >> want.cursor >>
              want.last_clock >> displayed;
          want.length = state.length;
          const auto actual = frontend_typewriter_draw(state, utf16(supplied),
              clock, static_cast<std::int16_t>(interval),
              static_cast<std::uint16_t>(std::stoul(glyph, nullptr, 16)), nonzero);
          check(actual == utf16(displayed),
                label + ": pre-advance UTF16 prefix differs from original22CD48");
          state_equal(state, want, label);
          ++draws;
        } else if (operation == 'U' || operation == 'B') {
          FrontendTypewriterState state, want;
          in >> state.cursor >> state.length >> state.last_clock;
          if (operation == 'U') {
            std::string supplied;
            in >> supplied;
            frontend_typewriter_set_text(state, utf16(supplied));
            ++updates;
          } else {
            frontend_typewriter_begin(state);
            ++begins;
          }
          in >> want.cursor >> want.length >> want.last_clock;
          state_equal(state, want, label);
        } else {
          throw std::runtime_error("unknown typewriter oracle operation");
        }
      }
      check(!in.fail(), "malformed original typewriter row");
    }
    check(draws == 527 && updates == 6 && begins == 2 && constructors == 1,
          "incomplete original typewriter fixture");
    std::cout << "Original typewriter: " << draws << " draw prefixes, " << updates
              << " text updates, " << begins << " activations, constructor exact\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
