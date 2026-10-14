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
 std::cout<<std::unitbuf;check(argc==4 || (argc==5 && std::string(argv[4])=="nonlocal"),"Supply TU1.5_XBE MAP OUTPUT [nonlocal]");const bool local=argc==4;const std::filesystem::path out=argv[3];std::filesystem::create_directories(out);
 Cache cache(argv[2]);const auto pistol=std::ranges::find_if(cache.tags(),[](const auto &t){return t.group=="weap" && t.name=="objects\\weapons\\pistol\\plasma_pistol\\plasma_pistol";});check(pistol!=cache.tags().end(),"Actual plasma pistol tag missing");
 unsigned choice{};for(unsigned n=3;n<=20;++n)try{if(cache.slayer_starting_weapon(n)==pistol->datum){check(!choice,"Ambiguous actual plasma pistol variant choice");choice=n;}}catch(const FormatError&){}check(choice,"Actual pistol has no starting-weapon choice");
 Directory saves;PhysicsLiveWorldTu15 world(cache,argv[1],saves.path);world.initialize_map();auto variant=source_game_variant_default(0);source_game_variant_set(variant,0x59,int(choice));world.configure_session(variant,1,30,0x12345678u);
 const auto biped=std::ranges::find_if(cache.tags(),[](const auto &t){return t.group=="bipd" && t.name=="objects\\characters\\masterchief\\masterchief_mp";});check(biped!=cache.tags().end() && !cache.spawns().empty(),"Actual represented biped/spawn missing");
 const auto unit=world.create_biped(biped->datum,cache.spawns().front().position,{1,0,0});check(unit!=0xffffffffu,"Source biped creation failed");DatumPool players(world.memory(),world.player_pool());const auto player=players.allocate_at(0);check(player!=DatumPool::none,"Source player allocation failed");world.construct_player(player,0,source_default_player_configuration(),identity());world.bind_player(player,unit);if(local)world.set_local_player(player,0);world.initialize_player_equipment(player,unit);
 std::array<std::byte,0x5c> neutral;initialize_source_neutral_command(neutral);for(unsigned tick=0;tick<60;++tick){world.deliver_command(player,unit,neutral);world.step_objects();}
 const auto equipment=world.player_equipment(unit);check(equipment.current[0]>=0 && equipment.current[0]<4,"Actual current hand missing");const auto weapon=equipment.weapons[unsigned(equipment.current[0])];check(equipment.definitions[unsigned(equipment.current[0])]==pistol->datum,"Original equipment did not create/select actual pistol");
 const auto metadata=cache.load_source_tag_metadata(pistol->datum);const Bytes w(metadata.bytes_at(metadata.source_address(),metadata.size()));check(w.u32(0x2c8)==1,"Actual pistol trigger count differs");const Bytes trigger(metadata.bytes_at(w.u32(0x2cc),0x40));const auto effect=trigger.u32(0x34);const auto definition=cache.load_source_tag_metadata(effect);const Bytes d(definition.bytes_at(definition.source_address(),definition.size()));check(d.u32(0x14)==1,"Actual charging effect event count differs");
 auto &bank=world.memory();const auto put=[&](std::uint32_t address,std::uint32_t value){bank.write(address,std::as_bytes(std::span(&value,1)));};
 SourcePhysicsWorldRuntimeAbi a{};a.gp[0]=effect;a.gp[1]=weapon;a.gp[4]=0x600f0000u;a.fs_base=0x60200000u;a.eflags=2;a.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};put(a.gp[4],0x60300000u);put(a.gp[4]+4,0xffffffffu);put(a.gp[4]+8,0);put(a.gp[4]+12,0);
 const auto dump=[&](const char *name){std::ofstream file(out/name,std::ios::binary);bank.visit_committed([&](auto p,auto bytes){write(file,p);write(file,std::uint32_t(bytes.size()));file.write(reinterpret_cast<const char*>(bytes.data()),std::streamsize(bytes.size()));});check(bool(file),"Cannot export actual source bank");};
 std::ofstream input(out/"native-input.bin",std::ios::binary);write(input,a.gp);write(input,a.xmm);write(input,a.eflags);write(input,a.fs_base);input.close();dump("before.bin");
 std::ofstream stores(out/"native-stores.bin",std::ios::binary);try {world.run_actual(0x108a70u,a,[&](const auto &s){write(stores,s.instruction);write(stores,s.address);write(stores,std::uint32_t(s.written.size()));stores.write(reinterpret_cast<const char*>(s.before.data()),std::streamsize(s.before.size()));stores.write(reinterpret_cast<const char*>(s.written.data()),std::streamsize(s.written.size()));});}catch(const std::exception &e){dump("after.bin");std::ofstream(out/"failure.txt")<<e.what()<<'\n';throw;}stores.close();dump("after.bin");
 std::ofstream exit(out/"native-exit.bin",std::ios::binary);write(exit,a.gp);write(exit,a.xmm);write(exit,a.eflags);write(exit,a.fs_base);write(exit,a.x87_depth);write(exit,a.x87_status);const auto fp=a.floating_point.value();write(exit,fp.x87_control_word);write(exit,fp.mxcsr);exit.close();
 std::ofstream summary(out/"native.json");summary<<"{\"entry\":1084016,\"player\":"<<player<<",\"unit\":"<<unit<<",\"weapon\":"<<weapon<<",\"definition\":"<<pistol->datum<<",\"effect\":"<<effect<<",\"effect_flags\":"<<d.u32(0)<<",\"variant_choice\":"<<choice<<",\"local_view\":"<<(local?"true":"false")<<",\"returned_effect\":"<<a.gp[0]<<",\"neutral_source_ticks\":60}\n";check(bool(summary),"Cannot retain source metadata");std::cout<<"PASS actual source pistol charging-effect call, return="<<std::hex<<a.gp[0]<<'\n';return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
