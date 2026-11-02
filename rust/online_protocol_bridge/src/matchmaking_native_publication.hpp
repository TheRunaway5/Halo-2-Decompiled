#pragma once
#include "rust/cxx.h"
#include <cstdint>
namespace halo2::online_protocol_bridge {
struct NativePublicationDecision;
struct NativePublicationSession;
struct NativePublicationResources;
struct NativePublicationSelection;
struct NativePublicationAgreement;
struct NativePublicationCommon;
struct NativePublicationAdmission;
struct NativePublicationAudio;
struct NativePublicationSnapshot;
struct NativePublicationSuccessor;
struct MigrationPreparationCut;
class NativePublicationIo {
public:
    virtual ~NativePublicationIo()=default;
    virtual std::uint64_t publication_now_ms()const=0;
    virtual NativePublicationSession publication_session(const NativePublicationDecision &)const=0;
    virtual NativePublicationResources publication_resources()const=0;
    virtual NativePublicationSelection publication_selection(const MigrationPreparationCut &)const=0;
    virtual NativePublicationAgreement publication_agreement(std::uint64_t)const=0;
    virtual NativePublicationCommon publication_common()const=0;
    virtual NativePublicationAdmission publication_admission()const=0;
    virtual NativePublicationAudio publication_audio()const=0;
    virtual NativePublicationSnapshot publication_snapshot()const=0;
    virtual bool publication_blocks()const=0;
    virtual void publication_invalidate(const MigrationPreparationCut &)const=0;
    virtual bool publication_observe_ready(std::uint8_t,const MigrationPreparationCut &)const=0;
    virtual void publication_fence(bool)const=0;
    virtual bool publication_ready()const=0;
    virtual bool publication_propose(std::uint32_t,std::uint64_t)const=0;
    virtual std::uint32_t publication_candidate(std::uint64_t)const=0;
    virtual void publication_allocate(const NativePublicationSuccessor &)const=0;
    virtual bool publication_has_commit()const=0;
    virtual bool publication_commit()const=0;
    virtual void publication_publish()const=0;
};
}
