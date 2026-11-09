#pragma once
#include "halo2/native_tu15_datum_initialize_kernel.hpp"
#include "halo2/native_tu15_datum_specific_kernel.hpp"
#include "physics/runtime/physics_step_internal.hpp"
#include <cstdlib>

namespace halo2::native_datum_specific_detail {
struct Noop {template<class State>void operator()(State&,std::uint32_t)const{}};
enum class InitializationPhase {clear_stride,clear_word,clear_byte,salt,salt_record,salt_increment,salt_check,salt_reset};
inline bool initialize_reference_mode(){static const bool value=[]{const auto*p=std::getenv("HALO2_REFERENCE_DATUM_INITIALIZE");return p&&*p;}();return value;}

template<class State>bool admitted(State&s,std::uint32_t first,std::uint32_t last){
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15)return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if constexpr(requires{s.active_mmx;})if(s.active_mmx)return false;
    if(s.services.instruction){
        if(!s.literal_service_sites||!s.instruction_cursor||!s.instruction_cursor->position)return false;
        const auto*c=s.instruction_cursor;
        if((c->first_trace&&*c->first_trace)||(c->second_trace&&*c->second_trace))return false;
        for(const auto site:c->service_sites)if(site>=first&&site<=last)return false;
    }
    if(s.observe&&(!s.instruction_cursor||!s.instruction_cursor->observation_only_stores))return false;
    return true;
}

template<class State,class Before>struct Storage {
    State&s;Before&before;
    void at(std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;before(s,pc);}
    std::uint32_t word(std::uint32_t address,unsigned width=4){
        std::array<std::byte,4>bytes{};s.memory.read(address,std::span(bytes).first(width));std::uint32_t value{};
        for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bytes[n])<<(n*8u);return value;
    }
    SourcePhysicsContactRecordStore write(std::uint32_t pc,std::uint32_t address,std::uint32_t value,unsigned width){
        SourcePhysicsContactRecordStore event{pc,address,{},std::vector<std::byte>(width)};
        if(s.observe){event.before.resize(width);s.memory.read(address,event.before);}
        for(unsigned n=0;n<width;++n)event.written[n]=std::byte(value>>(n*8u));s.memory.write(address,event.written);return event;
    }
    void observe(const SourcePhysicsContactRecordStore&event){if(s.observe)s.observe(event);}
    void push(std::uint32_t pc,std::uint32_t value){
        at(pc);const auto address=s.gp[4]-4u;auto event=write(pc,address,value,4);s.gp[4]=address;observe(event);
    }
    std::uint32_t pop(std::uint32_t pc){at(pc);const auto value=word(s.gp[4]);s.gp[4]+=4u;return value;}
};

// Record initialization is the shared typed clear/generation algorithm. This
// adapter carries only its authored frame, flag, and observable I/O boundaries.
template<class State,class Before>struct Initializer:Storage<State,Before> {
    using Base=Storage<State,Before>;using Base::s;using Base::at;using Base::word;
    using P=InitializationPhase;
    std::uint32_t bytes{};bool tail{},popped{};SourcePhysicsContactRecordStore event{};
    Initializer(State&state,Before&callback):Base{state,callback}{}
    void begin_tail(const NativeTu15DatumRecordInitialization&d){
        if(tail)return;
        if(!(bytes>>2u))at(0x16e86eu);
        s.gp[1]=bytes&3u;s.flags.logic(s.gp[1],4);tail=true;
        if(!s.gp[1])at(0x16e875u);
    }
    void saved_return(const NativeTu15DatumRecordInitialization&d){
        if(popped)return;
        s.flags.minus(d.salt_check,0xffffu,2);s.gp[7]=this->pop(0x16e887u);s.gp[3]=this->pop(0x16e888u);popped=true;
    }
    std::uint32_t read(P phase,const NativeTu15DatumRecordInitialization&d,std::uint32_t address,unsigned width){
        if(phase==P::clear_stride)return bytes;
        if(phase==P::salt){begin_tail(d);at(0x16e877u);}
        else if(phase==P::salt_increment)at(0x16e87eu);
        else if(phase==P::salt_check)at(0x16e882u);
        else throw std::runtime_error("Unknown datum initializer read phase");
        return word(address,width);
    }
    void write(P phase,const NativeTu15DatumRecordInitialization&d,std::uint32_t address,
        std::uint32_t value,unsigned width,std::uint32_t,bool){
        std::uint32_t pc{};
        switch(phase){
        case P::clear_word:pc=0x16e86eu;at(pc);break;
        case P::clear_byte:begin_tail(d);pc=0x16e875u;at(pc);break;
        case P::salt_record:s.gp[0]=(s.gp[0]&0xffff0000u)|d.salt;pc=0x16e87bu;at(pc);break;
        case P::salt_increment:pc=0x16e87eu;break;
        case P::salt_reset:saved_return(d);pc=0x16e88bu;at(pc);break;
        default:throw std::runtime_error("Unknown datum initializer write phase");
        }
        event=Base::write(pc,address,value,width);
    }
    void complete(P phase,const NativeTu15DatumRecordInitialization&d){
        if(phase==P::clear_word){s.gp[1]=d.word_count;s.gp[7]=d.clear_at;}
        else if(phase==P::clear_byte){s.gp[1]=d.tail_count;s.gp[7]=d.clear_at;}
        else if(phase==P::salt_increment)s.flags.increment(d.increment_old,2,false);
        this->observe(event);
    }
};

