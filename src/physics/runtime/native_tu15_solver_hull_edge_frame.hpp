#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "halo2/native_tu15_solver_hull_edge_kernel.hpp"
#include "physics/runtime/native_tu15_mopp_ray_scale_frame.hpp"
namespace halo2::native_solver_hull_edge_detail {
using P=NativeTu15SolverHullEdgePhase;
struct Noop{template<class State>void operator()(State&,std::uint32_t)const{}};
// Private complete80-site adapter. No source/default admission is installed.
// Only ordinary backed providers/readonly GP callbacks are claimed initially.
enum class Publication { physical,retained };
template<class State,class Call,class Before,class Checkpoint,Publication Mode>class Frame:
    public native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>> {
    using Base=native_mopp_ray_caller_detail::IntegerFrame<State,Before,native_mopp_ray_caller_detail::ScopeIo<State>>;
    using Base::s_;using Base::at;using Base::word;using Base::pop;
    native_raw_x87_detail::Scope<State>&fp_;Call&call_;Checkpoint&checkpoint_;
    void retired(std::uint32_t pc){
        if constexpr(Mode==Publication::retained){switch(pc){
        case 0x313470u:checkpoint_(s_,0x313471u);break;
        case 0x313479u:checkpoint_(s_,0x313480u);break;
        case 0x313480u:checkpoint_(s_,0x313481u);break;
        case 0x313481u:checkpoint_(s_,0x313484u);break;
        case 0x313484u:checkpoint_(s_,0x313487u);break;
        case 0x313487u:checkpoint_(s_,0x313488u);break;
        case 0x313488u:checkpoint_(s_,0x313489u);break;
        case 0x31348du:checkpoint_(s_,0x31348eu);break;
        case 0x313491u:checkpoint_(s_,0x313492u);break;
        case 0x313494u:checkpoint_(s_,0x313499u);break;
        case 0x313499u:checkpoint_(s_,0x31349eu);break;
        case 0x31349eu:checkpoint_(s_,0x3134a1u);break;
        case 0x3134a1u:checkpoint_(s_,0x3134a2u);break;
        case 0x3134a6u:checkpoint_(s_,0x3134a7u);break;
        case 0x3134a9u:checkpoint_(s_,0x3134aeu);break;
        case 0x3134aeu:checkpoint_(s_,0x3134b1u);break;
        case 0x3134b1u:checkpoint_(s_,0x3134b4u);break;
        case 0x3134b4u:checkpoint_(s_,0x3134b9u);break;
        case 0x3134b9u:checkpoint_(s_,0x3134beu);break;
        case 0x3134beu:checkpoint_(s_,0x3134c1u);break;
        case 0x3134dfu:checkpoint_(s_,0x3134e3u);break;
        case 0x3134e7u:checkpoint_(s_,0x3134ecu);break;
        case 0x3134f0u:checkpoint_(s_,0x3134f4u);break;
        case 0x3134f4u:checkpoint_(s_,0x3134f8u);break;
        case 0x3134f8u:checkpoint_(s_,0x3134fbu);break;
        case 0x313521u:checkpoint_(s_,0x313525u);break;
        case 0x313525u:checkpoint_(s_,0x313529u);break;
        case 0x313530u:checkpoint_(s_,0x313535u);break;
        case 0x313535u:checkpoint_(s_,0x313539u);break;
        case 0x313539u:checkpoint_(s_,0x31353cu);break;
        case 0x31353cu:checkpoint_(s_,0x313540u);break;
        case 0x313540u:checkpoint_(s_,0x313544u);break;
        case 0x313544u:checkpoint_(s_,0x313547u);break;
        case 0x313547u:checkpoint_(s_,0x313548u);break;
        case 0x313548u:checkpoint_(s_,0x313549u);break;
        case 0x313549u:checkpoint_(s_,0x31354au);break;
        case 0x31354cu:checkpoint_(s_,0x31354du);break;
        case 0x313550u:checkpoint_(s_,0x313554u);break;
        case 0x313554u:checkpoint_(s_,0x313555u);break;
        case 0x313555u:checkpoint_(s_,0x313558u);break;
        case 0x313558u:checkpoint_(s_,0x313559u);break;
        case 0x313559u:checkpoint_(s_,0x31355au);break;
        case 0x31355cu:checkpoint_(s_,0x31355du);break;
        default:throw std::logic_error("Unsupported hull-edge source frontier");}}
    }
    void retained_store(std::uint32_t pc,std::uint32_t address,std::uint32_t value,bool positioned=false){
        if(!positioned)at(pc);std::array<std::byte,4>bytes{};std::memcpy(bytes.data(),&value,4);
        // Retained w() selects its empty-observer fast path at entry. A
        // provider that installs an observer during this write does not get
        // a completed event for the already entered unobserved instruction.
        if(!s_.observe){fp_.io([&]{s_.memory.write(address,bytes);});return;}
        SourcePhysicsContactRecordStore event{pc,address,{},{}};event.before.resize(4);event.written.assign(bytes.begin(),bytes.end());
        fp_.io([&]{s_.memory.read(address,event.before);});fp_.io([&]{s_.memory.write(address,event.written);});
        if(s_.observe)fp_.io([&]{s_.observe(event);});
    }
    void push(std::uint32_t pc,std::uint32_t value){
        if constexpr(Mode==Publication::retained){at(pc);s_.gp[4]-=4u;retained_store(pc,s_.gp[4],value,true);}
        else Base::push(pc,value);
        retired(pc);
    }
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t value){if constexpr(Mode==Publication::retained)retained_store(pc,address,value);else Base::store(pc,address,value,[]{});retired(pc);}
    void call_pair(std::uint32_t pc,std::uint32_t return_pc){
        const auto address=s_.gp[4]-4u;
        if constexpr(Mode==Publication::retained){at(pc);s_.gp[4]=address;retained_store(pc,address,return_pc,true);}
        else Base::store(pc,address,return_pc,[&]{s_.gp[4]=address;});
        fp_.child([&]{call_(s_,0x313390u,pc);retired(pc);});
    }
    void vector_store(std::uint32_t pc,std::uint32_t address,__m128 value){
        at(pc);if(address&15u){const auto flags=s_.other_flags|s_.flags.bits();throw native_mopp_ray_caller_detail::AlignedLoadFault{pc,address,flags,flags|0x10000u};}
        std::array<std::byte,16>bytes{};std::memcpy(bytes.data(),&value,16);
        for(unsigned offset:{0u,8u}){
            const auto part=std::span(bytes).subspan(offset,8);
            if constexpr(Mode==Publication::retained){
                if(!s_.observe){fp_.io([&]{s_.memory.write(address+offset,part);});continue;}
            }
            SourcePhysicsContactRecordStore event{pc,address+offset,{},{}};
            if(s_.observe){event.before.resize(8);fp_.io([&]{s_.memory.read(event.address,event.before);});}
            fp_.io([&]{s_.memory.write(event.address,part);});
            if(s_.observe){event.written.assign(part.begin(),part.end());fp_.io([&]{s_.observe(event);});}
        }
        retired(pc);
    }
    static std::uint32_t low(__m128 value){std::uint32_t bits{};std::memcpy(&bits,&value,4);return bits;}
public:
    using Data=NativeTu15SolverHullEdge<__m128>;
    Frame(State&s,Call&call,Before&before,Checkpoint&checkpoint,native_raw_x87_detail::Scope<State>&scope):
        Base{s,before,native_mopp_ray_caller_detail::ScopeIo<State>{&scope}},fp_(scope),call_(call),checkpoint_(checkpoint){}
    void begin(Data&d){auto&g=s_.gp;push(0x313470u,g[5]);g[5]=g[4];g[4]=s_.flags.logic(g[4]&0xfffffff0u,4);g[4]=s_.flags.minus(g[4],0x54u,4);d.context=g[1];}
    std::uint32_t direction_owner(const Data&){push(0x313480u,s_.gp[3]);at(0x313481u);const auto value=s_.gp[3]=word(s_.gp[5]+8u);retired(0x313481u);return value;}
    void prepare_first(Data&d){auto&g=s_.gp;push(0x313487u,g[6]);push(0x313488u,g[7]);g[0]=g[4]+0x30u;push(0x31348du,g[0]);if constexpr(Mode==Publication::retained){d.axis=s_.xmm[1];d.sign=s_.xmm[0];}}
    void support_first(const Data&d){auto&g=s_.gp;s_.xmm[1]=d.negative;push(0x313491u,g[3]);g[7]=g[1];vector_store(0x313494u,g[4]+0x28u,s_.xmm[1]);call_pair(0x313499u,0x31349eu);}
    std::uint32_t output_owner(const Data&){at(0x31349eu);const auto value=s_.gp[6]=word(s_.gp[5]+0x10u);retired(0x31349eu);return value;}
    void support_second(const Data&){auto&g=s_.gp;push(0x3134a1u,g[6]);g[1]=g[4]+0x24u;push(0x3134a6u,g[1]);g[1]=g[7];call_pair(0x3134a9u,0x3134aeu);}
    std::uint32_t origin_owner(const Data&){at(0x3134aeu);const auto value=s_.gp[2]=word(s_.gp[5]+0xcu);retired(0x3134aeu);return value;}
    __m128 vector(P phase,Data&d){
        std::uint32_t pc{},address{};unsigned reg{};
        switch(phase){
        case P::sign:pc=0x313479u;address=0x461960u;reg=0;break;
        case P::axis:pc=0x313484u;address=s_.gp[3];reg=1;break;
        case P::origin:pc=0x3134b1u;address=s_.gp[2];reg=1;break;
        case P::first_point:pc=0x3134b4u;address=s_.gp[4]+0x30u;reg=3;break;
        case P::first_transformed:pc=0x3134b9u;address=s_.gp[4]+0x50u;reg=2;break;
        case P::axis_again:pc=0x3134beu;address=s_.gp[3];reg=4;break;
        case P::second_transformed:
            s_.xmm[0]=d.first_product;s_.xmm[4]=d.first_xy;s_.xmm[5]=d.first_z;
            pc=0x3134dfu;address=s_.gp[6]+0x20u;reg=0;break;
        case P::negative_axis:s_.xmm[5]=d.first_sum;pc=0x3134e7u;address=s_.gp[4]+0x20u;reg=4;break;
        case P::second_point:pc=0x3134f8u;address=s_.gp[6];reg=5;break;
        case P::first_local_second:pc=0x313530u;address=s_.gp[4]+0x40u;reg=0;break;
        }
        const auto value=s_.xmm[reg]=Base::vector(pc,address);
        if constexpr(Mode==Publication::retained){
            if(phase==P::axis_again){d.position=s_.xmm[1];d.first=s_.xmm[3];d.first_transformed=s_.xmm[2];}
            else if(phase==P::second_transformed){d.first_xy=s_.xmm[4];d.first_z=s_.xmm[5];}
            else if(phase==P::second_point){d.second_transformed=s_.xmm[0];d.position=s_.xmm[1];d.negative_again=s_.xmm[4];}
        }
        retired(pc);return value;
    }
    __m128 xor_bits(__m128 a,__m128 b){return _mm_xor_ps(a,b);}
    template<unsigned Mask>__m128 shuffle(__m128 a,__m128 b){return _mm_shuffle_ps(a,b,Mask);}
    __m128 subtract(__m128 a,__m128 b){asm volatile("subps %1,%0":"+x"(a):"x"(b):"memory");return a;}
    __m128 multiply(__m128 a,__m128 b){asm volatile("mulps %1,%0":"+x"(a):"x"(b):"memory");return a;}
    __m128 add_scalar(__m128 a,__m128 b){asm volatile("addss %1,%0":"+x"(a):"x"(b):"memory");return a;}
    void record_first_and_load_comparison(const Data&d){
        s_.gp[0]=s_.gp[4]+0x18u;store(0x3134f0u,s_.gp[0],low(s_.xmm[5]));
        at(0x3134f4u);const auto bits=word(s_.gp[4]+0x18u);fp_.require_capacity(1u);
        asm volatile("flds %0"::"m"(bits):"memory","st");fp_.publish();retired(0x3134f4u);
    }
    void record_second(const Data&d){
        s_.xmm[0]=d.second_product;s_.xmm[1]=d.second_xy;s_.xmm[4]=d.second_sum;s_.xmm[5]=d.second_delta;
        s_.gp[1]=s_.gp[4]+0x1cu;store(0x313521u,s_.gp[1],low(d.second_sum));
    }
    bool select_first(const Data&){
        at(0x313525u);const auto bits=word(s_.gp[4]+0x1cu);std::uint16_t status{};
        asm volatile("fcomps %1;fnstsw %0":"=a"(status):"m"(bits):"memory","st");fp_.publish();retired(0x313525u);
        s_.gp[0]=(s_.gp[0]&0xffff0000u)|status;s_.flags.logic((s_.gp[0]>>8u)&1u,1);
        return s_.flags.zf;
    }
    std::uint32_t first_projection_again(const Data&){at(0x313535u);const auto value=s_.gp[2]=word(s_.gp[4]+0x18u);retired(0x313535u);return value;}
    void promote_first(const Data&){
        vector_store(0x313539u,s_.gp[6],s_.xmm[3]);vector_store(0x31353cu,s_.gp[6]+0x10u,s_.xmm[0]);
        vector_store(0x313540u,s_.gp[6]+0x20u,s_.xmm[2]);store(0x313544u,s_.gp[6]+0x30u,s_.gp[2]);
    }
    std::uint32_t second_projection_again(const Data&){at(0x313550u);const auto value=s_.gp[0]=word(s_.gp[4]+0x1cu);retired(0x313550u);return value;}
    void keep_second(const Data&){s_.gp[7]=pop(0x313554u);retired(0x313554u);store(0x313555u,s_.gp[6]+0x30u,s_.gp[0]);}
    void finish(const Data&d){auto&g=s_.gp;
        if(d.selected_first){g[7]=pop(0x313547u);retired(0x313547u);g[6]=pop(0x313548u);retired(0x313548u);g[3]=pop(0x313549u);retired(0x313549u);g[4]=g[5];g[5]=pop(0x31354cu);retired(0x31354cu);pop(0x31354du);}
        else{g[6]=pop(0x313558u);retired(0x313558u);g[3]=pop(0x313559u);retired(0x313559u);g[4]=g[5];g[5]=pop(0x31355cu);retired(0x31355cu);pop(0x31355du);}
        g[4]+=12u;
    }
};
template<Publication Mode=Publication::physical,class State,class Call,class Before=Noop,class Checkpoint=Noop>void run_private_owner(State&s,Call call,Before before={},Checkpoint checkpoint={}){
    if(!native_raw_x87_detail::supported(s)||s.fp.count>7u)throw std::runtime_error("Private hull-edge requires dense masked FP with one free slot");
    native_raw_x87_detail::Scope scope{s};Frame<State,Call,Before,Checkpoint,Mode>view{s,call,before,checkpoint,scope};typename decltype(view)::Data data;
    native_tu15_solver_hull_edge_kernel(view,data);
}
} // namespace halo2::native_solver_hull_edge_detail
#endif
