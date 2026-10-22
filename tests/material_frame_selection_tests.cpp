#include "rendering/renderer/material_frame_selection.hpp"
#include "rendering/renderer/material_frame_oracle.hpp"
#include <iostream>
#include <stdexcept>
using namespace halo2;
void require(bool value) {if(!value)throw std::runtime_error("active material dependency closure differs");}
int main() {
 std::vector<Material> materials(8);
 auto shared=std::make_shared<Material>();auto nested=std::make_shared<Material>();
 shared->world_overlays_after.push_back(nested);
 materials[0].environment_pass=shared;materials[4].world_bloom_passes.push_back(shared);
 materials[6].world_overlays_before.push_back(nested);
 materials[2].planar_fog_pass=std::make_shared<Material>();
 {MaterialFrameSelection selection(materials);selection.add(0);require(selection.roots()==std::vector<unsigned>({0,4,6}));}
 {MaterialFrameSelection selection(materials);selection.add(4);selection.add(7);require(selection.roots()==std::vector<unsigned>({0,4,6,7}));}
 {MaterialFrameSelection selection(materials);selection.add(2);require(selection.roots()==std::vector<unsigned>({2}));}
 {MaterialFrameSelection selection(materials);require(selection.roots().empty());}
 {MaterialFrameSelection selection(materials);std::vector<Vertex> v(9);for(unsigned i=0;i<v.size();++i)v[i].material=i/3;
  selection.add(v);require(selection.roots()==std::vector<unsigned>({0,1,2,4,6}));
  bool rejected=false;try{selection.add(8);}catch(const FormatError&){rejected=true;}require(rejected);}
 // A retained selection must refresh after any live topology change.
 materials[4].world_bloom_passes.clear();MaterialFrameSelection selection(materials);selection.add(0);
 require(selection.roots()==std::vector<unsigned>({0,6}));
 selection.begin_frame(materials);require(selection.roots().empty());
 // A previously inactive owner introduces sharing, then breaks it by COW.
 materials[7].environment_pass=nested;
 selection.begin_frame(materials);selection.add(0);
 require(selection.roots()==std::vector<unsigned>({0,6,7}));
 materials[7].environment_pass=std::make_shared<Material>(*nested);
 selection.begin_frame(materials);selection.add(0);
 require(selection.roots()==std::vector<unsigned>({0,6}));
 materials.reserve(100);selection.begin_frame(materials);selection.add(0);
 require(selection.roots()==std::vector<unsigned>({0,6}));
 materials.emplace_back();materials.back().environment_pass=nested;
 selection.begin_frame(materials);selection.add(8);
 require(selection.roots()==std::vector<unsigned>({0,6,8}));
 std::uint32_t seed=1548967;
 const auto random=[&]{seed=seed*1664525+1013904223;return seed;};
 std::vector<Material> changing(64);MaterialFrameSelection retained(changing);
 std::vector<std::shared_ptr<Material>> passes;
 for(unsigned i=0;i<12;++i)passes.push_back(std::make_shared<Material>());
 for(unsigned frame=0;frame<2048;++frame) {
   auto &m=changing[random()%changing.size()];
   switch(frame%4) {
   case 0:m.environment_pass=passes[random()%passes.size()];break;
   case 1:m.world_overlays_before={passes[random()%passes.size()]};break;
   case 2:m.world_overlays_before.clear();break;
   case 3:m.environment_pass=std::make_shared<Material>();break;
   }
   retained.begin_frame(changing);MaterialFrameSelection fresh(changing);
   for(unsigned j=0;j<5;++j){const auto root=random()%changing.size();retained.add(root);fresh.add(root);}
   require(retained.roots()==fresh.roots());
   retained.begin_frame_with_detach_only_pass_graph(changing);
   // Value animation is legal while the child graph remains fixed.
   changing[frame%changing.size()].forward_program.words[frame%60]^=frame;
   MaterialFrameSelection same_graph(changing);
   for(unsigned j=0;j<5;++j){const auto root=random()%changing.size();retained.add(root);same_graph.add(root);}
   require(retained.roots()==same_graph.roots());
 }
 // Copy-on-write only splits the original sharing closure. The fast path
 // may retain extra owners, but must include every owner of a live child.
 std::vector<Material> detached(32);
 auto leaf=std::make_shared<Material>();auto branch=std::make_shared<Material>();
 branch->world_overlays_after.push_back(leaf);
 for(unsigned i=0;i<16;++i)detached[i].environment_pass=branch;
 for(unsigned i=16;i<32;++i)detached[i].world_bloom_passes.push_back(leaf);
 MaterialFrameSelection conservative(detached);
 for(unsigned frame=0;frame<256;++frame) {
   const auto i=random()%detached.size();
   if(i<16) {
     detached[i].environment_pass=std::make_shared<Material>(*detached[i].environment_pass);
     auto &child=detached[i].environment_pass->world_overlays_after[0];
     child=std::make_shared<Material>(*child);
   } else {
     auto &child=detached[i].world_bloom_passes[0];child=std::make_shared<Material>(*child);
   }
   conservative.begin_frame_with_detach_only_pass_graph(detached);
   MaterialFrameSelection live(detached);
   const auto selected=random()%detached.size();conservative.add(selected);live.add(selected);
   const auto superset=conservative.roots(),actual=live.roots();
   require(std::includes(superset.begin(),superset.end(),actual.begin(),actual.end()));
   require(superset.size()==detached.size());
 }
 std::array<unsigned,1> active{0};verify_material_frame(materials,active,[]{});
 bool mismatch=false;try{verify_material_frame(materials,active,[&]{materials[0].forward_program.words[10]^=1;});}
 catch(const FormatError&){mismatch=true;}require(mismatch);
 std::cout<<"shared, nested, independent, empty, changed, retained, reallocated and invalid material frames passed (2048 frames)\n";
}
