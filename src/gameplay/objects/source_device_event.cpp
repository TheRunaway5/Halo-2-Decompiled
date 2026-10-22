#include "halo2/source_device_event.hpp"
#include <stdexcept>

namespace halo2 {
namespace {
void require(bool ok,const char *message){if(!ok)throw std::runtime_error(message);}
std::uint32_t word(std::span<const std::byte> body,std::size_t at,unsigned size=4){
    require(at<=body.size() && size<=body.size()-at,"Truncated source device event body");
    std::uint32_t result=0;for(unsigned i=0;i<size;++i)
        result|=std::uint32_t(std::to_integer<unsigned>(body[at+i]))<<(8*i);
    return result;
}
}
SourceDeviceEventKind source_device_event_kind(std::uint32_t tag,std::uint32_t group){
    if(tag==~0u)return SourceDeviceEventKind::Ignore;
    if(group==0x65666665u)return SourceDeviceEventKind::Effect;
    if(group==0x736e6421u)return SourceDeviceEventKind::Sound;
    return SourceDeviceEventKind::Ignore;
}
void dispatch_source_device_event(std::uint32_t tag,std::uint32_t object,
    const SourceDeviceEventServices &services){
    if(tag==~0u)return;
    require(bool(services.object),"Source device event object owner unavailable");
    const auto body=services.object(object);
    SourceDeviceEffectRequest request;request.tag=tag;request.object=object;
    // Original copies these before selecting the tag group, including for
    // unknown groups. Keep the read extent and source call ordering.
    const std::array<std::uint32_t,3> fields{word(body,0xc4),word(body,0xc8),word(body,0xc2,2)};
    for(unsigned i=0;i<10;++i)request.location[i]=std::byte(fields[i/4]>>(8*(i%4)));
    require(bool(services.tag_group),"Source device event tag owner unavailable");
    switch(source_device_event_kind(tag,services.tag_group(tag))){
    case SourceDeviceEventKind::Effect:
        request.power_bits=word(body,0x134);request.position_bits=word(body,0x140);
        require(bool(services.effect),"Source device176780 effect owner unavailable");services.effect(request);break;
    case SourceDeviceEventKind::Sound:
        require(bool(services.sound),"Source device189060 sound owner unavailable");
        services.sound({tag,object});break;
    case SourceDeviceEventKind::Ignore:break;
    }
}
}
