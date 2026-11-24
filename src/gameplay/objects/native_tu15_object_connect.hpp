#pragma once
// Boundary adapter for the object connect, BA630
// (native_tu15_object_connect_systems.hpp). Its callers and the children keep
// the original ABI, which the adapter owns: the object and the location on
// the stack (RET 8); a 0x60-byte frame below the saved EBX, EBP, ESI and
// EDI; 11E600 (EDX the point, ESI the location), BA910 (EDX the object, EDI
// the summary), 1D2F10 (EAX the cluster set, nine pushed, RET 24), C0F60
// (five pushed, RET 14), B8D90 (EAX the object, one pushed, RET 4), B92C0
// (EDI the object), B1870 (EAX the object), BA570 (the object pushed, RET 4),
// B9330 (EDX the object) and 1CC290 (two pushed, RET 8) direct, 1496F0 and
// 282C10 through the dispatch, every register each returns coming back. The
// cluster set is filled as REP STOSD/STOSB fill it, in the direction the
// string direction flag gives. Each access publishes its original
// instruction.
// HALO2_REFERENCE_OBJECT_CONNECT=1 restores the translated body.
// HALO2_VERIFY_OBJECT_CONNECT=1 records the native run and replays the
// unmodified translated body against it, as in native_tu15_object_tick.hpp.
#include "gameplay/native_tu15_small_queries_common.hpp"
#include "gameplay/objects/native_tu15_object_connect_systems.hpp"

