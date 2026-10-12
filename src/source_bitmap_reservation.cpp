#include "halo2/source_bitmap_reservation.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu,capacity=512;
std::uint32_t get(std::span<const std::byte> data,std::size_t at){
    if(at>data.size() || data.size()-at<4)
        throw std::invalid_argument("source reservation bank missing word");
    std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::uint32_t(std::to_integer<unsigned char>(data[at+i]))<<(8*i);
    return value;
}
void put(std::span<std::byte> data,std::size_t at,std::uint32_t value){
    if(at>data.size() || data.size()-at<4)
        throw std::invalid_argument("source reservation bank missing output word");
    for(unsigned i=0;i<4;++i)data[at+i]=std::byte((value>>(8*i))&255u);
}
std::uint16_t word(std::span<const std::byte> data,std::size_t at){
    return std::uint16_t(std::to_integer<unsigned char>(data[at])|
        std::uint16_t(std::to_integer<unsigned char>(data[at+1]))<<8);
}
void put_word(std::span<std::byte> data,std::size_t at,std::uint16_t value){
    data[at]=std::byte(value&255u);data[at+1]=std::byte(value>>8);
}
std::int32_t signed_word(std::uint32_t value){return std::bit_cast<std::int32_t>(value);}
std::int32_t add(std::int32_t a,std::int32_t b){
    return signed_word(std::bit_cast<std::uint32_t>(a)+std::bit_cast<std::uint32_t>(b));
}
void storage(const SourceBitmapManagerRegion &region,std::size_t required){
    if(!region.address || region.bytes.size()<required ||
       std::uint64_t(region.address)+region.bytes.size()>0x100000000ull)
        throw std::invalid_argument("source reservation needs actual addressed storage");
}
bool occupied(const SourceBitmapManagerRegion &bits,unsigned index){
    return (std::to_integer<unsigned char>(bits.bytes[index>>3])&(1u<<(index&7)))!=0;
}
void pool(const SourceBitmapManagerRegion &header,const SourceBitmapManagerRegion &rows,
          const SourceBitmapManagerRegion &bits,std::uint32_t stride){
    storage(header,0x4c);storage(rows,capacity*stride);storage(bits,64);
    if(get(header.bytes,0x20)!=capacity || get(header.bytes,0x24)!=stride ||
       get(header.bytes,0x44)!=rows.address || get(header.bytes,0x48)!=bits.address ||
       get(header.bytes,0x2c)!=0x64407440u ||
       (std::to_integer<unsigned char>(header.bytes[0x2a])&8u))
        throw std::invalid_argument("source reservation pool layout or debug-poison branch is unproved");
    const auto high=get(header.bytes,0x38),next=get(header.bytes,0x34),count=get(header.bytes,0x3c);
    if(high>capacity || next>capacity || count>high)
        throw std::invalid_argument("source reservation pool counters are invalid");
    unsigned actual_count=0,actual_high=0;
    for(unsigned i=0;i<capacity;++i){
        const auto salt=word(rows.bytes,i*stride);
        if(occupied(bits,i)){
            if(!salt)throw std::invalid_argument("source reservation occupied slot lacks its salt");
            ++actual_count;actual_high=i+1;
        }else if(salt)throw std::invalid_argument("source reservation free slot retains an occupied salt");
    }
    if(count!=actual_count || high!=actual_high)
        throw std::invalid_argument("source reservation occupancy counters differ from actual slots");
}
void admit(const SourceBitmapManagerBanks &bank,const SourceBitmapReservationInput &input){
    if(input.bytes<=0 || (input.age!=2 && input.age!=5))
        throw std::invalid_argument("source reservation request/age branch is unproved");
    storage(bank.manager,0x70);
    if(get(bank.manager.bytes,0x2c)!=2 || get(bank.manager.bytes,0x34)!=12 ||
       signed_word(get(bank.manager.bytes,0x30))<0 ||
       get(bank.manager.bytes,0x20)!=0x12d200 || get(bank.manager.bytes,0x24)!=0x12d1a0 ||
       get(bank.manager.bytes,0x28)!=0x12d160 || get(bank.manager.bytes,0x64)!=bank.physical_pool.address ||
       get(bank.manager.bytes,0x68)!=0x77656565)
        throw std::invalid_argument("source reservation texture-manager branch is unproved");
    pool(bank.physical_pool,bank.physical_rows,bank.physical_bits,24);
    pool(bank.image_pool,bank.image_rows,bank.image_bits,40);
    if(!std::equal(bank.physical_bits.bytes.begin(),bank.physical_bits.bytes.begin()+64,bank.image_bits.bytes.begin()))
        throw std::invalid_argument("source reservation requires paired image/physical occupancy");
    const auto next_salt=word(bank.physical_pool.bytes,0x40);
    if(next_salt<0x8000u || next_salt==0xffffu)
        throw std::invalid_argument("source reservation next salt lacks its source producer");
    std::array<bool,capacity> seen{};
    auto current=get(bank.manager.bytes,0x3c),previous=none;
    std::int64_t end=0;unsigned count=0;
    while(current!=none){
        const auto index=current&65535u;
        if(index>=capacity || seen[index] || !occupied(bank.physical_bits,index))
            throw std::invalid_argument("source reservation address list is invalid or cyclic");
        seen[index]=true;
        const auto row=bank.physical_rows.bytes.subspan(index*24,24);
        if(word(row,0)!=(current>>16) || word(bank.image_rows.bytes,index*40)!=(current>>16) ||
           get(row,0x10)!=previous)
            throw std::invalid_argument("source reservation list generation or paired salt differs");
        const auto offset=signed_word(get(row,8)),pages=signed_word(get(row,4));
        if(offset<end || pages<=0 || std::int64_t(offset)+pages>signed_word(get(bank.manager.bytes,0x30)))
            throw std::invalid_argument("source reservation needs ordered nonoverlapping in-capacity blocks");
        end=std::int64_t(offset)+pages;previous=current;current=get(row,0xc);++count;
    }
    if(previous!=get(bank.manager.bytes,0x40) || count!=get(bank.physical_pool.bytes,0x3c))
        throw std::invalid_argument("source reservation complete address-list membership differs");
}
std::uint32_t allocate(SourceBitmapManagerBanks &bank){
    auto &header=bank.physical_pool;
    auto index=get(header.bytes,0x34);const auto high=get(header.bytes,0x38);
    while(index<high && occupied(bank.physical_bits,index))++index;
    if(index==high){if(high>=capacity)return none;index=high;}
    // Source16B840 uses high-water fallback when first-free exceeds it too.
    else if(index>high){if(high>=capacity)return none;index=high;}
    const auto at=(index>>5)*4,mask=1u<<(index&31);
    put(bank.physical_bits.bytes,at,get(bank.physical_bits.bytes,at)|mask);
    put(header.bytes,0x3c,get(header.bytes,0x3c)+1u);
    put(header.bytes,0x34,index+1u);
    if(high<=index)put(header.bytes,0x38,index+1u);
    auto row=bank.physical_rows.bytes.subspan(index*24,24);
    std::fill(row.begin(),row.end(),std::byte{});
    const auto salt=word(header.bytes,0x40);put_word(row,0,salt);
    auto next=std::uint16_t(salt+1u);if(next==0xffffu)next=0x8000u;
    put_word(header.bytes,0x40,next);
    return std::uint32_t(salt)<<16|index;
}
struct Candidate {std::uint32_t previous{none},time{};std::int32_t offset{},pages{};};
bool better(const Candidate &candidate,const Candidate &best,std::uint32_t now){
    const auto candidate_age=now-candidate.time,best_age=now-best.time;
    const auto left=std::bit_cast<std::uint32_t>(best.pages)*candidate_age;
    const auto right=std::bit_cast<std::uint32_t>(candidate.pages)*best_age;
    return signed_word(left)>signed_word(right);
}
}
SourceBitmapReservationResult source_bitmap_reserve(SourceBitmapManagerBanks &bank,
    const SourceBitmapReservationInput &input,const SourceBitmapManagerContext &context){
    admit(bank,input);
    const auto pages=input.bytes/4096+(input.bytes%4096!=0);
    const auto limit_at=0x44+unsigned(input.age)*4;
    if(pages>=signed_word(get(bank.manager.bytes,limit_at)))return {};
    const auto capacity_pages=signed_word(get(bank.manager.bytes,0x30));
    const auto now=get(bank.manager.bytes,0x38);
    std::array<Candidate,256> candidates{};Candidate best{};
    bool have_best=false;unsigned head=0,tail=0;
    std::int32_t offset=0;auto previous=none,current=get(bank.manager.bytes,0x3c),oldest=none;
    std::uint32_t oldest_time=0;
    while(offset<capacity_pages){
        const auto next_tail=(tail+1)&255u;
        if(next_tail!=head){candidates[tail]={previous,0,offset,0};tail=next_tail;}
        std::int32_t segment_pages;std::uint32_t segment_time=0;bool segment_busy=false;
        if(current==none){segment_pages=capacity_pages-offset;offset=capacity_pages;}
        else{
            const auto row=bank.physical_rows.bytes.subspan((current&65535u)*24,24);
            const auto block_offset=signed_word(get(row,8));
            if(offset==block_offset){
                segment_pages=signed_word(get(row,4));segment_time=get(row,0x14);
                segment_busy=source_bitmap_image_busy(bank,current,context);
                if(segment_time+std::uint32_t(input.age)>=now)segment_busy=true;
                else if(!segment_busy && (oldest==none || segment_time<oldest_time)){
                    oldest=current;oldest_time=segment_time;
                }
                previous=current;current=get(row,0xc);offset=add(block_offset,segment_pages);
            }else{segment_pages=block_offset-offset;offset=block_offset;}
        }
        if(segment_busy)head=tail;
        else for(auto i=head;i!=tail;i=(i+1)&255u){
            auto &candidate=candidates[i];candidate.time=std::max(candidate.time,segment_time);
            candidate.pages=add(candidate.pages,segment_pages);
            if(candidate.pages>=pages){
                if(!have_best || better(candidate,best,now)){best=candidate;have_best=true;}
                head=(head+1)&255u;
            }
        }
    }
    if(have_best){
        // The source eviction pass traverses occupancy by absolute index,
        // independently of its preceding address-list scan.
        for(unsigned index=0;index<get(bank.physical_pool.bytes,0x38);++index){
            if(!occupied(bank.physical_bits,index))continue;
            const auto row=bank.physical_rows.bytes.subspan(index*24,24);
            const auto block_offset=signed_word(get(row,8)),block_pages=signed_word(get(row,4));
            if(block_offset<add(best.offset,pages) && add(block_offset,block_pages)>best.offset){
                const auto handle=std::uint32_t(word(row,0))<<16|index;
                const auto boundary=source_bitmap_manager_delete(bank,handle,context);
                if(boundary!=SourceBitmapManagerBoundary::Complete)return {boundary,none,handle};
            }
        }
        if(get(bank.physical_pool.bytes,0x3c)==capacity && oldest!=none){
            if(best.previous==oldest)
                best.previous=get(bank.physical_rows.bytes,(oldest&65535u)*24+0x10);
            const auto boundary=source_bitmap_manager_delete(bank,oldest,context);
            if(boundary!=SourceBitmapManagerBoundary::Complete)return {boundary,none,oldest};
        }
        const auto handle=allocate(bank);
        if(handle!=none){
            auto row=bank.physical_rows.bytes.subspan((handle&65535u)*24,24);
            if(best.previous==none){
                const auto first=get(bank.manager.bytes,0x3c);put(row,0x10,none);
                if(first==none)put(bank.manager.bytes,0x40,handle);
                else put(bank.physical_rows.bytes,(first&65535u)*24+0x10,handle);
                put(row,0xc,first);put(bank.manager.bytes,0x3c,handle);
            }else{
                auto before=bank.physical_rows.bytes.subspan((best.previous&65535u)*24,24);
                const auto next=get(before,0xc);
                if(next==none){put(row,0x10,get(bank.manager.bytes,0x40));put(bank.manager.bytes,0x40,handle);}
                else{
                    auto after=bank.physical_rows.bytes.subspan((next&65535u)*24,24);
                    put(row,0x10,get(after,0x10));put(after,0x10,handle);
                }
                put(row,0xc,next);put(before,0xc,handle);
            }
            put(row,8,std::bit_cast<std::uint32_t>(best.offset));
            put(row,4,std::bit_cast<std::uint32_t>(pages));put(row,0x14,now);
            return {SourceBitmapManagerBoundary::Complete,handle,none};
        }
    }
    // Ages2/5 equal their literal clamp. Source failure uses a signed minimum.
    put(bank.manager.bytes,limit_at,std::bit_cast<std::uint32_t>(
        std::min(signed_word(get(bank.manager.bytes,limit_at)),pages)));
    return {};
}
} // namespace halo2
