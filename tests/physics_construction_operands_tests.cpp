#include "halo2/original_data.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
void require(bool b,const char *m){if(!b)throw std::runtime_error(m);}
std::uint32_t word(const std::string &s){return std::stoul(s,nullptr,16);}
}
int main(int argc,char **argv){
  try {
    if(argc!=5)return 77;
    std::ifstream input(argv[1]);if(!input)return 77;
    const auto base=halo2::OriginalData::load_xbe(argv[2]);
    require(!base->object_pressure_operands&&!base->physics_component_construction_constants&&!base->physics_component_rebuild_operands,"Unverified base physical operands supplied");
    const std::array images{halo2::OriginalData::load_xbe(argv[3]),halo2::OriginalData::load_xbe(argv[4])};
    unsigned current=0,cases=0,fields=0;bool active=false;const halo2::PhysicsComponentRebuildImmutableOperands *dispatch=nullptr;
    for(std::string line;std::getline(input,line);){
      std::istringstream row(line);char kind;std::string extra;require(bool(row>>kind),"Empty original physical operand row");
      if(kind=='I'){
        std::array<std::string,9> values;require(bool(row>>current)&&current==cases+1&&current<=2&&!active,"Incorrect original physical operand image");
        for(auto &s:values)require(bool(row>>s),"Missing original physical scalar/address");
        const auto &data=*images[current-1];require(data.object_pressure_operands&&data.physics_component_construction_constants&&data.physics_component_rebuild_operands,"Authenticated physical operands absent");
        const auto &p=*data.object_pressure_operands;
        const std::array words{p.main_usage_scale_bits,p.top_limit_bits,p.high_limit_bits,p.medium_limit_bits,p.low_limit_bits,p.statistic_usage_scale_bits,data.physics_component_construction_constants->tick_scale_bits};
        for(unsigned i=0;i<words.size();++i)require(words[i]==word(values[i]),"Original physical scalar raw word differs");
        dispatch=&*data.physics_component_rebuild_operands;
        require(dispatch->type_remap_address==word(values[7])&&dispatch->branch_table_address==word(values[8]),"Original indexed physical table address differs");
        active=true;fields=0;
      }else if(kind=='T'){
        require(active&&fields++==0,"Incorrect original physical target table order");
        for(const auto value:dispatch->branch_targets){std::string s;require(bool(row>>s)&&value==word(s),"Original physical target table differs");}
        const auto &b=dispatch->branches;const std::array labels{b.unit,b.item,b.direct_grouping,b.ordinary,b.creature};
        for(unsigned i=0;i<labels.size();++i)require(labels[i]==dispatch->branch_targets[i],"Original physical branch labels differ");
      }else if(kind=='B'){
        std::string raw;require(bool(row>>raw)&&active&&fields++==1&&raw.size()==26,"Incorrect physical type remap extent");
        for(unsigned i=0;i<13;++i)require(dispatch->type_remap[i]==word(raw.substr(i*2,2)),"Original physical type remap differs");
      }else if(kind=='E'){
        require(active&&fields==2,"Incomplete physical immutable adapter");++cases;active=false;
      }else throw std::runtime_error("Unknown physical operand row");
      require(!(row>>extra),"Extra original physical operand");
    }
    require(!active&&cases==2,"Missing authenticated physical image");
    std::cout<<"PASS physical construction operands: 56 scalar bytes, 74 indexed table bytes, two authenticated images; base absent, mutable state excluded\n";return 0;
  }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
