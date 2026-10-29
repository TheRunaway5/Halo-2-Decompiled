#include "halo2/native_campaign_ai_path_heap_tu15.hpp"
#include <array>
#include <bit>

namespace halo2 {
namespace {
std::int16_t word(std::uint32_t value) {
    return std::bit_cast<std::int16_t>(std::uint16_t(value));
}
class PathHeap {
    SourcePhysicsContactMemory &memory_;
    std::uint32_t workspace_;
    const SourcePhysicsContactRecordObserver &observe_;
    std::uint32_t slot_address(std::int16_t index,std::uint32_t field) const {
        return workspace_+field+std::uint32_t(std::int32_t(index))*4u;
    }
    std::int16_t read(std::uint32_t address) const {
        std::array<std::byte,2> bytes{};memory_.read(address,bytes);
        return word(std::to_integer<unsigned>(bytes[0])|(std::to_integer<unsigned>(bytes[1])<<8u));
    }
    void write(std::uint32_t site,std::uint32_t address,std::int16_t value) {
        const auto bits=std::uint16_t(value);
        const std::array<std::byte,2> bytes{std::byte(bits),std::byte(bits>>8u)};
        if(!observe_){memory_.write(address,bytes);return;}
        SourcePhysicsContactRecordStore event{site,address,{}, {bytes.begin(),bytes.end()}};
        event.before.resize(2);memory_.read(address,event.before);
        memory_.write(address,bytes);observe_(event);
    }
public:
    PathHeap(SourcePhysicsContactMemory &memory,std::uint32_t workspace,
        const SourcePhysicsContactRecordObserver &observe):memory_(memory),workspace_(workspace),observe_(observe){}
    std::int16_t count()const{return read(workspace_+0x110b0u);}
    void count(std::int16_t value){write(0x27bcf3,workspace_+0x110b0u,value);}
    std::int16_t node(std::int16_t index)const{return read(slot_address(index,0x110b2u));}
    std::int16_t priority(std::int16_t index)const{return read(slot_address(index,0x110b4u));}
    void node(std::int16_t index,std::int16_t value,std::uint32_t site){write(site,slot_address(index,0x110b2u),value);}
    void priority(std::int16_t index,std::int16_t value,std::uint32_t site){write(site,slot_address(index,0x110b4u),value);}
    void backpointer(std::int16_t node,std::int16_t index,std::uint32_t site) {
        write(site,workspace_+0xf0u+std::uint32_t(std::int32_t(node))*0x44u,index);
    }
};
void sift_up(PathHeap &heap,std::int16_t slot) {
    const auto priority=heap.priority(slot),node=heap.node(slot);
    while(slot>1) {
        const auto parent=std::int16_t(slot/2);
        const auto parent_priority=heap.priority(parent),parent_node=heap.node(parent);
        if(priority>=parent_priority)break;
        heap.node(slot,parent_node,0x27bba5);
        heap.priority(slot,parent_priority,0x27bbad);
        heap.backpointer(parent_node,slot,0x27bbbf);
        slot=parent;
    }
    heap.node(slot,node,0x27bbd4);
    heap.priority(slot,priority,0x27bbdc);
    heap.backpointer(node,slot,0x27bbeb);
}
}
void native_campaign_ai_path_heap_sift_up_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t workspace,std::int16_t slot,const SourcePhysicsContactRecordObserver &observe) {
    PathHeap heap(memory,workspace,observe);sift_up(heap,slot);
}
void native_campaign_ai_path_heap_sift_down_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t workspace,std::int16_t slot,const SourcePhysicsContactRecordObserver &observe) {
    PathHeap heap(memory,workspace,observe);
    const auto node=heap.node(slot),priority=heap.priority(slot);
    for(;;) {
        auto selected=slot,best_node=node,best_priority=priority;
        for(unsigned side=0;side<2;++side) {
            const auto child=word(std::uint32_t(std::int32_t(slot))*2u+side);
            if(child>=heap.count())break;
            const auto child_priority=heap.priority(child);
            if(child_priority<best_priority) {
                best_node=heap.node(child);best_priority=child_priority;selected=child;
            }
        }
        if(selected==slot)break;
        heap.priority(slot,best_priority,0x27bc8c);
        heap.node(slot,best_node,0x27bc9a);
        heap.backpointer(best_node,slot,0x27bca2);
        slot=selected;
    }
    heap.priority(slot,priority,0x27bcb7);
    heap.node(slot,node,0x27bcbf);
    heap.backpointer(node,slot,0x27bcd0);
}
void native_campaign_ai_path_heap_insert_tu15(SourcePhysicsContactMemory &memory,
    std::uint32_t workspace,std::int16_t node,std::int16_t priority,
    const SourcePhysicsContactRecordObserver &observe) {
    PathHeap heap(memory,workspace,observe);const auto tail=heap.count();
    if(tail>=0x400)return;
    heap.count(word(std::uint32_t(std::int32_t(tail))+1u));
    heap.node(tail,node,0x27bd03);heap.priority(tail,priority,0x27bd10);
    sift_up(heap,tail);
}
}
