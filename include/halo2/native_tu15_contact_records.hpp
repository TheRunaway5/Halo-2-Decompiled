#pragma once
#include "halo2/native_tu15_physics_frame.hpp"
#include <array>
namespace halo2 {
struct NativeTu15ContactRecord {std::uint32_t identity{};};
struct NativeTu15ContactRecordArray {std::uint32_t identity{};};
struct NativeTu15ContactRecordSettings {std::uint32_t identity{};};
using NativeTu15ContactRecordVector=std::array<std::uint32_t,4>;
enum class NativeTu15ContactRecordPublication {copy,selected,tail,filter_false,filter_true};
// Offsets name verified opaque fields. Bytes28..2F and7E..7F are absent from
// the original copy/swap, and remain attached to their actual storage slots.
// A publication commits its memory before invoking its optional completion
// callback and then its observer. This distinguishes a rejected scalar FSTP
// from an observer failure after the real destination accepted that store.
class NativeTu15ContactRecordStorage {
public:
    virtual ~NativeTu15ContactRecordStorage()=default;
    virtual NativeTu15ContactRecordVector vector(NativeTu15ContactRecord,unsigned)const=0;
    virtual std::uint32_t word(NativeTu15ContactRecord,unsigned)const=0;
    virtual std::uint8_t byte(NativeTu15ContactRecord,unsigned)const=0;
    virtual std::uint32_t setting(NativeTu15ContactRecordSettings,unsigned)const=0;
    virtual std::uint32_t comparison_limit()const=0;
    virtual NativeTu15ContactRecord data(NativeTu15ContactRecordArray)const=0;
    virtual void vector(NativeTu15ContactRecord,unsigned,const NativeTu15ContactRecordVector&,NativeTu15ContactRecordPublication)=0;
    virtual void word(NativeTu15ContactRecord,unsigned,std::uint32_t,NativeTu15ContactRecordPublication,const std::function<void()>& committed={})=0;
    virtual void byte(NativeTu15ContactRecord,unsigned,std::uint8_t,NativeTu15ContactRecordPublication)=0;
};
// Actual303BA0. The semantic result is the supplied destination reference.
NativeTu15ContactRecord native_tu15_copy_contact_record(NativeTu15ContactRecordStorage&,
    NativeTu15ContactRecord destination,NativeTu15ContactRecord source);
// Actual303800 computes a byte result; its incoming ECX is overwritten before
// use. The first caller converted below consumes this result directly.
std::uint8_t native_tu15_test_contact_record(NativeTu15ContactRecordStorage&,
    NativeTu15ContactRecord first,NativeTu15ContactRecord second,
    NativeTu15ContactRecordSettings,NativeTu15PhysicsFrameFp&,unsigned incoming_x87_depth=0);
// Actual304130: signed index/count comparisons with32-bit wrapping offsets,
// exact flag==1 selection, real303800 predicate, and ordered fieldwise swaps.
std::uint32_t native_tu15_filter_contact_records(NativeTu15ContactRecordStorage&,
    NativeTu15ContactRecord probe,NativeTu15ContactRecordArray,
    std::uint32_t first_index,std::uint32_t count,NativeTu15ContactRecordSettings,
    NativeTu15PhysicsFrameFp&,unsigned incoming_x87_depth=0);
class NativeTu15ContactRecordMemory final:public NativeTu15ContactRecordStorage {
    SourcePhysicsContactMemory&memory_;SourcePhysicsContactRecordObserver observer_;
    std::uint32_t read(std::uint32_t,unsigned)const;
    void publish(std::uint32_t,unsigned,std::span<const std::byte>,NativeTu15ContactRecordPublication,const std::function<void()>& = {});
public:
    explicit NativeTu15ContactRecordMemory(SourcePhysicsContactMemory&,const SourcePhysicsContactRecordObserver& = {});
    NativeTu15ContactRecordVector vector(NativeTu15ContactRecord,unsigned)const override;
    std::uint32_t word(NativeTu15ContactRecord,unsigned)const override;
    std::uint8_t byte(NativeTu15ContactRecord,unsigned)const override;
    std::uint32_t setting(NativeTu15ContactRecordSettings,unsigned)const override;
    std::uint32_t comparison_limit()const override;
    NativeTu15ContactRecord data(NativeTu15ContactRecordArray)const override;
    void vector(NativeTu15ContactRecord,unsigned,const NativeTu15ContactRecordVector&,NativeTu15ContactRecordPublication)override;
    void word(NativeTu15ContactRecord,unsigned,std::uint32_t,NativeTu15ContactRecordPublication,const std::function<void()>& = {})override;
    void byte(NativeTu15ContactRecord,unsigned,std::uint8_t,NativeTu15ContactRecordPublication)override;
    // Retained ABI publication for303800's output byte, after the typed result
    // has been calculated. No virtual register result is synthesized here.
    void filter_result(std::uint32_t destination,std::uint8_t);
};
} // namespace halo2
