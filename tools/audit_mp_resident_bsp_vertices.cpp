// Reuse the immutable capture reader and complete output comparison types.
// This does not alter or rerun the frozen packing experiment.
#define main frozen_vertex_run_pack_main
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"
#include "benchmark_mp_vertex_run_pack.cpp"
#pragma GCC diagnostic pop
#undef main
#include "halo2/original_data.hpp"
#include "halo2/source_bsp_shader_scene.hpp"
#include <set>

int main(int argc,char **argv) {
  using namespace halo2;
  try {
    if(argc!=6)throw FormatError("Supply MAP SHARED UPDATE_XBE GROUP_CAPTURE_STEM OUTPUT_TSV");
    const Cache cache(argv[1],argv[2],OriginalData::load_xbe(argv[3]));
    auto mesh=cache.load_bsp(0);const auto raw=mesh.vertices;
    // Execute the real resident LOD/lightmap-bank construction without a
    // platform renderer, shader edit or fabricated descriptor allocation.
    const SourceBspShaderScene scene(cache,mesh,0);
    const auto captured=load(argv[4]);
    using Geometry=std::tuple<int,std::uint32_t,std::uint32_t,std::uint32_t>;
    const auto geometry=[](const Vertex &v)->Geometry{return {v.bsp_cluster,v.source_section,v.source_part,v.lightmap};};
    std::map<Geometry,std::vector<Vertex>> resident;
    for(const auto &v:raw)if(v.bsp_cluster!=-1)resident[geometry(v)].push_back(v);
    std::ofstream out(argv[5]);if(!out)throw FormatError("open resident BSP audit output");
    out<<"call\tgeometry\tsection\tpart\tlightmap\tmaterial\tstipple\tvertices\tmaterial_diff_vertices\tstipple_diff_vertices\n";
    std::set<std::tuple<Geometry,unsigned,unsigned>> variants;
    unsigned groups{},vertices{},material_changes{},stipple_changes{},bsp_calls{};
    for(unsigned call=0;call<captured.calls.size();++call) {
      std::map<Geometry,std::vector<Vertex>> submitted;
      for(const auto &v:captured.calls[call].vertices)if(v.bsp_cluster!=-1)submitted[geometry(v)].push_back(v);
      if(!submitted.empty())++bsp_calls;
      for(const auto &[g,selected]:submitted) {
        const auto found=resident.find(g);if(found==resident.end())throw FormatError("submitted BSP identity missing from raw resident data");
        if(found->second.size()!=selected.size())throw FormatError("submitted BSP part has a different vertex count/order contract");
        auto expected=found->second;const auto material=selected.front().material,stipple=selected.front().source_stipple_level;
        unsigned md{},sd{};
        for(unsigned i=0;i<expected.size();++i) {
          if(selected[i].material!=material || selected[i].source_stipple_level!=stipple)
            throw FormatError("BSP part binding or stipple is not constant across its captured stream");
          if(expected[i].material!=material)++md;
          if(expected[i].source_stipple_level!=stipple)++sd;
          // The two exact fields changed by SourceBspShaderScene::select.
          // Everything else, including every floating-point bit, must match.
          expected[i].material=material;expected[i].source_stipple_level=stipple;
        }
        if(std::memcmp(expected.data(),selected.data(),expected.size()*sizeof(Vertex)))
          throw FormatError("BSP geometry differs beyond its exact material/stipple selection fields");
        if(stipple>16)throw FormatError("captured stipple outside the verified source helper output");
        if(selected.front().lightmap && material!=found->second.front().material &&
            std::find(mesh.resident_material_lightmaps.begin(),mesh.resident_material_lightmaps.end(),
                std::array<std::uint32_t,2>{material,selected.front().lightmap})==mesh.resident_material_lightmaps.end())
          throw FormatError("selected BSP LOD/lightmap pair missing from the real resident declaration");
        out<<call<<'\t'<<std::get<0>(g)<<'\t'<<std::get<1>(g)<<'\t'<<std::get<2>(g)<<'\t'<<std::get<3>(g)
            <<'\t'<<material<<'\t'<<stipple<<'\t'<<selected.size()<<'\t'<<md<<'\t'<<sd<<'\n';
        variants.insert({g,material,stipple});++groups;vertices+=selected.size();material_changes+=md;stipple_changes+=sd;
      }
    }
    if(!out)throw FormatError("write resident BSP audit");
    std::cout<<"PASS "<<groups<<" complete resident part reconstructions, "<<vertices<<" captured BSP vertices across "<<bsp_calls
        <<" actual calls; "<<variants.size()<<" distinct selected geometry/material/stipple combinations.\n"
        <<"RAW "<<raw.size()<<" vertices, "<<resident.size()<<" resident part/lightmap identities; exact changed fields: "
        <<material_changes<<" material vertices and "<<stipple_changes<<" stipple vertices.\n"
        <<"LIMIT CPU byte evidence only; no persistent GPU draw plan, shader change, admission substitution or frame-rate claim.\n";
  }catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
}
