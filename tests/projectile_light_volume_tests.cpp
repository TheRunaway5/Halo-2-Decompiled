#include "halo2/light_volume.hpp"
#include "halo2/cache.hpp"
#include "halo2/original_data.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
using namespace halo2;
template<class T>void read(std::istream &in,T &v){in.read(reinterpret_cast<char *>(&v),sizeof(v));if(!in)throw FormatError("Short original projectile volume input");}
template<class T>void write(std::ostream &out,const T &v){out.write(reinterpret_cast<const char *>(&v),sizeof(v));if(!out)throw FormatError("Short native projectile volume output");}
int main(int argc,char **argv)try {
  if(argc!=6 && argc!=7)throw FormatError("Supply MAP SHARED XBE INPUT OUTPUT [ORIGINAL_EXPECTED]");
  Cache cache(argv[1],argv[2],OriginalData::load_xbe(argv[3]));
  std::ifstream input(argv[4],std::ios::binary);std::ofstream output(argv[5],std::ios::binary);
  if(!input || !output)throw FormatError("Cannot open projectile volume fixtures");
  std::uint32_t cases{};read(input,cases);unsigned sprites{};
  for(unsigned i=0;i<cases;++i) {
    std::uint32_t datum{};LightVolumeState state;LightVolumeView view;
    read(input,datum);read(input,state.marker);read(input,state.opacity);read(input,state.color);
    read(input,view.eye);read(input,view.world_to_view);read(input,view.projection_scale);
    const auto definition=cache.load_light_volume(datum);
    const auto batches=light_volume_sprites(definition,state,view,*cache.shader_function_tables());
    write(output,std::uint32_t(batches.size()));
    for(const auto &b:batches) {
      write(output,b.bitmap);write(output,std::uint32_t(b.fuzzy));write(output,std::uint32_t(b.sprites.size()));
      write(output,b.cosine);write(output,b.sine);
      for(const auto &s:b.sprites){write(output,s);++sprites;}
    }
  }
  output.close();
  if(argc==7) {
    std::ifstream expected_file(argv[6],std::ios::binary),actual_file(argv[5],std::ios::binary);
    if(!expected_file || !actual_file)throw FormatError("Missing original/native light-volume records");
    const std::string expected{std::istreambuf_iterator<char>(expected_file),std::istreambuf_iterator<char>()};
    const std::string actual{std::istreambuf_iterator<char>(actual_file),std::istreambuf_iterator<char>()};
    if(!cases || !sprites || actual!=expected)
      throw FormatError("Native projectile light-volume records differ from unchanged original generator");
  }
  std::cout<<cases<<" native projectile light-volume inputs, "<<sprites<<" sprites\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
