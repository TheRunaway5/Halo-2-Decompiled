#include "halo2/source_runtime_memory.hpp"
#include <algorithm>
#include <cstring>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace halo2 {
namespace {
std::uint64_t end(std::uint32_t p, std::size_t n) {
  if (n > (std::uint64_t{1} << 32) - p)
    throw std::out_of_range("Original runtime memory extent wraps32 bits");
  return std::uint64_t(p) + n;
}
[[noreturn]] void missing(std::uint32_t p) {
  std::ostringstream message;
  message << "Required original runtime backing unavailable at0x" << std::hex
          << p;
  throw std::out_of_range(message.str());
}
} // namespace
void SourceRuntimeMemory::mount(std::uint32_t p,
                                std::span<const std::byte> initial,
                                Access access, std::string owner) {
  if (initial.empty() || owner.empty())
    throw std::invalid_argument(
        "Original runtime region requires backing and an owner");
  check_extent(p, initial.size());
  regions_.emplace(
      p, Region{{initial.begin(), initial.end()}, access, std::move(owner)});
}
void SourceRuntimeMemory::check_extent(std::uint32_t p, std::size_t bytes) const {
  const auto limit = end(p, bytes);
  const auto next = regions_.lower_bound(p);
  if (next != regions_.end() && next->first < limit)
    throw std::invalid_argument("Original runtime regions overlap");
  if (next != regions_.begin()) {
    const auto previous = std::prev(next);
    if (std::uint64_t(previous->first) + previous->second.bytes.size() > p)
      throw std::invalid_argument("Original runtime regions overlap");
  }
}
void SourceRuntimeMemory::reserve(std::uint32_t p, std::size_t bytes,
                                  std::size_t page_bytes, std::string owner) {
  if (!bytes || !page_bytes || p % page_bytes || bytes % page_bytes ||
      owner.empty())
    throw std::invalid_argument("Original reservation requires aligned pages and an owner");
  check_extent(p, bytes);
  regions_.emplace(p, Region{std::vector<std::byte>(bytes), Access::ReadWrite,
                             std::move(owner), page_bytes,
                             std::vector<bool>(bytes / page_bytes, false)});
}
void SourceRuntimeMemory::commit(std::uint32_t p,
                                 std::span<const std::byte> initial) {
  end(p, initial.size());
  const auto found = locate(p);
  auto &region = regions_.at(found->first);
  const auto offset = std::size_t(p - found->first);
  if (!region.page_bytes || initial.empty() || offset % region.page_bytes ||
      initial.size() % region.page_bytes ||
      initial.size() > region.bytes.size() - offset)
    throw std::invalid_argument("Original commit must fit aligned reserved pages");
  const std::vector<std::byte> staged(initial.begin(), initial.end());
  for (std::size_t done = 0; done < staged.size(); done += region.page_bytes) {
    const auto page = (offset + done) / region.page_bytes;
    if (!region.committed[page]) {
      std::memcpy(region.bytes.data() + offset + done, staged.data() + done,
                  region.page_bytes);
      region.committed[page] = true;
    }
  }
}
void SourceRuntimeMemory::release(std::uint32_t p) {
  if (!regions_.erase(p))
    throw std::out_of_range("Original runtime release has no region owner");
}
void SourceRuntimeMemory::visit_committed(const std::function<void(std::uint32_t,
    std::span<const std::byte>)>&visit) const {
  for(const auto&[address,region]:regions_) {
    if(!region.page_bytes) {visit(address,region.bytes);continue;}
    for(std::size_t page=0;page<region.committed.size();) {
      if(!region.committed[page]) {++page;continue;}
      const auto first=page;
      while(page<region.committed.size() && region.committed[page])++page;
      const auto offset=first*region.page_bytes,bytes=(page-first)*region.page_bytes;
      visit(address+std::uint32_t(offset),std::span(region.bytes).subspan(offset,bytes));
    }
  }
}
SourceRuntimeMemory::Position
SourceRuntimeMemory::locate(std::uint32_t p) const {
  auto found = regions_.upper_bound(p);
  if (found == regions_.begin())
    missing(p);
  --found;
  if (std::uint64_t(p) - found->first >= found->second.bytes.size())
    missing(p);
  return found;
}
void SourceRuntimeMemory::validate(std::uint32_t p, std::size_t n,
                                   bool writable) const {
  const auto limit = end(p, n);
  auto current = std::uint64_t(p);
  while (current < limit) {
    const auto found = locate(std::uint32_t(current));
    if (writable && found->second.access != Access::ReadWrite)
      throw std::runtime_error("Original runtime write to read-only owner " +
                               found->second.owner);
    const auto &region = found->second;
    const auto region_end = std::min(limit, std::uint64_t(found->first) +
                                               region.bytes.size());
    if (region.page_bytes) {
      const auto first = (current - found->first) / region.page_bytes;
      const auto last = (region_end - 1 - found->first) / region.page_bytes;
      for (auto page = first; page <= last; ++page)
        if (!region.committed[page])
          missing(found->first + std::uint32_t(page * region.page_bytes));
    }
    current = region_end;
  }
}
void SourceRuntimeMemory::read(std::uint32_t p,
                               std::span<std::byte> output) const {
  validate(p, output.size(), false);
  // Stage before publishing to preserve aliases and leave the destination
  // untouched when the requested source extent includes missing backing.
  std::vector<std::byte> staged(output.size());
  std::size_t done = 0;
  while (done < staged.size()) {
    const auto found = locate(p + std::uint32_t(done));
    const auto offset = std::size_t(std::uint64_t(p) + done - found->first);
    const auto count =
        std::min(staged.size() - done, found->second.bytes.size() - offset);
    std::memcpy(staged.data() + done, found->second.bytes.data() + offset,
                count);
    done += count;
  }
  if (!staged.empty())
    std::memmove(output.data(), staged.data(), staged.size());
}
void SourceRuntimeMemory::write(std::uint32_t p,
                                std::span<const std::byte> input) {
  validate(p, input.size(), true);
  const std::vector<std::byte> staged(input.begin(), input.end());
  std::size_t done = 0;
  while (done < staged.size()) {
    const auto found = locate(p + std::uint32_t(done));
    auto &region = regions_.at(found->first);
    const auto offset = std::size_t(std::uint64_t(p) + done - found->first);
    const auto count =
        std::min(staged.size() - done, region.bytes.size() - offset);
    std::memcpy(region.bytes.data() + offset, staged.data() + done, count);
    done += count;
  }
}
std::span<const std::byte> SourceRuntimeMemory::view(std::uint32_t p,
                                                     std::size_t n) const {
  end(p, n);
  const auto found = locate(p);
  const auto offset = std::size_t(p - found->first);
  if (n > found->second.bytes.size() - offset)
    throw std::out_of_range("Original runtime view crosses its region owner");
  validate(p, n, false);
  return std::span(found->second.bytes).subspan(offset, n);
}
std::span<std::byte> SourceRuntimeMemory::mutable_view(std::uint32_t p,
                                                       std::size_t n) {
  const auto bytes = view(p, n);
  const auto found = locate(p);
  if (found->second.access != Access::ReadWrite)
    throw std::runtime_error(
        "Original runtime mutable view of read-only owner " +
        found->second.owner);
  return std::span(regions_.at(found->first).bytes)
      .subspan(p - found->first, bytes.size());
}
} // namespace halo2
