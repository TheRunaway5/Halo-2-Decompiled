#pragma once
#include "halo2/native_tu15_arena_initialize_kernel.hpp"
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_mopp_ray_caller_frame.hpp"
#include "physics/runtime/native_tu15_raw_x87_scope.hpp"
#include <span>
#include <utility>

namespace halo2::native_arena_initialize_detail {
// Private raw transport only. No admission/default/source prefix is installed.
struct Operands {
    bool canonical_positions{};
    // Source API publication differs from CPU instruction-prefix publication
    // at virtual PUSH and REP observer failures. Keep the policies explicit.
    bool source_publication{};
    constexpr std::uint32_t position(std::uint32_t canonical)const{return canonical_positions?canonical:canonical+0x13c0u;}
    static constexpr std::uint32_t initialized=0x4f0430u,backing=0x4f2950u,offset=0x4f2954u;
    static constexpr std::uint32_t checksum=0x4f295cu,metadata=0x4f2964u,root=0x51df2cu,vtable=0x45f1d8u;
    static constexpr std::uint32_t reserve_child=0x21b920u,savegame_child=0x21b990u,crc_child=0x166a80u;
};
template<class State>struct Io {
    native_raw_x87_detail::Scope<State>*scope;
    template<class Callback>void operator()(Callback&&callback)const{scope->io(std::forward<Callback>(callback));}
};
template<class State,class Before,class Call>class Frame:
    public native_mopp_ray_caller_detail::Frame<State,Before,Io<State>> {
    using Base=native_mopp_ray_caller_detail::Frame<State,Before,Io<State>>;
    using Base::s_;using Base::word;using Base::io;
    native_raw_x87_detail::Scope<State>&scope_;Call&call_;Operands operands_;
    void at(std::uint32_t canonical){Base::at(operands_.position(canonical));}
    void push(std::uint32_t canonical,std::uint32_t value){
        if(!operands_.source_publication){Base::push(operands_.position(canonical),value);return;}
        const auto address=s_.gp[4]-4u;at(canonical);s_.gp[4]=address;
        Base::store(operands_.position(canonical),address,value,[]{},true);
    }
    std::uint32_t pop(std::uint32_t canonical){return Base::pop(operands_.position(canonical));}
    template<class Commit>void store(std::uint32_t canonical,std::uint32_t address,std::uint32_t value,Commit commit){Base::store(operands_.position(canonical),address,value,commit);}
    void byte_store(std::uint32_t canonical,std::uint32_t address,std::uint8_t value){
        at(canonical);SourcePhysicsContactRecordStore event{operands_.position(canonical),address,{},{}};
        if(s_.observe){event.before.resize(1);io([&]{s_.memory.read(address,event.before);});}
        const std::byte payload{value};io([&]{s_.memory.write(address,std::span(&payload,1));});
        if(s_.observe){event.written.push_back(payload);io([&]{s_.observe(event);});}
    }
    void child(std::uint32_t canonical,std::uint32_t target,std::uint32_t return_word){
        push(canonical,return_word);
        scope_.child([&]{call_(s_,target,operands_.position(canonical),false);});
    }
public:
    Frame(State&s,Before&before,Call&call,native_raw_x87_detail::Scope<State>&scope,Operands operands={})
        :Base{s,before,{&scope}},scope_(scope),call_(call),operands_(operands){}
    bool needs_initialization(){
        push(0x125280u,s_.gp[1]);at(0x125281u);
        const auto flag=word(Operands::initialized,1);s_.gp[0]=(s_.gp[0]&0xffffff00u)|flag;
        s_.flags.logic(flag,1);return flag==0u;
    }
    void begin_storage(){push(0x12528eu,s_.gp[6]);push(0x12528fu,s_.gp[7]);}
    void reserve_backing(std::uint32_t usable,std::uint32_t extra,std::uint32_t initial_checksum){
        auto&g=s_.gp;g[0]=extra;g[1]=usable;
        store(0x12529au,Operands::checksum,initial_checksum,[]{});
        child(0x1252a4u,Operands::reserve_child,0x126669u);
        backing_reserved();
    }
    void backing_reserved(){
        auto&g=s_.gp;
        g[7]=g[0];g[0]=s_.flags.logic(g[0]^g[0],4);
        store(0x1252adu,Operands::backing,g[7],[]{});g[1]=backing_words;
    }
    void clear_word(){
        auto&g=s_.gp;const auto destination=g[7],value=g[0];
        auto advance=[&]{g[7]+=(s_.other_flags&0x400u)?0xfffffffcu:4u;--g[1];};
        if(operands_.source_publication){store(0x1252b8u,destination,value,[]{});advance();}
        else store(0x1252b8u,destination,value,advance);
    }
    void initialize_savegames(){child(0x1252bau,Operands::savegame_child,0x12667fu);}
    std::uint32_t offset(Allocation allocation){at(allocation==Allocation::metadata?0x1252bfu:0x1252f2u);return s_.gp[0]=word(Operands::offset);}
    std::uint32_t base(Allocation allocation){
        at(allocation==Allocation::metadata?0x1252c4u:0x1252f7u);
        return s_.gp[allocation==Allocation::metadata?1u:2u]=word(Operands::backing);
    }
    void metadata_ready(std::uint32_t address){s_.gp[6]=address;}
    void root_ready(std::uint32_t address){s_.gp[6]=address;}
    std::uint32_t root_charge_bytes()const{return s_.gp[7];}
    void reserve_offset(Allocation allocation,std::uint32_t end){
        const auto bytes=allocation==Allocation::metadata?metadata_bytes:s_.gp[7];
        s_.gp[0]=s_.flags.plus(s_.gp[0],bytes,4);
        store(allocation==Allocation::metadata?0x1252d2u:0x125308u,Operands::offset,end,[]{});
    }
    void account_allocation(Allocation allocation,std::uint32_t bytes){
        auto&g=s_.gp;
        if(allocation==Allocation::metadata){
            push(0x1252d7u,Operands::checksum);g[7]=root_bytes;g[0]=g[4]+12u;
            store(0x1252e5u,g[4]+12u,bytes,[]{});
            child(0x1252edu,Operands::crc_child,0x1266b2u);
        }else{
            push(0x12530du,Operands::checksum);g[0]=g[4]+12u;
            byte_store(0x125316u,Operands::initialized,1);
            store(0x12531du,g[4]+12u,g[7],[]{});
            child(0x125321u,Operands::crc_child,0x1266e6u);
        }
    }
    std::uint32_t resumed_pointer()const{return s_.gp[6];}
    void publish_metadata(std::uint32_t address){store(0x1252fdu,Operands::metadata,address,[]{});}
    void publish_root(std::uint32_t address){s_.flags.logic(s_.gp[6],4);store(0x125328u,Operands::root,address,[]{});}
    void initialize_root(std::uint32_t address){store(0x125330u,address,Operands::vtable,[]{});}
    void end_storage(){s_.gp[7]=pop(0x125336u);s_.gp[6]=pop(0x125337u);}
    void finish(){s_.gp[1]=pop(0x125338u);pop(0x125339u);}
};
template<class State,class Before,class Call>Initialization run_private(State&s,Before before,Call call,Operands operands={}){
    native_raw_x87_detail::Scope scope{s};Frame frame{s,before,call,scope,operands};return initialize(frame);
}
}
#endif
