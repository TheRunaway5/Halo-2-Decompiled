#include "halo2/source_particle_constant_storage_tu15.hpp"
#include <algorithm>
#include <cstring>

namespace halo2 {
namespace {
constexpr std::uint32_t cursor=0x40f0c0,shadow=0x411818,fifo=0xf7700000;
constexpr std::size_t capacity=65536,shadow_bytes=192*16;
bool within(std::uint32_t p,std::size_t n,std::uint32_t base,std::size_t size) {
    return p>=base && n<=size && std::size_t(p-base)<=size-n;
}
}
SourceParticleConstantStorageTu15::SourceParticleConstantStorageTu15() {
    for(const auto [p,n]:std::array<std::pair<std::uint32_t,std::size_t>,3>{{
        {cursor,8},{shadow,shadow_bytes},{fifo,capacity}}})
        storage_.mount(p,std::vector<std::byte>(n),SourceRuntimeMemory::Access::ReadWrite,
            "native particle vertex-constant command storage");
}
void SourceParticleConstantStorageTu15::check() const {
    if(std::this_thread::get_id()!=thread_ || !source_)
        throw FormatError("Particle constant storage lacks its active native caller/thread");
}
bool SourceParticleConstantStorageTu15::owns(std::uint32_t p,std::size_t n) const {
    return within(p,n,cursor,8) || within(p,n,shadow,shadow_bytes) || within(p,n,fifo,capacity);
}
void SourceParticleConstantStorageTu15::reject_partial_ownership(std::uint32_t p,std::size_t n) const {
    if(n>0x100000000ull-p)throw FormatError("Particle constant access wraps source storage");
    for(const auto [base,size]:std::array<std::pair<std::uint32_t,std::size_t>,3>{{
        {cursor,8},{shadow,shadow_bytes},{fifo,capacity}}})
        if(p<std::uint64_t(base)+size && std::uint64_t(p)+n>base)
            throw FormatError("Particle constant access crosses native ownership boundaries");
}
void SourceParticleConstantStorageTu15::begin(SourcePhysicsContactMemory &source,
    std::span<const ShaderVector,192> inherited) {
    if(std::this_thread::get_id()!=thread_ || &source==this || (source_ && source_!=&source))
        throw FormatError("Particle constant storage cannot cross native worlds or threads");
    source_=&source;std::copy(inherited.begin(),inherited.end(),inherited_.begin());
    storage_.write(shadow,std::as_bytes(inherited));
    //401410 checks before each68-byte chunk, not after. Retain a full chunk
    // beyond the logical emission limit inside the actual allocation.
    const std::array<std::uint32_t,2> bounds{fifo,fifo+capacity-68};
    storage_.write(cursor,std::as_bytes(std::span(bounds)));
}
void SourceParticleConstantStorageTu15::read(std::uint32_t p,std::span<std::byte> out) const {
    check();if(owns(p,out.size()))storage_.read(p,out);
    else {reject_partial_ownership(p,out.size());source_->read(p,out);}
}
void SourceParticleConstantStorageTu15::write(std::uint32_t p,std::span<const std::byte> in) {
    check();if(owns(p,in.size()))storage_.write(p,in);
    else {reject_partial_ownership(p,in.size());source_->write(p,in);}
}
std::span<const std::byte> SourceParticleConstantStorageTu15::commands() const {
    check();const auto end=Bytes(storage_.view(cursor,4)).u32(0);
    if(end<fifo || end>fifo+capacity || (end-fifo)%4)
        throw FormatError("Particle constant command cursor escapes its actual allocation");
    // A new pass has no packet; SourceRuntimeMemory intentionally rejects
    // empty views, so return an empty span without constructing one.
    return end==fifo?std::span<const std::byte>{}:storage_.view(fifo,end-fifo);
}
void SourceParticleConstantStorageTu15::consume(std::span<ShaderVector,192> destination) const {
    consume_packet(commands(),destination);
}
void SourceParticleConstantStorageTu15::consume_packet(std::span<const std::byte> packet,
    std::span<ShaderVector,192> destination) const {
    check();const Bytes bytes(packet);auto result=inherited_;
    unsigned first=192;bool loaded=false;
    for(std::size_t at=0;at<packet.size();) {
        if(packet.size()-at<4)throw FormatError("Truncated particle constant method");
        const auto header=bytes.u32(at),count=(header>>18)&2047u;
        if(!count || (header&~0x1ffc1fffu) || std::size_t(count)*4>packet.size()-at-4)
            throw FormatError("Particle constant method has an invalid extent/header");
        if(header==0x41ea4u) {
            first=bytes.u32(at+4);loaded=true;
            if(first>=192)throw FormatError("Particle constant load escapes c0..191");
        }else if((header&0x3ffffu)==0xb80u && loaded && count%4==0) {
            if(count/4>192-first)throw FormatError("Particle constant data escapes c0..191");
            std::memcpy(result.data()+first,packet.data()+at+4,count*4);first+=count/4;
        }else throw FormatError("Particle constant stream requires an unowned GPU method");
        at+=4+count*4;
    }
    std::copy(result.begin(),result.end(),destination.begin());
}
void SourceParticleConstantStorageTu15::visit_committed(const std::function<void(
    std::uint32_t,std::span<const std::byte>)> &visitor) const {
    check();storage_.visit_committed(visitor);
}
}
