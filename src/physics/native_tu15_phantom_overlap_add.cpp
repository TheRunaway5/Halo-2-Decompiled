#include "halo2/native_tu15_phantom_overlap_add.hpp"
#include "halo2/native_tu15_phantom_creation_activity.hpp"
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
    std::uint32_t word(std::uint32_t address)const {
        std::array<std::byte,4> raw{};memory_.read(address,raw);std::uint32_t result{};
        for(unsigned n=0;n<4;++n)result|=std::to_integer<unsigned>(raw[n])<<(n*8u);
        return result;
    }
    void word(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned width=4) {
        std::array<std::byte,4> raw{};for(unsigned n=0;n<width;++n)raw[n]=std::byte(value>>(n*8u));
        SourcePhysicsContactRecordStore event{pc,address,{},{}};
        if(observe_){event.before.resize(width);memory_.read(address,event.before);event.written.assign(raw.begin(),raw.begin()+width);}
        memory_.write(address,std::span(raw).first(width));if(observe_)observe_(event);
    }
};
bool nonnegative(std::uint32_t value){return std::bit_cast<std::int32_t>(value)>=0;}
void add(SourcePhysicsContactMemory& memory,Memory& m,std::uint32_t phantom,
    NativeTu15ContactPayload other,std::uint32_t flag_word,NativeTu15PhantomOverlapAddedEvent event,
    NativeTu15PhysicsFrameFp& fp,const NativeTu15PhantomOverlapAddServices& services,
    const SourcePhysicsContactRecordObserver& observe,bool simple) {
    if(!simple && !m.word(other.identity))return;
    const auto header=phantom+(simple?0x80u:0xc0u);
    bool present=false;
    if(std::uint8_t(flag_word)) {
        const auto count=m.word(header+4u);
        if(std::bit_cast<std::int32_t>(count)>0) {
            auto cursor=m.word(header);
            for(std::uint32_t n=0;std::bit_cast<std::int32_t>(n)<std::bit_cast<std::int32_t>(count);++n,cursor+=4u)
                if(m.word(cursor)==other.identity){present=true;break;}
        }
    }
    const auto result=native_tu15_notify_phantom_overlap_added(memory,phantom,other,
        std::uint8_t(present),event,fp,services,observe);
    if(result || present)return;
    const auto capacity=m.word(header+8u)&0x7fffffffu,count=m.word(header+4u);
    if(count==capacity) {
        NativeTu15ArrayGrowthMemory storage(memory,{header},observe);
        native_tu15_grow_array(storage,4u,fp,services.allocation);
        ++native_tu15_phantom_creation_counters.growths;
    }
    const auto current_count=m.word(header+4u),data=m.word(header),destination=data+current_count*4u;
    if(simple) {
        m.word(0x315385u,destination,other.identity);
        m.word(0x315388u,header+4u,m.word(header+4u)+1u);
    }else{
        m.word(0x315b8bu,header+4u,current_count+1u);
        m.word(0x315b8eu,destination,other.identity);
    }
    ++native_tu15_phantom_creation_counters.appends;
}
} // namespace

std::uint32_t native_tu15_notify_phantom_overlap_added(SourcePhysicsContactMemory& memory,
    std::uint32_t phantom,NativeTu15ContactPayload other,std::uint8_t was_present,
    NativeTu15PhantomOverlapAddedEvent event,NativeTu15PhysicsFrameFp& fp,
    const NativeTu15PhantomOverlapAddServices& services,const SourcePhysicsContactRecordObserver& observe) {
    ++native_tu15_phantom_creation_counters.notification_calls;
    Memory m(memory,observe);
    m.word(0x314f88u,event.identity+4u,other.identity);
    auto index=m.word(phantom+0x40u)-1u;
    m.word(0x314f9eu,event.identity+8u,was_present,1);
    m.word(0x314fa2u,event.identity,phantom);
    const auto initial=std::uint32_t(was_present!=0);
    m.word(0x314fa8u,event.identity+0xcu,initial);
    if(!nonnegative(index))return initial;
    do {
        const auto data=m.word(phantom+0x3cu),listener=m.word(data+index*4u);
        if(listener) {
            const auto table=m.word(listener);
            const NativeTu15ContactVirtualOwner owner{m.word(table)};
            if(!services.added)throw std::runtime_error("Native TU1.5 phantom overlap addition requires actual listener owner");
            services.added(listener,owner,event,fp);
        }
        --index;
    }while(nonnegative(index));
    return m.word(event.identity+0xcu);
}
void native_tu15_add_phantom_overlap(SourcePhysicsContactMemory& memory,std::uint32_t phantom,
    NativeTu15ContactPayload other,std::uint32_t flag_word,NativeTu15PhantomOverlapAddedEvent event,
    NativeTu15PhysicsFrameFp& fp,const NativeTu15PhantomOverlapAddServices& services,
    const SourcePhysicsContactRecordObserver& observe) {
    ++native_tu15_phantom_creation_counters.complex_calls;
    Memory m(memory,observe);add(memory,m,phantom,other,flag_word,event,fp,services,observe,false);
}
void native_tu15_add_simple_phantom_overlap(SourcePhysicsContactMemory& memory,std::uint32_t phantom,
    NativeTu15ContactPayload other,std::uint32_t flag_word,NativeTu15PhantomOverlapAddedEvent event,
    NativeTu15PhysicsFrameFp& fp,const NativeTu15PhantomOverlapAddServices& services,
    const SourcePhysicsContactRecordObserver& observe) {
    ++native_tu15_phantom_creation_counters.simple_calls;
    Memory m(memory,observe);add(memory,m,phantom,other,flag_word,event,fp,services,observe,true);
}
} // namespace halo2
