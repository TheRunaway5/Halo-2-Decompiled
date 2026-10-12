#include "halo2/source_geometry_resource_request.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <cstring>

namespace halo2 {
namespace {
template<class T>T read(const SourceGeometryResourceMemory& m,std::uint32_t at) {
    if(!m.read)throw FormatError("geometry resource request reader absent");
    const auto bytes=m.read(at,sizeof(T));
    if(bytes.size()!=sizeof(T))throw FormatError("geometry resource request input truncated");
    T value;std::memcpy(&value,bytes.data(),sizeof(T));return value;
}
template<class T>void put(const SourceGeometryResourceMemory& m,std::uint32_t at,T value) {
    if(!m.write)throw FormatError("geometry resource request writer absent");
    auto bytes=m.write(at,sizeof(T));
    if(bytes.size()!=sizeof(T))throw FormatError("geometry resource request output truncated");
    std::memcpy(bytes.data(),&value,sizeof(T));
}
}
bool source_geometry_resource_request(std::uint32_t header,std::uint32_t flags,
    const SourceGeometryResourceGlobals& g,const SourceGeometryResourceMemory& m,
    const SourceGeometryResourceRequestServices& services) {
    const auto word=[&](auto at){return read<std::uint32_t>(m,at);};
    const auto byte=[&](auto at){return read<std::uint8_t>(m,at);};
    if(byte(header+0x1e)) {
        const auto index=word(header+0x20)&65535u;
        const auto physical=word(g.physical),pool=word(physical+0x64);
        const auto clock=word(physical+0x38),records=word(pool+0x44);
        put(m,records+index*24+0x14,clock);return true;
    }
    if(read<std::int32_t>(m,header+8)<=0 || read<std::int32_t>(m,header+0xc)<0)return false;
    bool blocking=flags&1u;
    const bool loading=flags&2u,flag2=flags&4u,flag3=flags&8u;
    const auto tag=std::uint32_t(std::int32_t(read<std::int16_t>(m,header+0x18)));
    const auto group=word(word(g.tags)+tag*16u);
    const bool special=group==0x6c746d70u || group==0x73627370u;
    if(byte(g.disable_blocking))blocking=false;
    bool selective=false;std::int32_t priority;
    const auto policy=read<std::int32_t>(m,g.policy);
    if(policy>0 && loading && !flag2 && !blocking) {
        const auto filter=word(g.policy_filter);
        selective=filter==0 || (filter==1 && special);
        if(policy==1 && selective)blocking=true;
    }
    // A policy-selected request retains priority2 even if policy1 made it
    // blocking. Caller blocking that skipped selection uses priority7.
    if(selective)priority=2;
    else if(blocking)priority=7;
    else if(flag3)priority=5;
    else priority=flag2?1:2;
    if(word(header+0x20)==0xffffffffu && loading) {
        if(!services.acquire)throw FormatError("geometry resource acquisition owner absent");
        services.acquire(header,blocking,selective,priority);
    }
    const auto token=word(header+0x20);
    if(token!=0xffffffffu) {
        const auto index=token&65535u;
        const auto inflight=word(word(g.inflight)+0x44)+index*12;
        auto physical=word(g.physical);
        const auto pool=word(physical+0x64);
        const auto ready=byte(inflight+2);
        const auto records=word(pool+0x44),clock=word(physical+0x38);
        put(m,records+index*24+0x14,clock);
        if(!ready) {
            if(priority>std::int32_t(byte(inflight+3))) {
                const auto job=word(inflight+4);
                if(!services.promote)throw FormatError("geometry resource job promotion owner absent");
                services.promote(job,7);physical=word(g.physical);
                put(m,inflight+3,std::uint8_t(priority));
            }
            if(blocking && !byte(inflight+2) && !byte(inflight+2)) {
                do {
                    if(!services.yield)throw FormatError("geometry resource blocking yield owner absent");
                    services.yield();
                    if(!services.pump)throw FormatError("geometry resource completion pump owner absent");
                    services.pump();
                } while(!byte(inflight+2));
                physical=word(g.physical);
            }
        }
        if(byte(inflight+2)) {
            if(!byte(header+0x1e)) {
                const auto current_index=word(header+0x20)&65535u;
                const auto current_records=word(word(physical+0x64)+0x44);
                const auto page=word(current_records+current_index*24+8);
                const auto shift=word(physical+0x34)&31u;
                const auto allocation=(page<<shift)+word(g.backing);
                source_geometry_resource_publish(header,allocation,m);
                put(m,header+0x1e,std::uint8_t(1));
            }
            return true;
        }
    } else if(!flag2 && loading)put(m,g.pending_failure,std::uint8_t(1));
    if(special)put(m,g.missing_special,word(g.missing_special)+1u);
    return false;
}
}
