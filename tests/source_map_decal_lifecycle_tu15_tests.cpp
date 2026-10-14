#include "halo2/multiplayer_world.hpp"
#include <algorithm>
#include <iostream>
using namespace halo2;
int main(int argc,char**argv)try{
 if(argc!=3)throw std::runtime_error("Supply owned TU1.5 XBE and Zanzibar map");
 if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]))return 77;
 MultiplayerOptions options;options.gameplay_xbe=argv[1];options.map=argv[2];options.shared_maps=std::filesystem::path(argv[2]).parent_path();options.starting_weapon=5;options.session_random_seed=0x12345678u;
 MultiplayerPreparedWorld world(options);if(!world.source_objects || world.local_slots.size()!=1)throw std::runtime_error("Real retained source world/player was not prepared");
 const auto &memory=world.source_objects->memory();const auto word=[&](std::uint32_t p){auto b=memory.view(p,4);std::uint32_t v=0;for(unsigned i=0;i<4;++i)v|=std::to_integer<unsigned>(b[i])<<(i*8);return v;};
 if(word(0x44b40cu)!=0x180170u)throw std::runtime_error("Wrong original new-map decal lifecycle registration");
 if(word(0x44b350u)!=0x2b480u || word(0x44b358u)!=0x2b520u)
  throw std::runtime_error("Wrong original cached render-state lifecycle registrations");
 const auto cached=word(0x515f14u);
 if(!cached || word(cached+0x20u)!=0x100u || word(cached+0x24u)!=0x100u ||
     memory.view(cached+0x29u,1)[0]!=std::byte{1})
  throw std::runtime_error("Original cached render-state constructor/reset did not publish an active real pool");
 const auto scenario=word(0x4ecc20u),pool=word(0x4f7220u),context=word(0x4f721cu);
 (void)scenario;
 if(!pool || !context || memory.view(pool+0x29u,1)[0]!=std::byte{1} || (word(pool+0x38u)==0 && (word(context+0x3800u)!=0xffffffffu || !std::ranges::all_of(memory.view(context,0x3800),[](auto v){return v==std::byte{0xff};}))))
  throw std::runtime_error("Original decal map reset missing: active pool and NONE per-cluster links required before first player tick");
 // First-tick graphics acceptance is separate: this regression checks the
 // actual initialization state before any native renderer has been bound.
 std::cout<<"PASS real Zanzibar preparation, original active decal pool/NONE links and cached render-state lifecycle\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
