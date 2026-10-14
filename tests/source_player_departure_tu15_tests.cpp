#include "halo2/multiplayer_world.hpp"
#include "halo2/source_player_departure.hpp"
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <iostream>
using namespace halo2;
namespace {
void check(bool v,const char *why){if(!v)throw FormatError(why);}
std::uint32_t word(const SourceRuntimeMemory &m,NativeRuntimeAddress p,unsigned bytes=4){const auto b=m.view(p,bytes);std::uint32_t v{};
  for(unsigned i=0;i<bytes;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);return v;}
struct Files{std::filesystem::path path;Files(){auto s=(std::filesystem::temp_directory_path()/"halo2-source-departure-XXXXXX").string();
  check(::mkdtemp(s.data()),"Cannot create source departure files");path=s;}~Files(){std::error_code e;std::filesystem::remove_all(path,e);}};
SourceDeparturePlayer player(const DatumPool &pool,std::uint32_t datum){const auto bytes=pool.lookup(datum);check(bytes.size()==sizeof(SourceDeparturePlayer),"Missing real player record");
  SourceDeparturePlayer result;std::memcpy(&result,bytes.data(),sizeof(result));return result;}
}
int main(int argc,char **argv)try {
  std::cout<<std::unitbuf;if(argc!=3||!std::filesystem::exists(argv[1])||!std::filesystem::exists(argv[2]))return 77;
  Files files;MultiplayerOptions options;options.gameplay_xbe=argv[1];options.map=argv[2];options.starting_weapon=5;
  options.local_players=4;options.source_object_world=true;options.source_object_savegame_directory=files.path;
  MultiplayerPreparedWorld prepared(options);
  prepared.match.configure_local_views(prepared.local_slots,prepared.cache->load_first_person_binding(prepared.rules.weapon,2));
  auto &world=*prepared.source_objects;auto &bank=world.memory();
  DatumPool pool(bank,world.player_pool());
  const auto slot=prepared.local_slots[1],datum=prepared.match.player(slot).source_datum,unit=prepared.match.player(slot).source_current_object;
  const auto before=player(pool,datum);const auto body=world.object_body(unit);const auto count=pool.count();
  check(before.local_index==1,"Fixture did not bind the real second source local view");
  const auto census=bank.native_address(word(bank,bank.native_address(0x4f54f0u,4)),sizeof(SourceDepartureCensus));
  const auto engine=bank.native_address(word(bank,bank.native_address(0x4f63b8u,4)),0x304u+sizeof(SourceGameEndStats));
  const auto before_engine=bank.view(engine+0x304u,sizeof(SourceGameEndStats));
  const std::vector<std::byte> engine_stats(before_engine.begin(),before_engine.end());
  std::cout<<"Before departure: source census="<<word(bank,census)<<", pool="<<count<<", local users="<<word(bank,census+8u,2)<<'\n';
  check(word(bank,census)==4,"Source fixture lacks four actual active players");
  const auto statistics=bank.view(0x51eef4u,sizeof(SourceResultStatistics));const std::vector<std::byte> stats(statistics.begin(),statistics.end());
  check(world.depart_player(datum),"Original TU1.5 departure gate rejected its real current identity");
  const auto after=player(pool,datum);
  check(after.salt==before.salt&&after.identity==before.identity&&after.configuration==before.configuration&&pool.count()==count,
      "Ordinary source departure deleted its allocation, identity or profile");
  check((after.flags&3u)==2u&&after.current_unit==null_datum&&after.previous_unit==null_datum&&after.local_index==-1&&after.controller_index==-1&&
      after.machine_index==-1&&after.machine_user==-1&&after.machine_controller==-1,
      "Original source departure did not clear its actual control/machine/current-unit bindings");
  check(word(bank,census)==3&&word(bank,census+0x10u)==null_datum&&after.leave_tick==word(bank,world.source_clock()+8u),
      "Original departure did not decrement its actual census, clear the second local user or preserve the source tick");
  check(word(bank,body+0x10au,2)&0x400u,"Original departure omitted deferred unit request");
  check(!(word(bank,body+0x10au,2)&4u)&&word(bank,body+0x13cu)==null_datum&&word(bank,body+0x140u)==datum,
      "Departure killed immediately or lost actual source previous ownership");
  const auto settled=bank.checkpoint();check(!world.depart_player(datum)&&bank.checkpoint()==settled,"Repeated departure changed the actual source state");
  check(!world.depart_player(datum^0x10000u),"Source departure accepted a stale salt");
  unsigned ticks=0;
  for(;ticks<12&&!(word(bank,body+0x10au,2)&4u);++ticks)world.step_objects();
  check(word(bank,body+0x10au,2)&4u,"Actual source world did not consume departure's deferred unit damage");
  check(std::ranges::equal(bank.view(0x51eef4u,stats.size()),stats),"Departure published ordinary lethal statistics");
  check(std::ranges::equal(bank.view(engine+0x304u,engine_stats.size()),engine_stats),"Departure changed the original numeric match-stat bank");
  check(pool.count()==count&&player(pool,datum).identity==before.identity,"Deferred cleanup retired the departed player allocation");
  // A pending replacement bank excludes every mutating public entry, including
  // this newly integrated original departure owner. Cancellation preserves all
  // borrowed source views used below.
  const auto third=prepared.match.player(prepared.local_slots[2]).source_datum;
  const auto cut=world.authority_checkpoint();
  {auto staged=world.prepare_authority(cut);bool rejected=false;
    try{world.depart_player(third);}catch(const FormatError&){rejected=true;}
    check(rejected,"Departure mutated a world with staged replacement backing");}
  check(bank.checkpoint()==cut.world.bank,"Cancelled staging or rejected departure changed the source bank");
  // The active census is distinct from retained allocation. Removing every
  // remaining source participant must reach zero while all four full salts
  // remain valid; it must never wrap from the old uninitialized census.
  unsigned active=3;
  for(const unsigned local:{0u,2u,3u}) {
    const auto other=prepared.match.player(prepared.local_slots[local]).source_datum;
    const auto identity=player(pool,other).identity;
    check(world.depart_player(other),"Original remaining-player departure failed");
    check(word(bank,census)==--active&&pool.count()==count&&player(pool,other).identity==identity,
        "Source departure lost retained allocation or underflowed the active census");
    check(!world.depart_player(other),"Repeated remaining-player departure succeeded");
  }
  check(word(bank,census+8u,2)==0,"Last departure retained a source local control user");
  std::cout<<"PASS actual TU1.5 source departure: four source players, retained salt/profile/identity, real local-control/census removal, repeat/stale rejection, deferred original unit death after "<<ticks<<" world ticks and unchanged numeric match/retained statistics (raw recording="<<word(bank,bank.native_address(0x51dfa0u,1),1)<<")\n";
  std::cout<<"LIMIT source-world owner only; native Match/transport/retained PCR integration remains separate\n";
  return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
