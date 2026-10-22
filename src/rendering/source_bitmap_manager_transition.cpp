#include "halo2/source_bitmap_manager_transition.hpp"
#include <array>
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu,capacity=512;
std::uint32_t get(std::span<const std::byte> bytes,std::size_t at){
    if(at+4>bytes.size())throw std::invalid_argument("source bitmap manager bank missing word");
    std::uint32_t result=0;
    for(unsigned i=0;i<4;++i)result|=std::uint32_t(std::to_integer<unsigned char>(bytes[at+i]))<<(i*8);
    return result;
}
void put(std::span<std::byte> bytes,std::size_t at,std::uint32_t value){
    if(at+4>bytes.size())throw std::invalid_argument("source bitmap manager bank missing output word");
    for(unsigned i=0;i<4;++i)bytes[at+i]=std::byte((value>>(i*8))&255u);
}
std::uint16_t salt(std::span<const std::byte> row){
    return std::uint16_t(std::to_integer<unsigned char>(row[0])|
        std::uint16_t(std::to_integer<unsigned char>(row[1]))<<8);
}
void size(const SourceBitmapManagerRegion &region,std::size_t required){
    if(!region.address || region.bytes.size()<required || region.address>UINT32_MAX-required)
        throw std::invalid_argument("source bitmap manager needs actual addressed storage");
}
void pool(const SourceBitmapManagerRegion &header,const SourceBitmapManagerRegion &rows,
          const SourceBitmapManagerRegion &bits,std::uint32_t stride){
    size(header,0x4c);size(rows,capacity*stride);size(bits,64);
    if(get(header.bytes,0x20)!=capacity || get(header.bytes,0x24)!=stride ||
       get(header.bytes,0x44)!=rows.address || get(header.bytes,0x48)!=bits.address ||
       get(header.bytes,0x2c)!=0x64407440u)
        throw std::invalid_argument("source bitmap manager pool layout is unproved");
}
std::span<std::byte> image_row(const SourceBitmapManagerBanks &bank,std::uint32_t handle){
    pool(bank.image_pool,bank.image_rows,bank.image_bits,40);
    const auto index=handle&65535u;
    if(index>=capacity)throw std::invalid_argument("source bitmap image handle outside actual512-slot bank");
    return bank.image_rows.bytes.subspan(index*40,40);
}
bool occupied(const SourceBitmapManagerRegion &bits,unsigned index){
    return (std::to_integer<unsigned char>(bits.bytes[index>>3])&(1u<<(index&7)))!=0;
}
void release_input(const SourceBitmapManagerRegion &header,const SourceBitmapManagerRegion &rows,
                   const SourceBitmapManagerRegion &bits,std::uint32_t stride,std::uint32_t handle){
    pool(header,rows,bits,stride);
    const auto index=handle&65535u;
    if(index>=capacity || !occupied(bits,index) || salt(rows.bytes.subspan(index*stride,stride))!=(handle>>16) ||
       !salt(rows.bytes.subspan(index*stride,stride)))
        throw std::invalid_argument("source bitmap release needs an occupied matching salted slot");
    if((std::to_integer<unsigned char>(header.bytes[0x2a])&8u)!=0)
        throw std::invalid_argument("source bitmap debug-poison pool release is unproved");
    const auto high=get(header.bytes,0x38),count=get(header.bytes,0x3c),next=get(header.bytes,0x34);
    if(high>capacity || high<=index || !count || count>high || next>capacity)
        throw std::invalid_argument("source bitmap release pool counters are invalid");
}
void release(SourceBitmapManagerRegion &header,SourceBitmapManagerRegion &rows,
             SourceBitmapManagerRegion &bits,std::uint32_t stride,std::uint32_t handle){
    // Complete16BA40 after the explicit matching-slot/nonpoison admission.
    const auto index=handle&65535u;
    auto row=rows.bytes.subspan(index*stride,stride);
    const auto word=index>>5,mask=1u<<(index&31);
    put(bits.bytes,word*4,get(bits.bytes,word*4)&~mask);
    row[0]=row[1]=std::byte{};
    if(std::bit_cast<std::int32_t>(index)<std::bit_cast<std::int32_t>(get(header.bytes,0x34)))
        put(header.bytes,0x34,index);
    auto high=get(header.bytes,0x38);
    if(index+1==high){
        do{--high;}while(high && salt(rows.bytes.subspan((high-1)*stride,stride))==0);
        put(header.bytes,0x38,high);
    }
    put(header.bytes,0x3c,get(header.bytes,0x3c)-1u);
}
std::span<std::byte> owner(SourceBitmapManagerBanks &bank,std::uint32_t address){
    for(auto &region:bank.owners)if(region.address==address){size(region,116);return region.bytes;}
    throw std::invalid_argument("source bitmap deletion needs actual owner record storage");
}
bool busy(std::span<std::byte> row,const SourceBitmapManagerContext &context){
    const auto flags=std::to_integer<unsigned char>(row[2]);
    if(flags&1)return true;
    if(flags&2){
        if(!context.cache_busy_override)
            throw std::invalid_argument("source bitmap busy flags need actual4E6479");
        if(!*context.cache_busy_override)return true;
    }
    if(get(row,0xc)==none)return false;
    if(row[3]==std::byte{})return true;
    // Complete3FB710 for the proved texture header class, without a parent
    // surface/view resource. Its device pointer is unused when Lock is zero.
    const auto common=get(row,0x14);
    if((common&0x70000u)!=0x40000u)
        throw std::invalid_argument("source bitmap resource header class is unproved");
    if(common&0x780000u)return true;
    const auto lock=get(row,0x1c);
    if(lock){
        if(!context.fence)throw std::invalid_argument("source bitmap resource needs actual fence RAM");
        if(context.fence->queued-lock<context.fence->queued-context.fence->completed)return true;
    }
    put(row,0x1c,0);return false;
}
void manager_input(const SourceBitmapManagerBanks &bank,std::uint32_t handle){
    size(bank.manager,0x70);
    if(get(bank.manager.bytes,0x20)!=0x12d200 || get(bank.manager.bytes,0x24)!=0x12d1a0 ||
       get(bank.manager.bytes,0x28)!=0x12d160 || get(bank.manager.bytes,0x64)!=bank.physical_pool.address ||
       get(bank.manager.bytes,0x68)!=0x77656565)
        throw std::invalid_argument("source bitmap manager callback bank is unproved");
    release_input(bank.image_pool,bank.image_rows,bank.image_bits,40,handle);
    release_input(bank.physical_pool,bank.physical_rows,bank.physical_bits,24,handle);
    // Validate the admitted complete source list before any callback mutation.
    std::array<bool,capacity> seen{};
    auto at=get(bank.manager.bytes,0x3c),previous=none;unsigned count=0;
    while(at!=none){
        const auto index=at&65535u;
        if(index>=capacity || seen[index] || !occupied(bank.physical_bits,index))
            throw std::invalid_argument("source bitmap physical list is invalid or cyclic");
        seen[index]=true;const auto row=bank.physical_rows.bytes.subspan(index*24,24);
        if(salt(row)!=(at>>16) || get(row,0x10)!=previous)
            throw std::invalid_argument("source bitmap physical list generation or links differ");
        previous=at;at=get(row,0xc);++count;
    }
    if(previous!=get(bank.manager.bytes,0x40) || count!=get(bank.physical_pool.bytes,0x3c) ||
       !seen[handle&65535u])
        throw std::invalid_argument("source bitmap physical list membership differs");
}
}
std::uint8_t source_bitmap_image_state(const SourceBitmapManagerBanks &bank,std::uint32_t handle){
    const auto row=image_row(bank,handle);
    return (std::to_integer<unsigned char>(row[2])&1u)?0x10u:(get(row,4)?0x20u:0u);
}
bool source_bitmap_image_busy(SourceBitmapManagerBanks &bank,std::uint32_t handle,
    const SourceBitmapManagerContext &context){return busy(image_row(bank,handle),context);}
