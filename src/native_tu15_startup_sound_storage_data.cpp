#include "halo2/source_startup_sound_storage_tu15.hpp"
#include "halo2/source_render_object_startup_tu15.hpp"
#include "physics_world_runtime_internal.hpp"
#include <algorithm>
#include <array>
#include <stdexcept>

namespace halo2 {
namespace {
void cold(SourceRuntimeMemory& memory,std::uint32_t address,std::size_t bytes) {
    memory.mount(address,std::vector<std::byte>(bytes),SourceRuntimeMemory::Access::ReadWrite,
        "source startup45..50 loader-zeroed constructor destination");
}
template<std::size_t N>void literal(SourceRuntimeMemory& memory,std::uint32_t address,const std::array<std::uint8_t,N>& bytes) {
    memory.mount(address,std::as_bytes(std::span(bytes)),SourceRuntimeMemory::Access::ReadOnly,
        "source startup45..50 immutable sound policy or constructor name");
}
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t address) {
    const auto raw=memory.view(address,4);std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(raw[i])<<(8*i);
    return value;
}
constexpr std::array<std::pair<std::uint32_t,std::uint32_t>,7> bindings{{
    {0x52be94,0x1880},{0x52be98,0x1880},{0x4f9b5c,0x60cc},{0x4f9b58,0x244},
    {0x52d684,0x28},{0x50ebf8,0x22c},{0x51df5c,0x2b0}}};
NativeTu15StartupSoundStorage::Region region(const SourceRuntimeMemory& memory,unsigned index) {
    const auto [binding,bytes]=bindings[index];const auto source=word(memory,binding);
    if(!source)throw std::invalid_argument("Source startup46..50 lacks constructed native storage");
    memory.validate_mutable_view(source,bytes);const auto native=memory.native_address(source,bytes);
    if(native.value<=0xffffffffu)throw std::invalid_argument("Source startup46..50 backing is not native64");
    return {binding,source,bytes,native};
}
void validate(const SourceRuntimeMemory& memory,std::uint32_t entry) {
    switch(entry) {
    case 0x2131b0:
        (void)region(memory,0);(void)region(memory,1);
        if(word(memory,0x52be90)!=word(memory,0x52be98))throw std::invalid_argument("Source startup46 virtual publication differs");
        break;
    case 0x18b5c0:(void)region(memory,2);(void)region(memory,3);break;
    case 0x22c440:(void)region(memory,4);break;
    case 0x2293e0:(void)region(memory,5);break;
    case 0x1563a0:(void)region(memory,6);break;
    case 0x127ee0:break; // Its genuine driver owner remains a required call.
    default:throw std::invalid_argument("Source startup45..50 entry is not an initializer");
    }
}
SourcePhysicsWorldRuntimeServices services_for(SourcePhysicsPoolHostStorage& host,
    SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsWorldRuntimeServices& services) {
    auto joined=services;
    joined.required_owner=[&,fp](auto& bank,const auto& call,auto& abi,const auto& observe) {
        if(host.try_call(bank,call,abi))return;
        if(source_render_object_startup_tu15_owns_entry(call.target)) {
            source_run_render_object_startup_tu15(bank,call.target,abi,fp,services_for(host,fp,services),observe);return;
        }
        if(!services.required_owner)throw std::runtime_error("Source startup45..50 has an unowned native dependency");
        services.required_owner(bank,call,abi,observe);
    };
    return joined;
}
}
void mount_native_tu15_startup_sound_storage_data(SourceRuntimeMemory& memory) {
#include "native_tu15_startup_sound_storage_data.inc"
}
bool source_startup_sound_storage_tu15_is_initializer(std::uint32_t entry) {
    switch(entry) {case 0x127ee0:case 0x2131b0:case 0x18b5c0:case 0x22c440:case 0x2293e0:case 0x1563a0:return true;default:return false;}
}
void source_run_startup_sound_storage_with_native_storage(SourceRuntimeMemory& memory,std::uint32_t entry,
    SourcePhysicsWorldRuntimeAbi& abi,SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsWorldRuntimeServices& services,
    SourcePhysicsPoolHostStorage& host,const SourcePhysicsContactRecordObserver& observe) {
    if(!host.backs(memory)||!source_startup_sound_storage_tu15_is_initializer(entry))
        throw std::invalid_argument("Source startup45..50 initializer or native storage owner differs");
    source_run_startup_sound_storage_tu15(memory,entry,abi,fp,services_for(host,fp,services),observe);
    validate(memory,entry);
}
void source_prepare_native_tu15_sound_arena(SourceRuntimeMemory& memory,SourcePhysicsWorldRuntimeAbi& abi,
    SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsWorldRuntimeServices& services,
    SourcePhysicsPoolHostStorage& host,const SourcePhysicsContactRecordObserver& observe) {
    if(!host.backs(memory)||abi.gp[3]||word(memory,0x51db00)||word(memory,0x51db04)||word(memory,0x51db08))
        throw std::invalid_argument("Native sound arena prefix lacks its cold owner or source12001 EBX input");
    const auto joined=services_for(host,fp,services);
    physics_world_runtime_detail::WorldState state(memory,observe,joined,SourcePhysicsContactImage::TitleUpdate15);state.load(abi);
    auto instruction=[&](std::uint32_t pc){if(services.instruction)services.instruction(pc);};
    instruction(0x12008);state.push(0x12008,4);
    instruction(0x1200a);state.push(0x1200a,0x1000);
    instruction(0x1200f);state.push(0x1200f,0x23000);
    instruction(0x12014);state.push(0x12014,state.gp[3]);
    instruction(0x12015);state.push(0x12015,0x1201a);auto call=state.save();
    source_run_startup_sound_storage_tu15(memory,0x33b3ac,call,fp,joined,observe);state.load(call);
    if(!state.gp[0])throw std::runtime_error("Native sound arena allocation failed");
    memory.validate_mutable_view(state.gp[0],0x23000);
    instruction(0x1201a);physics_contact_runtime_detail::w(memory,observe,0x1201a,0x51db00,4,state.gp[0]);
    instruction(0x1201f);physics_contact_runtime_detail::w(memory,observe,0x1201f,0x51db04,4,state.gp[3]);
    abi=state.save();
}
NativeTu15StartupSoundStorage native_tu15_startup_sound_storage_views(const SourceRuntimeMemory& memory) {
    for(const auto entry:{0x2131b0u,0x18b5c0u,0x22c440u,0x2293e0u,0x1563a0u})validate(memory,entry);
    NativeTu15StartupSoundStorage result;
    for(unsigned i=0;i<bindings.size();++i)result.regions.push_back(region(memory,i));return result;
}
}
