#pragma once
#include "rust/cxx.h"
#include <cstdint>
namespace halo2::online_protocol_bridge {
struct NativeExchangeSession;
struct NativeExchangeCommon;
struct NativePublicationContext;
struct NativePublicationAdmission;
struct SourceAdmissionsPlayer;
struct MigrationPreparationCut;
enum class NativeExchangeCommonAction : std::uint8_t;
class NativeExchangeIo {
public:
    virtual ~NativeExchangeIo()=default;
    virtual NativeExchangeSession exchange_session()const=0;
    virtual std::uint64_t exchange_now_ms()const=0;
    virtual std::uint16_t exchange_direct_ready()const=0;
    virtual bool exchange_source_present()const=0;
    virtual NativePublicationAdmission exchange_continuity()const=0;
    virtual bool exchange_publish_source()const=0;
    virtual NativePublicationAdmission exchange_admission()const=0;
    virtual bool exchange_source_confirmed(std::uint32_t)const=0;
    virtual bool exchange_authority_present()const=0;
    virtual MigrationPreparationCut exchange_publish_authority(bool,std::uint64_t)const=0;
    virtual bool exchange_authority_supported(std::uint32_t,std::uint32_t)const=0;
    virtual NativeExchangeCommon exchange_common()const=0;
    virtual bool exchange_common_perform(NativeExchangeCommonAction)const=0;
    virtual void exchange_common_view()const=0;
    virtual void exchange_selection_abort()const=0;
    virtual void exchange_selection_progress()const=0;
    virtual NativeExchangeCommon exchange_quality()const=0;
    virtual void exchange_candidates(std::uint64_t)const=0;
};
}