template<class State,class Before=Noop>bool initialize(State&s,Before before={}){
    if(initialize_reference_mode()||!admitted(s,0x16e860u,0x16e891u))return false;
    Initializer<State,Before>memory{s,before};NativeTu15DatumRecordInitialization data;data.pool=s.gp[2];data.record=s.gp[6];
    memory.at(0x16e860u);s.gp[1]=memory.word(s.gp[2]+0x24u);memory.bytes=s.gp[1];
    memory.push(0x16e863u,s.gp[3]);s.gp[3]=s.gp[1];memory.push(0x16e866u,s.gp[7]);
    s.gp[1]>>=2u;s.gp[0]=s.flags.logic(0,4);s.gp[7]=s.gp[6];
    native_tu15_initialize_datum_record<InitializationPhase>(memory,data,(s.other_flags&0x400u)!=0);
    memory.saved_return(data);memory.pop(0x16e891u);return true;
}

inline bool specific_reference_mode(){static const bool value=[]{const auto*p=std::getenv("HALO2_REFERENCE_DATUM_SPECIFIC");return p&&*p;}();return value;}
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
inline std::uint32_t ordered_shift(std::uint32_t value,unsigned count,std::uint32_t context,bool arithmetic=false){
    auto flags=((context&0xd5u)<<8u)|((context>>11u)&1u);auto shift=count;
    if(arithmetic)asm volatile("addb $127,%%al;sahf;sarl %%cl,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
        :"+a"(flags),"+c"(shift),"+r"(value)::"cc");
    else asm volatile("addb $127,%%al;sahf;shll %%cl,%2;seto %%cl;lahf;movzbl %%ah,%%eax"
        :"+a"(flags),"+c"(shift),"+r"(value)::"cc");
    return(flags&0xd5u)|((shift&1u)<<11u);
}
#endif
template<class State,class Call,class Before>struct Specific:Storage<State,Before>{
    using Base=Storage<State,Before>;using Base::s;using Base::at;using Base::word;using P=NativeTu15DatumSpecificPhase;
    Call&call;SourcePhysicsContactRecordStore event{};
    Specific(State&state,Call&owner,Before&callback):Base{state,callback},call(owner){}
    std::uint32_t read(P phase,const NativeTu15DatumSpecific&d,std::uint32_t address,unsigned width){
        auto&g=s.gp;
        switch(phase){
        case P::capacity:at(0x16e783u);break;
        case P::rows:s.flags.minus(d.index,d.capacity,4);at(0x16e788u);break;
        case P::stride:g[1]=d.rows;this->push(0x16e78bu,g[6]);at(0x16e78cu);break;
        case P::record_tag:g[6]=d.record;s.flags.plus(d.product,d.rows,4);at(0x16e794u);break;
        case P::bitmap:s.flags.minus(d.tag,0,2);at(0x16e79au);break;
        case P::occupancy_word:
            g[1]=d.bitmap;g[0]=d.occupied_word;s.flags.load(ordered_shift(d.index,5u,s.flags.bits(),true));this->push(0x16e7a5u,g[5]);
            g[1]=d.index&31u;s.flags.logic(g[1],4);g[5]=d.mask;s.flags.load(ordered_shift(1u,g[1],s.flags.bits()));at(0x16e7b2u);break;
        case P::used:at(0x16e7b4u);break;
        case P::extent:g[1]=d.used;at(0x16e7b7u);break;
        default:throw std::runtime_error("Unknown specific datum read phase");
        }
        return word(address,width);
    }
    void write(P phase,const NativeTu15DatumSpecific&d,std::uint32_t address,std::uint32_t value,unsigned width){
        std::uint32_t pc{};auto&g=s.gp;
        switch(phase){
        case P::occupancy:pc=0x16e7b2u;break;
        case P::used_count:g[1]=d.next_used;g[0]=d.extent;s.flags.minus(d.index,d.extent,4);pc=0x16e7bdu;at(pc);break;
        case P::grown_extent:g[0]=d.next_index;pc=0x16e7c6u;at(pc);break;
        case P::requested_salt:g[0]=d.issued_generation;pc=0x16e7d1u;at(pc);break;
        default:throw std::runtime_error("Unknown specific datum write phase");
        }
        event=Base::write(pc,address,value,width);
    }
    void complete(P phase,const NativeTu15DatumSpecific&d){
        if(phase==P::occupancy)s.flags.logic(d.new_word,4);
        this->observe(event);if(phase==P::used_count)s.gp[5]=this->pop(0x16e7c0u);
    }
    NativeTu15DatumSpecificResume initialize(const NativeTu15DatumSpecific&){
        this->push(0x16e7c9u,0x16e7ceu);
        if(!native_datum_specific_detail::initialize(s,this->before))call(s,0x16e860u,0x16e7c9u,false);
        at(0x16e7ceu);
        return{s.gp[2],s.gp[6],s.gp[7],s.gp[3]};
    }
    void finished(const NativeTu15DatumSpecific&d){
        auto&g=s.gp;
        if(d.success){
            g[0]=d.issued_generation<<16u;s.flags.load(ordered_shift(d.issued_generation,16u,s.flags.bits()));g[6]=this->pop(0x16e7d7u);
            g[0]=s.flags.logic(d.handle,4);g[7]=this->pop(0x16e7dau);g[3]=this->pop(0x16e7dbu);this->pop(0x16e7dcu);return;
        }
        const bool occupied=d.failure==NativeTu15DatumSpecificFailure::occupied;
        if(occupied){s.flags.minus(d.tag,0,2);g[6]=this->pop(0x16e7ddu);g[7]=this->pop(0x16e7deu);}
        else {if(d.failure==NativeTu15DatumSpecificFailure::bounds)s.flags.minus(d.index,d.capacity,4);g[7]=this->pop(0x16e7e4u);}
        g[0]=s.flags.logic(0xffffffffu,4);g[3]=this->pop(occupied?0x16e7e2u:0x16e7e8u);this->pop(occupied?0x16e7e3u:0x16e7e9u);
    }
};
template<class State,class Call,class Before=Noop>bool bridge(State&s,Call call,Before before={}){
#if !(defined(__i386__) || defined(__x86_64__)) || defined(_MSC_VER)
    return false;
#else
    if(specific_reference_mode()||!admitted(s,0x16e770u,0x16e7e9u))return false;
    Specific<State,Call,Before>memory{s,call,before};NativeTu15DatumSpecific data;data.pool=s.gp[2];data.requested=s.gp[0];
    memory.push(0x16e770u,s.gp[3]);memory.push(0x16e771u,s.gp[7]);s.gp[7]=data.requested&0xffffu;
    s.gp[0]=std::uint32_t(std::bit_cast<std::int32_t>(data.requested)>>16u);s.flags.logic(s.gp[7],4);s.gp[3]=s.gp[0];
    native_tu15_allocate_specific_datum(memory,data,[&](const auto&d){return memory.initialize(d);});return true;
#endif
}
} // namespace halo2::native_datum_specific_detail
