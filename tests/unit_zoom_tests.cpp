#include "halo2/unit_zoom.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void check(bool result,const std::string &what) {if(!result)throw std::runtime_error(what);}
std::uint32_t hex(const std::string &s) {return std::stoul(s,nullptr,16);}
float real(const std::string &s) {return std::bit_cast<float>(hex(s));}
}
int main(int argc,char **argv) {
 try {
  if(argc!=2)throw std::runtime_error("Supply captured original zoom fixture export");
  std::ifstream input(argv[1]);if(!input)return 77;
  unsigned admission=0,cycles=0,magnification=0,unsupported=0,updates=0,initial=0;
  for(std::string line;std::getline(input,line);) {
   std::istringstream in(line);std::vector<std::string> r;
   for(std::string s;in>>s;)r.push_back(s);
   if(r.empty())continue;
   const std::string context="Original zoom differs: "+r.at(1);
   if(r[0]=="I") {
    const halo2::UnitZoomState state;
    check(state.current==std::stoi(r[2]) && state.requested==std::stoi(r[3]) && state.motion==hex(r[4]),context);++initial;continue;
   }
   const std::int16_t mode=std::stoi(r.at(11));
   const halo2::ZoomWeaponView weapon{{hex(r[5]),std::int16_t(std::stoi(r[4])),real(r[6]),real(r[7])},std::span(&mode,1)};
   const halo2::BinocularZoomDefinition binocular{std::stoi(r[8]),real(r[9]),real(r[10])};
   const halo2::UnitZoomContext gate{hex(r[2]),hex(r[3]),{&weapon,nullptr}};
   if(r[0]=="A") {
    check(halo2::source_zoom_admitted(gate)==bool(hex(r[13])&255),context+" admission AL");++admission;
   }else if(r[0]=="C") {
    check(std::uint16_t(halo2::source_zoom_next(std::stoi(r[12]),&weapon,binocular))==std::uint16_t(hex(r[13])),context+" selector AX");++cycles;
   }else if(r[0]=="M") {
    const auto current=std::int16_t(std::stoi(r[12]));
    const auto value=halo2::source_zoom_magnification(current,&weapon,binocular);
    if(weapon.definition.levels>2 && current>=0 && current<weapon.definition.levels && (mode<1 || mode>6)) {
     check(!value,context+" explicit unported power boundary");++unsupported;
    }else {check(value && std::bit_cast<std::uint32_t>(*value)==hex(r[14]),context+" exact XMM0 float");++magnification;}
   }else if(r[0]=="U") {
    halo2::UnitZoomState state{std::int8_t(std::stoi(r[12])),std::int8_t(std::stoi(r[13])),hex(r[14])};
    const auto enabled=bool(std::stoi(r[15]));
    if(enabled)check(halo2::source_zoom_admitted(gate)==bool(hex(r[20])&255),context+" nested original admission");
    halo2::source_zoom_update(state,gate,enabled);
    check(state.current==std::stoi(r[16]) && state.requested==std::stoi(r[17]) && state.motion==hex(r[18]),context+" zoom fields after full unit phase");
    check(halo2::source_zoom_weapon_input(0,state)==(hex(r[19])&0x40u),context+" carried weapon zoom bit; other flags remain external");++updates;
   }else throw std::runtime_error("Unknown source zoom fixture row");
  }
  check(initial==1 && admission==43 && cycles==51 && magnification==38 && unsupported==0 && updates==10,"Complete original zoom fixture coverage");
  std::cout<<admission<<" admissions, "<<cycles<<" selectors, "<<magnification<<" exact magnifications, "<<updates<<" full unit zoom phases matched; "<<unsupported<<" power cases explicitly unavailable\n";
 }catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
