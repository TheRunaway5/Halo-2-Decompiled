#include "halo2/effect_light_spatial.hpp"
#include <algorithm>

namespace halo2 {
namespace {
constexpr auto none=0xffffffffu;
auto& record(SourceObjectSpatialDatumPool& pool,std::uint32_t token) {
    if(token==none || (token&0xffffu)>=pool.capacity)
        throw FormatError("Effect light spatial link exceeds original pool");
    return pool.records[token&0xffffu];
}
void valid_cluster(const SourceEffectLightSpatialPool& pool,std::int16_t cluster) {
    if(cluster<0 || std::size_t(cluster)>=pool.heads.size())
        throw FormatError("Effect light spatial cluster exceeds actual heads");
}
}
SourceEffectLightSpatialPool::SourceEffectLightSpatialPool(std::size_t clusters)
    :heads(clusters,none) {
    if(clusters>512)throw FormatError("Effect light spatial heads exceed512");
    entries.record_words=references.record_words=3;activate();
}
void SourceEffectLightSpatialPool::activate(bool debug) {
    std::fill(heads.begin(),heads.end(),none);
    entries.activate(0x6c63,debug); // "cluster light reference"
    references.activate(0x696c,debug); // "light cluster reference"
}
void SourceEffectLightSpatialPool::insert(std::uint32_t identity,std::uint32_t& head,
    std::span<const std::int16_t> memberships) {
    if(identity==none || memberships.size()>256)
        throw FormatError("Effect light spatial insertion requires original identity/membership prefix");
    for(auto c:memberships)valid_cluster(*this,c);
    for(auto c:memberships) {
        const auto reference=references.allocate();
        if(reference!=none) {
            auto& r=record(references,reference);r[1]=std::uint32_t(std::int32_t(c));r[2]=head;head=reference;
        }
        const auto entry=entries.allocate();
        if(entry!=none) {
            auto& r=record(entries,entry);r[1]=identity;r[2]=heads[c];heads[c]=entry;
        }
    }
}
void SourceEffectLightSpatialPool::erase(std::uint32_t identity,std::uint32_t& head) {
    unsigned references_seen=0;
    for(auto token=head;token!=none;) {
        if(++references_seen>references.capacity)
            throw FormatError("Effect light spatial owner reference cycle");
        const auto& r=record(references,token);const auto c=std::int16_t(r[1]);const auto next=r[2];
        valid_cluster(*this,c);references.release(token);
        auto* link=&heads[c];unsigned entries_seen=0;
        while(*link!=none) {
            if(++entries_seen>entries.capacity)throw FormatError("Effect light spatial cluster cycle");
            auto& entry=record(entries,*link);
            if(entry[1]==identity) {const auto tail=entry[2];entries.release(*link);*link=tail;break;}
            link=&entry[2];
        }
        token=next;
    }
    head=none;
}
std::vector<std::uint32_t> SourceEffectLightSpatialPool::cluster(std::int16_t c) const {
    valid_cluster(*this,c);std::vector<std::uint32_t> result;unsigned visited=0;
    for(auto token=heads[c];token!=none;) {
        if((token&0xffffu)>=entries.capacity || ++visited>entries.capacity)
            throw FormatError("Effect light spatial cluster link exceeds original pool");
        const auto& r=entries.records[token&0xffffu];result.push_back(r[1]);token=r[2];
    }
    return result;
}
}
