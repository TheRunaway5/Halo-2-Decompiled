#include "halo2/source_contrail_snapshot.hpp"
#include "halo2/cache.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/raw_datum_pool.hpp"
#include <algorithm>
#include <cstring>
#include <map>
#include <set>
namespace halo2 {
namespace {
constexpr std::size_t maximum_bytes=2u*1024u*1024u;
void check(bool ok,const char *why){if(!ok)throw FormatError(why);}
}
void validate_source_contrail_snapshot(const SourceContrailSnapshot &s) {
    check(s.authoritative || s.slices.empty(),"Unauthoritative contrail input carries source data");
    check(s.slices.size()<=32768,"Contrail input slice capacity exceeded");
    std::uint64_t end{};std::size_t total{};
    for(const auto &r:s.slices) {
        check(r.address && !r.bytes.empty() && r.address>=end && std::uint64_t(r.address)+r.bytes.size()<=0x100000000ull,
            "Contrail input ranges overlap or escape original address storage");
        total+=r.bytes.size();check(total<=maximum_bytes,"Contrail input byte capacity exceeded");end=std::uint64_t(r.address)+r.bytes.size();
    }
}
SourceContrailSnapshot capture_source_contrail_snapshot(const SourceRuntimeMemory &m) {
    const auto word=[&](std::uint32_t p,unsigned n=4){const Bytes b(m.view(p,n));return n==2?b.u16(0):b.u32(0);};
    const SourceRawDatumPoolServices storage{.read=[&](auto p,auto n){return m.view(p,n);}};
    std::vector<std::pair<std::uint32_t,std::size_t>> ranges;
    const auto add=[&](std::uint32_t p,std::size_t n){if(n){m.view(p,n);ranges.emplace_back(p,n);}};
    for(const auto p:{0x4f7210u,0x4f7214u,0x4ecbd0u,0x4f0414u})add(p,4);
    // Literal42FDD/4329F use these mutable pointer cells and their RGB/vector
    // inputs. Retain their live bytes rather than the XBE initial values.
    for(const auto p:{0x474c70u,0x474d10u}) {add(p,4);add(word(p),12);}
    const auto trails=word(0x4f7214u),points=word(0x4f7210u),objects=word(0x4ecbd0u),tags=word(0x4f0414u);
    check(trails && points && objects && tags,"Contrail snapshot lacks its actual constructed source pools");
    const auto pool=[&](std::uint32_t p,unsigned stride,unsigned limit) {
        check(word(p+0x24u)==stride && word(p+0x38u)<=word(p+0x20u) && word(p+0x20u)<=limit,
            "Contrail snapshot lost original pool stride or bounds");
        add(p,0x4c);add(word(p+0x48u),((word(p+0x20u)+31u)/32u)*4u);
    };
    pool(trails,72,4096);pool(points,56,16384);pool(objects,12,2048);
    std::set<std::uint32_t> definitions,owners;
    for(unsigned index=0;index<word(trails+0x38u);++index) {
        if(!(word(word(trails+0x48u)+index/32u*4u)&(1u<<(index%32u))))continue;
        const auto row=word(trails+0x44u)+index*72u;add(row,72);
        check(source_raw_datum_lookup(trails,(word(row,2)<<16u)|index,storage)==row,"Contrail snapshot has a stale source row");
        const auto datum=word(row+4u),descriptor=tags+(datum&65535u)*16u;
        check(word(descriptor+4u)==datum,"Contrail source definition identity differs");
        definitions.insert(datum);
        // Literal42FD4 skips the owner's color path for definition flag80.
        const auto tag=word(descriptor+8u);const auto flags=std::to_integer<unsigned>(m.view(tag,1).front());
        if(word(row+8u)!=0xffffffffu && !(flags&0x80u))owners.insert(word(row+8u));
    }
    for(unsigned index=0;index<word(points+0x38u);++index)
        if(word(word(points+0x48u)+index/32u*4u)&(1u<<(index%32u)))add(word(points+0x44u)+index*56u,56);
    for(const auto owner:owners) {
        const auto header=source_raw_datum_lookup(objects,owner,storage);check(header!=0,"Contrail snapshot has a stale object owner");
        add(header,12);const auto body=word(header+8u);add(body,0x128);definitions.insert(word(body));
        // Original BCD80 reads this local color bank: signed byte extent at
        // +124, local signed offset at+126, two RGB banks of12-byte entries.
        const auto bytes=std::int16_t(word(body+0x124u,2)),offset=std::int16_t(word(body+0x126u,2));
        if(bytes/24>0)add(std::uint32_t(body+std::int32_t(offset)),unsigned(bytes/24)*24u);
    }
    std::set<std::uint32_t> copied;
    const auto definition=[&](std::uint32_t datum) {
        check(datum!=0xffffffffu,"Contrail snapshot has no actual cache definition");
        const auto row=tags+(datum&65535u)*16u;check(word(row+4u)==datum,"Contrail snapshot definition generation differs");
        // Same exact16-byte resident descriptor validated by the TU1.5 tag
        // binding: group, full datum, source payload address and payload size.
        const auto size=word(row+12u);check(size && size<=maximum_bytes,"Contrail source definition extent is invalid");
        add(row,16);add(word(row+8u),size);
        return word(row+8u);
    };
    for(const auto datum:definitions) {
        const auto tag=definition(datum);copied.insert(datum);
        if(word(tags+(datum&65535u)*16u)==0x636f6e74u) {
            const auto bitmap=word(tag+0x34u);
            if(copied.insert(bitmap).second)definition(bitmap);
        }
    }
    std::sort(ranges.begin(),ranges.end());std::vector<std::pair<std::uint32_t,std::size_t>> merged;
    for(const auto [p,n]:ranges) {
        if(!merged.empty() && p<=std::uint64_t(merged.back().first)+merged.back().second) {
            auto &last=merged.back();last.second=std::max(std::uint64_t(last.first)+last.second,std::uint64_t(p)+n)-last.first;
        }else merged.emplace_back(p,n);
    }
    SourceContrailSnapshot result;result.authoritative=true;
    for(const auto [p,n]:merged) {const auto data=m.view(p,n);result.slices.push_back({p,{data.begin(),data.end()}});}
    validate_source_contrail_snapshot(result);return result;
}
SourceContrailSnapshotMemory::SourceContrailSnapshotMemory(const SourceContrailSnapshot &s,const SourceRuntimeMemory &m):snapshot_(s),original_data_(m) {validate_source_contrail_snapshot(s);check(s.authoritative,"Contrail view requires authoritative source inputs");}
void SourceContrailSnapshotMemory::read(std::uint32_t p,std::span<std::byte> out)const {
    const auto at=std::upper_bound(snapshot_.slices.begin(),snapshot_.slices.end(),p,[](auto address,const auto &r){return address<r.address;});
    if(at!=snapshot_.slices.begin()) {
        const auto &r=*std::prev(at);
        if(p>=r.address && std::uint64_t(p)+out.size()<=std::uint64_t(r.address)+r.bytes.size()) {
            std::memcpy(out.data(),r.bytes.data()+p-r.address,out.size());return;
        }
    }
    if(!original_data_.is_read_only(p,out.size()))throw FormatError("Contrail snapshot omitted mutable source bytes at "+std::to_string(p)+" size "+std::to_string(out.size()));
    original_data_.read(p,out);
}
void SourceContrailSnapshotMemory::write(std::uint32_t,std::span<const std::byte>){throw FormatError("Contrail replica attempted to mutate source input");}
}
