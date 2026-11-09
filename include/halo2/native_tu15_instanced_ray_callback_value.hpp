#pragma once
#include "halo2/native_tu15_instanced_ray_callback_kernel.hpp"
#include <utility>

namespace halo2::native_instanced_ray_callback_detail {
// Ordinary value views for the existing public callback owner. Memory owns word
// reads; Publish owns width-aware writes/observation. Query/Compare execute the
// actual provider/native FP policy selected by that owner. These views carry no
// raw register/stack or FP alias state and are not raw-owner admission.
template<class Memory,class Publish,class Invoke>struct ForwardValues {
    Memory&memory;Publish&publish;Invoke&invoke_owner;
    std::uint32_t collector_value{},body_value{},packet_value{};
    void begin(){}
    std::uint32_t collector()const{return collector_value;}
    std::uint32_t one(){return memory.word(0x46a0d0u);}
    std::uint32_t table(const Forward&d){return memory.word(d.collector);}
    std::uint32_t packet()const{return packet_value;}
    void push_packet(const Forward&){}
    std::uint32_t body()const{return body_value;}
    void push_body(const Forward&){}
    void fraction(const Forward&d){publish(0x24df0eu,d.packet+0x14u,d.one,4u);}
    std::uint32_t owner(const Forward&d){return memory.word(d.table);}
    void invoke(const Forward&d){invoke_owner(d.owner,d.collector,d.body,d.packet);}
    void finish(){}
};
template<class Memory,class Publish,class Key,class Query,class Compare>struct CollectorValues {
    Memory&memory;Publish&publish;Key&key;Query&query_owner;Compare&compare_owner;
    std::uint32_t collector_value{};
    std::uint32_t body()const{return 0u;}
    std::uint32_t instance(const Collector&){return key();}
    std::uint32_t epoch(){return memory.word(0x4f44ecu);}
    std::uint32_t begin(const Collector&)const{return collector_value;}
    void index(const Collector&){}
    std::uint32_t read(Read kind,const Collector&d){
        switch(kind){
        case Read::stamp:return memory.word(0x4f44f0u+d.index*4u);
        case Read::result_before:case Read::result_after:return memory.word(d.collector+0x1cu);
        case Read::displacement:return memory.word(d.collector+0x14u);
        case Read::origin:return memory.word(d.collector+0x10u);
        case Read::filter:return memory.word(d.collector+0xcu);
        case Read::mask:case Read::accepted_mask:return memory.word(d.collector+8u);
        case Read::current:return memory.word(d.collector+4u);
        case Read::candidate:return memory.word(d.result+4u);
        case Read::selected:return memory.word((d.keep?d.collector:d.result)+4u);
        }
        return 0u;
    }
    void compare_stamp(const Collector&){}
    void write(Write kind,std::uint32_t value,const Collector&d){
        if(kind==Write::stamp){publish(0x24df40u,0x4f44f0u+d.index*4u,value,4u);return;}
        const auto pc=kind==Write::fraction?(any_hit?0x24df6cu:d.keep?0x24dfa0u:0x24df8du):any_hit?0x24df71u:d.keep?0x24dfa5u:0x24df92u;
        publish(pc,d.collector+(kind==Write::fraction?4u:0x18u),value,kind==Write::fraction?4u:1u);
    }
    void push_displacement(const Collector&){}
    void push_origin(const Collector&){}
    void push_filter(const Collector&){}
    void push_mask(const Collector&){}
    bool query(const Collector&d){return query_owner(d.instance,d.mask,d.filter,d.origin,d.displacement,d.result);}
    void resumed(Collector&){}
    void zero(const Collector&){any_hit=true;}
    bool compare(const Collector&d){return compare_owner(d.current,d.candidate);}
    bool any_hit{};
};
} // namespace halo2::native_instanced_ray_callback_detail
