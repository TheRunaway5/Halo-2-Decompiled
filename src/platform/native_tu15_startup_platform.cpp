#include "halo2/native_tu15_startup_platform.hpp"
#include "halo2/source_tu15_settings.hpp"

namespace halo2 {
namespace {
std::uint32_t word(const SourceRuntimeMemory &m, std::uint32_t p) {
    const auto bytes=m.view(p,4);std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);
    return value;
}
void put(SourceRuntimeMemory &m, std::uint32_t site, std::uint32_t p,
         std::uint32_t value, const SourcePhysicsContactRecordObserver &observe) {
    const auto old=m.view(p,4);
    SourcePhysicsContactRecordStore change{site,p,{old.begin(),old.end()},std::vector<std::byte>(4)};
    for(unsigned i=0;i<4;++i)change.written[i]=std::byte(value>>(8*i));
    m.write(p,change.written);if(observe)observe(change);
}
}
NativeTu15StartupPlatform::NativeTu15StartupPlatform(NativeTu15StartupSettings settings)
    :settings_(settings),hardware_{0x90000000u,{},"Halo2 native desktop startup capabilities"} {}
SourceRuntimeMemory NativeTu15StartupPlatform::load_image(const std::filesystem::path &path) const {
    return load_source_runtime_image(path,SourcePhysicsContactImage::TitleUpdate15,hardware_);
}
SourceProcessHeapDataImportOwner NativeTu15StartupPlatform::data_owner() const {
    return source_tu15_hardware_import_owner(hardware_);
}
bool NativeTu15StartupPlatform::try_call(SourceRuntimeMemory &m,
    const SourcePhysicsContactRuntimeCall &call, SourcePhysicsContactRuntimeAbi &abi,
    const SourcePhysicsContactRecordObserver &observe) const {
    if(call.instruction==0x2e101cu && call.target==0x80000018u) {
        const auto sp=abi.gp[4],bp=abi.gp[5];m.view(sp,24);m.view(bp,28);
        if(!call.tail || word(m,sp)!=0x2e0ff9u || sp!=bp-24 ||
            word(m,sp+4)!=17 || word(m,sp+16)!=4)
            throw std::runtime_error("Native startup setting requires the original XC_MISC frame");
        for(unsigned i=1;i<=5;++i)
            if(word(m,sp+i*4)!=word(m,bp+4+i*4))
                throw std::runtime_error("Native startup setting arguments lost their original owner");
        const auto type=word(m,sp+8),value=word(m,sp+12),length=word(m,sp+20);
        // Validate all outputs before publication. There is no EEPROM checksum
        // or fabricated EEPROM success: this operation queries owned native
        // application configuration with a declared preserving stdcall ABI.
        m.mutable_view(length,4);m.mutable_view(type,4);m.mutable_view(value,4);
        put(m,call.instruction,length,4,observe);
        put(m,call.instruction,type,4,observe);
        put(m,call.instruction,value,settings_.automatic_power_down?1u:0u,observe);
        abi.gp[0]=0;abi.gp[4]=sp+24;return true;
    }
    if(call.instruction==0x2e0ffau && call.target==0x8000012du) {
        const auto sp=abi.gp[4];m.view(sp,8);
        if(call.tail || word(m,sp)!=0x2e1000u || word(m,sp+4)!=abi.gp[0])
            throw std::runtime_error("Native startup status requires the original settings frame");
        // This native query has exactly one returning status. The original
        // reference converter maps STATUS_SUCCESS to ERROR_SUCCESS. Other
        // conversions belong to an explicit owner, not a BIOS table fallback.
        if(abi.gp[0]!=0)
            throw std::runtime_error("Native startup status conversion outside its supported query domain");
        abi.gp[4]=sp+8;return true;
    }
    return false;
}
void NativeTu15StartupPlatform::run_startup(SourceRuntimeMemory &m,std::uint32_t entry,
    SourcePhysicsContactRuntimeAbi &abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsContactRuntimeServices &services,SourceRuntimeHostVirtualMemory &vm,
    PhysicsPlatformLocks &locks,const PhysicsPlatformLockServices &platform,
    SourceProcessShutdownRegistry &shutdown,const SourcePhysicsContactRecordObserver &observe,
    const SourcePhysicsContactRecordObserver &record_observe,
    const SourceProcessShutdownObserver &shutdown_observe,
    const SourcePhysicsContactRecordObserver &query_observe) const {
    auto joined=services;
    joined.required_owner=[&](auto &bank,const auto &call,auto &state,const auto &observer) {
        if(&bank!=&m)throw std::runtime_error("Native TU1.5 startup bank owner mismatch");
        if(try_call(m,call,state,query_observe))return;
        if(call.instruction==0x2dc64cu && call.target==0x2e0fe2u && !call.tail) {
            if(word(m,state.gp[4])!=0x2e0101u)
                throw std::runtime_error("Native TU1.5 settings wrapper return mismatch");
            source_run_tu15_settings_runtime(bank,state,fp,joined,observer);return;
        }
        if(!services.required_owner)
            throw std::runtime_error("Native TU1.5 startup still requires its actual timer/heap owner");
        services.required_owner(bank,call,state,observer);
    };
    source_run_tu15_process_startup_records(m,entry,abi,fp,joined,vm,locks,platform,shutdown,
        observe,record_observe,shutdown_observe,data_owner());
}
} // namespace halo2
