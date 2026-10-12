#pragma once
#include "halo2/datum_pool.hpp"
#include "halo2/source_script.hpp"
#include <span>
#include <vector>

namespace halo2 {
//11C5F0 consumes the spatial owner's ordered, at-most128 candidate bank.
//Transform/location admission and point containment are explicit services.
struct SourceHsVolumeServices {
  std::function<bool()> admit;
  std::function<std::vector<std::uint32_t>()> candidates;
  std::function<bool(std::uint32_t)> contains;
};
std::uint32_t source_hs_volume_objects(const SourceHsObjectListServices &,
                                     const SourceHsVolumeServices &);
//1DED00/1DED60/1DEDB0/1DEED0: one native-owned bank with the source's
//48 headers and128 references. Value copies include salts/links for checkpoints.
//This does not construct the Xbox game arena or supply spatial-query order.
class SourceHsListBank {
public:
  static constexpr std::uint32_t none=0xffffffffu;
  SourceHsListBank();
  std::uint32_t allocate();
  void append(std::uint32_t list,std::uint32_t object);
  std::uint16_t count(std::uint32_t list)const;
  //2A0280 treats the index as a signed short; nonpositive indices select0.
  std::uint32_t get(std::uint32_t list,std::int16_t index)const;
  std::vector<std::uint32_t> objects(std::uint32_t list)const;
  //Native VM root delivery. The source collector itself consumes header+4.
  void roots(std::span<const std::uint32_t>);
  void collect();
  const DatumPool &headers()const{return headers_;}
  const DatumPool &references()const{return references_;}
private:
  DatumPool headers_,references_;
};
}
