#pragma once
#include "rust/cxx.h"
#include <cstdint>
namespace halo2::online_protocol_bridge {
struct NativePreparationTicket;
struct NativePreparationSelection;
struct MigrationDriverAction;
struct MigrationPreparationCut;
enum class MigrationDriverRetirement : std::uint8_t;
enum class MigrationDriverFreezeKind : std::uint8_t;
class NativePreparationIo {
public:
    virtual ~NativePreparationIo()=default;
    virtual bool preparation_driver_present()const=0;
    virtual bool preparation_owner_matches()const=0;
    virtual void preparation_observe()const=0;
    virtual MigrationDriverAction preparation_step()const=0;
    virtual bool preparation_reserve(const NativePreparationTicket &)const=0;
    virtual bool preparation_retained(const NativePreparationTicket &,std::uint64_t)const=0;
    virtual MigrationDriverRetirement preparation_retired(const NativePreparationTicket &)const=0;
    virtual MigrationDriverFreezeKind preparation_admit(const NativePreparationTicket &)const=0;
    virtual MigrationDriverFreezeKind preparation_admit_external()const=0;
    virtual NativePreparationSelection preparation_selection()const=0;
    virtual bool preparation_publish()const=0;
    virtual bool preparation_invalidate(const MigrationPreparationCut &)const=0;
    virtual void preparation_abort()const=0;
    virtual void preparation_progress()const=0;
    virtual void preparation_begin()const=0;
    virtual void preparation_terminal()const=0;
    virtual bool preparation_retirement_pending()const=0;
};
}
