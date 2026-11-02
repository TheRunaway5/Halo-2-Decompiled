#include "halo2/native_tu15_light_receiver_flags.hpp"
#include <algorithm>
#include <bit>

namespace halo2 {
namespace {
std::uint32_t read(const SourcePhysicsContactMemory& m,std::uint32_t at,unsigned size=4) {
    std::array<std::byte,4> bytes{};m.read(at,std::span(bytes).first(size));std::uint32_t value{};
    for(unsigned i=0;i<size;++i)value|=std::to_integer<unsigned>(bytes[i])<<(8*i);return value;
}
std::int32_t count(const SourcePhysicsContactMemory& m,std::uint32_t queue) {
    return std::bit_cast<std::int16_t>(std::uint16_t(read(m,queue+4u,2)));
}
class Stores {
    SourcePhysicsContactMemory& memory_;
    const SourcePhysicsContactRecordObserver& observer_;
public:
    Stores(SourcePhysicsContactMemory& m,const SourcePhysicsContactRecordObserver& o):memory_(m),observer_(o){}
    void operator()(std::uint32_t pc,std::uint32_t at,std::uint32_t value,unsigned size)const {
        std::array<std::byte,4> bytes{};for(unsigned i=0;i<size;++i)bytes[i]=std::byte(value>>(8*i));
        if(!observer_){memory_.write(at,std::span(bytes).first(size));return;}
        SourcePhysicsContactRecordStore event{pc,at,std::vector<std::byte>(size),std::vector<std::byte>(bytes.begin(),bytes.begin()+size)};
        // The diagnostic read is not an original operand. Borrowing the
        // current bytes keeps it out of source-read observations when possible.
        if(const auto before=memory_.try_read_view(at,size))std::copy(before->begin(),before->end(),event.before.begin());
        else memory_.read(at,event.before);
        memory_.write(at,event.written);observer_(event);
    }
};
std::uint32_t row_offset(std::uint32_t index,unsigned stride) {
    return std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(index))))*stride;
}
}
void native_tu15_light_receiver_flags_31650(SourcePhysicsContactMemory& m,
    const NativeTu15LightReceiverFlagCall& in,NativeTu15LightReceiverFlagResult& out,
    const SourcePhysicsContactRecordObserver& observe) {
    out={};const Stores store(m,observe);
    //31653..3168A: pin the two pointers before any queue edit. The owner is
    // read only after the first definition test, as in the original prologue.
    const auto pool=read(m,0x4ecbdcu),record=read(m,pool+0x44u)+0x110u*(in.light&65535u);
    const auto tags=read(m,0x4f0414u),definition=read(m,tags+16u*(in.definition&65535u)+8u);
    const bool environment=(read(m,definition)&0x10000u)!=0;
    const auto owner=read(m,in.stack+4u);
    const auto no_cast=[&](unsigned offset,std::uint32_t pc,bool objects) {
        auto queue=read(m,in.context+offset);std::uint32_t index{};
        if(count(m,queue)<=0)return;
        do {
            const auto at=read(m,queue+8u)+row_offset(index,2),before=read(m,at,2);
            const auto after=(before&0xdfffu)|0x1000u;store(pc,at,after,2);
            ++(objects?out.object_rows:out.bsp_rows);out.changed_rows+=before!=after;
            queue=read(m,in.context+offset);const auto bound=count(m,queue);++index;
            if(std::int32_t(index)>=bound)break;
        }while(true);
    };
    if(environment){no_cast(0xcu,0x316bau,false);no_cast(0x18u,0x316eeu,false);}
    if(read(m,definition)&0x20000u)no_cast(0x14u,0x3172au,true);
    const auto flags=read(m,definition);
    if(flags&0x3800u) {
        const auto bits=std::uint32_t((!(flags&0x800u)||!(flags&0x2000u)?0x1000u:0u)|
            (!(flags&0x1000u)||!(flags&0x2000u)?0x2000u:0u));
        const auto edit=[&](std::uint32_t selected,bool root) {
            auto queue=read(m,in.context+0x14u);std::uint32_t index{};
            const auto bound=count(m,queue);store(root?0x317ddu:0x31771u,in.stack+4u,0,4);
            out.owner_none_passes+=selected==0xffffffffu;
            if(bound<=0)return;
            do {
                if(selected==0xffffffffu || read(m,read(m,queue+0xcu)+row_offset(index,4))==selected) {
                    const auto at=read(m,queue+8u)+row_offset(index,2),before=read(m,at,2);
                    const auto after=(before&~0x3000u)|bits;store(root?0x31826u:0x317b9u,at,after,2);
                    ++out.object_rows;out.changed_rows+=before!=after;
                    if(root)selected=read(m,in.stack+8u);
                }
                queue=read(m,in.context+0x14u);const auto bound=count(m,queue);++index;
                store(root?0x31837u:0x317c6u,in.stack+4u,index,4);
                if(std::int32_t(index)>=bound)break;
            }while(true);
        };
        edit(owner,false);const auto root=read(m,in.stack+8u);
        if(root!=owner){++out.root_passes;edit(root,true);}
    }
    const auto clusters=read(m,in.context+0xcu);const auto cluster_count=count(m,clusters);
    if(cluster_count<=0 || !(read(m,definition,1)&4u) || cluster_count<=1)return;
    out.xmm0_word=read(m,0x46a0d0u);store(0x31867u,record+0xf8u,out.xmm0_word,4);out.f8_written=true;
    const auto shadow=read(m,in.stack+16u);
    if(shadow || read(m,in.stack+12u,1))return;
    // The sole real caller always passes a non-null shadow byte. Keep the
    // original null-pointer branch too, including its eventual source fault.
    auto queue=read(m,in.context+0xcu);std::uint32_t index{};
    if(count(m,queue)>0)do {
        const auto at=read(m,queue+8u)+row_offset(index,2)+1u,before=read(m,at,1);
        store(0x31896u,at,before|0x30u,1);++out.bsp_rows;out.changed_rows+=(before|0x30u)!=before;
        queue=read(m,in.context+0xcu);const auto bound=count(m,queue);++index;
        if(std::int32_t(index)>=bound)break;
    }while(true);
    store(0x318aau,shadow,1,1);
}
}
