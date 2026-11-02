#include "halo2/native_tu15_mopp_aabb_traversal.hpp"
#include <bit>
#include <stdexcept>
namespace halo2 {
namespace {
class Memory {
    SourcePhysicsContactMemory&memory_;const SourcePhysicsContactRecordObserver&observe_;
public:
    Memory(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o):memory_(m),observe_(o){}
    std::uint32_t word(std::uint32_t p,unsigned width=4)const{
        std::array<std::byte,4>b{};memory_.read(p,std::span(b).first(width));std::uint32_t value{};
        for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(b[n])<<(n*8u);return value;
    }
    void word(std::uint32_t pc,std::uint32_t p,std::uint32_t value){
        SourcePhysicsContactRecordStore event{pc,p,{},{}};
        if(observe_){event.before.resize(4);memory_.read(p,event.before);}
        std::array<std::byte,4>b{};for(unsigned n=0;n<4;++n)b[n]=std::byte(value>>(n*8u));
        memory_.write(p,b);if(observe_){event.written.assign(b.begin(),b.end());observe_(event);}
    }
};
std::int32_t signed32(std::uint32_t n){return std::bit_cast<std::int32_t>(n);}
std::uint32_t shift_right(std::uint32_t n,std::uint32_t count){return std::uint32_t(signed32(n)>>(count&31u));}
class Traversal {
    SourcePhysicsContactMemory&storage_;Memory m_;std::uint32_t cache_;
    NativeTu15PhysicsFrameFp&fp_;const NativeTu15MoppAabbTraversalServices&services_;
    const SourcePhysicsContactRecordObserver&observe_;NativeTu15MoppAabbTraversalStats*stats_;
    std::uint32_t byte(std::uint32_t p){return m_.word(p,1);}
    std::uint32_t big_endian(std::uint32_t p,unsigned bytes){
        std::uint32_t n{};for(unsigned k=0;k<bytes;++k)n=(n<<8u)+byte(p+k);return n;
    }
    void leaf(std::uint32_t key,const NativeTu15MoppAabbVariables&v){
        if(stats_)++stats_->leaves;
        key+=v.translation_and_properties[3];const auto array=m_.word(cache_);
        const auto count=m_.word(array+4u),capacity=m_.word(array+8u)&0x7fffffffu;
        if(count==capacity){
            NativeTu15ArrayGrowthMemory growth(storage_,{array},observe_);
            native_tu15_grow_array(growth,4u,fp_,services_.allocation);if(stats_)++stats_->growths;
        }
        const auto index=m_.word(array+4u),data=m_.word(array);
        m_.word(0x309f1bu,data+index*4u,key);
        const auto current=m_.word(array+4u);m_.word(0x309f24u,array+4u,current+1u);
    }
public:
    Traversal(SourcePhysicsContactMemory&m,std::uint32_t cache,NativeTu15PhysicsFrameFp&fp,
        const NativeTu15MoppAabbTraversalServices&s,const SourcePhysicsContactRecordObserver&o,
        NativeTu15MoppAabbTraversalStats*stats):storage_(m),m_(m,o),cache_(cache),fp_(fp),services_(s),observe_(o),stats_(stats){}
    void run(std::uint32_t program,const NativeTu15MoppAabbVariables&input,unsigned depth=0){
        if(depth>512u)throw std::runtime_error("Native MOPP AABB requires its recursion-capacity owner");
        auto variables=input;
        for(;;){
            const auto opcode=byte(program);if(stats_)++stats_->opcodes;
            if(opcode==0)return;
            if(opcode>=1u&&opcode<=4u){
                auto transformed=variables;auto&fields=transformed.translation_and_properties;
                fields[0]=(byte(program+1u)+variables.translation_and_properties[0])<<opcode;
                fields[1]=(byte(program+2u)+variables.translation_and_properties[1])<<opcode;
                fields[2]=(byte(program+3u)+variables.translation_and_properties[2])<<opcode;
                fields[4]=variables.translation_and_properties[4]+opcode;const auto shift=16u-fields[4];
                for(unsigned axis=0;axis<3;++axis)transformed.lower[axis]=shift_right(m_.word(cache_+0x20u+axis*4u),shift)-fields[axis];
                for(unsigned axis=0;axis<3;++axis)transformed.upper[axis]=shift_right(m_.word(cache_+0x10u+axis*4u),shift)+1u-fields[axis];
                variables=transformed;program+=4u;continue;
            }
            if(opcode>=5u&&opcode<=7u){const auto bytes=opcode-4u;const auto jump=big_endian(program+1u,bytes);program+=jump+bytes+1u;continue;}
            if(opcode==9u||opcode==10u||opcode==11u){
                const auto bytes=opcode==9u?1u:opcode==10u?2u:4u;const auto value=big_endian(program+1u,bytes);
                if(opcode==11u)variables.translation_and_properties[3]=value;else variables.translation_and_properties[3]+=value;
                program+=bytes+1u;continue;
            }
            if(opcode>=0x10u&&opcode<=0x1cu){
                std::uint32_t a,b,upper{},lower{},low{},high{};
                if(opcode<=0x12u){b=byte(program+2u);a=byte(program+1u);}
                else{a=byte(program+1u);b=byte(program+2u);}
                const auto&u=variables.upper;const auto&l=variables.lower;
                if(opcode<=0x12u){const auto axis=opcode-0x10u;upper=u[axis];lower=l[axis];low=a;high=b;}
                else{
                    const bool triple=opcode>=0x19u;
                    const auto bias=(opcode==0x14u||opcode==0x16u||opcode==0x18u)?255u:
                        (opcode==0x1au||opcode==0x1bu)?0x55u:opcode==0x1cu?0xaau:0u;
                    low=triple?(a-bias)*3u:a*2u-bias;high=triple?(b-bias)*3u:b*2u-bias;
                    switch(opcode){
                    case 0x13:upper=u[2]+u[1];lower=l[2]+l[1];break;
                    case 0x14:upper=u[1]-l[2];lower=l[1]-u[2];break;
                    case 0x15:upper=u[2]+u[0];lower=l[2]+l[0];break;
                    case 0x16:upper=u[0]-l[2];lower=l[0]-u[2];break;
                    case 0x17:upper=u[1]+u[0];lower=l[1]+l[0];break;
                    case 0x18:upper=u[0]-l[1];lower=l[0]-u[1];break;
                    case 0x19:upper=u[2]+u[1]+u[0];lower=l[2]+l[1]+l[0];break;
                    case 0x1a:upper=u[1]+u[0]-l[2];lower=l[1]+l[0]-u[2];break;
                    case 0x1b:upper=u[0]-l[1]+u[2];lower=l[0]-u[1]+l[2];break;
                    case 0x1c:upper=u[0]-l[2]-l[1];lower=l[0]-u[2]-u[1];break;
                    }
                }
                program+=4u;
                if(signed32(upper)<=signed32(high)){if(signed32(lower)>=signed32(low))return;continue;}
                const auto jump=byte(program-1u);
                if(signed32(lower)<signed32(low)){if(stats_)++stats_->recursive_calls;run(program,variables,depth+1u);}
                program+=jump;continue;
            }
            if(opcode>=0x20u&&opcode<=0x22u){
                const auto threshold=byte(program+1u),axis=opcode-0x20u;program+=3u;
                if(signed32(variables.upper[axis])<=signed32(threshold))continue;
                const auto jump=byte(program-1u);program+=jump;
                if(signed32(variables.lower[axis])<=signed32(threshold)){if(stats_)++stats_->recursive_calls;run(program-jump,variables,depth+1u);}
                continue;
            }
            if(opcode>=0x23u&&opcode<=0x25u){
                const auto a=byte(program+1u),left_low=byte(program+4u),b=byte(program+2u),axis=opcode-0x23u;
                const auto left=(byte(program+3u)<<8u)+left_low;program+=7u;
                if(signed32(variables.upper[axis])<=signed32(b)){
                    if(signed32(variables.lower[axis])>=signed32(a))return;program+=left;continue;
                }
                const auto right_high=byte(program-2u),right_low=byte(program-1u);
                const auto right=(right_high<<8u)+right_low;program+=right;
                if(signed32(variables.lower[axis])<signed32(a)){if(stats_)++stats_->recursive_calls;run(program+left-right,variables,depth+1u);}
                continue;
            }
            if(opcode>=0x26u&&opcode<=0x28u){
                const auto a=byte(program+1u),axis=opcode-0x26u;
                if(signed32(variables.upper[axis])<signed32(a))return;
                const auto b=byte(program+2u);if(signed32(variables.lower[axis])>=signed32(b))return;
                program+=3u;continue;
            }
            if(opcode>=0x29u&&opcode<=0x2bu){
                const auto a=big_endian(program+1u,3),axis=opcode-0x29u;
                if(signed32(m_.word(cache_+0x10u+axis*4u))<signed32(a))return;
                const auto b=big_endian(program+4u,3);
                if(signed32(m_.word(cache_+0x20u+axis*4u))>signed32(b))return;
                program+=7u;continue;
            }
            if(opcode>=0x30u&&opcode<=0x53u){
                const auto key=opcode<0x50u?opcode-0x30u:big_endian(program+1u,opcode-0x4fu);
                leaf(key,variables);return;
            }
            if(opcode>=0x60u&&opcode<=0x6bu){
                if(opcode&3u)throw std::runtime_error("Native MOPP AABB requires its additional property-slot lifetime owner");
                const auto bytes=opcode<0x64u?1u:opcode<0x68u?2u:4u;const auto value=big_endian(program+1u,bytes);
                // Only the first property lies in the copied14-word query
                // variables. Other slots reach caller/control lifetime in
                // this Source frame; do not invent native storage for them.
                variables.translation_and_properties[5]=value;
                program+=bytes+1u;continue;
            }
            throw std::runtime_error("Native MOPP AABB requires its invalid-program diagnostic owner");
        }
    }
};
}
void native_tu15_traverse_mopp_aabb(SourcePhysicsContactMemory&memory,
    std::uint32_t cache,std::uint32_t program,const NativeTu15MoppAabbVariables&variables,
    NativeTu15PhysicsFrameFp&fp,unsigned depth,const NativeTu15MoppAabbTraversalServices&services,
    const SourcePhysicsContactRecordObserver&observe,NativeTu15MoppAabbTraversalStats*stats){
    if(stats)++stats->calls;
    try{
        if(depth)throw std::runtime_error("Native MOPP AABB requires its nonempty x87 owner");
        Traversal(memory,cache,fp,services,observe,stats).run(program,variables);if(stats)++stats->completed;
    }catch(...){if(stats)++stats->refusals;throw;}
}
} // namespace halo2
