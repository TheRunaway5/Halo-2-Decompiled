#include "halo2/source_geometry_resource_retirement.hpp"
#include "halo2/cache.hpp"
#include <bit>
#include <cstring>

namespace halo2 {
namespace {
template<class T>T read(const SourceGeometryResourceMemory& m,std::uint32_t at) {
    if(!m.read)throw FormatError("geometry retirement reader absent");
    const auto bytes=m.read(at,sizeof(T));
    if(bytes.size()!=sizeof(T))throw FormatError("geometry retirement input truncated");
    T value;std::memcpy(&value,bytes.data(),sizeof(T));return value;
}
template<class T>void put(const SourceGeometryResourceMemory& m,std::uint32_t at,T value) {
    if(!m.write)throw FormatError("geometry retirement writer absent");
    auto bytes=m.write(at,sizeof(T));
    if(bytes.size()!=sizeof(T))throw FormatError("geometry retirement output truncated");
    std::memcpy(bytes.data(),&value,sizeof(T));
}
std::uint32_t resource_token(std::uint32_t descriptor,std::uint32_t data,
    const SourceGeometryResourceMemory& m) {
    const auto selector=read<std::uint16_t>(m,descriptor+6);
    const auto locator=read<std::uint16_t>(m,descriptor+4);
    const auto buffer=selector==65535?data+locator:
        read<std::uint32_t>(m,data+locator+4)+std::uint32_t(selector)*32u;
    return read<std::uint32_t>(m,buffer+0x10);
}
std::uint32_t inflight(std::uint32_t datum,const SourceGeometryRetirementGlobals& g,
    const SourceGeometryResourceMemory& m) {
    return read<std::uint32_t>(m,read<std::uint32_t>(m,g.resource.inflight)+0x44)+(datum&65535u)*12u;
}
std::uint32_t physical_entry(std::uint32_t datum,std::uint32_t physical,
    const SourceGeometryResourceMemory& m) {
    return read<std::uint32_t>(m,read<std::uint32_t>(m,physical+0x64)+0x44)+(datum&65535u)*24u;
}
std::uint32_t allocation(std::uint32_t datum,std::uint32_t physical,
    const SourceGeometryRetirementGlobals& g,const SourceGeometryResourceMemory& m) {
    return (read<std::uint32_t>(m,physical_entry(datum,physical,m)+8)<<
        (read<std::uint32_t>(m,physical+0x34)&31u))+read<std::uint32_t>(m,g.resource.backing);
}
}
bool source_geometry_resource_in_use(std::uint32_t header,std::uint32_t base,
    const SourceGeometryResourceMemory& m,const SourceGeometryRetirementServices& s) {
    for(std::uint32_t i=0;std::bit_cast<std::int32_t>(i)<read<std::int32_t>(m,header+0x10);++i) {
        const auto d=read<std::uint32_t>(m,header+0x14)+i*16u;
        if(read<std::uint32_t>(m,d+0xc)==0xffffffffu || read<std::uint8_t>(m,d)!=2)continue;
        const auto token=resource_token(d,base+8,m);
        if(!token)continue;
        if(!s.resource_busy)throw FormatError("geometry GPU resource query owner absent");
        if(s.resource_busy(token)>0)return true;
    }
    return false;
}
void source_geometry_resource_wait(std::uint32_t header,std::uint32_t base,
    const SourceGeometryResourceMemory& m,const SourceGeometryRetirementServices& s) {
    for(std::uint32_t i=0;std::bit_cast<std::int32_t>(i)<read<std::int32_t>(m,header+0x10);++i) {
        const auto d=read<std::uint32_t>(m,header+0x14)+i*16u;
        if(read<std::uint32_t>(m,d+0xc)==0xffffffffu || read<std::uint8_t>(m,d)!=2)continue;
        const auto token=resource_token(d,base+8,m);
        if(!token)continue;
        if(!s.wait_resource)throw FormatError("geometry GPU resource wait owner absent");
        s.wait_resource(token);
    }
}
bool source_geometry_cache_busy(std::uint32_t datum,const SourceGeometryRetirementGlobals& g,
    const SourceGeometryResourceMemory& m,const SourceGeometryRetirementServices& s) {
    const auto row=inflight(datum,g,m),physical=read<std::uint32_t>(m,g.resource.physical);
    const auto entry=physical_entry(datum,physical,m);
    const auto age=read<std::uint32_t>(m,physical+0x38)-read<std::uint32_t>(m,entry+0x14);
    if(!age || !read<std::uint8_t>(m,row+2))return true;
    if(std::bit_cast<std::int32_t>(age)>=5)return false;
    const auto header=read<std::uint32_t>(m,row+8);
    if(!read<std::uint8_t>(m,header+0x1e))return false;
    return source_geometry_resource_in_use(header,allocation(datum,physical,g,m),m,s);
}
void source_geometry_cache_deleted(std::uint32_t datum,const SourceGeometryRetirementGlobals& g,
    const SourceGeometryResourceMemory& m,const SourceGeometryRetirementServices& s,
    const SourceRawDatumPoolObserver& observer) {
    const auto row=inflight(datum,g,m);
    if(read<std::uint32_t>(m,row+8)) {
        const auto physical=read<std::uint32_t>(m,g.resource.physical);
        const auto entry=physical_entry(datum,physical,m);
        const auto clock=read<std::uint32_t>(m,physical+0x38);
        if(read<std::uint32_t>(m,entry+0x14)==clock)put(m,entry+0x14,clock-1u);
        while(source_geometry_cache_busy(datum,g,m,s)) {
            if(!s.wait_mutex)throw FormatError("geometry async queue mutex owner absent");
            s.wait_mutex(read<std::uint32_t>(m,g.async_mutex),0xffffffffu);
            std::uint32_t count=0,node=read<std::uint32_t>(m,g.async_head);
            while(node){node=read<std::uint32_t>(m,node+0x3c);++count;}
            if(!s.release_mutex)throw FormatError("geometry async queue mutex release owner absent");
            s.release_mutex(read<std::uint32_t>(m,g.async_mutex));
            put(m,g.async_count,count);
            const auto header=read<std::uint32_t>(m,row+8);
            if(read<std::uint8_t>(m,header+0x1e)) {
                const auto current=read<std::uint32_t>(m,g.resource.physical);
                source_geometry_resource_wait(header,allocation(datum,current,g,m),m,s);
            }
        }
        const auto header=read<std::uint32_t>(m,row+8);
        if(read<std::uint8_t>(m,header+0x1e)) {
            const auto root=header+std::uint32_t(std::int32_t(read<std::int16_t>(m,header+0x1c)));
            put(m,root,0u);put(m,root+4,0u);
            put(m,read<std::uint32_t>(m,row+8)+0x1e,std::uint8_t(0));
        }
        put(m,read<std::uint32_t>(m,row+8)+0x20,0xffffffffu);
    }
    source_raw_datum_release_unchecked(read<std::uint32_t>(m,g.resource.inflight),datum,
        {.memory=m.write,.read=m.read},observer);
}
}
