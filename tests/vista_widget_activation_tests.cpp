#include "halo2/vista_widget_activation.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string_view>
#include <vector>

using namespace halo2;
using Json = nlohmann::json;
namespace {
std::vector<std::byte> unhex(std::string_view raw) {
  if (raw.size() % 2) throw FormatError("Invalid widget animation hex extent");
  const auto digit = [](char c) -> unsigned {
    if (c >= '0' && c <= '9') return unsigned(c - '0');
    if (c >= 'a' && c <= 'f') return unsigned(c - 'a') + 10;
    throw FormatError("Invalid widget animation hex digit");
  };
  std::vector<std::byte> out(raw.size() / 2);
  for (std::size_t i = 0; i < out.size(); ++i)
    out[i] = std::byte{std::uint8_t(16 * digit(raw[2 * i]) + digit(raw[2 * i + 1]))};
  return out;
}
std::string hex(std::span<const std::byte> bank) {
  constexpr char digits[] = "0123456789abcdef";
  std::string out;
  for (const auto b : bank) {
    const auto value = std::to_integer<unsigned>(b);
    out += digits[value >> 4]; out += digits[value & 15];
  }
  return out;
}
std::uint32_t read(std::span<const std::byte> bank, std::size_t offset) {
  std::uint32_t value{};
  for (std::size_t i = 0; i < 4; ++i)
    value |= std::uint32_t(std::to_integer<unsigned>(bank[offset + i])) << (8 * i);
  return value;
}
void write(std::span<std::byte> bank, std::size_t offset, std::uint32_t value) {
  for (std::size_t i = 0; i < 4; ++i) bank[offset + i] = std::byte{std::uint8_t(value >> (8 * i))};
}
template<class F> void rejects(F&& f) {
  try { f(); } catch (const FormatError&) { return; }
  throw FormatError("Invalid widget activation boundary was accepted");
}
} // namespace

