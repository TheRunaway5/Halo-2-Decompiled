#pragma once
#include "halo2/native_tu15_mopp_aabb_traversal_kernel.hpp"
#include "halo2/physics_contact_runtime.hpp"
#include <cstdlib>

namespace halo2::native_mopp_aabb_abi_detail {
using namespace native_mopp_aabb_detail;
inline bool reference_mode(){static const bool enabled=[]{const auto*p=std::getenv("HALO2_REFERENCE_MOPP_AABB");return p&&*p;}();return enabled;}
struct Activity {std::uint64_t calls{},recursive_calls{},growths{},diagnostics{};};
inline Activity&activity(){thread_local Activity value;return value;}
struct Noop {template<class State>void operator()(State&,std::uint32_t)const{}};
template<class State,class Call,class Before=Noop>bool bridge(State&,Call,Before={});
template<class State>std::uint32_t observation_pc(const State&s,std::uint32_t actual){
    if(s.image==SourcePhysicsContactImage::MapPack11)return actual-0x3b00u;
    if constexpr(requires{s.selected_observation_pcs;})return s.selected_observation_pcs?actual:actual-0x3b00u;
    return actual-0x3b00u;
}
template<class State>std::uint32_t image_operand(const State&s,std::uint32_t tu15,std::uint32_t legacy){return s.image==SourcePhysicsContactImage::MapPack11?legacy:tu15;}

// This is the immutable DATA-format class mapping, used only to publish EAX
// at the retained boundary. Geometric control belongs to the typed kernel.
inline std::uint32_t source_class(std::uint32_t op){
    if(!op)return 0;if(op<=4)return 1;if(op<=7)return op-3;if(op>=9&&op<=11)return op-4;
    if(op>=0x10&&op<=0x12)return 8;if(op>=0x13&&op<=0x1c)return op-10;
    if(op>=0x20&&op<=0x22)return 19;if(op>=0x23&&op<=0x25)return 20;
    if(op>=0x26&&op<=0x28)return 21;if(op>=0x29&&op<=0x2b)return 22;
    if(op>=0x30&&op<=0x4f)return 23;if(op>=0x50&&op<=0x53)return op-0x38;
    if(op>=0x60&&op<=0x6b)return 28+(op-0x60)/4;return 31;
}
template<class State>std::uint32_t shifted(State&s,std::uint32_t value,unsigned count,bool right=false){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    const auto context=s.flags.bits();auto flags=((context&0xd5u)<<8u)|((context>>11u)&1u);auto shift=count;
    if(right)asm volatile("addb $127,%%al;sahf;sarl %%cl,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
        :"+a"(flags),"+c"(shift),"+r"(value)::"cc");
    else asm volatile("addb $127,%%al;sahf;shll %%cl,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
        :"+a"(flags),"+c"(shift),"+r"(value)::"cc");
    s.flags.load((flags&0xd5u)|((shift&1u)<<11u));return value;
#else
    return right?s.flags.right(value,count,4,true):s.flags.shift(value,count,4);
#endif
}
template<unsigned Count,class State>std::uint32_t shifted_immediate(State&s,std::uint32_t value){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    const auto context=s.flags.bits();auto flags=((context&0xd5u)<<8u)|((context>>11u)&1u);unsigned overflow{};
    asm volatile("addb $127,%%al;sahf;shll %3,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
        :"+a"(flags),"+c"(overflow),"+r"(value):"I"(Count):"cc");
    s.flags.load((flags&0xd5u)|((overflow&1u)<<11u));return value;
#else
    return s.flags.shift(value,Count,4);
#endif
}
template<class State,class Call,class Before>class Frame {
    State&s_;Call&call_;Before&before_;bool first_{true};
    std::uint32_t word(std::uint32_t address,unsigned width=4)const{
        std::array<std::byte,4>bytes{};s_.memory.read(address,std::span(bytes).first(width));std::uint32_t value{};
        for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);return value;
    }
    void at(std::uint32_t pc){const auto published=observation_pc(s_,pc);if(s_.instruction_cursor&&s_.instruction_cursor->position)*s_.instruction_cursor->position=published;before_(s_,published);}
    std::uint32_t load(unsigned reg,std::uint32_t pc,std::uint32_t address,unsigned width=4){at(pc);return s_.gp[reg]=word(address,width);}
    void push(std::uint32_t pc,std::uint32_t value,bool positioned=false){const auto address=s_.gp[4]-4u;committed_store(pc,address,value,[&]{s_.gp[4]=address;},positioned);}
    std::uint32_t pop(std::uint32_t pc){at(pc);return s_.pop();}
    template<class Commit>void committed_store(std::uint32_t pc,std::uint32_t address,std::uint32_t value,Commit commit,bool positioned=false){
        if(!positioned)at(pc);SourcePhysicsContactRecordStore event{observation_pc(s_,pc),address,{},{}};
        if(s_.observe){event.before.resize(4);s_.memory.read(address,event.before);}
        std::array<std::byte,4>bytes{};for(unsigned n=0;n<4;++n)bytes[n]=std::byte(value>>(8u*n));s_.memory.write(address,bytes);
        commit();
        if(s_.observe){event.written.assign(bytes.begin(),bytes.end());s_.observe(event);}
    }
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t value,bool positioned=false){committed_store(pc,address,value,[]{},positioned);}
    void combine(unsigned reg,unsigned other){auto&g=s_.gp;g[reg]=shifted_immediate<8>(s_,g[reg]);g[reg]=s_.flags.plus(g[reg],g[other],4);}
    void copy_variables(std::uint32_t pc){
        auto&g=s_.gp;
        while(g[1]){
            at(pc);const auto value=word(g[6]);
            committed_store(pc,g[7],value,[&]{const auto delta=(s_.other_flags&1024u)?0xfffffffcu:4u;g[6]+=delta;g[7]+=delta;--g[1];},true);
        }
    }
    void finish(bool leaf){
        auto&g=s_.gp;
        if(leaf){g[5]=pop(0x309f27u);g[3]=pop(0x309f28u);g[4]=s_.flags.plus(g[4],0x38u,4);at(0x309f2cu);}
        else{g[7]=pop(0x309e97u);g[6]=pop(0x309e98u);g[5]=pop(0x309e99u);g[3]=pop(0x309e9au);g[4]=s_.flags.plus(g[4],0x38u,4);at(0x309e9eu);}
        s_.pop();g[4]+=8u;
    }
    std::uint32_t encoded(Progress&d){
        auto&g=s_.gp;const auto n=d.byte_index,offset=d.byte_offset;const auto address=d.program+offset;
        switch(d.encoding){
        case Encoding::scale:{constexpr std::array pc{0x309cbdu,0x309ccau,0x309cd7u};constexpr std::array reg{0u,2u,7u};return load(reg[n],pc[n],address,1);}
        case Encoding::jump:
            if(d.opcode==5)return load(0,0x309c7bu,address,1);
            if(d.opcode==6)return load(n?2:1,n?0x309c8cu:0x309c88u,address,1);
            return load(n==0?0:n==1?1:2,std::array{0x309c9eu,0x309ca2u,0x309ca6u}[n],address,1);
        case Encoding::key_base:
            if(d.opcode==9)return load(0,0x309d59u,address,1);
            if(d.opcode==10)return load(n?1:0,n?0x309d80u:0x309d7cu,address,1);
            if(n==3)combine(0,1);
            return load(n==0?0:n==2?2:1,std::array{0x309da8u,0x309dacu,0x309db0u,0x309db9u}[n],address,1);
        case Encoding::property:
            if(d.byte_count==1)return load(0,0x309de3u,address,1);
            if(d.byte_count==2)return load(n?2:0,n?0x309df7u:0x309df3u,address,1);
            if(n>=2)combine(0,2);
            return load(n?2:0,std::array{0x309e0cu,0x309e10u,0x309e19u,0x309e22u}[n],address,1);
        case Encoding::leaf:
            if(d.byte_count==1)return load(1,0x309ea1u,address,1);
            if(d.byte_count==2)return load(n?2:1,n?0x309eabu:0x309ea7u,address,1);
            if(d.byte_count==4&&n==3)combine(1,0);
            return load(n==0?1:n==2?2:0,(d.byte_count==3?std::array{0x309eb6u,0x309ebau,0x309ebeu,0u}:std::array{0x309eceu,0x309ed2u,0x309ed6u,0x309edfu})[n],address,1);
        case Encoding::wide_low:case Encoding::wide_high:
            if(n==2)combine(0,2);
            return load(n?2:0,(d.encoding==Encoding::wide_low?std::array{0x309c2du,0x309c31u,0x309c3au}:std::array{0x309c50u,0x309c54u,0x309c5du})[n],address,1);
        case Encoding::single:
            if(d.opcode<=0x22u){if(offset==0xffffffffu)return load(0,0x309b6au,address,1);return load(2,0x309b57u,address,1);}
            return load(offset==1u?0:2,offset==1u?0x309c03u:0x309c14u,address,1);
        case Encoding::offset:
            if(offset==0xfffffffeu)return load(1,0x309bd2u,address,1);
            if(offset==0xffffffffu)return load(2,0x309bd6u,address,1);
            return load(std::array{0u,7u,2u,0u}[n],std::array{0x309b8eu,0x309b92u,0x309b96u,0x309ba6u}[n],address,1);
        case Encoding::none:
            if(offset==0xffffffffu)return load(d.opcode<=0x12u?0:7,d.opcode<=0x12u?0x309b37u:0x309af8u,address,1);
            // Projection preparation owns the exact original PC and GP slot.
            return load(projection_destination_,projection_pc_,address,1);
        }
        throw std::runtime_error("Invalid native MOPP byte boundary");
    }
    std::uint32_t projection_pc_{};unsigned projection_destination_{};
    void projection_prepare(const Progress&d){
        auto&g=s_.gp;const auto op=d.opcode;const auto count=d.projection.component_count;const auto phase=d.projection_read;
        // Each row names the source's DATA reads, in order. Arithmetic at
        // the gaps publishes only the retained caller boundary state.
        if(op<=0x12u){
            if(phase==ProjectionRead::high_threshold){projection_pc_=0x309b15u;projection_destination_=0;}
            else if(phase==ProjectionRead::low_threshold){projection_pc_=0x309b19u;projection_destination_=2;}
            else if(phase==ProjectionRead::lower_component){projection_pc_=0x309b1du;projection_destination_=7;}
            else{g[3]=s_.flags.plus(g[3],4u,4);projection_pc_=0x309b24u;projection_destination_=8;}
            return;
        }
        const bool low=phase==ProjectionRead::low_threshold,high=phase==ProjectionRead::high_threshold;
        if(op==0x13u||op==0x15u||op==0x17u){
            const auto base=op==0x13u?0x30996eu:op==0x15u?0x3099b6u:0x3099fcu;
            if(low){projection_pc_=base+3;projection_destination_=2;}
            else if(high){projection_pc_=base+7;projection_destination_=7;}
            else if(count==0){projection_pc_=base;projection_destination_=1;}
            else if(count==1){projection_pc_=base+11;projection_destination_=0;}
            else if(count==2){g[0]=s_.flags.plus(g[0],g[1],4);projection_pc_=base+16;projection_destination_=1;}
            else{g[2]=shifted_immediate<1>(s_,g[2]);g[7]=shifted_immediate<1>(s_,g[7]);projection_pc_=base+23;projection_destination_=9;}
            return;
        }
        if(op==0x14u||op==0x16u){
            const auto base=op==0x14u?0x30998du:0x3099d4u;
            if(low){projection_pc_=base+6;projection_destination_=2;}
            else if(high){projection_pc_=base+10;projection_destination_=7;}
            else if(count==0){projection_pc_=base;projection_destination_=1;}
            else if(count==1){projection_pc_=base+3;projection_destination_=0;}
            else if(count==2){g[0]=s_.flags.minus(g[0],g[1],4);projection_pc_=base+16;projection_destination_=1;}
            else{projection_pc_=base+(op==0x14u?19u:18u);projection_destination_=10;}
            return;
        }
        if(op==0x18u){
            if(low){projection_pc_=0x309a1au;projection_destination_=2;}
            else if(high){projection_pc_=0x309a1eu;projection_destination_=7;}
            else if(count==0){projection_pc_=0x309a22u;projection_destination_=1;}
            else if(count==1){projection_pc_=0x309a25u;projection_destination_=0;}
            else if(count==2){g[0]=s_.flags.minus(g[0],g[1],4);projection_pc_=0x309a2au;projection_destination_=1;}
            else{g[2]=0xffffff01u+g[2]*2u;g[7]=0xffffff01u+g[7]*2u;projection_pc_=0x309adfu;projection_destination_=10;}
            return;
        }
        constexpr std::array<std::array<std::uint32_t,8>,4>pcs{{
            {0x309a3fu,0x309a49u,0x309a43u,0x309a50u,0x309a55u,0x309a58u,0x309a5bu,0x309a5eu},
            {0x309a65u,0x309a72u,0x309a69u,0x309a7cu,0x309a81u,0x309a84u,0x309a87u,0x309a8au},
            {0x309a8eu,0x309a9bu,0x309a92u,0x309aa5u,0x309aaau,0x309aadu,0x309ab0u,0x309ab3u},
            {0x309ab7u,0x309ac6u,0x309abbu,0x309ad2u,0x309ad7u,0x309adau,0x309adcu,0x309adfu}}};
        const auto&row=pcs[op-0x19u];
        const auto bias=op==0x19u?0u:op==0x1cu?0xaau:0x55u;
        if(low){projection_pc_=row[0];projection_destination_=0;}
        else if(high){if(bias)g[0]=s_.flags.minus(g[0],bias,4);g[2]=g[0]*3u;projection_pc_=row[1];projection_destination_=0;}
        else if(count==0){projection_pc_=row[2];projection_destination_=1;}
        else if(count==1){if(bias)g[0]=s_.flags.minus(g[0],bias,4);g[7]=g[0]*3u;projection_pc_=row[3];projection_destination_=0;}
        else if(count==2){g[0]=op==0x19u?s_.flags.plus(g[0],g[1],4):s_.flags.minus(g[0],g[1],4);projection_pc_=row[4];projection_destination_=op==0x1cu?11:12;}
        else if(count==3){projection_pc_=row[5];projection_destination_=1;}
        else if(count==4){projection_pc_=row[6];projection_destination_=op==0x19u?9:10;}
        else{projection_pc_=row[7];projection_destination_=op==0x1cu?10:9;}
    }
