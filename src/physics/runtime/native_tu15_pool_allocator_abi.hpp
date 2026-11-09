#pragma once
#include "halo2/native_tu15_array_allocator.hpp"
#include "physics/runtime/native_tu15_partition_temporary_abi.hpp"

namespace halo2::native_pool_allocator_detail {
struct Activity {std::uint64_t allocations{},releases{},oversize_release_owners{};std::uint32_t last_owner{};};
inline Activity &activity(){thread_local Activity value;return value;}
inline bool reference_mode(){static const bool enabled=[]{const auto *p=std::getenv("HALO2_REFERENCE_POOL_ALLOCATOR");return p&&*p;}();return enabled;}
template<class State> class AllocationStorage final:public NativeTu15PoolStorage {
    State &s_;NativeTu15PoolMemory memory_;std::uint32_t bytes_;
    mutable unsigned head_reads_{},use_reads_{},cursor_reads_{},end_reads_{};
    mutable bool has_free_{},restored_{};
    void scratch(std::uint32_t pc,std::uint32_t value)const{physics_step_detail::w(s_.memory,s_.observe,pc,s_.gp[4]+20u,4,value);}
public:
    mutable bool exhausted{};
    AllocationStorage(State &s,std::uint32_t bytes):s_(s),memory_(s.memory,s.gp[1],s.observe),bytes_(bytes){}
    bool has_free()const{return has_free_;}
    bool small_request()const{return std::bit_cast<std::int32_t>(bytes_)<=0x200;}
    std::uint32_t identity()const override{return memory_.identity();}
    std::uint32_t small_class(std::uint32_t bytes)const override{return s_.gp[2]=memory_.small_class(bytes);}
    std::uint32_t large_class(std::uint32_t bytes)const override {
        s_.gp[0]=bytes-1u;s_.gp[0]=s_.flags.right(s_.gp[0],10u,4,true);return s_.gp[2]=memory_.large_class(bytes);
    }
    std::uint32_t free_head(std::uint32_t index)const override {
        const auto value=memory_.free_head(index);
        if(head_reads_++==0u){s_.gp[0]=value;has_free_=value!=0;}else s_.gp[5]=value;
        return value;
    }
    std::uint32_t class_size(std::uint32_t index)const override {
        s_.push(0x2332b2u,s_.gp[5]);s_.flags.minus(s_.gp[6],0x200u,4);const auto value=memory_.class_size(index);
        s_.gp[6]=value;s_.push(0x2332bdu,s_.gp[7]);return value;
    }
    std::uint32_t counter(NativeTu15PoolCounter kind,std::uint32_t index=0)const override {
        using Counter=NativeTu15PoolCounter;auto &g=s_.gp;
        if(kind==Counter::in_use && use_reads_++==0u) {
            s_.push(0x233284u,g[3]);const auto value=memory_.counter(kind,index);g[3]=s_.flags.increment(value,4,false);s_.flags.logic(g[0],4);return value;
        }
        const auto value=memory_.counter(kind,index);
        if(has_free_) {
            if(kind==Counter::free_blocks){g[6]=s_.flags.increment(value,4,true);g[3]=s_.pop();}return value;
        }
        if(kind==Counter::pages) {
            g[small_request()?3:0]=value;s_.flags.minus(value,0x76u,4);exhausted=std::bit_cast<std::int32_t>(value)>=0x76;
        }else if(kind==Counter::reservations) {
            if(small_request()){g[0]=value;g[3]=s_.flags.increment(g[3],4,false);g[0]=s_.flags.increment(g[0],4,false);}
            else {g[5]=value;g[7]=s_.flags.shift(g[0],13u,4);g[0]=s_.flags.increment(g[0],4,false);g[5]=s_.flags.increment(g[5],4,false);s_.flags.minus(g[6],0x2000u,4);}
        }else if(kind==Counter::in_use && use_reads_==2u) {
            if(small_request()){g[5]=value;scratch(0x233325u,g[5]);}
            else {g[0]=value;g[3]=identity()+0x4340u+g[7];scratch(0x2333c4u,g[0]);g[0]=g[3]+g[6];if(std::bit_cast<std::int32_t>(g[6])<0x2000)g[7]=s_.flags.minus(g[0],g[3],4);}
        }else if(kind==Counter::total_blocks && restored_) {
            g[0]=value;g[7]=s_.pop();g[0]=s_.flags.increment(g[0],4,false);g[5]=s_.pop();
        }
        return value;
    }
    std::uint32_t small(NativeTu15PoolSmallField kind)const override {
        auto &g=s_.gp;using Small=NativeTu15PoolSmallField;
        if(kind==Small::cursor) {
            ++cursor_reads_;if(cursor_reads_==2u)s_.flags.minus(g[6],0x100u,4);
            const auto value=memory_.small(kind);g[cursor_reads_==2u?3:0]=value;return value;
        }
        const auto value=memory_.small(kind);
        if(kind==Small::end) {
            if(end_reads_++==0u){g[3]=value;g[0]=s_.flags.plus(g[0],g[6],4);s_.flags.minus(g[0],g[3],4);g[7]=g[6];}
            else {g[5]=g[0]+g[6];s_.flags.minus(g[5],value,4);}
        }
        return value;
    }
    std::uint32_t link(std::uint32_t p)const override{return s_.gp[6]=memory_.link(p);}
    std::uint32_t restore_in_use_count(std::uint32_t,std::uint32_t)const override {
        restored_=true;return s_.gp[0]=physics_step_detail::r(s_.memory,s_.gp[4]+20u,4);
    }
    void free_head(std::uint32_t index,std::uint32_t value,std::uint32_t site)override {
        memory_.free_head(index,value,site);
        if(site==0x2333ecu){s_.gp[7]=s_.flags.plus(s_.gp[7],s_.gp[6],4);s_.gp[0]=s_.flags.plus(s_.gp[0],s_.gp[6],4);s_.flags.minus(s_.gp[7],0x2000u,4);}
    }
    void counter(NativeTu15PoolCounter kind,std::uint32_t index,std::uint32_t value,std::uint32_t site)override {
        if(site==0x233338u||site==0x23333fu||site==0x2333d1u||site==0x2333d8u)s_.flags.increment(value-1u,4,false);
        else if(site==0x233346u||site==0x2333dfu)s_.flags.increment(value+1u,4,true);
        memory_.counter(kind,index,value,site);
    }
    void small(NativeTu15PoolSmallField kind,std::uint32_t value,std::uint32_t site)override {
        auto &g=s_.gp;
        if(site==0x2332edu){s_.flags.shift(g[3],13u,4);g[0]=value;}
        else if(site==0x2332f8u)g[0]=s_.flags.plus(g[0],0x1fc0u,4);
        else if(site==0x23331bu)g[0]=value;
        else if(site==0x233364u){g[0]=value;g[7]=s_.flags.plus(g[7],g[6],4);s_.flags.minus(g[7],0x100u,4);}
        memory_.small(kind,value,site);
    }
    void link(std::uint32_t p,std::uint32_t value,std::uint32_t site)override{memory_.link(p,value,site);}
};
template<class State> bool allocate(State &s) {
    if(reference_mode()||!native_partition_temporary_detail::admitted(s))return false;
    ++activity().allocations;auto &g=s.gp;s.push(0x233250u,g[6]);g[6]=physics_step_detail::r(s.memory,g[4]+8u,4);const auto bytes=g[6];
    s.flags.minus(bytes,0x2000u,4);if(std::bit_cast<std::int32_t>(bytes)<=0x2000)s.flags.minus(bytes,0x200u,4);
    AllocationStorage<State> storage(s,bytes);const auto result=native_tu15_pool_allocate(storage,bytes);std::uint32_t final_pc;
    if(std::bit_cast<std::int32_t>(bytes)>0x2000){g[0]=s.flags.logic(0,4);g[6]=s.pop();final_pc=0x23340fu;}
    else if(storage.has_free()){g[6]=s.pop();final_pc=0x2332afu;}
    else if(storage.exhausted){g[7]=s.pop();g[3]=s.flags.logic(0,4);g[5]=s.pop();g[0]=g[3];g[3]=s.pop();g[6]=s.pop();final_pc=0x233409u;}
    else {g[0]=result;g[3]=s.pop();g[6]=s.pop();final_pc=0x233389u;}
    s.pop();g[4]+=8u;if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=final_pc;return true;
}
// Transport the retained caller ABI at the shared pool DATA owner's semantic
// class lookup, accounting, freelist and block-acquisition boundaries.
template<class State> class ReleaseStorage final:public NativeTu15PoolStorage {
    State &s_;NativeTu15PoolMemory memory_;
public:
    explicit ReleaseStorage(State &s):s_(s),memory_(s.memory,s.gp[1],s.observe){}
    std::uint32_t identity()const override{return memory_.identity();}
    std::uint32_t small_class(std::uint32_t bytes)const override{return s_.gp[0]=memory_.small_class(bytes);}
    std::uint32_t large_class(std::uint32_t bytes)const override {
        s_.gp[0]=s_.flags.increment(bytes,4,true);s_.gp[0]=s_.flags.right(s_.gp[0],10u,4,true);
        return s_.gp[0]=memory_.large_class(bytes);
    }
    std::uint32_t free_head(std::uint32_t index)const override{return s_.gp[6]=memory_.free_head(index);}
    std::uint32_t class_size(std::uint32_t index)const override{return memory_.class_size(index);}
    std::uint32_t counter(NativeTu15PoolCounter kind,std::uint32_t index=0)const override {
        if(kind==NativeTu15PoolCounter::in_use)return s_.gp[2]=memory_.counter(kind,index);
        if(kind==NativeTu15PoolCounter::free_blocks) {
            s_.push(0x23344eu,s_.gp[6]);const auto value=memory_.counter(kind,index);
            s_.gp[6]=s_.flags.increment(value,4,false);s_.gp[2]=s_.flags.increment(s_.gp[2],4,true);return value;
        }
        return memory_.counter(kind,index);
    }
    std::uint32_t small(NativeTu15PoolSmallField kind)const override{return memory_.small(kind);}
    std::uint32_t link(std::uint32_t p)const override{return memory_.link(p);}
    void free_head(std::uint32_t index,std::uint32_t value,std::uint32_t site)override{memory_.free_head(index,value,site);}
    void counter(NativeTu15PoolCounter kind,std::uint32_t index,std::uint32_t value,std::uint32_t site)override{memory_.counter(kind,index,value,site);}
    void small(NativeTu15PoolSmallField kind,std::uint32_t value,std::uint32_t site)override{memory_.small(kind,value,site);}
    void link(std::uint32_t p,std::uint32_t value,std::uint32_t site)override{memory_.link(p,value,site);}
};
template<class State,class Call> bool release(State &s,Call call) {
    if(reference_mode()||!native_partition_temporary_detail::admitted(s))return false;
    ++activity().releases;auto &g=s.gp;using physics_step_detail::r;
    const auto at=[&](std::uint32_t pc){if(s.instruction_cursor&&s.instruction_cursor->position)*s.instruction_cursor->position=pc;};
    at(0x233420u);g[0]=r(s.memory,g[4]+8u,4);const auto bytes=g[0];s.flags.minus(bytes,0x2000u,4);
    ReleaseStorage<State> storage(s);
    if(std::bit_cast<std::int32_t>(bytes)<=0x2000)s.flags.minus(bytes,0x200u,4);
    const bool completed=native_tu15_pool_release_delayed_data(storage,[&]{at(0x233466u);return g[2]=r(s.memory,g[4]+8u,4);},bytes);
    if(completed){g[6]=s.pop();s.pop();g[4]+=12u;at(0x233475u);return true;}
    // Oversized blocks retain the actual imported CRT free entry, including
    // its caller stack and completion ABI; no platform free is substituted.
    at(0x233478u);g[0]=r(s.memory,g[4]+4u,4);s.push(0x23347cu,g[0]);at(0x23347du);
    const auto owner=r(s.memory,0x46e33cu,4);s.push(0x23347du,0x233483u);++activity().oversize_release_owners;activity().last_owner=owner;
    call(s,owner,0x22eb7du);g[1]=s.pop();s.pop();g[4]+=12u;at(0x233484u);return true;
}
} // namespace halo2::native_pool_allocator_detail
