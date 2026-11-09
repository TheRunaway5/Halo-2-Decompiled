#pragma once
#include "halo2/native_tu15_array_reserve.hpp"
#include "physics/runtime/native_tu15_partition_temporary_abi.hpp"
#include <optional>

namespace halo2::native_array_abi_detail {
struct Activity {std::uint64_t reserves{},growths{},allocations{},releases{},copies{};std::uint32_t last_owner{};};
inline Activity &activity(){thread_local Activity value;return value;}
inline bool reference_mode() {
    static const bool enabled=[]{const auto *p=std::getenv("HALO2_REFERENCE_ARRAY_OWNERS");return p&&*p;}();
    return enabled;
}
template<class State> class CopyStorage final:public NativeTu15ArrayCopyStorage {
    State &state_;bool actual_pcs_;
    SourcePhysicsContactRecordObserver observer_;
    NativeTu15ArrayGrowthMemory memory_;
    std::uint32_t pc(std::uint32_t actual)const{return actual_pcs_?actual:actual-0x3ad0u;}
public:
    CopyStorage(State &s,bool actual_pcs):state_(s),actual_pcs_(actual_pcs),
        observer_([&](const auto &event){if(state_.observe){auto mapped=event;mapped.instruction=pc(event.instruction);state_.observe(mapped);}}),
        memory_(s.memory,{},s.observe?observer_:SourcePhysicsContactRecordObserver{}){}
    std::uint32_t begin()const {
        auto &s=state_;auto &g=s.gp;using physics_step_detail::r;
        g[1]=r(s.memory,g[4]+12u,4);s.push(pc(0x2e2e34u),g[6]);g[6]=r(s.memory,g[4]+12u,4);g[0]=g[1];
        s.push(pc(0x2e2e3bu),g[7]);g[7]=r(s.memory,g[4]+12u,4);g[1]=s.flags.right(g[1],2u,4,false);return g[0];
    }
    std::uint32_t copy_word(std::uint32_t p)const override{return memory_.copy_word(p);}
    std::uint8_t copy_byte(std::uint32_t p)const override{return memory_.copy_byte(p);}
    void copy_word(std::uint32_t p,std::uint32_t value)override {
        memory_.copy_word(p,value);const auto step=std::uint32_t(copy_direction())*4u;
        state_.gp[6]+=step;state_.gp[7]+=step;--state_.gp[1];
    }
    void copy_byte(std::uint32_t p,std::uint8_t value)override {
        memory_.copy_byte(p,value);const auto step=std::uint32_t(copy_direction());
        state_.gp[6]+=step;state_.gp[7]+=step;--state_.gp[1];
    }
    std::int32_t copy_direction()const override{return(state_.other_flags&1024u)?-1:1;}
    void copy_words_complete(std::uint32_t)override {
        state_.gp[1]=state_.gp[0];state_.gp[1]=state_.flags.logic(state_.gp[1]&3u,4);
    }
    void copy_complete()override {
        state_.gp[7]=state_.pop();state_.gp[6]=state_.pop();state_.pop();++activity().copies;
        if(state_.instruction_cursor&&state_.instruction_cursor->position)*state_.instruction_cursor->position=pc(0x2e2e4eu);
    }
};
template<class State,class Call> class Storage final:public NativeTu15ArrayGrowthStorage {
    State &state_;Call &call_;bool reserve_,actual_pcs_;
    SourcePhysicsContactRecordObserver observer_;
    CopyStorage<State> copy_;
    NativeTu15ArrayGrowthMemory allocator_;
    std::optional<NativeTu15ArrayGrowthMemory> data_;
    std::uint32_t pc(std::uint32_t actual)const{return actual_pcs_?actual:actual-0x3ad0u;}
    std::uint32_t read(std::uint32_t p,unsigned width=4)const{return physics_step_detail::r(state_.memory,p,width);}
    void push(std::uint32_t site,std::uint32_t value)const{state_.push(pc(site),value);}
    void begin_copy()const {
        auto &s=state_;auto &g=s.gp;
        push(reserve_?0x2e320du:0x2e3288u,g[0]);push(reserve_?0x2e320eu:0x2e3289u,g[1]);
        push(reserve_?0x2e320fu:0x2e328au,g[7]);push(reserve_?0x2e3210u:0x2e328bu,reserve_?0x2e3215u:0x2e3290u);
        copy_.begin();
    }
public:
    Storage(State &state,Call &call,bool reserve,bool actual_pcs):state_(state),call_(call),reserve_(reserve),actual_pcs_(actual_pcs),
        observer_([&](const auto &event){if(state_.observe){auto mapped=event;mapped.instruction=pc(event.instruction);state_.observe(mapped);}}),
        copy_(state,actual_pcs),
        allocator_(state.memory,{},state.observe?observer_:SourcePhysicsContactRecordObserver{}){}
    void bind(NativeTu15ArrayHeader header){data_.emplace(state_.memory,header,state_.observe?observer_:SourcePhysicsContactRecordObserver{});}
    std::uint32_t count(NativeTu15ArrayCountRead phase)const override {
        auto &s=state_;auto &g=s.gp;const auto value=data_->count(phase);g[0]=value;
        if(phase==NativeTu15ArrayCountRead::initial) {
            s.flags.logic(g[0],4);push(0x2e325cu,g[7]);g[5]=value?value+value:1u;
        }else{g[0]=s.flags.product(g[0],g[3]);begin_copy();}
        return value;
    }
    std::uint32_t data(NativeTu15ArrayDataRead phase)const override {
        auto &s=state_;auto &g=s.gp;
        if(phase==NativeTu15ArrayDataRead::copy_source) {
            const auto value=data_->data(phase);g[1]=value;g[7]=g[0];return value;
        }
        g[0]=s.flags.logic(g[0]&0x7fffffffu,4);g[0]=s.flags.product(g[0],g[3]);
        push(reserve_?0x2e322fu:0x2e32aau,0x12u);push(reserve_?0x2e3231u:0x2e32acu,g[0]);
        g[0]=data_->data(phase);return g[0];
    }
    std::uint32_t capacity_flags()const override {
        auto &s=state_;auto &g=s.gp;g[0]=data_->capacity_flags();g[4]=s.flags.plus(g[4],12u,4);s.flags.logic(g[0],4);return g[0];
    }
    NativeTu15ArrayAllocator allocator(bool releasing)const override {
        const auto owner=allocator_.allocator(releasing);state_.gp[1]=owner.receiver;
        state_.gp[releasing?2u:0u]=owner.method_table;return owner;
    }
    std::uint32_t allocate_owner(NativeTu15ArrayAllocator owner)const override {
        auto &s=state_;auto &g=s.gp;g[2]=s.flags.product(g[5],g[3]);
        if(reserve_){push(0x2e31f7u,g[6]);push(0x2e31f8u,g[7]);}
        push(reserve_?0x2e31f9u:0x2e3278u,0x12u);push(reserve_?0x2e31fbu:0x2e327au,g[2]);
        return allocator_.allocate_owner(owner);
    }
    std::uint32_t release_owner(NativeTu15ArrayAllocator owner)const override {
        push(reserve_?0x2e3234u:0x2e32afu,state_.gp[0]);return allocator_.release_owner(owner);
    }
    std::uint32_t copy_word(std::uint32_t p)const override{return copy_.copy_word(p);}
    std::uint8_t copy_byte(std::uint32_t p)const override{return copy_.copy_byte(p);}
    void copy_word(std::uint32_t p,std::uint32_t value)override{copy_.copy_word(p,value);}
    void copy_byte(std::uint32_t p,std::uint8_t value)override{copy_.copy_byte(p,value);}
    std::int32_t copy_direction()const override{return copy_.copy_direction();}
    void copy_words_complete(std::uint32_t bytes)override{copy_.copy_words_complete(bytes);}
    void copy_complete()override{copy_.copy_complete();}
    void publish_data(std::uint32_t value)override{publish_data_at(0x2e32b3u,value);}
    void publish_capacity(std::uint32_t value)override{publish_capacity_at(0x2e32b6u,value);}
    void publish_data_at(std::uint32_t site,std::uint32_t value)override {
        data_->publish_data_at(site,value);state_.gp[7]=state_.pop();
    }
    void publish_capacity_at(std::uint32_t site,std::uint32_t value)override {
        data_->publish_capacity_at(site,value);state_.gp[6]=state_.pop();state_.gp[5]=state_.pop();
        state_.gp[3]=state_.pop();state_.pop();
        if(state_.instruction_cursor&&state_.instruction_cursor->position)*state_.instruction_cursor->position=pc(reserve_?0x2e3241u:0x2e32bcu);
    }
    std::uint32_t allocate(const NativeTu15ArrayAllocationRequest &request,NativeTu15PhysicsFrameFp &fp) {
        ++activity().allocations;activity().last_owner=request.owner;
        if(state_.instruction_cursor&&state_.instruction_cursor->position)*state_.instruction_cursor->position=pc(reserve_?0x2e31fcu:0x2e327bu);
        push(reserve_?0x2e31fcu:0x2e327bu,reserve_?0x2e31ffu:0x2e327eu);
        call_(state_,request.owner,reserve_?0x2df72cu:0x2df7abu);fp=native_partition_temporary_detail::floating_point(state_);return state_.gp[0];
    }
    void release(const NativeTu15ArrayReleaseRequest &request,NativeTu15PhysicsFrameFp &fp) {
        ++activity().releases;activity().last_owner=request.owner;
        if(state_.instruction_cursor&&state_.instruction_cursor->position)*state_.instruction_cursor->position=pc(reserve_?0x2e3235u:0x2e32b0u);
        push(reserve_?0x2e3235u:0x2e32b0u,reserve_?0x2e3238u:0x2e32b3u);
        call_(state_,request.owner,reserve_?0x2df765u:0x2df7e0u);fp=native_partition_temporary_detail::floating_point(state_);
    }
};
template<class State,class Call> bool run(State &s,Call call,bool reserve,bool actual_pcs) {
    if(reference_mode()||!native_partition_temporary_detail::admitted(s))return false;
    using namespace physics_step_detail;
    auto &g=s.gp;Storage<State,Call> storage(s,call,reserve,actual_pcs);
    NativeTu15PhysicsFrameFp fp=native_partition_temporary_detail::floating_point(s);
    NativeTu15ArrayGrowthServices services;
    services.allocate=[&storage](const auto &request,auto &context){return storage.allocate(request,context);};
    services.release=[&storage](const auto &request,auto &context){storage.release(request,context);};
    const auto mapped=[&](std::uint32_t pc){return actual_pcs?pc:pc-0x3ad0u;};
    if(reserve) {
        ++activity().reserves;
        NativeTu15ArrayReserveArguments args;
        args.stride=[&]{s.push(mapped(0x2e31e8u),g[3]);g[3]=r(s.memory,g[4]+16u,4);return g[3];};
        args.capacity=[&]{s.push(mapped(0x2e31edu),g[5]);g[5]=r(s.memory,g[4]+16u,4);return g[5];};
        args.header=[&]{g[6]=r(s.memory,g[4]+20u,4);return NativeTu15ArrayHeader{g[6]};};
        native_tu15_reserve_array_in_storage(storage,args,[&](NativeTu15ArrayHeader header)->NativeTu15ArrayGrowthStorage&{
            storage.bind(header);return storage;
        },fp,services);
    }else {
        ++activity().growths;
        s.push(mapped(0x2e3250u),g[3]);s.push(mapped(0x2e3251u),g[5]);s.push(mapped(0x2e3252u),g[6]);
        g[6]=r(s.memory,g[4]+16u,4);storage.bind({g[6]});
        native_tu15_grow_array_delayed_stride(storage,[&]{g[3]=r(s.memory,g[4]+24u,4);return g[3];},fp,services);
    }
    return true;
}
template<class State> bool copy(State &s,bool actual_pcs) {
    if(reference_mode()||!native_partition_temporary_detail::admitted(s))return false;
    CopyStorage<State> storage(s,actual_pcs);const auto bytes=storage.begin();
    native_tu15_copy_array_bytes(storage,s.gp[7],s.gp[6],bytes);return true;
}
} // namespace halo2::native_array_abi_detail
