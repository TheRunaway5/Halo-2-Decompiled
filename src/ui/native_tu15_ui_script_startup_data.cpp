#include "halo2/source_ui_script_startup_tu15.hpp"
#include <algorithm>
#include <array>
#include <stdexcept>
namespace halo2 {
namespace {
void cold(SourceRuntimeMemory& memory,std::uint32_t address,std::size_t bytes) {
    memory.mount(address,std::vector<std::byte>(bytes),SourceRuntimeMemory::Access::ReadWrite,
        "source startup51..58 loader-zeroed constructor destination");
}
template<std::size_t N>void literal(SourceRuntimeMemory& memory,std::uint32_t address,const std::array<std::uint8_t,N>& bytes) {
    memory.mount(address,std::as_bytes(std::span(bytes)),SourceRuntimeMemory::Access::ReadOnly,
        "source startup51..58 immutable constructor metadata");
}
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t address) {
    const auto raw=memory.view(address,4);std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(raw[i])<<(8*i);return value;
}
struct UiObject {std::uint32_t object,vtable;};
constexpr std::array<UiObject,19> ui_objects{{
    {0x55c454,0x465c14},
    {0x55c4bc,0x465c40},{0x55c4fc,0x465c40},{0x55c53c,0x465c40},{0x55c57c,0x465c40},{0x55c5bc,0x465c40},
    {0x55c5fc,0x465ca0},{0x55c634,0x465ca0},{0x55c66c,0x465ca0},{0x55c6a4,0x465ca0},{0x55c6dc,0x465ca0},
    {0x55c714,0x465ca0},{0x55c74c,0x465ca0},{0x55c784,0x465ca0},{0x55c7bc,0x465ca0},{0x55c7f4,0x465ca0},
    {0x55c82c,0x465c74},{0x55c870,0x465ccc},{0x55ca38,0x465ca0}}};
void validate_ui_objects(const SourceRuntimeMemory& memory) {
    for(const auto& object:ui_objects)if(word(memory,object.object)!=object.vtable||!memory.is_read_only(object.vtable,4))
        throw std::invalid_argument("Original prior UI static constructor is absent or its immutable dispatch differs");
}
struct Binding {std::uint32_t cell,bytes;bool direct;};
constexpr std::array<Binding,22> bindings{{
    {0x55c428,0x1268,true},{0x55d770,0x3218,true},{0x4d61e0,0x3ea4,true},
    {0x56d370,0xc2c,true},{0x4fad90,0x22,true},{0x4f3220,0xa40,true},
    {0x51df98,3,false},{0x50eecc,0x13a8,false},{0x50eed0,0xc0,false},{0x52be3c,0xbc8,false},
    {0x51df4c,0x1e4,false},{0x4f6498,0x8330,false},{0x4f6494,0x8000,false},{0x50ed20,0x14,false},
    {0x51df50,0x24,false},{0x505e6c,0x124c,false},{0x5020b0,0x294,false},{0x5020ac,0x65c,false},
    {0x505e5c,0x51e74,false},{0x505e58,0x61cc,false},{0x5021fc,0xf0,false},{0x4ecbf4,4,false}}};
NativeTu15UiScriptStartupStorage::Region region(const SourceRuntimeMemory& memory,Binding binding) {
    const auto source=binding.direct?binding.cell:word(memory,binding.cell);
    if(!source)throw std::invalid_argument("Source startup51..58 storage binding is absent");
    memory.validate_mutable_view(source,binding.bytes);const auto native=memory.native_address(source,binding.bytes);
    if(native.value<=0xffffffffu)throw std::invalid_argument("Source startup51..58 backing is not native64");
    return {binding.cell,source,binding.bytes,native};
}
void validate_constructor_storage(const SourceRuntimeMemory& memory,std::uint32_t entry) {
    std::size_t first=0,count=0;
    switch(entry) {
    case 0x14a1ff:first=0;count=5;validate_ui_objects(memory);break;
    case 0x1410a7:first=5;count=9;break;
    case 0x13e9e0:first=14;count=1;break;
    case 0x2129d0:first=15;count=5;break;
    case 0x2035b0:first=20;count=1;break;
    case 0xd7320:first=21;count=1;break;
    case 0x31d400:return;
    default:throw std::invalid_argument("Source startup51..58 entry is not an initializer");
    }
    for(std::size_t i=first;i<first+count;++i)(void)region(memory,bindings[i]);
    if(entry==0x1410a7&&(word(memory,0x4f6494)&15u))
        throw std::invalid_argument("Original aligned UI arena allocation lost its16-byte alignment");
    if(entry==0x2129d0) {
        const auto definition=word(memory,0x505e6c);
        if(word(memory,definition+0x20)!=0x9000||word(memory,definition+0x24)!=0x14||
            word(memory,definition+0x44)||word(memory,definition+0x48)!=definition+0x4c)
            throw std::invalid_argument("Original script-node cold header/bitmap or deferred payload differs");
    }
}
}
void mount_native_tu15_ui_script_startup_data(SourceRuntimeMemory& memory) {
#include "ui/native_tu15_ui_script_startup_data.inc"
}
bool source_ui_script_startup_tu15_is_initializer(std::uint32_t entry) {
    switch(entry){case 0x14a1ff:case 0x1410a7:case 0x31d400:case 0x13e9e0:case 0x2129d0:case 0x2035b0:case 0xd7320:return true;default:return false;}
}
void source_run_ui_script_startup_prior_constructor(SourceRuntimeMemory& memory,
    SourcePhysicsWorldRuntimeAbi& abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices& services,const SourcePhysicsContactRecordObserver& observer) {
    for(const auto& object:ui_objects)if(word(memory,object.object))
        throw std::invalid_argument("Prior original UI static constructor would replace a constructed object");
    source_run_ui_script_startup_tu15(memory,0x381390,abi,fp,services,observer);validate_ui_objects(memory);
}
void source_run_ui_script_startup_with_native_storage(SourceRuntimeMemory& memory,
    std::uint32_t entry,SourcePhysicsWorldRuntimeAbi& abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices& services,SourcePhysicsPoolHostStorage& host,
    NativeTu15StorageFormatting& formatting,const SourcePhysicsContactRecordObserver& observer) {
    if(!host.backs(memory)||!source_ui_script_startup_tu15_is_initializer(entry))
        throw std::invalid_argument("Source startup51..58 storage owner or initializer differs");
    if(entry==0x14a1ff)validate_ui_objects(memory);
    if(entry==0x2129d0&&word(memory,0x474c20)!=0xffffffffu)
        throw std::invalid_argument("Cold script initializer requires the existing no-scenario startup owner");
    auto joined=services;joined.required_owner=[&](auto& bank,const auto& call,auto& state,const auto& observe){
        if(host.try_call(bank,call,state)||formatting.try_call(bank,call,state))return;
        if(!services.required_owner)throw std::runtime_error("Source startup51..58 has an unowned dependency");
        services.required_owner(bank,call,state,observe);
    };
    source_run_ui_script_startup_tu15(memory,entry,abi,fp,joined,observer);validate_constructor_storage(memory,entry);
}
NativeTu15UiScriptStartupStorage native_tu15_ui_script_startup_views(const SourceRuntimeMemory& memory) {
    for(const auto entry:std::array<std::uint32_t,6>{0x14a1ff,0x1410a7,0x13e9e0,0x2129d0,0x2035b0,0xd7320})validate_constructor_storage(memory,entry);
    NativeTu15UiScriptStartupStorage result;result.regions.reserve(bindings.size());
    for(const auto binding:bindings)result.regions.push_back(region(memory,binding));return result;
}
}
