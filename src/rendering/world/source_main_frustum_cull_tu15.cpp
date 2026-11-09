#include "halo2/source_main_frustum_cull_tu15.hpp"
#include "halo2/detail/native_tu15_arithmetic_environment.hpp"
#include "halo2/native_tu15_frustum_draw_cull.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/source_live_model_geometry_tu15.hpp"
#include <bit>
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace halo2 {
namespace {
bool stats_enabled(){const auto* p=std::getenv("HALO2_NATIVE_CULL_WALK_STATS");return p&&*p;}
}
SourceMainFrustumCullTu15::SourceMainFrustumCullTu15(const Cache& cache,std::string_view scope)
    :SourceMainFrustumCullTu15([&cache](auto datum,auto section){return load_source_model_range_geometry(cache,datum,section);},scope){}
SourceMainFrustumCullTu15::SourceMainFrustumCullTu15(RangeLoader ranges,std::string_view scope)
    :ranges_(std::move(ranges)),scope_(scope){}
SourceMainFrustumCullTu15::~SourceMainFrustumCullTu15() {
    if(!stats_enabled())return;
    const auto& s=stats_;
    std::cerr<<"Native main cull: views="<<s.views<<" native_entries="<<s.native_entries<<
        " residual_entries="<<s.residual_entries<<" fallbacks="<<s.fallbacks<<
        " unit_mismatches="<<s.unit_mismatches<<" mask_mismatches="<<s.mask_mismatches<<
        " fp_mismatches="<<s.fp_mismatches<<" native_model_entries="<<s.native_model_entries<<
        " residual_model_entries="<<s.residual_model_entries<<" scope="<<scope_<<'\n';
    for(const auto& [datum,count]:native_models_)
        std::cerr<<"Native main model cull: scope="<<scope_<<" datum="<<datum<<" entries="<<count<<'\n';
}

NativeTu15PhysicsFrameFp SourceMainFrustumCullTu15::run(PhysicsLiveWorldTu15& world,const SourcePortalVisibilitySet* visibility,const BspEntry& bsp) {
#if !((defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER))
    (void)visibility;(void)bsp;return source_native_cull_walk_tu15(world);
#else
    static const bool oracle=[]{const auto* p=std::getenv("HALO2_MAIN_CULL_ORACLE");return p&&*p;}();
    const bool census=stats_enabled();
    constexpr std::uint32_t context=0x556e18u,units_at=0x4c6958u,arena_at=context+0x25cu;
    const auto& memory=world.memory();
    const auto word=[&](std::uint32_t p){return Bytes(memory.view(p,4)).u32(0);};
    const auto read_unit=[&](std::uint32_t at) {
        SourceCullDrawUnit unit;std::memcpy(&unit,memory.view(at,sizeof(unit)).data(),sizeof(unit));return unit;
    };
    const auto read_arena=[&] {
        SourceBspRangeArena out;
        std::memcpy(out.words.data(),memory.view(arena_at,sizeof(out.words)).data(),sizeof(out.words));
        out.used=Bytes(memory.view(context+0xa5cu,2)).u16(0);return out;
    };
    const auto write=[&](std::uint32_t at,const auto& v) {
        world.write_source_call_input(at,std::as_bytes(std::span(&v,1)));
    };
    ++stats_.views;
    return source_native_cull_walk_tu15(world,[&](auto index,auto entry,auto& arithmetic) {
        bool is_model{};
        const auto residual=[&]{++stats_.residual_entries;stats_.residual_model_entries+=is_model;return false;};
        if(!visibility||word(0x4cd3a0u)!=context||word(context+0x2a78u))return residual();
        const auto at=units_at+32u*std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(index))));
        const auto before=read_unit(at);
        is_model=before.words[1]!=0xffffffffu;
        if(!is_model&&!bsp)return residual();
        auto unit=before;auto arena=read_arena();const auto initial_arena=arena;
        auto native_fp=arithmetic;
        try {
            if(is_model) {
                const auto key=std::pair{unit.words[1],unit.words[3]>>9&511u};
                auto found=model_ranges_.find(key);
                if(found==model_ranges_.end())found=model_ranges_.emplace(key,ranges_(key.first,key.second)).first;
                // The model binding authenticates literal linear bounds, not
                // the bytes or lifetime of an accelerated model tree.
                if(found->second.accelerator_count>0)return residual();
                auto bank=source_live_model_cull_geometry_tu15(world,unit,found->second);
                detail::NativeTu15ArithmeticEnvironment environment(native_fp);
                native_tu15_frustum_draw_cull_entry_44e80(unit,bank,arena,arena_at,0,*visibility,entry);
            }else if(!bsp(unit,arena,arena_at,entry,native_fp))return residual();
        }catch(const FormatError&) {++stats_.fallbacks;return residual();}
        if(native_fp.control_word!=std::uint16_t(arithmetic.control_word|0x40u))
            throw FormatError("Native main cull changed an x87 control field");
        native_fp.control_word=arithmetic.control_word;
        ++stats_.native_entries;stats_.native_model_entries+=is_model;
        if(is_model&&census)++native_models_[unit.words[1]];
        if(oracle) {
            auto actual=world.prepare_source_call(std::array{index,entry});actual.x87_status=arithmetic.status;
            actual.floating_point=SourcePhysicsFactoryFloatingPoint{arithmetic.control_word,arithmetic.mxcsr};
            world.run_actual(0x44e80u,actual);
            arithmetic={actual.floating_point->x87_control_word,actual.x87_status,actual.floating_point->mxcsr};
            const auto observed_unit=read_unit(at);const auto observed_arena=read_arena();
            const bool units_equal=observed_unit.words==unit.words;
            const bool mask_equal=observed_arena.words==arena.words&&observed_arena.used==arena.used;
            const bool fp_equal=arithmetic.control_word==native_fp.control_word&&arithmetic.status==native_fp.status&&arithmetic.mxcsr==native_fp.mxcsr;
            stats_.unit_mismatches+=!units_equal;stats_.mask_mismatches+=!mask_equal;stats_.fp_mismatches+=!fp_equal;
            if(!units_equal||!mask_equal||!fp_equal)
                throw FormatError("Ordered native44E80 entry differs at unit "+std::to_string(index)+" entry "+std::to_string(entry)+
                    " native FP="+std::to_string(native_fp.control_word)+":"+std::to_string(native_fp.status)+":"+std::to_string(native_fp.mxcsr)+
                    " actual FP="+std::to_string(arithmetic.control_word)+":"+std::to_string(arithmetic.status)+":"+std::to_string(arithmetic.mxcsr));
            return true;
        }
        if(unit.words[0]!=before.words[0])write(at,unit.words[0]);
        for(unsigned k=0;k<arena.words.size();++k)
            if(arena.words[k]!=initial_arena.words[k])write(arena_at+4u*k,arena.words[k]);
        arithmetic=native_fp;return true;
    });
#endif
}
}
