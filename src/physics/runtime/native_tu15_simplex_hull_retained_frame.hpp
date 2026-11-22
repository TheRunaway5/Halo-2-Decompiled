#pragma once
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
#include "physics/runtime/native_tu15_simplex_hull_owner_frame.hpp"

namespace halo2::native_simplex_hull_retained_detail {
// Private retained-source transport. This view preserves modeled Flags, the
// published SP before PUSH provider I/O, and the generated store helpers.
// Its Before/Memory/observer ABI policy is not a production admission.
template<class State,class Call,class Before,bool Actual> class Io {
    State& s_;
    Call& call_;
    Before& before_;
    native_raw_x87_detail::Scope<State>& fp_;
    static constexpr std::uint32_t observe_pc(std::uint32_t actual){return Actual?actual:actual-0x3af0u;}
    void at(std::uint32_t pc){
        const auto observed=observe_pc(pc);
        if(s_.instruction_cursor&&s_.instruction_cursor->position)*s_.instruction_cursor->position=observed;
        if(!native_raw_x87_detail::skip_hook<Before>())fp_.io([&]{before_(s_,observed);});
    }
    std::uint32_t read(std::uint32_t address,unsigned width){
        std::uint32_t value;fp_.io([&]{value=physics_step_detail::r(s_.memory,address,width);});return value;
    }
    void write(std::uint32_t pc,std::uint32_t address,unsigned width,std::uint32_t value){
        fp_.io([&]{physics_step_detail::w(s_.memory,s_.observe,observe_pc(pc),address,width,value);});
    }
public:
    Io(State& s,Call& call,Before& before,native_raw_x87_detail::Scope<State>& fp)
        :s_(s),call_(call),before_(before),fp_(fp){}
    std::uint32_t word(std::uint32_t pc,std::uint32_t address){at(pc);return read(address,4u);}
    std::uint32_t byte(std::uint32_t pc,std::uint32_t address){at(pc);return read(address,1u);}
    __m128 mask(std::uint32_t pc,std::uint32_t address){
        at(pc);__m128 value;fp_.io([&]{value=physics_step_detail::v(s_.memory,address);});return value;
    }
    void vector(std::uint32_t pc,unsigned reg,std::uint32_t address){s_.xmm[reg]=mask(pc,address);}
    void store(std::uint32_t pc,std::uint32_t address,std::uint32_t value){at(pc);write(pc,address,4u,value);}
    void store_byte(std::uint32_t pc,std::uint32_t address,std::uint32_t value){at(pc);write(pc,address,1u,value);}
    void store_vector(std::uint32_t pc,std::uint32_t address,__m128 value){
        at(pc);fp_.io([&]{physics_step_detail::vw(s_.memory,s_.observe,observe_pc(pc),address,value);});
    }
    void push(std::uint32_t pc,std::uint32_t value){at(pc);fp_.io([&]{s_.push(observe_pc(pc),value);});}
    std::uint32_t pop(std::uint32_t pc){at(pc);std::uint32_t value;fp_.io([&]{value=s_.pop();});return value;}
    void call(std::uint32_t site,std::uint32_t returned,std::uint32_t target){
        push(site,returned);fp_.child([&]{call_(s_,target,site);});
    }
    void shift_index(unsigned reg){s_.gp[reg]=s_.flags.shift(s_.gp[reg],4u,4u);}
    void decrement(std::uint32_t pc,std::uint32_t address){
        at(pc);const auto value=s_.flags.increment(read(address,4u),4u,true);write(pc,address,4u,value);
    }
};
template<class State,class Memory> class Frame {
    State& s_;
    Memory& io_;
public:
    Frame(State& s,Memory& io):s_(s),io_(io){}
    void seed(){
        native_simplex_hull_seed_detail::Schedule view{s_,io_};native_tu15_simplex_hull_seed_kernel(view);
    }
    bool copyback_returns(){
        native_simplex_hull_copyback_detail::Schedule view{s_,io_};return native_tu15_simplex_hull_copyback_kernel(view);
    }
    void compact(){
        native_simplex_hull_compact_detail::Schedule view{s_,io_};native_tu15_simplex_hull_compact_kernel(view);
    }
    auto initial_reduction(){
        native_simplex_hull_reduction_route_detail::Schedule view{s_,io_};return native_tu15_simplex_hull_reduction_route_kernel(view);
    }
    auto redispatch(){
        native_simplex_hull_reduction_route_detail::Schedule view{s_,io_};
        return view.repack_reduced_counts()?view.resolve_reduction():NativeTu15SimplexReductionKind::degenerate;
    }
    auto face(bool first){
        native_simplex_hull_face_call_detail::Schedule<State,Memory,0u> view{s_,io_};return native_tu15_simplex_hull_face_call_kernel(view,first);
    }
    auto triangle(bool first){
        native_simplex_hull_triangle_call_detail::Schedule<State,Memory,0u> view{s_,io_};
        return native_tu15_simplex_hull_triangle_call_kernel(view,first);
    }
    void triangle_edges(bool first){
        native_simplex_hull_triangle_edges_detail::Schedule<State,Memory,0u,0x18u> view{s_,io_};
        native_tu15_simplex_hull_triangle_edges_kernel(view,first);
    }
    auto paired(bool first_two){
        native_simplex_hull_paired_call_detail::Schedule view{s_,io_};return native_tu15_simplex_hull_paired_call_kernel(view,first_two);
    }
    auto segments(){
        native_simplex_hull_segments_detail::Schedule view{s_,io_};return native_tu15_simplex_hull_segments_kernel(view);
    }
    auto plane(NativeTu15SimplexHullPlaneEntry entry){
        native_simplex_hull_plane_detail::Schedule<State,Memory> view{s_,io_};return native_tu15_simplex_hull_plane_kernel(view,entry);
    }
    void finish(NativeTu15SimplexHullFinish entry){
        native_simplex_hull_finish_detail::Schedule view{s_,io_};native_tu15_simplex_hull_finish_kernel(view,entry);
    }
};
template<bool Actual=true,class State,class Call,class Before> void run_private_hull_retained(State& s,Call call,Before before){
    if(s.image!=SourcePhysicsContactImage::TitleUpdate15||!native_raw_x87_detail::supported(s)||s.fp.count>4u)
        throw std::runtime_error("Private retained simplex hull requires supported actual TU1.5 with four free x87 slots");
    native_raw_x87_detail::Scope fp{s};
    Io<State,Call,Before,Actual> io{s,call,before,fp};Frame view{s,io};native_tu15_simplex_hull_owner_kernel(view);
}
} // namespace halo2::native_simplex_hull_retained_detail
#endif
