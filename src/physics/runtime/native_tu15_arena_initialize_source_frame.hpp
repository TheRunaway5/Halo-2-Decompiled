#pragma once
#include "halo2/native_tu15_arena_initialize_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"

namespace halo2::native_arena_initialize_source_detail {
using native_arena_initialize_detail::Allocation;
using native_arena_initialize_detail::Initialization;
inline constexpr std::uint32_t initialized=0x4f0430u,backing=0x4f2950u,offset_cell=0x4f2954u;
inline constexpr std::uint32_t checksum=0x4f295cu,metadata=0x4f2964u,root_cell=0x51df2cu,vtable=0x45f1d8u;
struct Continuation {std::uint32_t next;const void*owner;};
enum class Phase {whole,reservation,repeat,metadata,root,finish};
struct Noop {template<class State>void operator()(State&,std::uint32_t)const{}};

// Source transport, not a physical-instruction fault envelope. The integer
// controller leaves the retained State and host FP file as the source does.
// Every virtual instruction completes before the next-policy checkpoint.
template<class State,class Before,class Call,class Checkpoint>class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before>;
    using Base::s_;using Base::at;using Base::word;
    Call&call_;Checkpoint&checkpoint_;
    void store(std::uint32_t pc,std::uint32_t address,unsigned width,std::uint32_t value){
        at(pc);std::array<std::byte,4>bytes{};
        for(unsigned n=0;n<width;++n)bytes[n]=std::byte(value>>(8u*n));
        // Preserve retained w()'s entry choice and its late observer recheck.
        if(!s_.observe){s_.memory.write(address,std::span(bytes).first(width));return;}
        SourcePhysicsContactRecordStore event{pc,address,{},{}};
        event.before.resize(width);event.written.assign(bytes.begin(),bytes.begin()+width);
        s_.memory.read(address,event.before);s_.memory.write(address,event.written);
        if(s_.observe)s_.observe(event);
    }
    void push(std::uint32_t pc,std::uint32_t value){
        const auto address=s_.gp[4]-4u;s_.gp[4]=address;store(pc,address,4u,value);
    }
    std::uint32_t pop(std::uint32_t pc){at(pc);const auto value=word(s_.gp[4]);s_.gp[4]+=4u;return value;}
    void child(std::uint32_t pc,std::uint32_t target,std::uint32_t return_word,std::uint32_t next){
        push(pc,return_word);call_(s_,target,pc,false);checkpoint_(next);
    }
public:
    Frame(State&s,Before&before,Call&call,Checkpoint&checkpoint):Base{s,before},call_(call),checkpoint_(checkpoint){}
    bool needs_initialization(){
        push(0x125280u,s_.gp[1]);checkpoint_(0x125281u);
        at(0x125281u);const auto flag=word(initialized,1);s_.gp[0]=(s_.gp[0]&0xffffff00u)|flag;
        checkpoint_(0x125286u);s_.flags.logic(s_.gp[0]&255u,1);return (s_.gp[0]&255u)==0u;
    }
    void begin_storage(){push(0x12528eu,s_.gp[6]);checkpoint_(0x12528fu);push(0x12528fu,s_.gp[7]);checkpoint_(0x125290u);}
    void reserve_backing(std::uint32_t usable,std::uint32_t extra,std::uint32_t initial_checksum){
        auto&g=s_.gp;g[0]=extra;g[1]=usable;store(0x12529au,checksum,4u,initial_checksum);checkpoint_(0x1252a4u);
        child(0x1252a4u,0x21b920u,0x126669u,0x1252a9u);backing_reserved();
    }
    void backing_reserved(){
        auto&g=s_.gp;g[7]=g[0];g[0]=s_.flags.logic(g[0]^g[0],4);
        store(0x1252adu,backing,4u,g[7]);checkpoint_(0x1252b3u);g[1]=native_arena_initialize_detail::backing_words;
    }
    void clear_word(){
        auto&g=s_.gp;store(0x1252b8u,g[7],4u,g[0]);
        g[7]+=(s_.other_flags&0x400u)?0xfffffffcu:4u;--g[1];
        // This code enters the original trailing repeat callback, including
        // after the final iteration. It never replays the completed store.
        checkpoint_(0x1252b8u);
    }
    void initialize_savegames(){child(0x1252bau,0x21b990u,0x12667fu,0x1252bfu);}
    std::uint32_t offset(Allocation allocation){
        const bool first=allocation==Allocation::metadata;at(first?0x1252bfu:0x1252f2u);
        s_.gp[0]=word(offset_cell);checkpoint_(first?0x1252c4u:0x1252f7u);return s_.gp[0];
    }
    std::uint32_t base(Allocation allocation){
        const bool first=allocation==Allocation::metadata;at(first?0x1252c4u:0x1252f7u);
        s_.gp[first?1u:2u]=word(backing);checkpoint_(first?0x1252cau:0x1252fdu);return s_.gp[first?1u:2u];
    }
    void metadata_ready(std::uint32_t address){s_.gp[6]=address;}
    void root_ready(std::uint32_t address){s_.gp[6]=address;}
    std::uint32_t root_charge_bytes()const{return s_.gp[7];}
    void reserve_offset(Allocation allocation,std::uint32_t end){
        const bool first=allocation==Allocation::metadata;
        s_.gp[0]=s_.flags.plus(s_.gp[0],first?native_arena_initialize_detail::metadata_bytes:s_.gp[7],4);
        store(first?0x1252d2u:0x125308u,offset_cell,4u,end);checkpoint_(first?0x1252d7u:0x12530du);
    }
    void account_allocation(Allocation allocation,std::uint32_t bytes){
        auto&g=s_.gp;
        if(allocation==Allocation::metadata){
            push(0x1252d7u,checksum);checkpoint_(0x1252dcu);g[7]=native_arena_initialize_detail::root_bytes;g[0]=g[4]+12u;
            store(0x1252e5u,g[4]+12u,4u,bytes);checkpoint_(0x1252edu);child(0x1252edu,0x166a80u,0x1266b2u,0x1252f2u);
        }else{
            push(0x12530du,checksum);checkpoint_(0x125312u);g[0]=g[4]+12u;
            store(0x125316u,initialized,1u,1u);checkpoint_(0x12531du);
            store(0x12531du,g[4]+12u,4u,g[7]);checkpoint_(0x125321u);child(0x125321u,0x166a80u,0x1266e6u,0x125326u);
        }
    }
    std::uint32_t resumed_pointer()const{return s_.gp[6];}
    void publish_metadata(std::uint32_t address){store(0x1252fdu,metadata,4u,address);checkpoint_(0x125303u);}
    void publish_root(std::uint32_t address){s_.flags.logic(s_.gp[6],4);store(0x125328u,root_cell,4u,address);checkpoint_(0x12532eu);}
    void initialize_root(std::uint32_t address){store(0x125330u,address,4u,vtable);checkpoint_(0x125336u);}
    void end_storage(){s_.gp[7]=pop(0x125336u);checkpoint_(0x125337u);s_.gp[6]=pop(0x125337u);checkpoint_(0x125338u);}
    void finish(){s_.gp[1]=pop(0x125338u);checkpoint_(0x125339u);pop(0x125339u);}
};

template<class State,class Continue,class Call,class Before=Noop>
std::uint32_t run_source(State&s,Continue continuation,Call call,Before before={},Phase phase=Phase::whole){
    const std::byte token{};
    const auto checkpoint=[&](std::uint32_t next){if(!continuation(s))throw Continuation{next,&token};};
    try{
        Frame view{s,before,call,checkpoint};
        switch(phase){
        case Phase::whole:native_arena_initialize_detail::initialize(view);break;
        case Phase::reservation:view.backing_reserved();[[fallthrough]];
        case Phase::repeat:native_arena_initialize_detail::clear_words(view,s.gp[1]);view.initialize_savegames();break;
        case Phase::metadata:native_arena_initialize_detail::initialize_metadata(view);break;
        case Phase::root:native_arena_initialize_detail::finish_initialization(view);break;
        case Phase::finish:native_arena_initialize_detail::finish_root(view);break;
        }
        return 0u;
    }
    catch(const Continuation&resume){if(resume.owner!=&token)throw;return resume.next;}
}
} // namespace halo2::native_arena_initialize_source_detail
