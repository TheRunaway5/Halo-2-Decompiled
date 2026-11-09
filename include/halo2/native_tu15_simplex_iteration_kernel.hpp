#pragma once
#include "halo2/native_tu15_simplex_projection_kernel.hpp"

namespace halo2 {
enum class NativeTu15SimplexReductionKind {
    points,second_edge,second_triangle,second_face,first_edge,segments,
    paired_two_three,first_triangle,paired_three_two,first_face,degenerate
};
enum class NativeTu15SimplexIterationPhase {
    first_count,epsilon,second_count,iteration,iteration_store,clear_progress
};
struct NativeTu15SimplexIteration {
    std::uint32_t work{},count_address{},first{},second{},iteration{},packed{},key{};
    NativeTu15SimplexReductionKind kind{NativeTu15SimplexReductionKind::degenerate};
};
namespace native_simplex_iteration_numeric {
inline void double_top(){
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("fadd %%st(0),%%st(0)":::"memory","st");
#else
    throw std::runtime_error("Exact native simplex iteration backend unavailable");
#endif
}
}
// Loop policy at30EA00..30EA37. Counts and counter are acquired at their live
// source boundaries. Shape classification owns the authentic immutable table
// access/semantic-kind mapping; this does not assume only well-formed pairs.
template<class Access>
NativeTu15SimplexReductionKind native_tu15_begin_simplex_iteration(Access&memory,NativeTu15SimplexIteration&d){
    using P=NativeTu15SimplexIterationPhase;
    d.kind=NativeTu15SimplexReductionKind::degenerate;
    d.first=memory.word_read(P::first_count,d,d.work);
    const auto epsilon=memory.word_read(P::epsilon,d,memory.frame(P::epsilon,d));
    memory.require_capacity(1u);native_simplex_projection_numeric::load(epsilon);
    d.second=memory.word_read(P::second_count,d,d.count_address);
    native_simplex_iteration_numeric::double_top();
    d.iteration=memory.word_read(P::iteration,d,memory.frame(P::iteration,d));
    d.packed=(d.first<<3u)|d.second;d.key=d.packed-9u;
    memory.prepare_packed(d);memory.store_epsilon(d);
    ++d.iteration;memory.prepare_counter(d);
    memory.word_write(P::iteration_store,d,memory.frame(P::iteration,d),d.iteration);
    memory.word_write(P::clear_progress,d,d.work+0x10u,0u);
    if(d.key<=24u)d.kind=memory.classify(d);
    memory.complete(d);return d.kind;
}
} // namespace halo2
