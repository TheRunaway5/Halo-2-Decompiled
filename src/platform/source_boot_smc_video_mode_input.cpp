#include "halo2/source_boot_smc_video_mode_input.hpp"
#include "halo2/source_boot_smc_video_mode.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <algorithm>
#include <fstream>
#include <stdexcept>

namespace halo2 {
namespace {
constexpr std::array magic{std::byte{'H'},std::byte{'2'},std::byte{'S'},std::byte{'M'},
    std::byte{'C'},std::byte{'R'},std::byte{'1'},std::byte{0}};
std::uint32_t word(std::span<const std::byte> bytes) {
  std::uint32_t result=0;
  for(unsigned i=0;i<4;++i)result|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);
  return result;
}
}
SourceBootSmcVideoModeInput::SourceBootSmcVideoModeInput(std::uint32_t prior,
    std::uint32_t status,std::uint32_t response,std::string owner)
    :inputs_{prior,status,response},owner_(std::move(owner)) {
  if(owner_.empty())throw std::invalid_argument("Boot SMC input requires an explicit resource owner");
}
SourceBootSmcVideoModeInput SourceBootSmcVideoModeInput::load(const std::filesystem::path &path) {
  std::ifstream input(path,std::ios::binary|std::ios::ate);
  if(!input || input.tellg()!=20)
    throw std::runtime_error("Boot SMC input requires an H2SMCR1 resource of exactly 20 bytes");
  std::array<std::byte,20> bytes;input.seekg(0);
  if(!input.read(reinterpret_cast<char*>(bytes.data()),bytes.size()) ||
      !std::ranges::equal(std::span(bytes).first(8),magic))
    throw std::runtime_error("Invalid owned boot SMC input resource");
  return {word(std::span(bytes).subspan(8,4)),word(std::span(bytes).subspan(12,4)),
      word(std::span(bytes).subspan(16,4)),path.string()};
}
std::uint32_t SourceBootSmcVideoModeInput::published_value() const noexcept {
  return source_boot_smc_video_mode_after_read(inputs_[0],inputs_[1],inputs_[2]);
}
void source_mount_tu15_boot_smc_video_mode_input(SourceRuntimeMemory &memory,
    const SourceBootSmcVideoModeInput &input) {
  constexpr std::array getter{std::byte{0xa1},std::byte{0x84},std::byte{0xb7},
      std::byte{0x41},std::byte{0},std::byte{0x8b},std::byte{0},std::byte{0xc3}};
  constexpr std::uint32_t encoded_import=0x80000164u;
  if(word(memory.view(0x41b784u,4))!=encoded_import ||
      !std::ranges::equal(memory.view(0x33c29du,getter.size()),getter))
    throw std::invalid_argument("Original TU1.5 boot SMC import/getter ownership differs");
  std::array<std::byte,4> bytes;
  const auto value=input.published_value();
  for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(8*i));
  memory.mount(encoded_import,bytes,SourceRuntimeMemory::Access::ReadOnly,
      "Explicit boot SMC transaction publication: "+input.owner());
}
}
