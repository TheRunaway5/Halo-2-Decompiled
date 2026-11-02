#pragma once
#include "rust/cxx.h"
#include <cstdint>
namespace halo2::online_protocol_bridge {
struct NativeFrontendTextIo {
  virtual ~NativeFrontendTextIo()=default;
  virtual rust::Vec<std::uint8_t> text_original_text(std::uint32_t,std::uint32_t,std::uint32_t) const=0;
  virtual rust::Vec<std::uint8_t> text_original_string(std::uint32_t,std::uint32_t) const=0;
  virtual rust::Vec<std::uint8_t> text_pc_string(std::uint32_t) const=0;
  virtual rust::Vec<std::uint8_t> text_pc_authored(std::uint32_t) const=0;
  virtual rust::Vec<std::uint8_t> text_pc_match_line() const=0;
  virtual rust::Vec<std::uint8_t> text_map_name() const=0;
  virtual rust::Vec<std::uint8_t> text_quick_option(std::uint32_t) const=0;
  virtual std::uint64_t text_party_count() const=0;
  virtual rust::Vec<std::uint8_t> text_expand(rust::Slice<const std::uint8_t>) const=0;
};
} // namespace halo2::online_protocol_bridge
