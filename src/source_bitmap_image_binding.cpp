#include "halo2/source_bitmap_image_binding.hpp"
#include <algorithm>
#include <bit>
#include <functional>
#include <stdexcept>

namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu,capacity=512,stride=40;
std::uint32_t get(std::span<const std::byte> data,std::size_t at){
    if(at>data.size() || data.size()-at<4)
        throw std::invalid_argument("source image binding bank missing word");
    std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::uint32_t(std::to_integer<unsigned char>(data[at+i]))<<(8*i);
    return value;
}
void put(std::span<std::byte> data,std::size_t at,std::uint32_t value){
    if(at>data.size() || data.size()-at<4)
        throw std::invalid_argument("source image binding bank missing output word");
    for(unsigned i=0;i<4;++i)data[at+i]=std::byte((value>>(8*i))&255u);
}
std::uint16_t word(std::span<const std::byte> data,std::size_t at){
    return std::uint16_t(std::to_integer<unsigned char>(data[at])|
        std::uint16_t(std::to_integer<unsigned char>(data[at+1]))<<8);
}
void put_word(std::span<std::byte> data,std::size_t at,std::uint16_t value){
    data[at]=std::byte(value&255u);data[at+1]=std::byte(value>>8);
}
void storage(const SourceBitmapManagerRegion &region,std::size_t required){
    if(!region.address || region.bytes.size()<required ||
       std::uint64_t(region.address)+region.bytes.size()>0x100000000ull)
        throw std::invalid_argument("source image binding needs actual addressed storage");
}
void separate(const SourceBitmapManagerRegion &a,const SourceBitmapManagerRegion &b){
    if(std::uint64_t(a.address)<std::uint64_t(b.address)+b.bytes.size() &&
       std::uint64_t(b.address)<std::uint64_t(a.address)+a.bytes.size())
        throw std::invalid_argument("source image binding bank overlap is unproved");
    const std::less<const std::byte*> less;
    if(less(a.bytes.data(),b.bytes.data()+b.bytes.size()) &&
       less(b.bytes.data(),a.bytes.data()+a.bytes.size()))
        throw std::invalid_argument("source image binding storage aliases different addressed banks");
}
}
std::uint32_t source_bitmap_bind_image(SourceBitmapImageBindingBank &bank,std::uint32_t handle){
    storage(bank.pool,0x4c);
    if(get(bank.pool.bytes,0x20)!=capacity || get(bank.pool.bytes,0x24)!=stride)
        throw std::invalid_argument("source image binding pool layout is unproved");
    const auto index=handle&65535u;
    if(index>=capacity)return none;
    storage(bank.rows,capacity*stride);
    if(get(bank.pool.bytes,0x44)!=bank.rows.address)
        throw std::invalid_argument("source image binding row address differs from the actual pool");
    separate(bank.pool,bank.rows);
    auto row=bank.rows.bytes.subspan(index*stride,stride);
    if(word(row,0))return none;
    storage(bank.bits,64);
    if(get(bank.pool.bytes,0x48)!=bank.bits.address)
        throw std::invalid_argument("source image binding occupied-bit address differs from the actual pool");
    separate(bank.pool,bank.bits);separate(bank.rows,bank.bits);
    const auto bit_at=(index>>5)*4;
    put(bank.bits.bytes,bit_at,get(bank.bits.bytes,bit_at)|(1u<<(index&31)));
    put(bank.pool.bytes,0x3c,get(bank.pool.bytes,0x3c)+1u);
    if(std::int32_t(index)>=std::bit_cast<std::int32_t>(get(bank.pool.bytes,0x38)))
        put(bank.pool.bytes,0x38,index+1u);
    // Complete 16BA00: its generated salt is consumed even though 16B910
    // immediately overwrites it with the caller's salt. No flag branch exists.
    std::fill(row.begin(),row.end(),std::byte{});
    const auto generated=word(bank.pool.bytes,0x40);put_word(row,0,generated);
    auto next=std::uint16_t(generated+1u);if(next==0xffffu)next=0x8000u;
    put_word(bank.pool.bytes,0x40,next);
    put_word(row,0,std::uint16_t(handle>>16));
    return handle;
}
} // namespace halo2
