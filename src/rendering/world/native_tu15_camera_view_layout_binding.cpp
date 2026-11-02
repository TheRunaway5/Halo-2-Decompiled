#include "halo2/native_tu15_camera_view_layout_binding.hpp"
#include "halo2/native_tu15_camera_view_layout.hpp"
#include <atomic>
#include <bit>
namespace halo2 {
namespace {
std::atomic<std::uint64_t> calls{};
std::uint32_t word(const SourcePhysicsContactMemory &memory,std::uint32_t address) {
    std::array<std::byte,4> raw{};memory.read(address,raw);std::uint32_t value{};
    for(unsigned n=0;n<4;++n)value|=std::to_integer<std::uint32_t>(raw[n])<<(8u*n);return value;
}
struct TileArguments final:NativeTu15CameraViewTileArguments {
    const SourcePhysicsContactMemory &memory;std::uint32_t stack;
    TileArguments(const SourcePhysicsContactMemory &m,std::uint32_t s):memory(m),stack(s){}
    std::int32_t count()const override{return std::bit_cast<std::int32_t>(word(memory,stack+4u));}
    std::int32_t split_mode()const override{return std::bit_cast<std::int32_t>(word(memory,stack+8u));}
    std::uint32_t tile_output()const override{return word(memory,stack+12u);}
};
struct RectangleArguments final:NativeTu15CameraViewRectangleArguments {
    const SourcePhysicsContactMemory &memory;std::uint32_t stack;
    RectangleArguments(const SourcePhysicsContactMemory &m,std::uint32_t s):memory(m),stack(s){}
    std::uint32_t grid()const override{return word(memory,stack+4u);}
    std::uint32_t span()const override{return word(memory,stack+8u);}
    std::uint32_t rectangle_output()const override{return word(memory,stack+12u);}
};
}
bool native_tu15_camera_view_layout_owns_entry(std::uint32_t entry){return entry==0x2f5e0u||entry==0x2f620u||entry==0x2f690u;}
void native_tu15_camera_view_layout_run(SourcePhysicsContactMemory &memory,std::uint32_t entry,
    std::uint32_t caller,SourcePhysicsWorldRuntimeAbi &abi,const SourcePhysicsContactRecordObserver &observe) {
    if(!((entry==0x2f5e0u&&caller==0x229b44u)||(entry==0x2f620u&&caller==0x229b5fu)||
         (entry==0x2f690u&&caller==0x229b7du)))throw std::invalid_argument("Unqualified native camera layout entry/caller");
    calls.fetch_add(1,std::memory_order_relaxed);
    if(entry==0x2f5e0u)native_tu15_camera_view_grid_publish(memory,std::bit_cast<std::int32_t>(abi.gp[6]),
        std::bit_cast<std::int32_t>(abi.gp[2]),abi.gp[7],observe);
    else if(entry==0x2f620u) {
        TileArguments argument(memory,abi.gp[4]);native_tu15_camera_view_tile(memory,
            std::bit_cast<std::int32_t>(abi.gp[0]),abi.gp[1],abi.gp[2],argument,observe);
    }else {
        RectangleArguments argument(memory,abi.gp[4]);native_tu15_camera_view_rectangles(memory,abi.gp[6],abi.gp[3],argument,observe);
    }
    (void)word(memory,abi.gp[4]);abi.gp[4]+=(entry==0x2f5e0u?4u:16u);
}
std::uint64_t native_tu15_camera_view_layout_calls(){return calls.load(std::memory_order_relaxed);}
}
