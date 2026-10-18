#include "halo2/cache.hpp"
#include "halo2/original_data.hpp"
#include "halo2/simulation.hpp"
#include "halo2/campaign_source_runtime.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>
using namespace halo2;
int main(int argc,char **argv)try {
 if(argc!=5)throw FormatError("Supply TU15_XBE OPENING_MAP TIMING_TSV OUTPUT_DIR");
 Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
 auto content=cache.load_content(0,false);
 auto source=std::make_shared<CampaignSourceRuntime>(cache,argv[1],0,1,0x12345678u);
 Simulation simulation(content,BspContent{std::move(content.collision),content.navigation},0,cache.spawns().at(0),
  [&](auto graph,auto name){return cache.load_named_animation(graph,name);},{},{},{},{},{},source);
 simulation.run_script("x01_02_setup");simulation.step({});
 if(!simulation.scripts().diagnostics().empty())throw FormatError(simulation.scripts().diagnostics().back());
 auto &world=source->objects().tu15_world();
 auto word=[&](std::uint32_t p){return Bytes(world.memory().view(p,4)).u32(0);};
 const auto gestalt=std::ranges::find_if(cache.tags(),[](const auto &tag){return tag.group=="ugh!";});
 if(gestalt==cache.tags().end())throw FormatError("Original sound gestalt absent");
 const auto entries=word(gestalt->address+0x54),count=word(gestalt->address+0x50);
 const std::filesystem::path out(argv[4]);
 if(std::filesystem::exists(out))throw FormatError("Preserve existing original facial-curve evidence");
 std::filesystem::create_directories(out);
 std::ofstream manifest(out/"curves.tsv");std::ifstream input(argv[3]);unsigned script,tick;std::uint32_t sound;
 unsigned indexed{},published{},curves{};
 while(input>>std::dec>>script>>tick>>std::hex>>sound) {
  const auto *tag=cache.find_tag({sound});if(!tag)throw FormatError("Original cue tag absent");const Bytes s(cache.tag_payload({sound}));
  const auto index=std::bit_cast<std::int16_t>(s.u16(0xe));
  if(index<0)continue;++indexed;
  if(unsigned(index)>=count)throw FormatError("Original extra-info index exceeds gestalt");
  const auto entry=entries+unsigned(index)*0x2cu;
  const auto cold=word(entry);
  const bool requested=world.request_render_geometry(2,entry+8);
  const bool ready=requested || world.request_render_geometry(0,entry+8);
  std::cout<<"sound="<<std::hex<<sound<<" extra="<<std::dec<<index<<" ready="<<ready
   <<" cold_count="<<cold<<" count="<<word(entry)<<" pointer="<<std::hex<<word(entry+4)<<std::dec<<'\n';
  if(!ready || !word(entry) || !word(entry+4))throw FormatError("Actual original sound resource owner did not publish");
  ++published;const auto encoded=word(entry+4);
  const auto pitches=word(gestalt->address+0x24),permutations=word(gestalt->address+0x2c);
  const auto pitch=pitches+s.u16(8)*12u;
  const auto permutation=permutations+Bytes(world.memory().view(pitch+8,2)).u16(0)*16u;
  const auto section=std::bit_cast<std::int8_t>(world.memory().view(permutation+5,1)[0]);
  if(section<0 || unsigned(section)>=word(encoded+8))throw FormatError("Original encoded section escapes resource");
  const auto descriptor=word(encoded+12)+unsigned(section)*16u;
  const auto data=word(encoded+4),offset=word(descriptor+8),size=word(descriptor+12);
  if(offset>word(encoded) || size>word(encoded)-offset)throw FormatError("Actual original facial curve escapes decoded resource");
  if(!size)continue;++curves;const auto curve=world.memory().view(data+offset,size);
  std::ostringstream name;name<<std::hex<<sound<<".bin";
  std::ofstream binary(out/name.str(),std::ios::binary);binary.write(reinterpret_cast<const char*>(curve.data()),curve.size());
  manifest<<std::hex<<sound<<' '<<std::dec<<size<<' '<<name.str()<<' '<<tag->name<<'\n';
 }
 if(indexed!=38 || published!=indexed)throw FormatError("Incomplete actual opening extra-info resource coverage");
 std::cout<<"PASS38 original resource publications; nonempty facial curves="<<curves<<"; no supplied readiness or Xbox voice handles\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
