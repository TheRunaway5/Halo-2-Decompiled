#pragma once
#include "halo2/physics_contact_runtime.hpp"
#include <array>
#include <functional>
namespace halo2 {
struct NativeTu15ObjectSpatialPacketData {std::array<std::byte,20> bytes{};};
struct NativeTu15ObjectSpatialPacketStats {
  std::uint64_t flag_queries{},packets{},chain_updates{},chain_rows{},record_searches{},
    searched_rows{},matched_rows{},missing_rows{},word_copies{},byte_copies{},zero_copies{};
};
// Lazy scalar DATA formals preserve source argument-read phases for aliased
// component inputs. They do not describe a CPU, stack frame or program counter.
struct NativeTu15SpatialCopyParameters {
  std::function<std::uint32_t()> bytes,source;
};
struct NativeTu15SpatialChainParameters {
  std::function<std::uint32_t()> key,bytes;
};
// Source16E290; datum table is the current4ECBD0 object pool.
std::uint32_t native_tu15_object_spatial_flags(SourcePhysicsContactMemory&,
  std::uint32_t datum,NativeTu15ObjectSpatialPacketStats* = nullptr);
// SourceBA910. Identity-backed form preserves every read/store alias. The
// typed host form is ordinary disjoint20B packet DATA for a native caller.
void native_tu15_write_object_spatial_packet(SourcePhysicsContactMemory&,
  std::uint32_t datum,std::uint32_t output,
  const SourcePhysicsContactRecordObserver& = {},NativeTu15ObjectSpatialPacketStats* = nullptr);
NativeTu15ObjectSpatialPacketData native_tu15_build_object_spatial_packet(
  SourcePhysicsContactMemory&,std::uint32_t datum,NativeTu15ObjectSpatialPacketStats* = nullptr);
// Source29FFF0, DF0: pointer-to-head, key, record table, then late count/source
// DATA parameters. Returns matched record identity orFFFFFFFF; no GP state.
std::uint32_t native_tu15_update_spatial_record(SourcePhysicsContactMemory&,
  std::uint32_t head_word,std::uint32_t key,std::uint32_t table,
  const NativeTu15SpatialCopyParameters&,const SourcePhysicsContactRecordObserver& = {},
  NativeTu15ObjectSpatialPacketStats* = nullptr);
// Source1D30A0: rootObject/firstDatum, late per-row key, entry-read byte count,
// stable source identity. The scalar overload has ordinary immutable DATA.
void native_tu15_update_object_spatial_chain(SourcePhysicsContactMemory&,
  std::uint32_t root,std::uint32_t first_datum,std::uint32_t source,
  const NativeTu15SpatialChainParameters&,const SourcePhysicsContactRecordObserver& = {},
  NativeTu15ObjectSpatialPacketStats* = nullptr);
void native_tu15_update_object_spatial_chain(SourcePhysicsContactMemory&,
  std::uint32_t root,std::uint32_t first_datum,std::uint32_t key,
  std::uint32_t bytes,std::uint32_t source,const SourcePhysicsContactRecordObserver& = {},
  NativeTu15ObjectSpatialPacketStats* = nullptr);
// Native BA570 consumes its locally computed packet here. Host packet storage
// is disjoint from retained spatial rows; no source-frame address is invented.
void native_tu15_update_object_spatial_chain(SourcePhysicsContactMemory&,
  std::uint32_t root,std::uint32_t first_datum,std::uint32_t key,
  const NativeTu15ObjectSpatialPacketData&,const SourcePhysicsContactRecordObserver& = {},
  NativeTu15ObjectSpatialPacketStats* = nullptr);
} // namespace halo2
