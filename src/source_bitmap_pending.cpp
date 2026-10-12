#include "halo2/source_bitmap_pending.hpp"
#include "halo2/source_bitmap_image_binding.hpp"
#include "halo2/source_bitmap_pending_header.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <functional>
#include <stdexcept>
#include <vector>

namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
std::uint32_t get(std::span<const std::byte> data,std::size_t at){
    if(at>data.size() || data.size()-at<4)
        throw std::invalid_argument("source pending bitmap bank missing word");
    std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::uint32_t(std::to_integer<unsigned char>(data[at+i]))<<(8*i);
    return value;
}
std::uint16_t word(std::span<const std::byte> data,std::size_t at){
    return std::uint16_t(std::to_integer<unsigned char>(data[at])|
        std::uint16_t(std::to_integer<unsigned char>(data[at+1]))<<8);
}
void put(std::span<std::byte> data,std::size_t at,std::uint32_t value){
    for(unsigned i=0;i<4;++i)data[at+i]=std::byte((value>>(8*i))&255u);
}
void storage(const SourceBitmapManagerRegion &region,std::size_t required){
    if(!region.address || region.bytes.size()<required ||
       std::uint64_t(region.address)+region.bytes.size()>0x100000000ull)
        throw std::invalid_argument("source pending bitmap needs actual addressed storage");
}
void separate(const SourceBitmapManagerRegion &a,const SourceBitmapManagerRegion &b){
    if(std::uint64_t(a.address)<std::uint64_t(b.address)+b.bytes.size() &&
       std::uint64_t(b.address)<std::uint64_t(a.address)+a.bytes.size())
        throw std::invalid_argument("source pending bitmap addressed bank overlap is unproved");
    const std::less<const std::byte*> less;
    if(less(a.bytes.data(),b.bytes.data()+b.bytes.size()) &&
       less(b.bytes.data(),a.bytes.data()+a.bytes.size()))
        throw std::invalid_argument("source pending bitmap native storage alias is unproved");
}
SourceBitmapPendingHeaderMetadata metadata(std::span<const std::byte> owner){
    return {std::bit_cast<std::int16_t>(word(owner,4)),std::bit_cast<std::int16_t>(word(owner,6)),
        std::bit_cast<std::int16_t>(word(owner,0xc)),std::bit_cast<std::int16_t>(word(owner,0x14)),
        std::bit_cast<std::int8_t>(std::to_integer<std::uint8_t>(owner[8])),word(owner,0xa),word(owner,0xe)};
}
std::array<SourceBitmapManagerRegion,7> core(const SourceBitmapManagerBanks &banks){
    return {banks.manager,banks.image_pool,banks.physical_pool,banks.image_rows,
        banks.physical_rows,banks.image_bits,banks.physical_bits};
}
struct StagedBanks {
    std::array<std::vector<std::byte>,7> data;
    std::vector<std::vector<std::byte>> owner_data;
    std::vector<SourceBitmapManagerRegion> owners;
    SourceBitmapManagerBanks banks;
    explicit StagedBanks(const SourceBitmapManagerBanks &source){
        const auto input=core(source);std::array<SourceBitmapManagerRegion,7> regions;
        for(unsigned i=0;i<7;++i){data[i].assign(input[i].bytes.begin(),input[i].bytes.end());regions[i]={input[i].address,data[i]};}
        owner_data.reserve(source.owners.size());owners.reserve(source.owners.size());
        for(const auto &owner:source.owners){
            owner_data.emplace_back(owner.bytes.begin(),owner.bytes.end());owners.push_back({owner.address,owner_data.back()});
        }
        banks={regions[0],regions[1],regions[2],regions[3],regions[4],regions[5],regions[6],owners};
    }
    void commit(SourceBitmapManagerBanks &target) const{
        const auto output=core(target);
        for(unsigned i=0;i<7;++i)std::copy(data[i].begin(),data[i].end(),output[i].bytes.begin());
        for(unsigned i=0;i<owners.size();++i)
            std::copy(owner_data[i].begin(),owner_data[i].end(),target.owners[i].bytes.begin());
    }
};
std::size_t admit(const SourceBitmapManagerBanks &banks,SourceAsyncQueue &queue,
    const SourceBitmapPendingInput &input,const SourceBitmapPendingContext &context){
    if(input.partition>=3)throw std::invalid_argument("source pending bitmap partition is unproved");
    const auto original=core(banks);constexpr std::array<std::size_t,7> sizes{112,76,76,512*40,512*24,64,64};
    std::vector<SourceBitmapManagerRegion> all(original.begin(),original.end());
    for(unsigned i=0;i<7;++i)storage(original[i],sizes[i]);
    std::size_t owner_index=banks.owners.size();
    for(std::size_t i=0;i<banks.owners.size();++i){
        storage(banks.owners[i],116);all.push_back(banks.owners[i]);
        if(banks.owners[i].address==input.owner_address)owner_index=i;
    }
    if(owner_index==banks.owners.size())throw std::invalid_argument("source pending bitmap owner record absent");
    const SourceBitmapManagerRegion queue_region{context.queue_address,
        std::as_writable_bytes(std::span<SourceAsyncQueue>(&queue,1))};
    static_assert(sizeof(SourceAsyncQueue)==9616);
    storage(queue_region,9616);all.push_back(queue_region);
    for(std::size_t i=0;i<all.size();++i)for(std::size_t j=i+1;j<all.size();++j)separate(all[i],all[j]);
    if(get(banks.image_pool.bytes,0x20)!=512 || get(banks.image_pool.bytes,0x24)!=40 ||
       get(banks.image_pool.bytes,0x44)!=banks.image_rows.address || get(banks.image_pool.bytes,0x48)!=banks.image_bits.address)
        throw std::invalid_argument("source pending bitmap image-pool layout is unproved");
    if(get(banks.manager.bytes,0x34)!=12 || std::bit_cast<std::int32_t>(get(banks.manager.bytes,0x30))<0)
        throw std::invalid_argument("source pending bitmap physical capacity/shift is unproved");
    const auto bytes=std::uint64_t(get(banks.manager.bytes,0x30))<<12;
    const auto extent_end=std::uint64_t(context.physical_storage.address)+context.physical_storage.bytes;
    if(!context.physical_storage.address || extent_end>0x100000000ull ||
       context.physical_storage.address>context.physical_base ||
       std::uint64_t(context.physical_base)+bytes>extent_end)
        throw std::invalid_argument("source pending bitmap physical addresses lack actual represented storage");
    for(const auto &region:all){
        if(std::uint64_t(region.address)<extent_end &&
           std::uint64_t(context.physical_storage.address)<std::uint64_t(region.address)+region.bytes.size())
            throw std::invalid_argument("source pending bitmap metadata aliases its physical pixel bank");
    }
    const auto owner=banks.owners[owner_index].bytes;
    // Full pure metadata/header admission before any owned-bank mutation.
    (void)source_bitmap_pending_header(metadata(owner),input.partition,get(owner,0x40+input.partition*4));
    if(std::bit_cast<std::int32_t>(get(owner,0x34+input.partition*4))<=0)
        throw std::invalid_argument("source pending bitmap signed byte-count branch is unproved");
    return owner_index;
}
} // namespace
SourceBitmapPendingResult source_bitmap_begin_pending(SourceBitmapManagerBanks &banks,
    SourceAsyncQueue &queue,const SourceBitmapPendingInput &input,const SourceBitmapPendingContext &context){
    const auto owner_index=admit(banks,queue,input,context);
    StagedBanks staged(banks);auto owner=staged.banks.owners[owner_index].bytes;
    const auto requested=get(owner,0x34+input.partition*4);
    const auto reservation=source_bitmap_reserve(staged.banks,
        {std::bit_cast<std::int32_t>(requested),(input.wait || input.force)?2:5},context.manager);
    SourceBitmapPendingResult result;
    if(reservation.boundary==SourceBitmapManagerBoundary::NeedsWorkMutexAcquire){
        if(!context.work_mutex)throw std::invalid_argument("source pending bitmap actual work mutex absent");
        result.boundary=SourceBitmapPendingBoundary::NeedsWorkMutexAcquire;
        result.pending_delete=reservation.pending_delete;result.mutex=context.work_mutex;
        staged.commit(banks);return result;
    }
    if(reservation.handle==none){staged.commit(banks);return result;}
    const auto handle=reservation.handle,index=handle&65535u;
    const auto physical_offset=get(staged.banks.physical_rows.bytes,index*24+8);
    const auto address=std::uint32_t((std::uint64_t(physical_offset)<<12)+context.physical_base);
    SourceBitmapImageBindingBank image_bank{staged.banks.image_pool,staged.banks.image_rows,staged.banks.image_bits};
    if(source_bitmap_bind_image(image_bank,handle)!=handle)
        throw std::invalid_argument("source pending bitmap physical/image binding phase differs");
    auto image=staged.banks.image_rows.bytes.subspan(index*40,40);
    image[3]=std::byte{};image[2]&=std::byte{0xfe};put(image,4,input.partition);put(image,0x10,input.owner_address);
    put(owner,0x28+input.partition*4,handle);put(owner,0x40+input.partition*4,address);
    if(!input.partition)put(owner,0x54,address);
    const auto header=source_bitmap_pending_header(metadata(owner),input.partition,address);
    for(unsigned i=0;i<5;++i)put(image,0x14+i*4,header[i]);
    auto aligned=requested;if(aligned&511u)aligned=(aligned|511u)+1u;
    const auto priority=input.wait && !input.force?7:input.priority;
    result.read=SourceBitmapReadRequest{get(owner,0x1c+input.partition*4),aligned,address,0,
        staged.banks.image_rows.address+index*40+3,3,priority};
    result.request=source_bitmap_async_request(*result.read,context.files);
    if(!context.free_mutex)throw std::invalid_argument("source pending bitmap actual free mutex absent");
    (void)source_async_begin_request(queue,*result.request);image[3]=std::byte{};
    result.boundary=SourceBitmapPendingBoundary::NeedsFreeMutexAcquire;result.handle=handle;result.mutex=context.free_mutex;
    staged.commit(banks);return result;
}
} // namespace halo2
