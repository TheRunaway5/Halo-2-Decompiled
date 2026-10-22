#include "halo2/native_tu15_debug_texture_startup.hpp"
#include <algorithm>
#include <stdexcept>

namespace halo2 {
namespace {
constexpr std::uint32_t handle_value=0xf9000501,header=0x4c2ac0,bitmap_cell=header+12;
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t address) {
    const auto bytes=memory.view(address,4);std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(8*i);
    return value;
}
void put(std::span<std::byte> bytes,std::size_t offset,std::uint32_t value,unsigned count=4) {
    for(unsigned i=0;i<count;++i)bytes[offset+i]=std::byte(value>>(i*8));
}
std::array<std::byte,116> expected_bitmap(std::uint32_t identity) {
    std::array<std::byte,116> result{};put(result,0,0x6269746d);put(result,4,256,2);put(result,6,256,2);
    put(result,8,1,1);put(result,12,9,2);put(result,14,0x911,2);put(result,0x50,identity);return result;
}
bool exact_bitmap(const SourceRuntimeMemory& memory,std::uint32_t bitmap,std::uint32_t identity) {
    const auto expected=expected_bitmap(identity);return std::ranges::equal(memory.view(bitmap,116),expected);
}
}
NativeTu15DebugTextureStartup::NativeTu15DebugTextureStartup(SourceRuntimeMemory& memory,NativeTu15GraphicsDevice& device,
    RendererDebugTextureFactory factory,SourcePhysicsPoolHostStorage& raw)
    :memory_(memory),device_(device),factory_(std::move(factory)),raw_(raw){}
bool NativeTu15DebugTextureStartup::initialized()const {
    if(std::this_thread::get_id()!=thread_||!resources_||!resources_->valid()||!device_.initialized()||!raw_.backs(memory_))return false;
    if(word(memory_,bitmap_cell)!=record_.source_bitmap||word(memory_,record_.source_bitmap+0x50)!=handle_value)return false;
    const auto allocations=raw_.ownership_checkpoint();
    return allocations.allocations.contains(record_.source_bitmap)&&
        memory_.native_address(record_.source_bitmap,116)==record_.bitmap;
}
bool NativeTu15DebugTextureStartup::try_call(SourcePhysicsContactMemory& bank,const SourcePhysicsContactRuntimeCall& call,SourcePhysicsWorldRuntimeAbi& abi) {
    if(call.target!=0x158d0)return false;
    const auto sp=abi.gp[4],bitmap=abi.gp[6];
    if(&bank!=&memory_||std::this_thread::get_id()!=thread_||call.tail||call.instruction!=0x1d09d||
        word(memory_,sp)!=0x1d0a2||resources_||!raw_.backs(memory_))
        throw std::invalid_argument("Native debugger texture has a foreign caller or duplicate cold owner");
    if((abi.gp[0]&255u)!=1||abi.gp[1]!=9||abi.gp[3]!=bitmap+0x50||
        word(memory_,sp+4)!=256||word(memory_,sp+8)!=256||word(memory_,sp+12)||word(memory_,sp+16))
        throw std::invalid_argument("Native debugger texture request differs from original1EFD0");
    if(!device_.initialized()||!factory_.valid()||factory_.device_address()!=device_.native_address())
        throw std::runtime_error("Native debugger texture lacks its live renderer owner");
    const auto allocations=raw_.ownership_checkpoint();const auto allocation=allocations.allocations.find(bitmap);
    if(allocation==allocations.allocations.end()||allocation->second<116||!exact_bitmap(memory_,bitmap,0))
        throw std::invalid_argument("Native debugger texture lacks its original constructed bitmap allocation");
    memory_.validate_mutable_view(bitmap,116);memory_.validate_mutable_view(header,4112);
    if(std::ranges::any_of(memory_.view(header,4112),[](auto value){return value!=std::byte{};}))
        throw std::invalid_argument("Native debugger constructor has an existing publication or uncleared pool");
    const auto native_bitmap=memory_.native_address(bitmap,116);
    if(native_bitmap.value<=0xffffffffu)throw std::invalid_argument("Native debugger bitmap lacks native64 backing");
    auto prepared=factory_.create();const auto& image=prepared.image();
    NativeTu15DebugTextureBinding record{bitmap,handle_value,0x11d29,0x70ff0ff,512,native_bitmap,image,prepared.components()};
    std::array<std::byte,4> identity{};put(identity,0,handle_value);memory_.write(bitmap+0x50,identity);
    resources_.emplace(std::move(prepared));record_=record;
    // Explicit native successful constructor ABI. No game1EFD0 BOOL is
    // supplied: original code computes that after this owned GPU allocation.
    abi.gp[0]=(abi.gp[0]&0xffffff00u)|1u;abi.gp[4]+=20;return true;
}
std::uint32_t NativeTu15DebugTextureStartup::handle()const {
    if(!initialized())throw std::runtime_error("Native debugger texture publication is unavailable or retired");
    return handle_value;
}
const NativeTu15DebugTextureBinding& NativeTu15DebugTextureStartup::resolve(std::uint32_t value)const {
    if(!initialized())throw std::runtime_error("Native debugger texture publication is unavailable or retired");
    if(value!=handle_value)throw std::invalid_argument("Native debugger identity belongs to another resource owner");
    return record_;
}
NativeRuntimeAddress NativeTu15DebugTextureStartup::native_address(std::uint32_t value)const {
    return NativeRuntimeAddress{std::uint64_t(reinterpret_cast<std::uintptr_t>(&resolve(value)))};
}
NativeRuntimeAddress NativeTu15DebugTextureStartup::initial_rows()const {
    (void)resolve(handle_value);
    if(word(memory_,header)!=1||word(memory_,header+4)!=0x10000||word(memory_,header+8)!=1||
        !exact_bitmap(memory_,record_.source_bitmap,handle_value))
        throw std::invalid_argument("Native debugger initial bitmap or publication differs");
    for(unsigned i=0;i<512;++i)if(word(memory_,header+16+i*8)!=0xffffffffu||word(memory_,header+20+i*8))
        throw std::invalid_argument("Native debugger initial row chain differs");
    const auto rows=memory_.native_address(header+16,4096);
    if(rows.value<=0xffffffffu)throw std::invalid_argument("Native debugger rows lack native64 backing");
    return rows;
}
bool NativeTu15DebugTextureStartup::content_initialized()const{(void)resolve(handle_value);return resources_->content_initialized();}
void NativeTu15DebugTextureStartup::release() {
    if(!initialized())throw std::invalid_argument("Native debugger texture cleanup lacks its owned publication");
    memory_.validate_mutable_view(record_.source_bitmap+0x50,4);
    memory_.write(record_.source_bitmap+0x50,std::array<std::byte,4>{});resources_.reset();record_={};
}
void mount_native_tu15_renderer_debug_storage_data(SourceRuntimeMemory& memory) {
    memory.mount(header,std::array<std::byte,4112>{},SourceRuntimeMemory::Access::ReadWrite,
        "source1EFD0 cold debugger bitmap binding, counters and512 row slots");
}
}