int main(int argc, char** argv) try {
  if (argc != 2) return 77;
  std::ifstream input(argv[1]);
  if (!input) return 77;
  Json fixture; input >> fixture;
  if (fixture.at("schema") != "H2_VISTA_WIDGET_ACTIVATION_ORIGINAL_V1")
    throw FormatError("Unexpected original widget animation fixture");
  std::size_t cases{}, clock_total{}, descriptor_total{};
  for (const auto& row : fixture.at("cases")) {
    const auto& q = row.at("input");
    std::vector<std::vector<std::byte>> storage;
    for (const auto& bank : q.at("banks")) storage.push_back(unhex(bank.get<std::string>()));
    std::vector<VistaWidgetActivationNode> nodes(storage.size());
    for (std::size_t i = 0; i < nodes.size(); ++i) nodes[i].bank = storage[i];
    const auto link = [&](std::uint32_t source) -> VistaWidgetActivationNode* {
      if (!source) return nullptr;
      if (source < 0x1a00000 || (source - 0x1a00000) % 0x100 ||
          (source - 0x1a00000) / 0x100 >= nodes.size())
        throw FormatError("Unowned original animation widget identity");
      return &nodes[(source - 0x1a00000) / 0x100];
    };
    for (auto& node : nodes) {
      node.first_child = link(read(node.bank, 0x14));
      node.next_sibling = link(read(node.bank, 0x18));
    }
    Json trace = Json::array();
    std::size_t current{}, clocks{}, descriptors{};
    const auto id = [&](const VistaWidgetActivationNode& node) {
      return std::size_t(&node - nodes.data());
    };
    const auto mutate = [&](const Json& action) {
      if (action.is_null()) return;
      auto& target = nodes.at(action.at("node").get<std::size_t>());
      const auto& field = action.at("field");
      if (field == "kind") write(target.bank, 4, action.at("value").get<std::uint32_t>());
      else {
        const auto value = action.at("value").is_null() ? 0u :
            0x1a00000u + 0x100u * action.at("value").get<std::uint32_t>();
        if (field == "next") { target.next_sibling = link(value); write(target.bank, 0x18, value); }
        else if (field == "child") { target.first_child = link(value); write(target.bank, 0x14, value); }
        else throw FormatError("Unknown original widget graph mutation");
      }
    };
    VistaWidgetActivationServices services;
    services.descriptor_scratch = [&](auto& node, auto) {
      const auto raw = unhex(q.at("scripts").at(id(node)).at("scratch").template get<std::string>());
      if (raw.size() != 0x34) throw FormatError("Original stack scratch size changed");
      VistaWidgetAnimationDescriptor out;
      std::copy(raw.begin(), raw.end(), out.begin()); return out;
    };
    services.descriptor = [&](auto& node, auto index, auto reason, auto output) -> std::uint8_t {
      current = id(node); ++descriptors;
      trace.push_back(Json::array({"descriptor", current, index, reason}));
      const auto& script = q.at("scripts").at(current);
      if (hex(output) != script.at("scratch").template get<std::string>())
        throw FormatError("Native descriptor scratch changed");
      for (const auto& operation : script.at("writes")) {
        const auto offset = operation.at(0).template get<std::size_t>();
        const auto raw = unhex(operation.at(1).template get<std::string>());
        if (offset > output.size() || raw.size() > output.size() - offset)
          throw FormatError("Descriptor service write exceeds original stack descriptor");
        std::copy(raw.begin(), raw.end(), output.begin() + offset);
      }
      mutate(script.value("mutation", Json()));
      return std::uint8_t(script.at("result").template get<std::uint32_t>());
    };
    services.read_cached_ui_clock = [&]() {
      trace.push_back(Json::array({"clock", current, hex(nodes.at(current).bank)}));
      const auto& values = q.at("clocks");
      return values.at(clocks++ % values.size()).get<std::uint32_t>();
    };
    services.kind2_animation = [&](auto& node, auto descriptor) {
      current = id(node);
      trace.push_back(Json::array({"kind2", current, hex(descriptor)}));
      write(node.bank, 0x70, 0xf00d0000u | std::uint32_t(current));
      mutate(q.at("scripts").at(current).value("virtual_mutation", Json()));
    };
    if (q.at("kind") == "birth") {
      std::vector<std::byte> raw;
      const std::byte* data;
      if (q.at("descriptor_alias").is_null()) {
        raw = unhex(q.at("descriptor").get<std::string>()); data = raw.data();
      } else data = nodes[0].bank.data() + q.at("descriptor_alias").get<std::size_t>();
      vista_widget_animation_birth(nodes[0].bank,
          std::span<const std::byte, 0x34>{data, 0x34}, services.read_cached_ui_clock);
    } else vista_widget_activate(nodes[0], q.at("reason").get<std::uint32_t>(), services);
    if (trace != row.at("trace")) {
      std::cerr << "activation case " << cases << " trace\nactual " << trace << "\noriginal " << row.at("trace") << '\n';
      throw FormatError("Original widget activation callback order differs");
    }
    for (std::size_t i = 0; i < nodes.size(); ++i)
      if (hex(nodes[i].bank) != row.at("banks").at(i).get<std::string>())
        throw FormatError("Original widget animation retained bank differs in case " + std::to_string(cases));
    if (clocks != row.at("clock_reads") || descriptors != row.at("descriptor_calls"))
      throw FormatError("Original widget animation clock/descriptor count differs");
    ++cases; clock_total += clocks; descriptor_total += descriptors;
  }
  std::array<std::byte, 0x80> bank{};
  VistaWidgetAnimationDescriptor descriptor{};
  VistaWidgetActivationNode node{bank, nullptr, nullptr};
  rejects([&] { vista_widget_activate(node, 1, {}); });
  rejects([&] { vista_widget_animation_birth(std::span{bank}.first(0x67), descriptor, [] { return 0u; }); });
  rejects([&] { vista_widget_animation_birth(bank, descriptor, {}); });
  std::cout << cases << " original activation cases; " << clock_total << " independent clock reads; "
            << descriptor_total << " descriptor calls matched\n";
  return 0;
} catch (const std::exception& error) {
  std::cerr << error.what() << '\n'; return 1;
}
