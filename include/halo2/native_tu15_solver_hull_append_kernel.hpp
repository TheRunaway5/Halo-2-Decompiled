#pragma once
#include <cstdint>
namespace halo2 {
enum class NativeTu15SolverHullAppendPhase {initial,first_point,second_point,final_count};
template<class Vector>struct NativeTu15SolverHullAppend {
    std::uint32_t context{},simplex{},index{},pair{},differences{},destination{};
    Vector second{},first{},difference{},point{};
};
template<class Memory>
void native_tu15_append_solver_hull_point(Memory&memory,NativeTu15SolverHullAppend<typename Memory::Vector>&data){
    using P=NativeTu15SolverHullAppendPhase;
    memory.begin(data);
    data.simplex=memory.simplex(P::initial,data);
    data.index=memory.count(P::initial,data);
    data.pair=memory.point_pair(data);
    data.second=memory.vector(P::initial,data,data.pair+0x20u);
    data.first=memory.first_point(data);
    data.differences=memory.differences(data);
    data.destination=data.differences+(data.index<<4u);
    memory.difference_destination(data);
    data.difference=memory.subtract(data.first,data.second);
    memory.store_difference(data);
    data.simplex=memory.simplex(P::first_point,data);
    data.index=memory.count(P::first_point,data);
    data.point=memory.vector(P::first_point,data,data.pair);
    data.destination=data.simplex+(data.index<<4u);
    memory.point_destination(P::first_point,data);
    memory.store_point(P::first_point,data);
    data.simplex=memory.simplex(P::second_point,data);
    data.index=memory.count(P::second_point,data);
    data.point=memory.vector(P::second_point,data,data.pair+0x10u);
    data.destination=data.simplex+((data.index+4u)<<4u);
    memory.point_destination(P::second_point,data);
    memory.store_point(P::second_point,data);
    data.simplex=memory.simplex(P::final_count,data);
    memory.increment_count(data);
    memory.finish(data);
}
} // namespace halo2
