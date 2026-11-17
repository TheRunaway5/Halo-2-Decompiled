void native_tu15_clear_solver_hull_polytope_flags(SourcePhysicsContactMemory&memory,
    std::uint32_t polytope,const SourcePhysicsContactRecordObserver&observe,
    NativeTu15SolverHullPolytopeStateStats*stats) {
    if(stats)++stats->clear_calls;
    try {
        const auto word=[&](std::uint32_t p) {
            std::array<std::byte,4>b{};memory.read(p,b);std::uint32_t n{};
            for(unsigned k=0;k<4;++k)n|=std::to_integer<unsigned>(b[k])<<(k*8u);
            return n;
        };
        const auto clear=[&](std::uint32_t pc,std::uint32_t p,unsigned width) {
            std::array<std::byte,4>zero{};SourcePhysicsContactRecordStore e{pc,p,{},{}};
            if(observe){e.before.resize(width);memory.read(p,e.before);}
            memory.write(p,std::span(zero).first(width));
            if(observe){e.written.assign(width,std::byte{});observe(e);}
        };
        auto count=word(polytope+0x1b4u);
        if(std::bit_cast<std::int32_t>(count)>0) {
            std::uint32_t index{},offset{};
            do {
                const auto array=word(polytope+0x1b0u);
                clear(0x314896u,array+offset+0x44u,1u);
                count=word(polytope+0x1b4u);++index;offset+=0x50u;
                if(stats)++stats->facets_cleared;
                if(std::bit_cast<std::int32_t>(index)>=std::bit_cast<std::int32_t>(count))break;
            }while(true);
        }
        count=word(polytope+0x2104u);
        if(std::bit_cast<std::int32_t>(count)>0) {
            std::uint32_t index{},offset{};
            do {
                const auto array=word(polytope+0x2100u);
                clear(0x3148c6u,array+offset+0x10u,4u);
                count=word(polytope+0x2104u);++index;offset+=0x20u;
                if(stats)++stats->edges_cleared;
                if(std::bit_cast<std::int32_t>(index)>=std::bit_cast<std::int32_t>(count))break;
            }while(true);
        }
        if(stats)++stats->clear_completed;
    }catch(...){if(stats)++stats->refusals;throw;}
}
