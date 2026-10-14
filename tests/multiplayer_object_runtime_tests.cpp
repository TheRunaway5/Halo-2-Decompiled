#include "halo2/multiplayer_object_runtime.hpp"
#include "halo2/cache.hpp"
#include "halo2/native_object_lifetimes.hpp"
#include "halo2/math.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/death_camera_binding.hpp"
#include "halo2/multiplayer_world.hpp"
#include "halo2/native_death_camera.hpp"
#include "halo2/original_data.hpp"
#include "halo2/source_camera.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/source_object_draw_collector.hpp"
#include <bit>
#include <iostream>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <cstring>

namespace {
struct Files {
  std::filesystem::path path;
  Files() {
#if defined(__linux__)
    auto name=(std::filesystem::temp_directory_path()/"halo2-live-objects-XXXXXX").string();
    auto p=::mkdtemp(name.data());if(!p)throw std::runtime_error("Native temporary directory failed");path=p;
#endif
  }
  ~Files(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}
};
template<class Address> std::uint32_t word(const halo2::SourceRuntimeMemory&m,Address p) {
  const auto b=m.view(p,4);std::uint32_t v=0;
  for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);return v;
}
void check_render_cache(halo2::MultiplayerObjectRuntime &runtime) {
  const auto &m=runtime.memory();
  const auto pool=word(m,0x510c24);
  if(!pool || word(m,pool+0x20)!=0x100 || word(m,pool+0x24)!=0x100 ||
      m.view(pool+0x29,1)[0]!=std::byte{1} || !(word(m,pool+0x2a)&4) ||
      word(m,pool+0x34) || word(m,pool+0x38) || word(m,pool+0x3c))
    throw std::runtime_error("Live source world lost its original constructed/activated render-cache pool");
  const auto records=word(m,pool+0x44),bitmap=word(m,pool+0x48);
  const auto name=m.view(pool,2),salt=m.view(pool+0x40,2);
  if(salt[0]!=name[0] || salt[1]!=(name[1]|std::byte{0x80}))
    throw std::runtime_error("Live render-cache reset lost its source name-derived salt");
  m.view(records,0x10000);m.view(bitmap,32);
  for(unsigned i=0;i<256;++i) {
    const auto token=m.view(records+i*0x100,2);
    if(token[0]!=std::byte{} || token[1]!=std::byte{})
      throw std::runtime_error("Live render-cache reset retained an allocated record");
  }
  for(unsigned i=0;i<8;++i)if(word(m,bitmap+i*4))
    throw std::runtime_error("Live render-cache reset retained occupied slots");
}
void check_resource_cache(halo2::MultiplayerObjectRuntime &runtime,bool configured) {
  const auto &m=runtime.memory();
  for(const auto [slot,capacity,stride]:{std::array{0x4eda5cu,512u,12u},
                                      std::array{0x4eda60u,350u,8u}}) {
    const auto pool=word(m,slot);
    if(!pool || word(m,pool+0x20)!=capacity || word(m,pool+0x24)!=stride ||
        m.view(pool+0x29,1)[0]!=std::byte{1} || !(word(m,pool+0x2a)&4))
      throw std::runtime_error("Live source world lost its original activated model-resource pool");
    m.view(word(m,pool+0x44),capacity*stride);
  }
  const auto cache=word(m,0x4eda6c),pool=word(m,cache+0x64);
  if(!cache || pool!=cache+0x70 || word(m,pool+0x20)!=512 || word(m,pool+0x24)!=24 ||
      m.view(pool+0x29,1)[0]!=std::byte{1} || word(m,cache+0x68)!=0x77656565 ||
      word(m,cache+0x20)!=0x12fa00 || word(m,cache+0x24)!=0x12f980 ||
      word(m,cache+0x28)!=0xa1450 || word(m,cache+0x6c)!=word(m,0x46fbf8))
    throw std::runtime_error("Live source resource cache lost its original allocator or callbacks");
  if(configured) {
    const auto backing=word(m,0x4eda64),bytes=word(m,0x4eda68);
    if(!backing || bytes!=0x700000 || word(m,cache+0x30)!=bytes/4096 || word(m,cache+0x2c)!=2)
      throw std::runtime_error("Actual multiplayer options did not drive the original resource reservation");
    m.view(backing,bytes);
  } else if(word(m,0x4eda64) || word(m,0x4eda68))
    throw std::runtime_error("Resource backing was reserved before actual session options delivery");
}
void check_owner_sphere(halo2::MultiplayerObjectRuntime &runtime,const halo2::Cache &cache,
    std::uint32_t handle,std::uint32_t definition,const std::vector<halo2::Transform> &nodes) {
  const auto actual=runtime.object_sphere(handle);
  const auto expected=halo2::source_shadow_sphere(cache.load_object_shadow_definition(definition),nodes.at(0),1.f);
  const std::array actual_values{actual.center.x,actual.center.y,actual.center.z,actual.radius};
  const std::array expected_values{expected.center.x,expected.center.y,expected.center.z,expected.radius};
  for(unsigned i=0;i<actual_values.size();++i)
    if(std::bit_cast<std::uint32_t>(actual_values[i])!=std::bit_cast<std::uint32_t>(expected_values[i]))
      throw std::runtime_error("Retained source sphere differs from the authored biped's completed WORLD bounds");
  for(const auto stale:{halo2::null_datum,handle^0x10000u}) {
    bool rejected=false;
    try {(void)runtime.object_sphere(stale);}catch(const halo2::FormatError&){rejected=true;}
    if(!rejected)throw std::runtime_error("Source presentation admitted a stale or NONE full object handle");
  }
}
void check_region_delivery(halo2::MultiplayerObjectRuntime &runtime,std::uint32_t handle) {
  const auto &m=runtime.memory();
  const auto headers=word(m,runtime.object_pool()+0x44);
  const auto body=word(m,headers+(handle&0xffffu)*12+8);
  const auto component=m.view(body+0x118,4);
  const auto extent=std::to_integer<unsigned>(component[0])|
      (std::to_integer<unsigned>(component[1])<<8);
  const auto offset=std::to_integer<unsigned>(component[2])|
      (std::to_integer<unsigned>(component[3])<<8);
  if(extent!=40 || !offset)
    throw std::runtime_error("Actual Spartan birth lost its four original region records");
  const auto bytes=m.view(body+offset,extent);
  const std::vector<std::byte> before(bytes.begin(),bytes.end());
  const auto unchanged=[&] {
    const auto after=m.view(body+offset,extent);
    if(!std::equal(before.begin(),before.end(),after.begin()))
      throw std::runtime_error("Live stock Spartan region delivery invented an unauthored damage state");
  };
  for(const auto mask:{0xffffffffu,0u}) {
    const std::array args{handle,0xffffffffu,2u,mask};
    runtime.call(0xbb6e0,args);unchanged();
  }
  for(unsigned region=0;region<4;++region)for(unsigned state=0;state<6;++state) {
    const std::array args{handle,region,state,0u};
    runtime.call(0xbb5e0,args);unchanged();
  }
  std::cout<<"Actual retained Spartan executes original shield/death region flags and state selectors\n";
}
}
int main(int argc,char **argv) {try {
  if((argc!=3 && argc!=4) || !std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
#if !defined(__linux__)
  return 77;
#endif
  Files files;halo2::Cache cache(argv[2]);
  if(argc==4 && (std::string_view(argv[3])=="--match" || std::string_view(argv[3])=="--match-step" ||
      std::string_view(argv[3])=="--match-camera")) {
    halo2::MultiplayerOptions options;
    options.map=argv[2];options.gameplay_xbe=argv[1];options.starting_weapon=5;
    options.local_players=2;options.source_object_world=true;
    options.source_object_savegame_directory=files.path;
    halo2::MultiplayerPreparedWorld world(options);
    if(!world.source_objects || !world.match.source_object_world_bound() || world.local_slots.size()!=2)
      throw std::runtime_error("Actual Match did not retain its source object world");
    check_render_cache(*world.source_objects);
    check_resource_cache(*world.source_objects,true);
    for(auto slot:world.local_slots) {
      const auto handle=world.match.player(slot).source_current_object;
      const auto &identity=world.match.simulation().object_lifetimes().owner(handle);
      const auto nodes=world.match.source_world_nodes(handle);
      if(handle==halo2::null_datum || !identity.published || nodes.empty() ||
          identity.definition!=world.match.unit(slot).definition.datum)
        throw std::runtime_error("Actual Match spawn lost its source-created unit or WORLD nodes");
      check_owner_sphere(*world.source_objects,cache,handle,identity.definition,nodes);
      check_region_delivery(*world.source_objects,handle);
      const auto sphere=world.match.source_object_sphere(handle);
      if(std::bit_cast<std::uint32_t>(sphere.radius)!=
          std::bit_cast<std::uint32_t>(world.source_objects->object_sphere(handle).radius))
        throw std::runtime_error("Actual Match detached its source owner sphere");
      std::cout<<"Source player "<<std::hex<<world.match.player(slot).source_datum
          <<" unit "<<handle<<std::dec<<" node0 "<<nodes.front().position.x<<' '
          <<nodes.front().position.y<<' '<<nodes.front().position.z<<'\n';
    }
    std::cout<<"Actual prepared Match shares source headers, completed biped births and WORLD node consumers\n";
    if(std::string_view(argv[3])=="--match-camera") {
      // Actual native local-view inputs, followed by the same confirmed-fatal
      // boundary used by Match::finish_death. No player/corpse flags are written.
      std::uint32_t auxiliary_random=0x12345678u;
      halo2::NativeDeathCamera camera(*world.source_objects,*world.content.original_data,auxiliary_random);
      const auto slot=world.local_slots.front();
      const auto &unit=world.match.unit(slot);const auto &player=world.match.player(slot);
      const auto state=world.match.state().players[slot];
      const auto forward=halo2::direction(state.yaw,state.pitch);
      const auto up=halo2::cross(forward,{-std::sin(state.yaw),std::cos(state.yaw),0});
      const halo2::NativeDeathCameraView ordinary{
          halo2::ordinary_unit_eye(state.position,unit.definition.camera_height,
              unit.definition.crouch_camera_height,state.crouch),forward,up,unit.definition.camera_field_of_view};
      if(camera.update(0,player.source_datum,player.source_flags,false,halo2::tick_seconds,false,ordinary))
        throw std::runtime_error("Source live player unexpectedly selected a death view");
      world.source_objects->kill_biped(unit.source_datum);
      const auto dead=halo2::source_death_camera_players(world.source_objects->memory(),world.source_objects->player_pool());
      if(dead.front().current_unit!=halo2::null_datum || dead.front().previous_unit!=unit.source_datum)
        throw std::runtime_error("Original fatal cleanup did not retain and detach its actual corpse");
      const auto view=camera.update(0,player.source_datum,player.source_flags,false,halo2::tick_seconds,false,ordinary);
      if(!view)throw std::runtime_error("Actual fatal cleanup did not select the connected death camera");
      std::cout<<"Actual native death view "<<view->position.x<<' '<<view->position.y<<' '<<view->position.z<<'\n';
    }
    if(std::string_view(argv[3])=="--match-step") {
      world.source_objects->step_objects();
      for(auto slot:world.local_slots) {
        const auto h=world.match.player(slot).source_current_object;
        if(!(world.match.simulation().object_lifetimes().header(h)[2]&1u))
          throw std::runtime_error("Actual source player/PVS did not activate its Match biped");
      }
      std::cout<<"Actual Match players drove original PVS admission and continuous source object/world phases\n";
    }
    return 0;
  }
  halo2::MultiplayerObjectRuntime runtime(argv[1],cache,0,files.path);
  check_render_cache(runtime);
  check_resource_cache(runtime,false);
  auto &m=runtime.memory();halo2::NativeObjectLifetimes objects(m,runtime.object_pool());
  if(objects.pool().count()!=0 || !word(m,0x526234) || !word(m,0x526538) || !word(m,0x487638))
    throw std::runtime_error("Original multiplayer object world construction did not complete");
  std::cout<<"Actual cache/BSP, native process heap, source object pools and complete new-map physical world constructed; no captured bank\n";
  if(argc==4) {
    runtime.configure_session(halo2::source_game_variant_default(0),1,30,0x12345678);
    check_resource_cache(runtime,true);
    if(std::string_view(argv[3])=="--resources" || std::string_view(argv[3])=="--models") {
      const auto tags=word(m,0x4eb114);
      const auto metadata=[&](std::uint32_t datum){return word(m,tags+(datum&0xffffu)*16+8);};
      unsigned completed=0;
      std::vector<std::pair<std::uint32_t,std::uint32_t>> model_owners;
      for(const auto species:{"masterchief_mp","elite_mp"}) {
        const auto definition=std::find_if(cache.tags().begin(),cache.tags().end(),[&](const auto&t) {
          return t.group=="bipd" && t.name.ends_with(species);
        });
        if(definition==cache.tags().end())throw std::runtime_error("Authored multiplayer species missing");
        const auto model=word(m,metadata(word(m,metadata(definition->datum)+0x38))+4);
        const auto mode=metadata(model),count=word(m,mode+0x24),sections=word(m,mode+0x28);
        if(std::string_view(argv[3])=="--models") {
          runtime.preload_render_model(model);
          const auto &spawn=cache.spawns().front();
          const auto owner=runtime.create_biped(definition->datum,spawn.position,
              halo2::authored_basis({spawn.facing,0,0}).forward);
          if(owner==halo2::null_datum)throw std::runtime_error("Original species birth rejected model presentation");
          model_owners.emplace_back(owner,model);
        }
        const std::array args{0u};
        // Queued, nonresident headers genuinely return false here. Admission
        // is not completion; only the worker and subsequent fixups supply it.
        runtime.call(0x1703d0,args,model);
        for(unsigned i=0;i<count;++i) {
          const auto header=sections+i*0x5cu+0x38u;
          if(std::bit_cast<std::int32_t>(word(m,header+8))<=0)continue;
          runtime.call(0x12f5e0,{},2u,header); // Genuine original nonblocking load policy.
          const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
          while(!(runtime.call(0x12f5e0,{},0u,header)&255u)) {
            if(std::chrono::steady_clock::now()>=deadline)
              throw std::runtime_error("Original model-resource worker did not complete");
            std::this_thread::yield();
          }
          const auto handle=word(m,header+0x20),physical=word(m,0x4eda6c);
          const auto page=word(m,word(m,word(m,physical+0x64)+0x44)+(handle&0xffffu)*24+8);
          const auto resident=word(m,0x4eda64)+(page<<word(m,physical+0x34));
          const auto offset_bytes=m.view(header+0x1c,2);
          const auto offset=std::int16_t(std::to_integer<unsigned>(offset_bytes[0])|
              (std::to_integer<unsigned>(offset_bytes[1])<<8));
          if(m.view(header+0x1e,1)[0]!=std::byte{1} || handle==0xffffffffu ||
              word(m,header+offset)!=1 || word(m,header+offset+4)!=resident+8)
            throw std::runtime_error("Original resource completion/fixup lost its actual resident pointer");
          m.view(resident,word(m,header+4));++completed;
        }
        std::cout<<species<<" original resource queue/read/fixups returned for "<<count<<" sections\n";
      }
      if(!completed)throw std::runtime_error("No authored species resources were read");
      std::cout<<completed<<" real geometry resources resident through the original async worker\n";
      if(!model_owners.empty()) {
        runtime.begin_model_frame();
        for(const auto &[owner,model]:model_owners)for(unsigned view=0;view<4;++view) {
          const auto collector=runtime.build_world_model(owner,view,4,false,1.f,1.f);
          const auto get=[&](unsigned offset,auto value) {
            std::memcpy(&value,collector.bytes.data()+offset,sizeof(value));return value;
          };
          if(get(0,std::uint32_t{})!=owner || get(0x166,std::uint16_t{})!=1 ||
              get(0x5c,std::uint32_t{})!=model || get(0x16c,std::uint8_t{})!=0)
            throw std::runtime_error("Original live world-model collector lost its owner/model/admission");
          const auto context=get(0x48,std::uint32_t{}),token=get(4,std::uint32_t{});
          if(!context || token==0xffffffffu)throw std::runtime_error("Live model collector did not produce cache/geometry");
          m.view(context+0x10,84);m.view(0x48f03c+(token&0xfffffffu),0x44);
          std::cout<<"Actual species "<<std::hex<<model<<" view "<<view<<" geometry "<<token
              <<" cache "<<context<<std::dec<<" produced by the live model/lighting owners\n";
        }
      }
      return 0;
    }
    const auto biped=std::find_if(cache.tags().begin(),cache.tags().end(),[](const auto&t) {
      return t.group=="bipd" && t.name.ends_with("masterchief_mp");
    });
    if(biped==cache.tags().end() || cache.spawns().empty())throw std::runtime_error("Actual player placement unavailable");
    const auto &spawn=cache.spawns().front();
    const auto handle=runtime.create_biped(biped->datum,spawn.position,
        halo2::authored_basis({spawn.facing,0,0}).forward);
    if(handle==0xffffffffu)throw std::runtime_error("Original biped admission rejected authored placement");
    objects.observe_source_birth(handle);
    if(objects.pool().count()!=1 || objects.owner(handle).definition!=biped->datum)
      throw std::runtime_error("Completed biped birth does not share the actual source header pool");
    const auto nodes=runtime.world_nodes(handle);
    check_owner_sphere(runtime,cache,handle,biped->datum,nodes);
    check_region_delivery(runtime,handle);
    const auto target=halo2::source_death_camera_target(m,runtime.object_pool(),handle);
    if(nodes.empty() || !target ||
        nodes.front().position.x!=target->node0.x ||
        nodes.front().position.y!=target->node0.y ||
        nodes.front().position.z!=target->node0.z)
      throw std::runtime_error("Presentation and death-camera target differ from actual WORLD node zero");
    std::cout<<"Actual authored biped birth completed in the same source bank\n";
    if(std::string_view(argv[3])=="--step") {
      runtime.step_objects();
      std::cout<<"Five original object/world phases returned with no source player activation\n";
    }
  }
  return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
