#include "halo2/weapon_hud.hpp"
#include "halo2/native_weapon_inventory.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
int main(int argc,char **argv)try {
  if(argc!=3)throw FormatError("Supply ORIGINAL_WEAPON_HUD_FIXTURE ORIGINAL_RESERVE_FIXTURE");
  std::ifstream input(argv[1]);if(!input)return 77;unsigned cases{},reserve_cases{};
  for(std::string line;std::getline(input,line);) {
    std::istringstream row(line);std::array<std::uint32_t,76> words;
    for(auto &word:words)if(!(row>>std::hex>>word))throw FormatError("Truncated original HUD projection");
    const auto sh=[](unsigned value){return std::bit_cast<std::int16_t>(std::uint16_t(value));};
    WeaponHudProjectionInput in;in.datum=words[0];in.heat=std::bit_cast<float>(words[1]);in.age=std::bit_cast<float>(words[2]);in.charge=std::uint8_t(words[3]);in.weapon_flags=std::uint16_t(words[4]);in.input_flags=std::uint16_t(words[5]);in.magazine_present=words[6];in.magazine_mode=sh(words[7]);in.loaded=sh(words[8]);in.capacity=sh(words[9]);in.maximum_reserve=sh(words[10]);in.shared_reserve=std::bit_cast<std::int32_t>(words[11]);in.trigger_present=words[12];in.trigger_behavior=sh(words[13]);in.charging_type=sh(words[14]);in.trigger_ticks=sh(words[15]);in.tick_rate=sh(words[16]);in.trigger_state=std::uint8_t(words[17]);in.acquire_target=words[18];
    std::array<std::uint32_t,14> initial;std::copy_n(words.begin()+20,14,initial.begin());auto state=std::bit_cast<SourceWeaponHudState>(initial);weapon_hud_project(state,in);const auto raw=std::bit_cast<std::array<std::uint32_t,14>>(state);
    for(unsigned n=0;n<14;++n)if(raw[n]!=words[n+34]){std::cerr<<"case="<<cases<<" projection="<<n<<" expected="<<std::hex<<words[n+34]<<" actual="<<raw[n]<<'\n';throw FormatError("Original HUD status bytes differ");}
    for(unsigned n=0;n<10;++n)if(std::bit_cast<std::uint32_t>(weapon_hud_input(&state,n+48,std::bit_cast<float>(words[19])))!=words[n+48])throw FormatError("Original HUD scalar differs at case "+std::to_string(cases)+" selector "+std::to_string(n+48));
    for(unsigned type=0;type<6;++type)if(weapon_hud_mask(type>=1 && type<=4?&state:nullptr,type)!=words[type+58])throw FormatError("Original HUD weapon mask differs");
    const auto mask=weapon_hud_cutoffs(&state,{words[64],words[65],words[66]},weapon_hud_mask(&state,1));
    const std::array<unsigned,9> bits{1,8,0x10,0x20,0x40,0x80,0x100,0x200,0x400};
    for(unsigned n=0;n<9;++n)if(bool(mask&bits[n])!=bool(words[67+n]))throw FormatError("Original HUD cutoff/visibility differs");
    ++cases;
  }
  std::ifstream reserves(argv[2]);if(!reserves)return 77;
  for(std::string line;std::getline(reserves,line);) {
    if(line.empty() || line[0]=='#')continue;std::istringstream row(line);std::array<std::uint32_t,96> w;
    for(auto &value:w)if(!(row>>std::hex>>value))throw FormatError("Truncated original reserve query");
    if(w[0] || w[2] || w[4] || w[5])continue; // Finite104CF0 magazine0, aggregate query.
    const auto sh=[](unsigned value){return int(std::bit_cast<std::int16_t>(std::uint16_t(value)));};
    std::array<WeaponState,5> items;
    for(unsigned n=0;n<5;++n){items[n].source_datum=0x12340000u+n;items[n].definition=n && (w[7]&(1u<<n))?8u:7u;items[n].loaded=sh(w[14+n*8+2]);items[n].reserve=sh(w[14+n*8+1]>>16);}
    Unit unit;unit.source_datum=w[3]?0x12340005u:null_datum;
    for(unsigned slot=0;slot<4;++slot)if(w[8+slot]!=null_datum){unit.source_inventory.slots[slot]=0x12340000u+w[8+slot];unit.source_inventory.inactive_weapons[slot]=items.at(w[8+slot]);}
    const auto expected=std::bit_cast<std::int32_t>(w[54])+items[0].loaded;
    if(native_weapon_ammunition_available(unit,items[0])!=expected)throw FormatError("Native finite HUD/pose ammunition differs from original reserve owner");++reserve_cases;
  }
  if(cases!=4032 || reserve_cases<100)throw FormatError("Incomplete original HUD coverage");
  std::cout<<"PASS "<<cases<<" original TU1.1/TU1.5 HUD projections,56 bytes,10 weapon inputs and masks; "<<reserve_cases<<" original finite inventory queries\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
