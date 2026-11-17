#pragma once
#include <cstdint>

namespace halo2 {
// DATA policy shared by the full314710 frame and the deliberately narrower
// one-vertex value facade. Views own transport, lazy formal acquisition and
// profiling scratch; the policy never supplies a hull result or clock value.
enum class NativeTu15SolverHullContextPhase {
    basis_first,basis_second,basis_third,basis_fourth,
    first_shape,second_shape,differences,simplex,initial_state,
};
struct NativeTu15SolverHullContext {
    std::uint32_t transform{},context{},first{},second{},differences{},simplex{},output{},result{};
};
template<class Memory>
void native_tu15_solver_hull_profile(Memory&memory,NativeTu15SolverHullContext&data,bool ending){
    const auto cursor=memory.profile_cursor(data,ending);
    const auto limit=memory.profile_limit(data,ending);
    memory.profile_ready(data,ending,cursor,limit);
    if(cursor>=limit)return;
    memory.profile_tag(data,ending,cursor,ending?0x41c394u:0x41d784u);
    const auto timestamp=memory.timestamp(data,ending);
    memory.stage_timestamp(data,ending,std::uint32_t(timestamp));
    const auto low=memory.reload_timestamp(data,ending);
    memory.profile_low(data,ending,cursor,low);
    memory.advance_profile(data,ending,cursor);
}
template<class Memory>
std::uint32_t native_tu15_finish_solver_hull_context(Memory&memory,NativeTu15SolverHullContext&data){
    native_tu15_solver_hull_profile(memory,data,true);
    return memory.finish(data);
}
template<class Memory>
std::uint32_t native_tu15_build_solver_hull_context(Memory&memory,NativeTu15SolverHullContext&data){
    using P=NativeTu15SolverHullContextPhase;
    memory.begin(data);
    native_tu15_solver_hull_profile(memory,data,false);
    data.transform=memory.relative_transform(data);
    const auto first=memory.vector(P::basis_first,data,data.transform);
    data.first=memory.first_shape(data);
    data.second=memory.second_shape(data);
    memory.store_vector(P::basis_first,data,first);
    const auto second=memory.vector(P::basis_second,data,data.transform+0x10u);
    memory.store_vector(P::basis_second,data,second);
    const auto third=memory.vector(P::basis_third,data,data.transform+0x20u);
    memory.store_word(P::first_shape,data,data.first);
    data.differences=memory.differences(data);
    memory.store_vector(P::basis_third,data,third);
    const auto fourth=memory.vector(P::basis_fourth,data,data.transform+0x30u);
    data.simplex=memory.simplex(data);
    memory.store_word(P::second_shape,data,data.second);
    data.output=memory.output(data);
    memory.store_word(P::differences,data,data.differences);
    memory.prepare_child(data);
    memory.store_vector(P::basis_fourth,data,fourth);
    memory.store_word(P::simplex,data,data.simplex);
    memory.store_word(P::initial_state,data,0u);
    data.result=memory.hull_child(data);
    return native_tu15_finish_solver_hull_context(memory,data);
}
} // namespace halo2
