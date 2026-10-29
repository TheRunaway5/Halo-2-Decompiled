#include "halo2/source_multiplayer_render_history_tu15.hpp"
#include "halo2/raw_datum_pool.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/source_render_cache.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <cstring>
#include <map>

namespace halo2 {
namespace {
std::uint32_t word(const SourceRuntimeMemory &m,std::uint32_t p,unsigned n=4) {
    const auto b=m.view(p,n);std::uint32_t v{};
    for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(i*8);
    return v;
}
struct Pool {std::uint32_t header{},records{},bits{};};
Pool cache(const SourceRuntimeMemory &m) {
    Pool p{word(m,0x515f14u)};
    if(!p.header || word(m,p.header+0x20u)!=256 || word(m,p.header+0x24u)!=256 ||
       word(m,p.header+0x34u)>256 || word(m,p.header+0x38u)>256 || word(m,p.header+0x3cu)>256)
        throw FormatError("Native presentation lost its actual256-record render cache");
    p.records=word(m,p.header+0x44u);p.bits=word(m,p.header+0x48u);
    m.view(p.records,256u*256u);m.view(p.bits,32);return p;
}
std::map<std::uint32_t,std::uint32_t> objects(const SourceRuntimeMemory &m) {
    const auto pool=word(m,0x4ecbd0u),count=word(m,pool+0x38u),records=word(m,pool+0x44u);
    if(word(m,pool+0x20u)>65536 || count>word(m,pool+0x20u) || word(m,pool+0x24u)!=12)
        throw FormatError("Native presentation lost its actual object-header pool");
    const SourceRawDatumPoolServices storage{.read=[&](auto p,auto n){return m.view(p,n);}};
    std::map<std::uint32_t,std::uint32_t> out;
    for(unsigned i=0;i<count;++i)if(const auto salt=word(m,records+i*12u,2)) {
        const auto owner=(salt<<16)|i,header=source_raw_datum_lookup(pool,owner,storage);
        const auto body=word(m,header+8u);
        if(!header || !body || word(m,header+6u,2)<0xd0)
            throw FormatError("Native presentation object lost its full-salt body/cache storage");
        m.view(body,word(m,header+6u,2));out.emplace(owner,body);
    }
    return out;
}
struct Patch {std::uint32_t address;std::vector<std::byte> bytes;};
}
void source_stage_multiplayer_render_history_tu15(const SourceRuntimeMemory &current,
    SourceRuntimeMemory &replacement) {
    if(&current==&replacement)throw FormatError("Native render history needs a distinct prepared world");
    const auto a=word(current,0x4f3218u),b=word(replacement,0x4f3218u);
    if(!a || !b || word(current,a+8u)==1 || word(current,a+8u)!=word(replacement,b+8u))
        throw FormatError("Native multiplayer render history has a different game owner");
    const auto old=cache(current),next=cache(replacement);
    const auto before=objects(current),after=objects(replacement);
    std::vector<Patch> patches;
    const auto retain=[&](std::uint32_t dst,std::uint32_t src,std::size_t bytes) {
        replacement.validate_mutable_view(dst,bytes);const auto value=current.view(src,bytes);
        patches.push_back({dst,{value.begin(),value.end()}});
    };
    // Keep actual allocator cursor/high-water/count/salt, occupancy and every
    // uncleared payload byte. Pointer fields remain the replacement's real
    // backing owners; no process pointer or source allocation is transplanted.
    retain(next.header+0x34u,old.header+0x34u,14);
    retain(next.records,old.records,256u*256u);retain(next.bits,old.bits,32);
    retain(0x4c6854u,0x4c6854u,4);
    // MCC cold composition preserves1D640's counter and bank as distinct
    // native owners. Validate/copy both complete extents independently.
    retain(0x494338u,0x494338u,4u);
    retain(0x49433cu,0x49433cu,0x27000u);
    static_assert(sizeof(SourceShadowSelectionCache)==0x484);
    retain(0x5332c8u,0x5332c8u,4u*sizeof(SourceShadowSelectionCache));
    for(unsigned view=0;view<4;++view) {
        SourceShadowSelectionCache history;
        std::memcpy(&history,current.view(0x5332c8u+view*0x484u,sizeof(history)).data(),sizeof(history));
        if(history.count>history.entries.size())throw FormatError("Native shadow history exceeds32 actual roots");
    }
    for(const auto &[owner,body]:after) {
        auto token=0xffffffffu;
        const auto found=before.find(owner);
        if(found!=before.end() && word(current,found->second)==word(replacement,body)) {
            const auto candidate=word(current,found->second+0xccu);
            if(candidate!=0xffffffffu) {
                if((candidate&65535u)>=256)throw FormatError("Native object render-cache token exceeds its owner");
                if(word(current,old.records+(candidate&65535u)*256u+4u)==owner)token=candidate;
            }
        }
        // Retired/replaced salts never acquire the predecessor's cache. A new
        // native3DDD0 request owns subsequent allocation and initialization.
        replacement.validate_mutable_view(body+0xccu,4);
        std::vector<std::byte> bytes(4);std::memcpy(bytes.data(),&token,4);
        patches.push_back({body+0xccu,std::move(bytes)});
    }
    for(const auto &patch:patches)replacement.write(patch.address,patch.bytes);
}
}
