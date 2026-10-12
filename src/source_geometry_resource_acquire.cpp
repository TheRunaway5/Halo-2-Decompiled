#include "halo2/source_geometry_resource_acquire.hpp"
#include "halo2/raw_datum_pool.hpp"
#include "halo2/cache.hpp"
#include <cstring>

namespace halo2 {
namespace {
template<class T>T read(const SourceGeometryResourceMemory& m,std::uint32_t at) {
    if(!m.read)throw FormatError("geometry resource acquisition reader absent");
    const auto bytes=m.read(at,sizeof(T));
    if(bytes.size()!=sizeof(T))throw FormatError("geometry resource acquisition input truncated");
    T value;std::memcpy(&value,bytes.data(),sizeof(T));return value;
}
template<class T>void put(const SourceGeometryResourceMemory& m,std::uint32_t at,T value) {
    if(!m.write)throw FormatError("geometry resource acquisition writer absent");
    auto bytes=m.write(at,sizeof(T));
    if(bytes.size()!=sizeof(T))throw FormatError("geometry resource acquisition output truncated");
    std::memcpy(bytes.data(),&value,sizeof(T));
}
}
bool source_geometry_resource_acquire(std::uint32_t header,bool blocking,
    bool selective,std::int32_t priority,const SourceGeometryResourceGlobals& g,
    const SourceGeometryResourceMemory& m,const SourceGeometryResourceAcquireServices& services) {
    const auto word=[&](auto at){return read<std::uint32_t>(m,at);};
    const auto size=word(header+4),physical=word(g.physical);
    if(!services.physical_allocate)throw FormatError("geometry physical admission owner absent");
    const auto token=services.physical_allocate(physical,size,(blocking || selective)?1:5);
    if(token==0xffffffffu)return false;
    const auto current_physical=word(g.physical);
    const auto records=word(word(current_physical+0x64)+0x44);
    const auto shift=word(current_physical+0x34)&31u;
    const auto page=word(records+(token&65535u)*24+8);
    const auto allocation=(page<<shift)+word(g.backing);
    const auto pool=word(g.inflight);
    const auto actual=source_raw_datum_allocate_identity(pool,token,{.memory=m.write,.read=m.read});
    const auto inflight=word(pool+0x44)+(actual&65535u)*12;
    put(m,inflight+8,header);put(m,header+0x20,token);
    put(m,word(inflight+8)+0x1e,std::uint8_t(0));
    auto bytes=word(header+4);if(bytes&511u)bytes=(bytes|511u)+1u;
    (void)word(header+0x18); // Original retained owner/flags read before IO.
    const SourceBitmapReadRequest request{word(header),bytes,allocation,0,inflight+2,4,priority};
    if(!services.async_read)throw FormatError("geometry asynchronous read owner absent");
    const auto job=services.async_read(request);
    put(m,inflight+4,job);put(m,inflight+3,std::uint8_t(priority));return true;
}
}
