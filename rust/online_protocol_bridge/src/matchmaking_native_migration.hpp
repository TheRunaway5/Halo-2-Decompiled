#pragma once
#include "rust/cxx.h"
#include <cstdint>
namespace halo2::online_protocol_bridge {
struct HostAgreementState;
struct HostAgreementContext;
struct NativeMigrationBinding;
struct NativeMigrationSelection;
struct NativeSessionFence;
struct NativeSessionCandidate;
// Caller already holds the real Session generation mutex. These methods
// observe actual resources or apply an existing Rust Session operation; no
// Session borrow may span any callback, including reentrant Rust operations.
class NativeMigrationIo {
public:
    virtual ~NativeMigrationIo()=default;
    virtual NativeMigrationBinding migration_binding()const=0;
    virtual bool migration_agreement_present()const=0;
    virtual std::uint64_t migration_now_ms()const=0;
    virtual HostAgreementState migration_agreement(std::uint64_t)const=0;
    virtual bool migration_handoff()const=0;
    virtual bool migration_selection_started()const=0;
    virtual NativeMigrationBinding migration_quality()const=0;
    virtual rust::Vec<std::uint32_t> migration_ranked(std::uint64_t)const=0;
    virtual bool migration_selection_supported()const=0;
    virtual NativeMigrationSelection migration_selection()const=0;
    virtual void migration_apply_fence(const NativeSessionFence &)const=0;
    virtual void migration_begin(const HostAgreementContext &)const=0;
    virtual std::uint32_t migration_pick(const NativeSessionCandidate &)const=0;
};
}