namespace halo2::native_object_connect_detail {
using native_object_tick_detail::Trace;
using namespace native_object_tick_detail::boundary;
using native_small_queries_detail::flag;
using native_small_queries_detail::VerifySummary;
using native_small_queries_detail::run_owner;
namespace logic=halo2::native_object_connect;
using Access=logic::object_connect::Access;
using Kept=logic::object_connect::Kept;
using Outcome=logic::object_connect::Outcome;
inline bool reference_mode(){static const bool v=flag("HALO2_REFERENCE_OBJECT_CONNECT");return v;}
inline bool verify_mode(){static const bool v=flag("HALO2_VERIFY_OBJECT_CONNECT");return v;}
enum Branch : unsigned {activated, released, parked, located, every_cluster, relinked, branch_count};
static_assert(unsigned(Outcome::parked)==Branch::parked);
inline std::atomic<std::uint64_t> &coverage(){static std::atomic<std::uint64_t> v{0};return v;}
inline VerifySummary &summary(){static VerifySummary v{"Object connect"};return v;}
inline constexpr std::uint32_t sites[]{0xba633u,0xba637u,0xba63du,0xba64au,0xba650u,0xba659u,0xba65du,0xba672u,0xba687u,0xba68du,
    0xba68fu,0xba692u,0xba698u,0xba69bu,0xba69fu,0xba6aau,0xba6bbu,0xba6beu,0xba6c8u,0xba6ceu,0xba6e6u,0xba6edu,0xba6f3u,0xba6f7u,
    0xba706u,0xba725u,0xba739u,0xba745u,0xba752u,0xba755u,0xba760u,0xba768u,0xba789u,0xba7adu,0xba7b5u};
static_assert(sizeof(sites)/sizeof(sites[0])==unsigned(Access::header_cluster_again)+1u);
template<class State,class Children,class Dispatch> struct Adapter : Port<State> {
    Children &children;      // the direct children, by entry address
    Dispatch &dispatch;
    std::uint32_t depth{};   // bytes below the return address
    unsigned located_calls{};
    Adapter(State &s,std::uint32_t *c,Trace<State> *t,Children &k,Dispatch &d):Port<State>(s,c,t,{}),children(k),dispatch(d){}
    std::uint32_t argument(std::uint32_t k)const{return this->gp[esp]+depth+4u+4u*k;}
    std::uint32_t local(std::int32_t offset)const{return this->gp[esp]+depth+std::uint32_t(offset);}
    void put(std::uint32_t pc,std::uint32_t v){this->push(pc,v);depth+=4u;}
    std::uint32_t take(std::uint32_t pc){depth-=4u;return this->pop(pc);}
    void refresh(Kept &kept){kept={this->gp[ebx],this->gp[ebp],this->gp[esi],this->gp[edi]};}
    void direct(std::uint32_t pc,std::uint32_t target,std::uint32_t popped,Kept &kept) {
        auto child=[&](State &v){children(v,target);};
        this->call(pc,pc+5u,target,child);depth-=popped;
        refresh(kept);
    }
    void routed(std::uint32_t pc,std::uint32_t target,Kept &kept) {
        auto child=[&](State &v){dispatch(v,target,pc);};
        this->call(pc,pc+5u,target,child);
        refresh(kept);
    }
    std::uint32_t step(std::uint32_t width)const{return (this->s.other_flags&0x400u)?0u-width:width;}
    std::uint32_t instruction(Access a,std::uint32_t){return sites[unsigned(a)];}
    void loaded(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        switch(a) {
        case object_argument:case partition_flags:gp[eax]=v;break;
        case headers:gp[ecx]=v;break;
        case datums:gp[edx]=v;break;
        case object_address:gp[ebp]=v;break;
        case location_argument:gp[edi]=v;break;
        case local_cluster:gp[edi]=gp[esi];break;                                                   // MOV EDI,ESI (after the CMP)
        case location_first:gp[ecx]=v;break;
        case location_second:gp[edx]=v;gp[ecx]=(gp[ecx]&0xffff0000u)|(v&0xffffu);break;                // MOV CX,DX
        case flags_placed:case flags_unplaced:case structure_address:case cluster_count:case radius:gp[ecx]=v;break;
        case object_again:gp[esi]=v;break;
        case set_argument:gp[eax]=v;break;
        case connected_flags:gp[edx]=v;break;
        case header_flags:set_low_byte(gp[ecx],v);break;
        case game_address:gp[edx]=v;break;
        case release_flags:gp[eax]=v;break;
        case unplaced_load:set_low_byte(gp[eax],v);break;
        default:break;
        }
    }
    void stored(Access a,std::uint32_t,std::uint32_t v) {
        auto &gp=this->gp;
        using enum Access;
        if(a==set_fill){gp[edi]+=step(4u);--gp[ecx];}
        if(a==set_fill_byte){gp[edi]+=step(1u);--gp[ecx];}
        if(a==set_store)gp[eax]=v;                                                                     // LEA EAX,[ESP+30]
    }
    void indexing(std::uint32_t handle) {                                                              // PUSH EBX; AND EAX,FFFF; PUSH EBP; LEA EAX,[EAX+EAX*2]
        auto &gp=this->gp;
        put(0xba640u,gp[ebx]);gp[eax]=this->flags.logic(handle&0xffffu,4);put(0xba646u,gp[ebp]);gp[eax]*=3u;
    }
    void unplaced_tested(std::uint32_t cluster) {                                                      // CMP word [header+4],-1 (EBX the header)
        this->flags.minus(cluster&0xffffu,0xffffu,2);
        this->gp[ebx]=this->gp[edx]+4u*this->gp[eax];
    }
    void saving(){put(0xba657u,this->gp[esi]);put(0xba658u,this->gp[edi]);}
    void location_tested(std::uint32_t location){this->test32(location);}                             // TEST EDI,EDI
    void locate(std::uint32_t point,std::uint32_t out,Kept &kept) {                                    // LEA EDX,point; (LEA ESI,out); CALL 11E600
        auto &gp=this->gp;
        gp[edx]=point;gp[esi]=out;
        direct(located_calls++?0xba67fu:0xba66du,0x11e600u,0u,kept);
        this->reached(Branch::located);
    }
    void cleared(){this->gp[eax]=this->flags.logic(0xffffffffu,4);}                                    // OR EAX,-1
    void cluster_tested(std::uint32_t cluster) {                                                       // CMP word [...],-1
        this->flags.minus(cluster&0xffffu,0xffffu,2);
    }
    void flagged(std::uint32_t flags) {                                                                // MOV EDX,ECX; SHR EDX,15; TEST DL,1
        auto &gp=this->gp;auto &fl=this->flags;
        gp[ecx]=flags;gp[edx]=fl.right(flags,logic::bit::every_cluster,4,false);fl.logic(gp[edx]&1u,1);
    }
    void filling(std::uint32_t clusters) {                                                             // ADD ECX,1F; SAR ECX,5; SHL ECX,2; MOV EDX,ECX; SHR ECX,2; LEA EDI,[ESP+30]
        auto &gp=this->gp;auto &fl=this->flags;
        gp[ecx]=fl.shift(fl.right(fl.plus(clusters,0x1fu,4),5u,4,true),2u,4);gp[edx]=gp[ecx];gp[ecx]=fl.right(gp[ecx],2u,4,false);
        gp[edi]=local(logic::local::cluster_set);this->reached(Branch::every_cluster);
    }
    void filling_rest(){auto &gp=this->gp;gp[ecx]=this->flags.logic(gp[edx]&3u,4);}                   // MOV ECX,EDX; AND ECX,3
    void summarize(Kept &kept) {                                                                       // LEA EDI,[ESP+1C]; MOV EDX,ESI; CALL BA910
        auto &gp=this->gp;
        gp[edi]=local(logic::local::summary);gp[edx]=kept.source;
        direct(0xba701u,0xba910u,0u,kept);
    }
    void partition_tested(std::uint32_t flags,std::uint32_t partition) {                               // SHR ECX,9; TEST CL,1; MOV EAX,partition
        auto &gp=this->gp;auto &fl=this->flags;
        gp[ecx]=fl.right(flags,logic::bit::active_partition,4,false);fl.logic(gp[ecx]&1u,1);gp[eax]=partition;
    }
    void insert_arguments(std::uint32_t overflow,std::uint32_t payload) {                              // LEA EDX; PUSH EDX; LEA ECX; PUSH ECX
        auto &gp=this->gp;
        gp[edx]=overflow;put(0xba71fu,overflow);gp[ecx]=payload;put(0xba724u,payload);
    }
    void insert_object(const Kept &kept,std::uint32_t radius_value,std::uint32_t partition) {
        auto &gp=this->gp;
        put(0xba728u,logic::summary_size);gp[edx]=kept.object+logic::object::location;put(0xba72du,gp[edx]);
        gp[ecx]=radius_value;put(0xba72eu,radius_value);gp[edx]=kept.object+logic::object::center;put(0xba732u,gp[edx]);
        gp[ecx]=kept.object+logic::object::head;put(0xba736u,gp[ecx]);put(0xba737u,kept.source);put(0xba738u,partition);
    }
    void insert(std::uint32_t,Kept &kept){direct(0xba740u,0x1d2f10u,0x24u,kept);}
    void connecting(std::uint32_t flags) {                                                             // PUSH 0; PUSH 0; OR EDX,100
        put(0xba748u,0u);put(0xba74au,0u);this->gp[edx]=this->flags.logic(flags,4);
    }
    void registering(std::uint32_t flags,const Kept &kept) {                                           // PUSH 1; PUSH 0; OR CL,40; PUSH ESI
        put(0xba758u,1u);put(0xba75au,0u);set_low_byte(this->gp[ecx],this->flags.logic(flags&0xffu,1));put(0xba75fu,kept.source);
    }
    void notify(Kept &kept){direct(0xba763u,0xc0f60u,0x14u,kept);}
    std::uint32_t in_active(std::uint32_t clusters,Kept &kept) {                                       // ADD EDX,11B8; PUSH EDX; MOV EAX,ESI; CALL B8D90; TEST AL,AL
        auto &gp=this->gp;
        gp[edx]=this->flags.plus(clusters-logic::game::active_clusters,logic::game::active_clusters,4);put(0xba774u,gp[edx]);gp[eax]=kept.source;
        direct(0xba777u,0xb8d90u,4u,kept);
        this->test8(gp[eax]&0xffu);
        return gp[eax];
    }
    void activate(Kept &kept){this->gp[edi]=kept.source;direct(0xba782u,0xb92c0u,0u,kept);}            // MOV EDI,ESI; CALL B92C0
    void release_tested(std::uint32_t flags) {                                                         // SHR EAX,11; TEST AL,1
        auto &gp=this->gp;auto &fl=this->flags;
        gp[eax]=fl.right(flags,logic::bit::releasable,4,false);fl.logic(gp[eax]&1u,1);
    }
    std::uint32_t keeps(Kept &kept) {                                                                  // MOV EAX,ESI; CALL B1870; TEST AL,AL
        auto &gp=this->gp;
        gp[eax]=kept.source;direct(0xba795u,0xb1870u,0u,kept);
        this->test8(gp[eax]&0xffu);
        return gp[eax];
    }
    void release(Kept &kept){put(0xba79eu,kept.source);direct(0xba79fu,0xba570u,4u,kept);}             // PUSH ESI; CALL BA570
    void park(Kept &kept){this->gp[edx]=kept.source;direct(0xba7a8u,0xb9330u,0u,kept);}                // MOV EDX,ESI; CALL B9330
    void unplaced_now(std::uint32_t unplaced){this->test8(unplaced&0xffu);}                            // TEST AL,AL
    void placed_tested(std::uint32_t cluster){this->flags.minus(cluster&0xffffu,0xffffu,2);}           // CMP word [EBX+4],-1
    void relink(Kept &kept) {                                                                          // CALL 1496F0; PUSH ESI; PUSH ESI; CALL 1CC290; CALL 282C10; CALL 1496F0
        routed(0xba7bcu,0x1496f0u,kept);
        put(0xba7c1u,kept.source);put(0xba7c2u,kept.source);direct(0xba7c3u,0x1cc290u,8u,kept);
        routed(0xba7c8u,0x282c10u,kept);
        routed(0xba7cdu,0x1496f0u,kept);
        this->reached(Branch::relinked);
    }
    void run() {
        auto &gp=this->gp;auto &fl=this->flags;
        gp[esp]=fl.minus(gp[esp],0x60u,4);depth=0x60u;                                                 // SUB ESP,60
        Memory<Adapter> memory{*this};
        this->reached(unsigned(logic::object_connect::connect(memory,*this)));
        gp[edi]=take(0xba7d2u);gp[esi]=take(0xba7d3u);gp[ebp]=take(0xba7d4u);gp[ebx]=take(0xba7d5u);
        gp[esp]=fl.plus(gp[esp],0x60u,4);                                                              // ADD ESP,60
        this->ret(0xba7d9u,8u);this->publish(0xba7d9u);
    }
};
template<class State,class Reference,class Children,class Dispatch> bool bridge(State &s,Reference reference,Children children,Dispatch dispatch) {
    return run_owner(s,0xba630u,0xba7d9u,reference_mode(),verify_mode(),coverage(),summary(),reference,
        [&](State &v,std::uint32_t *c,Trace<State> *t){return Adapter<State,Children,Dispatch>(v,c,t,children,dispatch);});
}
} // namespace halo2::native_object_connect_detail
