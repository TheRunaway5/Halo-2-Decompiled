#pragma once
#include "halo2/source_live_model_draws_tu15.hpp"

namespace halo2 {
// Owning caller values only. The built observer belongs to local presentation
// and is deliberately absent from a transferable source execution journal.
struct SourceExecutionModelInputTu15 {
    SourceObjectRenderEntryContext entry;
    SourceObjectDrawContext draw;
    Vec3 forward;
    unsigned view{},local_players=1;
    bool view_modifier{};
    std::uint32_t children=1;
};
inline SourceExecutionModelInputTu15 source_execution_model_input(const SourceLiveModelDrawContextTu15 &v){
    return {v.entry,v.draw,v.forward,v.view,v.local_players,v.view_modifier,v.children};
}
inline SourceLiveModelDrawContextTu15 source_execution_model_context(const SourceExecutionModelInputTu15 &v){
    SourceLiveModelDrawContextTu15 context;
    context.entry=v.entry;context.draw=v.draw;context.forward=v.forward;context.view=v.view;
    context.local_players=v.local_players;context.view_modifier=v.view_modifier;context.children=v.children;
    return context;
}
}
