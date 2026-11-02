#include "halo2/native_tu15_phantom_overlap_remove.hpp"
#include <bit>
#include <stdexcept>

namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory& memory_;
    const SourcePhysicsContactRecordObserver& observe_;
public:
    Memory(SourcePhysicsContactMemory& memory,const SourcePhysicsContactRecordObserver& observe)
        :memory_(memory),observe_(observe){}
    std::uint32_t word(std::uint32_t address)const{
        std::array<std::byte,4> bytes{};memory_.read(address,bytes);std::uint32_t value{};
        for(unsigned n=0;n<4;++n)value|=std::to_integer<unsigned>(bytes[n])<<(8u*n);
        return value;
    }
    void word(std::uint32_t site,std::uint32_t address,std::uint32_t value,unsigned width=4){
        std::array<std::byte,4> bytes{};for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(8u*n));
        SourcePhysicsContactRecordStore event{site,address,{},{}};
        if(observe_){event.before.resize(width);memory_.read(address,event.before);event.written.assign(bytes.begin(),bytes.begin()+width);}
        memory_.write(address,std::span(bytes).first(width));if(observe_)observe_(event);
    }
};
bool nonnegative(std::uint32_t value){return std::bit_cast<std::int32_t>(value)>=0;}
void notify(Memory& m,std::uint32_t phantom,NativeTu15ContactPayload other,
    std::uint8_t was_present,NativeTu15PhantomOverlapEvent event,
    NativeTu15PhysicsFrameFp& fp,const NativeTu15PhantomOverlapRemoveServices& services,
    bool helper){
    auto index=m.word(phantom+0x40u)-1u;
    m.word(helper?0x314ff3u:0x315aa2u,event.identity+4u,other.identity);
    m.word(helper?0x314ff7u:0x315aa6u,event.identity,phantom);
    m.word(helper?0x314ffbu:0x315aaau,event.identity+8u,was_present,1);
    if(!nonnegative(index))return;
    do{
        const auto data=m.word(phantom+0x3cu),listener=m.word(data+index*4u);
        if(listener){
            const auto table=m.word(listener);
            const NativeTu15ContactVirtualOwner owner{m.word(table+4u)};
            if(!services.removed)throw std::runtime_error("Native TU1.5 phantom overlap removal requires actual listener owner");
            services.removed(listener,owner,event,fp);
        }
        --index;
    }while(nonnegative(index));
}
}
void native_tu15_notify_phantom_overlap_removed(SourcePhysicsContactMemory& memory,
    std::uint32_t phantom,NativeTu15ContactPayload other,std::uint8_t was_present,
    NativeTu15PhantomOverlapEvent event,NativeTu15PhysicsFrameFp& fp,
    const NativeTu15PhantomOverlapRemoveServices& services,
    const SourcePhysicsContactRecordObserver& observe){
    Memory m(memory,observe);notify(m,phantom,other,was_present,event,fp,services,true);
}
void native_tu15_remove_phantom_overlap(SourcePhysicsContactMemory& memory,
    std::uint32_t phantom,NativeTu15ContactPayload other,NativeTu15PhantomOverlapEvent event,
    NativeTu15PhysicsFrameFp& fp,const NativeTu15PhantomOverlapRemoveServices& services,
    const SourcePhysicsContactRecordObserver& observe){
    Memory m(memory,observe);if(!m.word(other.identity))return;
    auto index=m.word(phantom+0xc4u)-1u;
    if(nonnegative(index)){
        const auto data=m.word(phantom+0xc0u);auto cursor=data+index*4u;
        do{
            if(m.word(cursor)==other.identity){
                notify(m,phantom,other,1,event,fp,services,true);
                const auto last=m.word(phantom+0xc4u)-1u;
                m.word(0x315ae5u,phantom+0xc4u,last);
                const auto current_data=m.word(phantom+0xc0u),replacement=m.word(current_data+last*4u);
                m.word(0x315af4u,current_data+index*4u,replacement);return;
            }
            --index;cursor-=4u;
        }while(nonnegative(index));
    }
    notify(m,phantom,other,0,event,fp,services,false);
}
} // namespace halo2
