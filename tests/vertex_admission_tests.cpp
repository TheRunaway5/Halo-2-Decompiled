#include "halo2/vertex_program.hpp"
#include <cstring>
#include <iostream>
#include <stdexcept>
using namespace halo2;
int main() {
  try {
    std::vector<std::byte> code(20);
    const auto word = [&](unsigned offset, std::uint32_t value) {
      std::memcpy(code.data() + offset, &value, 4);
    };
    word(0, 0x00012078); // Supported NV2A header, one instruction.
    word(16, 1); // Final marker.
    if (decode_vertex_program(code).count != 1)
      throw std::runtime_error("supported vertex program rejected");
    for (bool constant_destination : {false, true}) {
      word(8, constant_destination ? 0 : 14u << 21);
      word(16, constant_destination ? 0x1001 : 1);
      bool unavailable = false;
      try { (void)decode_vertex_program(code); }
      catch (const UnsupportedVertexProgram &) { unavailable = true; }
      if (!unavailable)
        throw std::runtime_error("unsupported program cannot select renderer fallback");
    }
    // A feature gap must not hide corruption in a known-format program.
    word(8, 14u << 21);
    word(16, 0);
    bool malformed = false;
    try { (void)decode_vertex_program(code); }
    catch (const UnsupportedVertexProgram &) {
      throw std::runtime_error("missing final marker hidden by feature fallback");
    } catch (const FormatError &) { malformed = true; }
    if (!malformed) throw std::runtime_error("missing final marker admitted");
    word(16, 1);
    code.pop_back();
    malformed = false;
    try { (void)decode_vertex_program(code); }
    catch (const UnsupportedVertexProgram &) {
      throw std::runtime_error("truncated program hidden by feature fallback");
    } catch (const FormatError &) { malformed = true; }
    if (!malformed) throw std::runtime_error("truncated program admitted");
    std::cout << "Unsupported vertex fallback and malformed-input rejection passed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