SourceBitmapManagerBoundary source_bitmap_image_delete(SourceBitmapManagerBanks &bank,
    std::uint32_t handle,const SourceBitmapManagerContext &context){
    release_input(bank.image_pool,bank.image_rows,bank.image_bits,40,handle);
    auto row=image_row(bank,handle);const auto address=get(row,0x10);
    if(address){
        if(busy(row,context))return SourceBitmapManagerBoundary::NeedsWorkMutexAcquire;
        const auto partition=get(row,4);
        if(partition>=3)throw std::invalid_argument("source bitmap owner partition is outside proved arrays");
        auto record=owner(bank,address);
        put(record,0x28+partition*4,none);put(record,0x40+partition*4,0);
        if(!partition){put(record,0x54,0);put(record,0x50,0);put(record,0x70,0);}
    }
    release(bank.image_pool,bank.image_rows,bank.image_bits,40,handle);
    return SourceBitmapManagerBoundary::Complete;
}
SourceBitmapManagerBoundary source_bitmap_manager_delete(SourceBitmapManagerBanks &bank,
    std::uint32_t handle,const SourceBitmapManagerContext &context){
    manager_input(bank,handle);
    const auto result=source_bitmap_image_delete(bank,handle,context);
    if(result!=SourceBitmapManagerBoundary::Complete)return result;
    const auto row=bank.physical_rows.bytes.subspan((handle&65535u)*24,24);
    const auto previous=get(row,0x10),next=get(row,0xc);
    if(previous!=none)put(bank.physical_rows.bytes,(previous&65535u)*24+0xc,next);
    else put(bank.manager.bytes,0x3c,next);
    if(next!=none)put(bank.physical_rows.bytes,(next&65535u)*24+0x10,previous);
    else put(bank.manager.bytes,0x40,previous);
    release(bank.physical_pool,bank.physical_rows,bank.physical_bits,24,handle);
    return SourceBitmapManagerBoundary::Complete;
}
} // namespace halo2
