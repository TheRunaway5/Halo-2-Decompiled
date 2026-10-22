#include "halo2/source_bitmap_initialization.hpp"
#include <algorithm>
#include <bit>
#include <stdexcept>
#include <string_view>

namespace halo2 {
namespace {
void word(std::span<std::byte> out,std::size_t at,std::uint32_t value){
    for(unsigned i=0;i<4;++i)out[at+i]=std::byte((value>>(i*8))&255u);
}
void pool(std::span<std::byte> out,std::uint32_t address,std::string_view name,
          std::uint32_t count,std::uint32_t stride,std::uint32_t allocator,bool activate){
    std::fill_n(out.begin(),0x4c,std::byte{});
    for(std::size_t i=0;i<std::min(name.size(),std::size_t(31));++i)
        out[i]=std::byte(static_cast<unsigned char>(name[i]));
    word(out,0x20,count);word(out,0x24,stride);word(out,0x2c,0x64407440);
    word(out,0x30,allocator);word(out,0x44,address+0x4c);
    const auto occupied=0x4cu+count*stride;
    word(out,0x48,address+occupied);
    std::fill_n(out.begin()+occupied,((count+31u)>>5)*4u,std::byte{});
    if(activate){
        out[0x29]=std::byte{1};
        //16B7A0 derives the first allocation salt from the first two name bytes.
        out[0x40]=out[0];out[0x41]=out[1]|std::byte{0x80};
        for(std::uint32_t i=0;i<count;++i){
            out[0x4c+i*stride]=std::byte{};out[0x4d+i*stride]=std::byte{};
        }
    }
}
}
std::size_t source_bitmap_storage_bytes(SourceBitmapStorageKind kind){
    switch(kind){
    case SourceBitmapStorageKind::Image:return 0x508c;
    case SourceBitmapStorageKind::PredictedImage:return 0x6a8;
    case SourceBitmapStorageKind::PhysicalManager:return 0x30fc;
    }
    throw std::invalid_argument("unknown source bitmap storage kind");
}
void source_bitmap_initialize_storage(SourceBitmapStorageKind kind,
    const SourceBitmapStorageInput &input,std::span<std::byte> out){
    const auto required=source_bitmap_storage_bytes(kind);
    if(out.size()<required || !input.address || !input.allocator_interface ||
       input.address>UINT32_MAX-required)
        throw std::invalid_argument("source bitmap constructor needs allocated storage and addresses");
    switch(kind){
    case SourceBitmapStorageKind::Image:
        pool(out,input.address,"xbox texture",512,40,input.allocator_interface,false);
        out[0x2a]=std::byte{4};return;
    case SourceBitmapStorageKind::PredictedImage:
        pool(out,input.address,"xbox predicted texture",200,8,input.allocator_interface,false);
        out[0x2a]=std::byte{4};return;
    case SourceBitmapStorageKind::PhysicalManager:
        if(!input.pool_allocator_interface)
            throw std::invalid_argument("source physical manager needs the actual pool allocator interface");
        pool(out.subspan(0x70),input.address+0x70,"xbox texture cache",512,24,
             input.pool_allocator_interface,true);
        std::fill_n(out.begin(),0x70,std::byte{});
        constexpr std::string_view name="xbox texture cache";
        for(std::size_t i=0;i<name.size();++i)out[i]=std::byte(static_cast<unsigned char>(name[i]));
        word(out,0x20,0x12d200);word(out,0x24,0x12d1a0);word(out,0x28,0x12d160);
        word(out,0x2c,2);word(out,0x34,12);word(out,0x38,1);
        word(out,0x3c,UINT32_MAX);word(out,0x40,UINT32_MAX);
        for(unsigned at=0x44;at<=0x60;at+=4)word(out,at,0x7fffffff);
        word(out,0x64,input.address+0x70);word(out,0x68,0x77656565);
        word(out,0x6c,input.allocator_interface);return;
    }
}
SourceBitmapPhysicalReservation source_bitmap_reserve_stage(
    std::uint32_t lower,std::uint32_t upper){
    const auto difference=upper-lower;
    const auto corrected=difference+((difference&0x80000000u)?0xfffu:0u);
    const auto pages=std::bit_cast<std::int32_t>(corrected)>>12;
    const auto bytes=std::bit_cast<std::uint32_t>(pages)<<12;
    const auto remaining=upper-bytes;
    SourceBitmapPhysicalReservation result{upper,0,bytes,pages,false};
    if(std::bit_cast<std::int32_t>(remaining)>=std::bit_cast<std::int32_t>(lower)){
        result.upper_after=remaining;
        if(remaining){result.physical_base=remaining|0x80000000u;result.calls_protection_wrapper=true;}
    }
    return result;
}
} // namespace halo2