public:
    Frame(State&s,Call&call,Before&before):s_(s),call_(call),before_(before){}
    void begin(Progress&){at(0x309940u);}
    std::uint32_t read(Read phase,Progress&d,unsigned axis){
        auto&g=s_.gp;
        switch(phase){
        case Read::opcode:{
            if(first_){
                first_=false;g[4]=s_.flags.minus(g[4],0x38u,4);push(0x309943u,g[3]);load(3,0x309944u,g[4]+0x44u);
                push(0x309948u,g[5]);g[5]=g[1];load(1,0x30994bu,g[3],1);s_.flags.minus(g[1],0x6bu,4);
                push(0x309951u,g[6]);push(0x309952u,g[7]);if(g[1]<=0x6bu)load(6,0x309959u,g[4]+0x4cu);
            }else{
                if(d.program!=g[3]||d.cache!=g[5]||d.variables!=g[6])throw std::runtime_error("Native MOPP semantic continuation disagrees with its live frame");
                load(1,0x309e54u,g[3],1);s_.flags.minus(g[1],0x6bu,4);
            }
            d.program=g[3];d.cache=g[5];d.variables=g[6];return g[1];
        }
        case Read::encoded_byte:return encoded(d);
        case Read::projection_upper:case Read::projection_lower:{
            const auto address=d.variables+(phase==Read::projection_upper?0u:16u)+axis*4u;at(projection_pc_);const auto value=word(address);
            if(projection_destination_<8)g[projection_destination_]=value;
            else if(projection_destination_==8)s_.flags.minus(value,g[0],4);
            else if(projection_destination_==9)g[1]=s_.flags.plus(g[1],value,4);
            else if(projection_destination_==10)g[1]=s_.flags.minus(g[1],value,4);
            else if(projection_destination_==11)g[0]=s_.flags.minus(g[0],value,4);
            else g[0]=s_.flags.plus(g[0],value,4);return value;
        }
        case Read::scale_translation:{
            constexpr std::array pc{0x309cc1u,0x309cceu,0x309cdbu};constexpr std::array reg{0u,2u,7u};at(pc[axis]);const auto value=word(d.variables+0x20u+axis*4u);g[reg[axis]]=s_.flags.plus(g[reg[axis]],value,4);return value;
        }
        case Read::scale_shift:{const auto value=load(7,0x309ce4u,d.variables+0x30u);g[7]=s_.flags.plus(g[7],g[1],4);return value;}
        case Read::scale_lower:{constexpr std::array pc{0x309cf4u,0x309cffu,0x309d0au};const auto value=load(7,pc[axis],d.cache+0x20u+axis*4u);g[7]=shifted(s_,g[7],g[1],true);if(axis<2)g[7]=s_.flags.minus(g[7],g[axis?2:0],4);return value;}
        case Read::scale_local_z_lower:{at(0x309d0fu);const auto value=word(g[4]+0x38u);g[7]=s_.flags.minus(g[7],value,4);return value;}
        case Read::scale_upper:{constexpr std::array pc{0x309d17u,0x309d1fu,0x309d2fu};const auto reg=axis?0u:7u;const auto value=load(reg,pc[axis],d.cache+0x10u+axis*4u);g[reg]=shifted(s_,g[reg],g[1],true);if(!axis){g[7]=s_.flags.increment(g[7],4,false);g[7]=s_.flags.minus(g[7],g[0],4);}return value;}
        case Read::scale_local_z_upper:return load(1,0x309d34u,g[4]+0x38u);
        case Read::scale_property:return load(0,0x309d3fu,d.variables+0x34u);
        case Read::scale_key:return load(1,0x309d46u,d.variables+0x2cu);
        case Read::key_base:at(d.opcode==9u?0x309d70u:0x309d9cu);return word(g[4]+0x3cu);
        case Read::preserved_property:return load(0,0x309e35u,g[4]+0x44u);
        case Read::single_upper:return load(0,0x309b5bu,d.variables+axis*4u);
        case Read::single_lower:return load(7,0x309b6eu,d.variables+0x10u+axis*4u);
        case Read::offset_lower:return load(0,0x309b9eu,d.variables+0x10u+axis*4u);
        case Read::offset_upper:g[0]=shifted_immediate<8>(s_,g[0]);g[0]=s_.flags.plus(g[0],g[7],4);return load(7,0x309bafu,d.variables+axis*4u);
        case Read::offset_saved_low_left:return load(1,0x309bbdu,g[4]+0x50u);
        case Read::offset_saved_lower_left:{at(0x309bc1u);const auto value=word(g[4]+0x4cu);s_.flags.minus(value,g[1],4);return value;}
        case Read::offset_saved_lower_right:return load(7,0x309bdau,g[4]+0x4cu);
        case Read::offset_saved_low_right:g[1]=shifted_immediate<8>(s_,g[1]);g[1]=s_.flags.plus(g[1],g[2],4);return load(2,0x309be3u,g[4]+0x50u);
        case Read::slab_upper:{at(0x309c07u);const auto value=word(d.variables+axis*4u);s_.flags.minus(value,g[0],4);return value;}
        case Read::slab_lower:{at(0x309c18u);const auto value=word(d.variables+0x10u+axis*4u);s_.flags.minus(value,g[2],4);return value;}
        case Read::wide_upper:case Read::wide_lower:{combine(0,2);at(phase==Read::wide_upper?0x309c43u:0x309c66u);const auto value=word(d.cache+(phase==Read::wide_upper?0x10u:0x20u)+axis*4u);s_.flags.minus(value,g[0],4);return value;}
        case Read::leaf_base:return load(6,0x309ef2u,d.variables+0x2cu);
        case Read::leaf_array:return load(5,0x309ef5u,d.cache);
        case Read::leaf_count:return load(0,0x309ef8u,d.array+4u);
        case Read::leaf_capacity:g[6]=s_.flags.plus(g[6],g[1],4);return load(1,0x309efdu,d.array+8u);
        case Read::leaf_index:return load(2,0x309f15u,d.array+4u);
        case Read::leaf_data:return load(0,0x309f18u,d.array);
        case Read::leaf_current:return load(0,0x309f1eu,d.array+4u);
        }
        throw std::runtime_error("Invalid native MOPP retained read phase");
    }
    void write(Write phase,Progress&d,std::uint32_t value,unsigned axis){
        auto&g=s_.gp;
        switch(phase){
        case Write::scale_translation:{constexpr std::array reg{0u,2u,7u};g[reg[axis]]=shifted(s_,g[reg[axis]],g[1]);store(std::array{0x309cc6u,0x309cd3u,0x309ce0u}[axis],g[4]+0x30u+axis*4u,value);break;}
        case Write::scale_shift:store(0x309ce9u,g[4]+0x40u,value);break;
        case Write::scale_lower:store(std::array{0x309cfbu,0x309d06u,0x309d13u}[axis],g[4]+0x20u+axis*4u,value);break;
        case Write::scale_upper:if(axis){g[0]=s_.flags.increment(g[0],4,false);g[0]=s_.flags.minus(g[0],g[axis==1?2:1],4);}store(std::array{0x309d24u,0x309d2bu,0x309d3bu}[axis],g[4]+0x10u+axis*4u,value);break;
        case Write::scale_property:store(0x309d42u,g[4]+0x44u,value);break;
        case Write::scale_key:store(0x309d49u,g[4]+0x3cu,value);break;
        case Write::key_base:{
            if(d.opcode==11u)store(0x309ddau,g[4]+0x3cu,value);
            else{auto next=s_.flags;next.plus(d.base,g[0],4);committed_store(d.opcode==9u?0x309d70u:0x309d9cu,g[4]+0x3cu,value,[&]{s_.flags=next;},true);}break;
        }
        case Write::property:
            if(d.byte_count!=1)combine(0,2);
            store(d.byte_count==1?0x309deau:d.byte_count==2?0x309e00u:0x309e2bu,g[4]+0x44u+axis*4u,value);break;
        case Write::preserved_property:store(0x309e50u,g[4]+0x44u,value);break;
        case Write::offset_low:store(0x309b9au,g[4]+0x50u,value);break;
        case Write::offset_lower:store(0x309ba2u,g[4]+0x4cu,value);break;
        case Write::leaf_key:store(0x309f1bu,d.data+d.count*4u,value);break;
        case Write::leaf_count:store(0x309f24u,d.array+4u,value);break;
        }
    }
    void event(Event phase,Progress&d){
        auto&g=s_.gp;
        switch(phase){
        case Event::initial:break;
        case Event::decoded:if(d.opcode<=0x6bu)g[0]=source_class(d.opcode);break;
        case Event::projection_read:projection_prepare(d);break;
        case Event::projection_ready:
            if(d.opcode>=0x13u){if(d.opcode==0x14u||d.opcode==0x16u){g[2]=0xffffff01u+g[2]*2u;g[7]=0xffffff01u+g[7]*2u;}g[3]=s_.flags.plus(g[3],4u,4);s_.flags.minus(g[1],g[7],4);}
            if(signed_word(d.upper)<=signed_word(d.high))s_.flags.minus(d.opcode<=0x12u?g[7]:g[0],g[2],4);break;
        case Event::projection_jump:if(d.opcode>=0x13u)s_.flags.minus(g[0],g[2],4);break;
        case Event::single_ready:g[3]=s_.flags.plus(g[3],3u,4);s_.flags.minus(g[0],g[2],4);break;
        case Event::single_jump:break;
        case Event::single_test:g[3]=s_.flags.plus(g[3],g[0],4);s_.flags.minus(g[7],g[2],4);break;
        case Event::offset_ready:g[3]=s_.flags.plus(g[3],7u,4);s_.flags.minus(g[7],g[2],4);break;
        case Event::offset_left_test:break;
        case Event::offset_right_test:g[3]=s_.flags.plus(g[3],g[1],4);s_.flags.minus(g[7],g[2],4);break;
        case Event::slab_first:case Event::slab_second:case Event::wide_first:case Event::wide_second:break;
        case Event::scale_shift_ready:g[1]=s_.flags.minus(16u,g[7],4);break;
        case Event::scale_commit:g[6]=g[4]+0x10u;d.variables=g[6];break;
        case Event::key_local:{
            if(d.opcode==10u){g[0]=shifted_immediate<8>(s_,g[0]);g[2]=g[4]+0x10u;g[0]=s_.flags.plus(g[0],g[1],4);}
            else if(d.opcode==11u){combine(0,2);g[0]=shifted_immediate<8>(s_,g[0]);g[2]=g[4]+0x10u;g[0]=s_.flags.plus(g[0],g[1],4);}
            else g[2]=g[4]+0x10u;
            s_.flags.minus(g[6],g[2],4);if(g[6]!=g[2]){g[1]=14;g[7]=g[2];copy_variables(d.opcode==9u?0x309d6cu:d.opcode==10u?0x309d98u:0x309dd6u);g[6]=g[2];}d.variables=g[6];break;
        }
        case Event::property_begin:break;
        case Event::property_local:g[1]=g[4]+0x10u;s_.flags.minus(g[6],g[1],4);if(g[6]!=g[1]){g[1]=14;g[7]=g[4]+0x10u;copy_variables(0x309e4au);g[6]=g[4]+0x10u;}d.variables=g[6];break;
        case Event::leaf_begin:
            if(d.opcode<0x50u)g[1]=s_.flags.plus(g[1],0xffffffd0u,4);
            else if(d.byte_count==2)combine(1,2);
            else if(d.byte_count==3){combine(1,0);combine(1,2);}
            else if(d.byte_count==4){combine(1,2);combine(1,0);}break;
        case Event::leaf_test:g[1]=s_.flags.logic(g[1]&0x7fffffffu,4);s_.flags.minus(g[0],g[1],4);break;
        case Event::leaf_count_ready:g[7]=pop(0x309f21u);g[0]=s_.flags.increment(g[0],4,false);g[6]=pop(0x309f23u);break;
        case Event::finish:finish(false);break;
        case Event::leaf_finish:finish(true);break;
        case Event::advance:
            switch(d.flow){
            case Flow::scale:case Flow::key_base:case Flow::slab:case Flow::wide:case Flow::property:g[3]=s_.flags.plus(g[3],d.program-d.previous_program,4);break;
            case Flow::jump:
                if(d.opcode==5u)g[3]=g[3]+2u+g[0];
                else if(d.opcode==6u){g[1]=shifted_immediate<8>(s_,g[1]);g[2]=s_.flags.plus(g[2],g[3],4);g[3]=g[2]+g[1]+3u;}
                else{combine(0,1);g[0]=shifted_immediate<8>(s_,g[0]);g[2]=s_.flags.plus(g[2],g[3],4);g[3]=g[2]+g[0]+4u;}break;
            case Flow::project_right:g[3]=s_.flags.plus(g[3],g[7],4);break;
            case Flow::project_axis_right:g[3]=s_.flags.plus(g[3],g[0],4);s_.flags.minus(g[7],g[2],4);break;
            case Flow::offset_left:g[3]=s_.flags.plus(g[3],g[0],4);break;
            default:break;
            }break;
        }
    }
    Continuation recurse(Progress&d){
        auto&g=s_.gp;std::uint32_t pc{},continuation{},argument{};
        switch(d.flow){
        case Flow::project_recursed:pc=0x309b09u;continuation=0x309b0eu;argument=g[3];push(0x309b05u,argument);push(0x309b06u,g[6]);break;
        case Flow::project_axis_recursed:g[1]=s_.flags.minus(g[3],g[0],4);pc=0x309b4du;continuation=0x309b52u;push(0x309b49u,g[1]);push(0x309b4au,g[6]);break;
        case Flow::single_recursed:g[2]=s_.flags.minus(g[3],g[0],4);pc=0x309b84u;continuation=0x309b89u;push(0x309b80u,g[2]);push(0x309b81u,g[6]);break;
        case Flow::offset_recursed:g[0]=s_.flags.minus(g[0],g[1],4);g[0]=s_.flags.plus(g[0],g[3],4);pc=0x309bf9u;continuation=0x309bfeu;push(0x309bf5u,g[0]);push(0x309bf6u,g[6]);break;
        default:throw std::runtime_error("Invalid native MOPP recursive boundary");
        }
        g[1]=g[5];push(pc,image_operand(s_,continuation,continuation-0x3b00u));++activity().recursive_calls;call_(s_,image_operand(s_,0x309940u,0x305e40u),pc-0x3b00u,false);
        return{g[5],g[6],g[3],g[7]};
    }
    Continuation grow(Progress&){
        auto&g=s_.gp;push(0x309f0au,4u);push(0x309f0cu,g[5]);push(0x309f0du,image_operand(s_,0x309f12u,0x306412u));++activity().growths;
        call_(s_,image_operand(s_,0x2e3250u,0x2df780u),0x30640du,false);g[4]=s_.flags.plus(g[4],8u,4);Continuation resumed;resumed.key=g[6];resumed.array=g[5];return resumed;
    }
    void diagnostic(Progress&){
        auto&g=s_.gp;++activity().diagnostics;const auto logger=image_operand(s_,0x48c99cu,0x48769cu);load(0,0x309e60u,logger);load(2,0x309e65u,g[0]);
        push(0x309e67u,g[1]);push(0x309e68u,image_operand(s_,0x41c4d4u,0x418a34u));g[1]=g[0];at(0x309e6fu);const auto first=word(g[2]+4u);push(0x309e6fu,image_operand(s_,0x309e72u,0x306372u),true);call_(s_,first,0x30636fu,false);
        push(0x309e72u,g[0]);push(0x309e73u,image_operand(s_,0x309e78u,0x306378u));call_(s_,image_operand(s_,0x2e3080u,0x2df5b0u),0x306373u,false);
        load(1,0x309e78u,logger);load(0,0x309e7eu,g[1]);g[4]=s_.flags.plus(g[4],12u,4);
        at(0x309e83u);const auto second=word(g[0]+4u);push(0x309e83u,image_operand(s_,0x309e86u,0x306386u),true);call_(s_,second,0x306383u,false);
        g[1]=g[0];push(0x309e88u,image_operand(s_,0x309e8du,0x30638du));call_(s_,image_operand(s_,0x2e30e0u,0x2df610u),0x306388u,false);store(0x309e8du,0,0);
    }
};
template<bool CanonicalState,class State,class Call,class Before>bool bridge_policy(State&s,Call call,Before before){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    return false;
#else
    if(reference_mode()||(s.image!=SourcePhysicsContactImage::TitleUpdate15&&s.image!=SourcePhysicsContactImage::MapPack11))return false;
    if constexpr(!requires{s.selected_observation_pcs;})if constexpr(!CanonicalState)return false;
    if(s.observe&&(!s.instruction_cursor||!s.instruction_cursor->observation_only_stores))return false;
    if(s.services.instruction){
        if(!s.literal_service_sites||!s.instruction_cursor||!s.instruction_cursor->position)return false;
        const auto*c=s.instruction_cursor;
        if((c->first_trace&&*c->first_trace)||(c->second_trace&&*c->second_trace))return false;
        if(s.observe&&!c->observation_only_stores)return false;
        for(const auto site:c->service_sites)if((site>=0x309940u&&site<=0x309f2cu)||(site>=0x305e40u&&site<=0x30642cu))return false;
    }
    ++activity().calls;Frame<State,Call,Before> frame(s,call,before);native_mopp_aabb_detail::Traversal<decltype(frame)>(frame).run();return true;
#endif
}
template<class State,class Call,class Before>bool bridge(State&s,Call call,Before before){return bridge_policy<false>(s,call,before);}
template<class State,class Call,class Before=Noop>bool bridge_canonical(State&s,Call call,Before before={}){return bridge_policy<true>(s,call,before);}
} // namespace halo2::native_mopp_aabb_abi_detail
