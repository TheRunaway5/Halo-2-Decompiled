#include "halo2/native_tu15_frustum_draw_cull.hpp"
#include "halo2/source_bsp_ranges.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <iostream>

using namespace halo2;using J=nlohmann::json;
namespace {
Transform transform(const J& input) {
    const auto words=input.get<std::array<std::uint32_t,13>>();return std::bit_cast<Transform>(words);
}
float scalar(const J& v){return std::bit_cast<float>(v.get<std::uint32_t>());}
}
int main()try {
    for(std::string line;std::getline(std::cin,line);)if(!line.empty()) {
        const auto input=J::parse(line);const auto& spec=input.at("spec");
        SourceBspRangeArena arena;arena.words=spec.at("arena").get<decltype(arena.words)>();arena.used=spec.at("arena_used");
        std::vector<SourceCullDrawUnit> units;std::vector<SourceCullDrawBanks> banks;
        for(const auto& row:spec.at("units")) {
            units.push_back({row.at("words").get<std::array<std::uint32_t,8>>()});auto& bank=banks.emplace_back();
            bank.geometry_resolved=true;if(row.value("absent",false))continue;
            bank.geometry.emplace();auto& geometry=*bank.geometry;
            geometry.subpart_count=row.at("count");geometry.subpart_bounds=row.at("parts").get<std::vector<std::uint16_t>>();
            const auto bound=[](const J& b){return SourceCullPartBound{{scalar(b[0]),scalar(b[1]),scalar(b[2])},scalar(b[3]),b[4].get<std::uint8_t>()};};
            for(const auto& b:row.at("bounds"))geometry.bounds.push_back(bound(b));
            if(row.contains("literal_ffff")){geometry.bounds.resize(65536);geometry.bounds.back()=bound(row.at("literal_ffff"));}
            SourceCullOwnerPose pose;pose.owner=row.at("owner");
            for(const auto& node:row.at("nodes"))pose.nodes.push_back(transform(node));
            for(const auto& node:row.at("inverse"))pose.inverse_bind.push_back(transform(node));
            geometry.resolve_owner_pose=[pose]{return pose;};
            if(row.contains("transform"))bank.transform=transform(row.at("transform"));
        }
        SourcePortalVisibilitySet visibility;visibility.groups.resize(spec.at("groups"));
        for(const auto& row:spec.at("ranges")) {
            SourceClusterFrustumRanges r;r.source_cluster=row.at("cluster");
            r.counts=row.at("counts").get<decltype(r.counts)>();r.first_indices=row.at("first").get<decltype(r.first_indices)>();visibility.clusters.push_back(r);
        }
        for(const auto& row:spec.at("frustums")) {
            SourceCameraFrustum f;for(unsigned n=0;n<6;++n)f.bounds[n]=scalar(row.at("bounds")[n]);
            for(unsigned p=0;p<6;++p)for(unsigned n=0;n<4;++n)f.planes[p][n]=scalar(row.at("planes")[p][n]);
            visibility.frustums.push_back(f);
        }
        const auto associations=spec.at("associations").get<std::vector<std::array<std::uint32_t,4>>>();
        SourceDrawUnitCullContext context;context.mode=spec.at("mode");context.query_mode=0;
        context.main_prefix=spec.at("prefix");context.draw_count=spec.at("total");const auto mask_base=spec.at("arena_address").get<std::uint32_t>();
        J points=J::array();NativeTu15FrustumBoundObserver observe;
        if(input.value("observe",false))observe=[&](const auto& b){points.push_back(J::array({
            std::bit_cast<std::uint32_t>(b.center.x),std::bit_cast<std::uint32_t>(b.center.y),
            std::bit_cast<std::uint32_t>(b.center.z),std::bit_cast<std::uint32_t>(b.radius)}));};
        const auto fp=input.at("before_fp").get<std::array<unsigned,3>>();const auto control=std::uint16_t(fp[0]);const auto mxcsr=fp[2];
        asm volatile("fninit; fldcw %0"::"m"(control):"memory");asm volatile("ldmxcsr %0"::"m"(mxcsr):"memory");
        std::vector<NativeTu15FrustumDrawCall> calls;
        bool rejected{};
        try {
        if(spec.at("walk").get<bool>()) {
            calls=native_tu15_frustum_draw_cull_44710(units,arena,mask_base,context,visibility,associations,
                [&](unsigned i,const auto&){return banks.at(i);});
        }else {
            const auto entry=spec.at("entry").get<unsigned>();calls.push_back({0,std::uint16_t(entry)});
            native_tu15_frustum_draw_cull_entry_44e80(units[0],banks[0],arena,mask_base,context.mode,visibility,entry,observe);
        }
        }catch(const FormatError&) {
            if(!input.value("expect_rejection",false))throw;
            rejected=true;
        }
        std::uint16_t after_control,after_status;std::uint32_t after_mxcsr;
        asm volatile("fnstcw %0; fnstsw %1":"=m"(after_control),"=m"(after_status)::"memory");
        asm volatile("stmxcsr %0":"=m"(after_mxcsr)::"memory");
        // These SSE-only owners never load/store the x87 control word.
        // FLDCW/FNSTCW used by this host harness canonicalize reserved bit6;
        // ptrace's original ABI retains the caller's exact supplied bits.
        if(after_control!=std::uint16_t(control|0x40u))throw FormatError("Frustum kernel changed an x87 control field");
        after_control=control;
        J output_units=J::array(),output_calls=J::array();
        for(const auto& unit:units)output_units.push_back(unit.words);
        for(const auto& call:calls)output_calls.push_back(J::array({call.argument,call.entry}));
        std::cout<<J{{"units",output_units},{"arena",arena.words},{"arena_used",arena.used},{"calls",output_calls},{"points",points},{"rejected",rejected},
            {"fp",{after_control,after_status,after_mxcsr}}}.dump()<<'\n'<<std::flush;
    }
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
