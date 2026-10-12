#include "halo2/source_tu15_settings.hpp"
#include <fstream>

namespace halo2 {
namespace {
std::uint32_t word(std::span<const std::byte> b,std::size_t p) {
    std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(b[p+i])<<(8*i);
    return value;
}
std::uint32_t word(const SourceRuntimeMemory &m,std::uint32_t p) { return word(m.view(p,4),0); }
void put(SourceRuntimeMemory &m,std::uint32_t p,std::uint32_t value,
    const SourcePhysicsContactRecordObserver &observe,unsigned width=4) {
    const auto old=m.view(p,width);
    SourcePhysicsContactRecordStore change{0x2e101c,p,{old.begin(),old.end()},std::vector<std::byte>(width)};
    for(unsigned i=0;i<width;++i)change.written[i]=std::byte(value>>(8*i));
    m.write(p,change.written);if(observe)observe(change);
}
}
SourceTu15UserSettings::SourceTu15UserSettings(std::array<std::byte,96> bytes,std::string owner)
    :bytes_(bytes),owner_(std::move(owner)) {
    if(owner_.empty())throw std::invalid_argument("TU1.5 settings require an actual named resource");
}
SourceTu15UserSettings SourceTu15UserSettings::load(const std::filesystem::path &path) {
    std::ifstream in(path,std::ios::binary|std::ios::ate);
    if(!in || in.tellg()!=256)throw std::runtime_error("TU1.5 settings require the actual 256-byte EEPROM resource");
    std::array<std::byte,96> bytes;in.seekg(96);
    if(!in.read(reinterpret_cast<char *>(bytes.data()),bytes.size()))throw std::runtime_error("Truncated TU1.5 EEPROM UserSettings");
    return {bytes,path.string()+" UserSettings at96,96"};
}
bool SourceTu15UserSettings::checksum_valid() const {
    std::uint64_t checksum=0;std::uint32_t carries=0;
    for(unsigned i=0;i<96;i+=4) {
        checksum+=word(bytes_,i);
        if(checksum>0xffffffffu) {++carries;checksum&=0xffffffffu;}
    }
    checksum+=carries;if(checksum>0xffffffffu)++checksum;
    return std::uint32_t(checksum)==0xffffffffu;
}
std::uint32_t SourceTu15UserSettings::miscellaneous_flags() const { return word(bytes_,88); }
bool source_try_tu15_settings_query(SourceRuntimeMemory &m,
    const SourcePhysicsContactRuntimeCall &call,SourcePhysicsContactRuntimeAbi &abi,
    const SourceTu15UserSettings &settings,const SourcePhysicsContactRecordObserver &observe) {
    if(call.instruction!=0x2e101c || call.target!=0x80000018)return false;
    const auto sp=abi.gp[4],bp=abi.gp[5];m.view(sp,24);m.view(bp,28);
    if(!call.tail || word(m,sp)!=0x2e0ff9 || sp!=bp-24 || word(m,sp+4)!=17 || word(m,sp+16)!=4)
        throw std::runtime_error("TU1.5 settings query requires its actual wrapper frame");
    for(unsigned i=1;i<=5;++i)
        if(word(m,sp+i*4)!=word(m,bp+4+i*4))throw std::runtime_error("TU1.5 settings query arguments lost their source owner");
    const auto type=word(m,sp+8),value=word(m,sp+12),length=word(m,sp+20);
    m.mutable_view(length,4);
    const bool valid=settings.checksum_valid();
    if(valid) {m.mutable_view(type,4);m.mutable_view(value,4);}
    put(m,length,4,observe);
    if(valid) {
        put(m,type,4,observe);
        for(unsigned i=0;i<4;++i)put(m,value+i,0,observe,1);
        for(unsigned i=0;i<4;++i)put(m,value+i,settings.miscellaneous_flags()>>(8*i),observe,1);
    }
    // Actual branches of the pinned ExQueryNonVolatileSetting expression.
    // These are query results, never supplied settings-success defaults.
    abi.gp[0]=valid?0u:0xc000009cu;abi.gp[4]=sp+24;
    return true;
}
void source_run_tu15_process_startup_with_settings(SourceRuntimeMemory &m,
    std::uint32_t entry,SourcePhysicsContactRuntimeAbi &abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsContactRuntimeServices &services,SourceRuntimeHostVirtualMemory &vm,
    PhysicsPlatformLocks &locks,const PhysicsPlatformLockServices &platform,
    SourceProcessShutdownRegistry &shutdown,const SourceTu15UserSettings &settings,
    const SourcePhysicsContactRecordObserver &observe,const SourcePhysicsContactRecordObserver &record_observe,
    const SourceProcessShutdownObserver &shutdown_observe,const SourcePhysicsContactRecordObserver &query_observe,
    const SourceProcessHeapDataImportOwner &data_owner) {
    auto joined=services;
    joined.required_owner=[&](auto &bank,const auto &call,auto &state,const auto &observer) {
        if(&bank!=&m)throw std::runtime_error("TU1.5 settings startup bank owner mismatch");
        if(source_try_tu15_settings_query(m,call,state,settings,query_observe))return;
        if(call.instruction==0x2dc64c && call.target==0x2e0fe2 && !call.tail) {
            if(word(m,state.gp[4])!=0x2e0101)throw std::runtime_error("TU1.5 settings wrapper return mismatch");
            source_run_tu15_settings_runtime(bank,state,fp,joined,observer);return;
        }
        if(!services.required_owner)throw std::runtime_error("Required TU1.5 settings status/timer owner unavailable");
        services.required_owner(bank,call,state,observer);
    };
    source_run_tu15_process_startup_records(m,entry,abi,fp,joined,vm,locks,platform,shutdown,
        observe,record_observe,shutdown_observe,data_owner);
}
} // namespace halo2
