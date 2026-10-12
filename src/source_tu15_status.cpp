#include "halo2/source_tu15_status.hpp"
#include <algorithm>
#include <fstream>

namespace halo2 { namespace {
std::uint64_t fingerprint(std::span<const std::byte> b) {
    std::uint64_t h=14695981039346656037ull;
    for(auto v:b)h=(h^std::to_integer<unsigned>(v))*1099511628211ull;return h;
}
std::vector<std::byte> read(const std::filesystem::path &p,std::size_t size) {
    std::ifstream in(p,std::ios::binary|std::ios::ate);
    if(!in||in.tellg()!=size)throw std::runtime_error("Owned kernel301 resource extent mismatch");
    std::vector<std::byte> b(size);in.seekg(0);
    if(!in.read(reinterpret_cast<char *>(b.data()),size))throw std::runtime_error("Truncated kernel301 resource");return b;
}
std::uint32_t word(const SourceRuntimeMemory &m,std::uint32_t p) {
    const auto b=m.view(p,4);std::uint32_t v=0;
    for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);return v;
}
}
SourceTu15StatusTables::SourceTu15StatusTables(std::vector<std::byte> ranges,std::vector<std::byte> values,std::string owner)
    :ranges_(std::move(ranges)),values_(std::move(values)),owner_(std::move(owner)) {
    if(owner_.empty()||ranges_.size()!=828||values_.size()!=1530||
        fingerprint(ranges_)!=0x778b67209d80f4ddull||fingerprint(values_)!=0xc3373855af7bdf33ull)
        throw std::runtime_error("Kernel301 requires the exact owned modified Complex4627 tables");
}
SourceTu15StatusTables SourceTu15StatusTables::load(const std::filesystem::path &p) {
    return {read(p/"ranges.bin",828),read(p/"values.bin",1530),p.string()+" owned kernel301 immutable tables"};
}
void SourceTu15StatusTables::mount(SourceRuntimeMemory &m) const {
    m.mount(0x80011f88,ranges_,SourceRuntimeMemory::Access::ReadOnly,owner_+" ranges");
    m.mount(0x80011988,values_,SourceRuntimeMemory::Access::ReadOnly,owner_+" values");
}
void SourceTu15StatusTables::validate(const SourceRuntimeMemory &m) const {
    if(!std::ranges::equal(m.view(0x80011f88,ranges_.size()),ranges_)||
        !std::ranges::equal(m.view(0x80011988,values_.size()),values_))
        throw std::runtime_error("Kernel301 table backing lost its actual resource owner");
}
bool source_try_tu15_status_conversion(SourceRuntimeMemory &m,const SourcePhysicsContactRuntimeCall &call,
    SourcePhysicsContactRuntimeAbi &abi,SourcePhysicsFactoryFloatingPoint fp,const SourceTu15StatusTables &tables,
    const std::function<void(std::uint32_t)> &instruction,const SourcePhysicsContactRecordObserver &observe) {
    if(call.instruction!=0x2e0ffa||call.target!=0x8000012d)return false;
    const auto sp=abi.gp[4];m.view(sp,8);
    if(call.tail||word(m,sp)!=0x2e1000||word(m,sp+4)!=abi.gp[0])
        throw std::runtime_error("TU1.5 status converter requires its actual complete settings frame");
    tables.validate(m);m.mutable_view(sp-12,12);
    SourcePhysicsContactRuntimeServices kernel;kernel.instruction=instruction;
    source_run_tu15_status_runtime(m,abi,fp,kernel,observe);return true;
}
void source_run_tu15_process_startup_with_status(SourceRuntimeMemory &m,std::uint32_t entry,
    SourcePhysicsContactRuntimeAbi &abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsContactRuntimeServices &services,SourceRuntimeHostVirtualMemory &vm,
    PhysicsPlatformLocks &locks,const PhysicsPlatformLockServices &platform,SourceProcessShutdownRegistry &shutdown,
    const SourceTu15UserSettings &settings,const SourceTu15StatusTables &tables,
    const SourcePhysicsContactRecordObserver &observe,const SourcePhysicsContactRecordObserver &record_observe,
    const SourceProcessShutdownObserver &shutdown_observe,const SourcePhysicsContactRecordObserver &query_observe,
    const SourceProcessHeapDataImportOwner &data_owner,const std::function<void(std::uint32_t)> &kernel_instruction,
    const SourcePhysicsContactRecordObserver &kernel_observe) {
    tables.validate(m);auto joined=services;
    joined.required_owner=[&](auto &bank,const auto &call,auto &state,const auto &observer) {
        if(&bank!=&m)throw std::runtime_error("TU1.5 status startup bank owner mismatch");
        if(source_try_tu15_status_conversion(m,call,state,fp,tables,kernel_instruction,kernel_observe))return;
        if(!services.required_owner)throw std::runtime_error("Required TU1.5 timer/startup owner unavailable");
        services.required_owner(bank,call,state,observer);
    };
    source_run_tu15_process_startup_with_settings(m,entry,abi,fp,joined,vm,locks,platform,shutdown,settings,
        observe,record_observe,shutdown_observe,query_observe,data_owner);
}
}
