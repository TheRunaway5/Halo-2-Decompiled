#pragma once
#include "halo2/native_tu15_async_queue_kernel.hpp"
#include "physics/runtime/physics_step_internal.hpp"
#include "physics/runtime/native_tu15_raw_x87_scope.hpp"
#include <cstdlib>
#include <thread>

namespace halo2::native_async_queue_abi_detail {
struct Noop {template<class State>void operator()(State&,std::uint32_t)const{}};
inline bool reference(){static const bool value=[]{const auto*p=std::getenv("HALO2_REFERENCE_ASYNC_QUEUE");return p&&*p;}();return value;}
template<class State>bool admitted(State&s){
    if(reference()||s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s))return false;
    if constexpr(requires{s.selected_observation_pcs;})if(!s.selected_observation_pcs)return false;
    if(!s.services.instruction&&(s.instruction_cursor||s.observe))return false;
    if(s.services.instruction){
        if(!s.literal_service_sites||!s.instruction_cursor||!s.instruction_cursor->position)return false;
        const auto*p=s.instruction_cursor;
        if(p!=&s.services.instruction_cursor||!p->callback_type||s.services.instruction.target_type()!=*p->callback_type||
           p->executor!=std::this_thread::get_id()||!p->literal_service_sites||p->service_sites.size()!=3||
           p->service_sites[0]!=0x38846eu||p->service_sites[1]!=0x21f590u||p->service_sites[2]!=0x33b595u)return false;
        if((p->first_trace&&*p->first_trace)||(p->second_trace&&*p->second_trace))return false;
    }
    if(s.observe&&(!s.instruction_cursor||!s.instruction_cursor->observation_only_stores))return false;
    return true;
}
template<class State,class Before>struct Storage {
    State&s;std::uint32_t base;Before&before;native_raw_x87_detail::Scope<State>&scope;
    void at(unsigned offset){const auto pc=base+offset;if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;scope.io([&]{before(s,pc);});}
    std::uint32_t word(std::uint32_t address,unsigned width=4){std::uint32_t value{};scope.io([&]{value=physics_step_detail::r(s.memory,address,width);});return value;}
    SourcePhysicsContactRecordStore write(unsigned offset,std::uint32_t address,std::uint32_t value,unsigned width=4){
        SourcePhysicsContactRecordStore event{base+offset,address,{},std::vector<std::byte>(width)};
        if(s.observe){event.before.resize(width);scope.io([&]{s.memory.read(address,event.before);});}
        for(unsigned n=0;n<width;++n)event.written[n]=std::byte(value>>(8u*n));scope.io([&]{s.memory.write(address,event.written);});return event;
    }
    void observe(const SourcePhysicsContactRecordStore&event){if(s.observe)scope.io([&]{s.observe(event);});}
    void store(unsigned offset,std::uint32_t address,std::uint32_t value,unsigned width=4){at(offset);auto event=write(offset,address,value,width);observe(event);}
    void push(unsigned offset,std::uint32_t value){at(offset);const auto address=s.gp[4]-4u;auto event=write(offset,address,value);s.gp[4]=address;observe(event);}
    std::uint32_t pop(unsigned offset){at(offset);const auto value=word(s.gp[4]);s.gp[4]+=4u;return value;}
};
template<class State,class Before>struct Order:Storage<State,Before>{
    using Base=Storage<State,Before>;using Base::s;using Base::at;using Base::word;using R=NativeAsyncOrderRead;
    Order(State&state,std::uint32_t site,Before&callback,native_raw_x87_detail::Scope<State>&scope):Base{state,site,callback,scope}{}
    std::uint32_t read(R kind,const NativeAsyncOrder&d){
        auto&g=s.gp;std::uint32_t address{};unsigned offset{},width=4;int destination=-1;
        switch(kind){
        case R::other_callback:this->push(0,g[5]);offset=1;address=g[4]+16u;destination=5;break;
        case R::history:
            g[0]&=0xffffff00u;s.flags.minus(g[2],g[1],4);this->push(9,g[6]);offset=10;address=g[4]+12u;destination=6;break;
        case R::incoming_callback:offset=0x1a;address=g[4]+16u;break;
        case R::valid:s.flags.minus(d.other_callback,d.read_callback,4);offset=0x3c;address=d.history+8u;width=1;break;
        case R::history_file:s.flags.logic(g[0]&255u,1);offset=0x47;address=d.history;destination=2;break;
        case R::incoming_file:offset=0x49;address=d.incoming;destination=0;break;
        case R::other_file:offset=0x4b;address=d.other;destination=1;break;
        case R::history_offset:
            g[0]=d.incoming_file_distance;g[1]=d.other_file_distance;s.flags.minus(g[0],g[1],4);
            offset=0x5f;address=d.history+4u;destination=0;break;
        case R::incoming_offset:offset=0x62;address=d.incoming+12u;destination=1;break;
        case R::other_offset:offset=0x65;address=d.other+12u;destination=2;break;
        case R::record_file:offset=0x79;address=d.other;destination=1;break;
        case R::record_offset:offset=0x7d;address=d.other+12u;destination=2;break;
        }
        at(offset);const auto value=word(address,width);if(destination>=0)g[unsigned(destination)]=value;
        else if(kind==R::valid)g[0]=(g[0]&0xffffff00u)|(value&255u);return value;
    }
    void decision(const NativeAsyncOrder&d){
        auto&g=s.gp;
        if(d.level==NativeAsyncOrderLevel::file){g[0]=d.incoming_file_distance;g[1]=d.other_file_distance;}
        else if(d.level==NativeAsyncOrderLevel::offset){g[0]=d.origin_offset;g[1]=d.incoming_offset_distance;g[2]=d.other_offset_distance;}
        g[0]=(g[0]&0xffffff00u)|std::uint32_t(d.before);s.flags.minus(d.other_callback,d.read_callback,4);
    }
    void write(NativeAsyncOrderWrite kind,const NativeAsyncOrder&d,std::uint32_t value){
        switch(kind){
        case NativeAsyncOrderWrite::file:this->store(0x7b,d.history,value);break;
        case NativeAsyncOrderWrite::offset:this->store(0x80,d.history+4u,value);break;
        case NativeAsyncOrderWrite::valid:this->store(0x83,d.history+8u,value,1);break;
        }
    }
    void finish(const NativeAsyncOrder&){s.gp[6]=this->pop(0x87);s.gp[5]=this->pop(0x88);this->pop(0x89);s.gp[4]+=12u;}
};
template<class State,class Before=Noop>bool compare(State&s,std::uint32_t site,Before before={}){
    if((site!=0x1a5080u&&site!=0x1ab330u)||!admitted(s))return false;
    NativeAsyncOrder data;data.incoming_priority=std::bit_cast<std::int32_t>(s.gp[2]);data.other_priority=std::bit_cast<std::int32_t>(s.gp[1]);
    data.incoming=s.gp[7];data.other=s.gp[3];data.read_callback=0x1aaa80u;
    native_raw_x87_detail::Scope scope{s};Order<State,Before>access{s,site,before,scope};native_async_before(access,data);return true;
}
template<class State,class Compare,class Before>struct Insertion:Storage<State,Before>{
    using Base=Storage<State,Before>;using Base::s;using Base::at;using Base::word;using R=NativeAsyncInsertRead;
    Compare&child;
    Insertion(State&state,std::uint32_t site,Compare&compare,Before&callback,native_raw_x87_detail::Scope<State>&scope):Base{state,site,callback,scope},child(compare){}
    static bool is_null(std::uint32_t value){return value==0;}
    static std::uint32_t null(){return 0;}
    void begin(const NativeAsyncInsertion&){
        s.gp[4]=s.flags.minus(s.gp[4],0x14u,4);s.gp[0]=s.flags.logic(0,4);
        this->store(5,s.gp[4]+8u,0);this->push(9,s.gp[5]);
    }
    void history(const NativeAsyncInsertion&d){s.flags.logic(d.head,4);this->store(0x12,s.gp[4]+0x10u,0);this->store(0x16,s.gp[4]+0x14u,0);}
    std::uint32_t read(R kind,const NativeAsyncInsertion&d){
        auto&g=s.gp;std::uint32_t address{};unsigned offset{};int destination=-1;
        switch(kind){
        case R::head:offset=0xa;address=0x4ef1bcu;destination=5;break;
        case R::incoming:offset=0x1a;address=g[4]+0x1cu;destination=0;break;
        case R::incoming_priority:offset=0x2f;address=d.incoming;destination=2;break;
        case R::head_priority:offset=0x31;address=d.head;break;
        case R::head_next:offset=0x49;address=d.head+0x3cu;destination=1;break;
        case R::incoming_callback:offset=0x53;address=d.incoming+0x34u;destination=1;break;
        case R::successor:offset=0x63;address=d.previous+0x3cu;destination=6;break;
        case R::successor_callback:offset=0x66;address=d.successor+0x34u;destination=0;break;
        case R::saved_callback:offset=0x69;address=g[4]+0x14u;destination=1;break;
        case R::successor_priority:
            this->push(0x6d,g[0]);this->push(0x6e,g[1]);offset=0x6f;address=d.successor;destination=1;break;
        case R::after_compare_next:g[5]=d.previous;offset=0x84;address=d.previous+0x3cu;destination=0;break;
        case R::saved_incoming:
            if(d.exit!=NativeAsyncInsertExit::before_successor)s.flags.logic(d.next,4);
            offset=d.exit==NativeAsyncInsertExit::before_successor?0xa6:0x8b;address=g[4]+0x28u;destination=0;break;
        case R::saved_priority:s.flags.logic(d.next,4);offset=0x5f;address=g[4]+0x10u;destination=2;break;
        case R::insertion_next:offset=0xa3;address=d.previous+0x3cu;destination=1;break;
        }
        at(offset);const auto value=word(address);if(destination>=0)g[unsigned(destination)]=value;
        if(kind==R::head_priority)s.flags.minus(g[2],value,4);return value;
    }
    void priority(const NativeAsyncInsertion&d){this->store(0x34,s.gp[4]+4u,d.incoming_priority);}
    void candidates(const NativeAsyncInsertion&d){
        s.flags.logic(d.next,4);this->push(0x4e,s.gp[3]);this->push(0x4f,s.gp[6]);this->push(0x50,s.gp[7]);
    }
    void callback(const NativeAsyncInsertion&d){s.gp[7]=d.incoming+8u;this->store(0x59,s.gp[4]+0x14u,d.incoming_callback);}
    NativeAsyncCompareResume compare(const NativeAsyncInsertion&d){
        s.gp[0]=s.gp[4]+0x20u;s.gp[3]=d.successor+8u;
        this->push(0x78,s.gp[0]);this->push(0x79,0x12333eu);this->scope.child([&]{child(s);});
        s.flags.logic(s.gp[0]&255u,1);return{bool(s.gp[0]&255u),s.gp[5],s.gp[6]};
    }
    void restore_candidates(const NativeAsyncInsertion&d){
        const bool before=d.exit==NativeAsyncInsertExit::before_successor;
        s.gp[7]=this->pop(before?0xaa:0x8f);s.gp[6]=this->pop(before?0xab:0x90);
    }
    void restore_previous(const NativeAsyncInsertion&d){s.gp[3]=this->pop(d.exit==NativeAsyncInsertExit::before_successor?0xaf:0x94);}
    void write(NativeAsyncInsertWrite kind,const NativeAsyncInsertion&d,std::uint32_t value){
        using E=NativeAsyncInsertExit;
        switch(kind){
        case NativeAsyncInsertWrite::head:this->store(d.exit==E::empty?0x20:0x3d,0x4ef1bcu,value);break;
        case NativeAsyncInsertWrite::new_next:
            this->store(d.exit==E::empty?0x25:d.exit==E::head?0x3a:d.exit==E::before_successor?0xac:0x95,d.incoming+0x3cu,value);break;
        case NativeAsyncInsertWrite::previous_next:this->store(d.exit==E::before_successor?0xb0:0x91,d.previous+0x3cu,value);break;
        }
    }
    void finish(const NativeAsyncInsertion&d){
        using E=NativeAsyncInsertExit;unsigned restore{},add{},ret{};
        switch(d.exit){
        case E::empty:restore=0x28;add=0x29;ret=0x2c;break;
        case E::head:restore=0x42;add=0x43;ret=0x46;break;
        case E::before_successor:restore=0xb3;add=0xb4;ret=0xb7;break;
        case E::append:restore=0x9c;add=0x9d;ret=0xa0;break;
        }
        s.gp[5]=this->pop(restore);at(add);s.gp[4]=s.flags.plus(s.gp[4],0x14u,4);this->pop(ret);s.gp[4]+=4u;
    }
};
template<class State,class Compare,class Before=Noop>bool insert(State&s,std::uint32_t site,Compare&&compare,Before before={}){
    if((site!=0x122020u&&site!=0x1232c0u)||!admitted(s))return false;
    native_raw_x87_detail::Scope scope{s};NativeAsyncInsertion data;Insertion<State,Compare,Before>access{s,site,compare,before,scope};native_async_insert(access,data);return true;
}
} // namespace halo2::native_async_queue_abi_detail
