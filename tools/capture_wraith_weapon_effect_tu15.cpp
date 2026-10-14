#include "halo2/physics_live_world_tu15.hpp"
#include "halo2/cache.hpp"
#include "halo2/datum_pool.hpp"
#include "halo2/player_command.hpp"
#include "halo2/source_game_variant.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <ifaddrs.h>
#include <net/if.h>
#include <netpacket/packet.h>
#include <unistd.h>
using namespace halo2;
namespace {
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
struct Directory {std::filesystem::path path;Directory(){auto s=(std::filesystem::temp_directory_path()/"halo2-charge-source-XXXXXX").string();check(::mkdtemp(s.data()),"Cannot create native save directory");path=s;}~Directory(){std::error_code e;std::filesystem::remove_all(path,e);}};
SourceMachineIdentity identity(){ifaddrs *all{};check(!::getifaddrs(&all),"Cannot inspect actual native interfaces");SourceMachineIdentity result{};bool found{};for(auto *i=all;i;i=i->ifa_next)if(i->ifa_addr && i->ifa_addr->sa_family==AF_PACKET && (i->ifa_flags&IFF_UP) && !(i->ifa_flags&IFF_LOOPBACK)){const auto *a=reinterpret_cast<const sockaddr_ll*>(i->ifa_addr);if(a->sll_halen==result.size()){std::copy_n(a->sll_addr,result.size(),result.begin());found=true;break;}}::freeifaddrs(all);check(found,"Native machine identity unavailable");return result;}
template<class T>void write(std::ostream &out,const T &value){out.write(reinterpret_cast<const char*>(&value),sizeof(value));check(bool(out),"Cannot save source fixture");}
}
int main(int argc,char **argv)try {
 std::cout<<std::unitbuf;check(argc==4 || (argc==5 && std::string(argv[4])=="nonlocal"),"Supply TU1.5_XBE MAP OUTPUT [nonlocal]");const bool local=argc==4;std::uint32_t vehicle=0xffffffffu;unsigned vehicle_row=0xffffffffu;const std::filesystem::path out=argv[3];std::filesystem::create_directories(out);
 Cache cache(argv[2]);Directory saves;PhysicsLiveWorldTu15 world(cache,argv[1],saves.path);world.initialize_map();auto variant=source_game_variant_default(0);source_game_variant_set(variant,0x59,5);
 // Source15CEF0 category3 reads variant+CF (descriptor78). Choice2 reaches
 // Source15CC70's actual multiplayer vehicle palette+1C: Coagulation Wraith.
 source_game_variant_set(variant,78,2);world.configure_session(variant,1,30,0x12345678u);
 auto &source=world.memory();const auto word=[&](std::uint32_t address){return Bytes(source.view(address,4)).u32(0);};
 const auto scenario=std::ranges::find_if(cache.tags(),[](const auto &t){return t.group=="scnr";});check(scenario!=cache.tags().end(),"Actual scenario missing");const auto scenario_metadata=cache.load_source_tag_metadata(scenario->datum);const Bytes scenario_root(scenario_metadata.bytes_at(scenario_metadata.source_address(),0x128));const Bytes placements(scenario_metadata.bytes_at(scenario_root.u32(0x124),std::size_t(scenario_root.u32(0x120))*0x90));
 const auto engine_type=word(word(0x4f63b8u)+0xc14u);std::cout<<"Source engine type="<<engine_type<<" registered="<<std::hex<<word(0x56dfa0u+engine_type*4u)<<std::dec<<'\n';
 {SourcePhysicsWorldRuntimeAbi select{};select.gp[0]=0x60010000u;select.gp[1]=word(0x4f63b8u)+0x7dcu;select.gp[2]=0xffffffffu;select.gp[4]=0x600f0000u;select.fs_base=0x60200000u;select.eflags=2;select.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};const std::array args{0x60300000u,3u};source.write(select.gp[4],std::as_bytes(std::span(args)));world.run_actual(0x15cef0u,select);std::cout<<"Actual Wraith selector="<<std::hex<<select.gp[0]<<std::dec<<'\n';}
 for(unsigned n=0;n<scenario_root.u32(0x120);++n)if(placements.slice(n*0x90+0x14,1)[0]==std::byte{3}){vehicle_row=n;std::cout<<"Source netgame candidate row="<<n<<" variant+CF="<<source_game_variant_get(variant,78)<<" delivered+20B="<<unsigned(source.view(word(0x4f3218u)+0x20bu,1)[0])<<'\n';vehicle=world.create_netgame_object(n);std::cout<<"Original netgame result="<<std::hex<<vehicle<<std::dec<<'\n';break;}check(vehicle!=0xffffffffu,"Original row/variant did not produce a Wraith");
 const auto root_body=world.source_object_body(vehicle);const auto vehicle_tag=cache.query_tag({.handle=TagHandle{word(root_body)}});check(vehicle_tag && vehicle_tag->group=="vehi" && vehicle_tag->name=="objects\\vehicles\\wraith\\wraith","Original vehicle selector did not choose Wraith");
 std::uint32_t weapon=0xffffffffu,weapon_definition=0xffffffffu;unsigned visited{};
 const auto visit=[&](auto &&self,std::uint32_t parent)->void {for(auto child=word(world.source_object_body(parent)+0x10u);child!=0xffffffffu;){check(++visited<0x800u,"Actual Wraith child chain cycles");const auto body=world.source_object_body(child);check(word(body+0x14u)==parent,"Wraith child lost its actual parent");const auto tag=cache.query_tag({.handle=TagHandle{word(body)}});if(tag && tag->group=="weap" && tag->name.find("\\wraith\\turrets\\mortar\\")!=std::string::npos){check(weapon==0xffffffffu,"Multiple actual Wraith mortar weapons");weapon=child;weapon_definition=tag->datum;}self(self,child);child=word(body+0xcu);}};visit(visit,vehicle);check(weapon!=0xffffffffu,"Actual Wraith birth has no mortar child weapon");
 std::cout<<"Actual Wraith row="<<vehicle_row<<" vehicle="<<std::hex<<vehicle<<" weapon="<<weapon<<" definition="<<weapon_definition<<std::dec<<'\n';
 const Vec3 declared_player_position{std::bit_cast<float>(word(root_body+0x64u)),std::bit_cast<float>(word(root_body+0x68u)),std::bit_cast<float>(word(root_body+0x6cu))+3.f};
 const auto biped=std::ranges::find_if(cache.tags(),[](const auto &t){return t.group=="bipd" && t.name=="objects\\characters\\masterchief\\masterchief_mp";});check(biped!=cache.tags().end() && !cache.spawns().empty(),"Actual represented biped/spawn missing");
 const auto unit=world.create_biped(biped->datum,declared_player_position,{1,0,0});check(unit!=0xffffffffu,"Source biped creation failed");DatumPool players(world.memory(),world.player_pool());const auto player=players.allocate_at(0);check(player!=DatumPool::none,"Source player allocation failed");world.construct_player(player,0,source_default_player_configuration(),identity());world.bind_player(player,unit);if(local)world.set_local_player(player,0);world.initialize_player_equipment(player,unit);
 std::array<std::byte,0x5c> neutral;initialize_source_neutral_command(neutral);for(unsigned tick=0;tick<60;++tick){world.deliver_command(player,unit,neutral);world.step_objects();}
 // Mortar_mp shares the original mortar's trigger block. Follow the actual
 // completed source tag bank rather than restricting it to one tag payload.
 const auto metadata=cache.load_source_tag_metadata(weapon_definition);const Bytes w(source.view(metadata.source_address(),0x310));check(w.u32(0x2c8)==1,"Actual mortar trigger count differs");const Bytes trigger(source.view(w.u32(0x2cc),0x40));const auto effect=trigger.u32(0x34);const auto definition=cache.load_source_tag_metadata(effect);const Bytes d(source.view(definition.source_address(),0x18));check(d.u32(0x14)==1,"Actual charging effect event count differs");std::cout<<"Actual diagnostic effect="<<std::hex<<effect<<std::dec<<" trigger behavior="<<trigger.u16(6)<<'\n';
 auto &bank=world.memory();const auto put=[&](std::uint32_t address,std::uint32_t value){bank.write(address,std::as_bytes(std::span(&value,1)));};
 SourcePhysicsWorldRuntimeAbi a{};a.gp[0]=effect;a.gp[1]=weapon;a.gp[4]=0x600f0000u;a.fs_base=0x60200000u;a.eflags=2;a.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};put(a.gp[4],0x60300000u);put(a.gp[4]+4,0xffffffffu);put(a.gp[4]+8,0);put(a.gp[4]+12,0);
 const auto capture=[&](const std::filesystem::path &directory,std::uint32_t entry,SourcePhysicsWorldRuntimeAbi input) {
  std::filesystem::create_directories(directory);
  const auto dump=[&](const char *name){std::ofstream file(directory/name,std::ios::binary);bank.visit_committed([&](auto p,auto bytes){write(file,p);write(file,std::uint32_t(bytes.size()));file.write(reinterpret_cast<const char*>(bytes.data()),std::streamsize(bytes.size()));});check(bool(file),"Cannot export actual source bank");};
  std::ofstream file(directory/"native-input.bin",std::ios::binary);write(file,input.gp);write(file,input.xmm);write(file,input.eflags);write(file,input.fs_base);file.close();dump("before.bin");
  std::ofstream stores(directory/"native-stores.bin",std::ios::binary);
  try {world.run_actual(entry,input,[&](const auto &s){write(stores,s.instruction);write(stores,s.address);write(stores,std::uint32_t(s.written.size()));stores.write(reinterpret_cast<const char*>(s.before.data()),std::streamsize(s.before.size()));stores.write(reinterpret_cast<const char*>(s.written.data()),std::streamsize(s.written.size()));});}
  catch(const std::exception &e){dump("after.bin");std::ofstream(directory/"failure.txt")<<e.what()<<'\n';throw;}
  stores.close();dump("after.bin");
  std::ofstream exit(directory/"native-exit.bin",std::ios::binary);write(exit,input.gp);write(exit,input.xmm);write(exit,input.eflags);write(exit,input.fs_base);write(exit,input.x87_depth);write(exit,input.x87_status);const auto fp=input.floating_point.value();write(exit,fp.x87_control_word);write(exit,fp.mxcsr);check(bool(exit),"Cannot retain native exit ABI");return input;
 };
 const auto retain_metadata=[&](const std::filesystem::path &directory,std::uint32_t entry,const SourcePhysicsWorldRuntimeAbi &exit,std::uint32_t instance) {
  std::ofstream summary(directory/"native.json");summary<<"{\"entry\":"<<entry<<",\"player\":"<<player<<",\"vehicle\":"<<vehicle<<",\"vehicle_row\":"<<vehicle_row<<",\"unit\":"<<unit<<",\"weapon\":"<<weapon<<",\"definition\":"<<weapon_definition<<",\"effect\":"<<effect<<",\"effect_flags\":"<<d.u32(0)<<",\"variant_choice\":5,\"vehicle_variant_parameter\":78,\"vehicle_variant_choice\":2,\"trigger_behavior\":"<<trigger.u16(6)<<",\"manual_diagnostic\":true,\"local_view\":"<<(local?"true":"false")<<",\"exit_eax\":"<<exit.gp[0]<<",\"effect_instance\":"<<instance;
  if(entry==0x108a70u)summary<<",\"returned_effect\":"<<exit.gp[0];summary<<",\"prebirth_player_position_bits\":["<<std::bit_cast<std::uint32_t>(declared_player_position.x)<<','<<std::bit_cast<std::uint32_t>(declared_player_position.y)<<','<<std::bit_cast<std::uint32_t>(declared_player_position.z)<<"],\"neutral_source_ticks\":60}\n";check(bool(summary),"Cannot retain source metadata");
 };
 const auto allocated=capture(out,0x108a70u,a);const auto instance=allocated.gp[0];retain_metadata(out,0x108a70u,allocated,instance);
 if(instance==0xffffffffu){std::cout<<"PASS actual source graph returned NONE; no live instance to stop\n";return 0;}
 const auto pool=word(0x4f720cu);check(word(pool+0x24u)==400 && (instance&65535u)<word(pool+0x38u),"Actual effect instance escapes its source pool");const auto record=word(pool+0x44u)+(instance&65535u)*400u;check(Bytes(bank.view(record,2)).u16(0)==instance>>16 && word(record+4u)==effect,"Actual effect lost its full source identity/definition");
 const auto stop=out/"stop";std::filesystem::create_directories(stop);
 const auto retain_record=[&](const char *name){std::ofstream file(stop/name,std::ios::binary);const auto bytes=bank.view(record,400);file.write(reinterpret_cast<const char*>(bytes.data()),400);check(bool(file),"Cannot retain actual effect record");};retain_record("effect-before.bin");
 SourcePhysicsWorldRuntimeAbi stopping{};stopping.gp[7]=instance;stopping.gp[4]=0x600f0000u;stopping.fs_base=0x60200000u;stopping.eflags=2;stopping.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};put(stopping.gp[4],0x60300000u);put(stopping.gp[4]+4,0);
 const auto stopped=capture(stop,0x17a0f0u,stopping);retain_record("effect-after.bin");retain_metadata(stop,0x17a0f0u,stopped,instance);
 std::cout<<"PASS actual source Wraith diagnostic graph allocation and stop, instance="<<std::hex<<instance<<'\n';return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
