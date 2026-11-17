#pragma once
#include "halo2/native_tu15_solver_hull_clear_kernel.hpp"
#include "halo2/native_tu15_solver_hull_polytope_state.hpp"
#include <array>
#include <bit>

namespace halo2::native_solver_hull_clear_value_detail {
using Table=NativeTu15SolverHullClearTable;
class View {
    SourcePhysicsContactMemory&memory_;
    std::uint32_t owner_;
    const SourcePhysicsContactRecordObserver&observe_;
    NativeTu15SolverHullPolytopeStateStats*stats_;
    std::uint32_t index_{},offset_{};
    std::uint32_t word(std::uint32_t address){
        std::array<std::byte,4>bytes{};memory_.read(address,bytes);std::uint32_t value{};
        for(unsigned i=0;i<bytes.size();++i)value|=std::to_integer<unsigned>(bytes[i])<<(8u*i);
        return value;
    }
public:
    View(SourcePhysicsContactMemory&memory,std::uint32_t owner,const SourcePhysicsContactRecordObserver&observe,
         NativeTu15SolverHullPolytopeStateStats*stats):memory_(memory),owner_(owner),observe_(observe),stats_(stats){}
    void begin(){}
    bool start(Table table){
        index_=offset_=0u;
        return std::bit_cast<std::int32_t>(word(owner_+native_tu15_solver_hull_clear_layout(table).count))>0;
    }
    void clear(Table table){
        const auto layout=native_tu15_solver_hull_clear_layout(table);
        const auto address=word(owner_+layout.base)+offset_+layout.field;
        SourcePhysicsContactRecordStore event{layout.store_pc,address,{},{}};
        if(observe_){event.before.resize(layout.width);memory_.read(address,event.before);}
        std::array<std::byte,4>zero{};memory_.write(address,std::span(zero).first(layout.width));
        // The old public view rechecks after write, independently of the
        // exact retained-source helper's empty-observer entry policy.
        if(observe_){event.written.assign(layout.width,std::byte{});observe_(event);}
    }
    bool next(Table table){
        const auto layout=native_tu15_solver_hull_clear_layout(table);
        const auto count=word(owner_+layout.count);++index_;offset_+=layout.stride;
        if(stats_){if(table==Table::facets)++stats_->facets_cleared;else ++stats_->edges_cleared;}
        return std::bit_cast<std::int32_t>(index_)<std::bit_cast<std::int32_t>(count);
    }
    void finish(){}
};
} // namespace halo2::native_solver_hull_clear_value_detail
