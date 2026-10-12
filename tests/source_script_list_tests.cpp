#include "halo2/source_script_lists.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <array>
using namespace halo2;
namespace {
void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
void dump(std::uint32_t result,const SourceHsListBank &bank) {
  std::cout<<result;
  for(const auto *pool:{&bank.headers(),&bank.references()}) {
    std::cout<<' '<<pool->count()<<' '<<pool->cursor()<<' '<<pool->high_water()<<' '<<pool->next_salt();
    for(const auto v:pool->bitmap())std::cout<<' '<<v;
    const auto bytes=pool->elements();
    for(unsigned i=0;i<bytes.size();i+=4) {
      std::uint32_t v=0;for(unsigned j=0;j<4;++j)v|=std::uint32_t(bytes[i+j])<<(j*8);
      std::cout<<' '<<v;
    }
  }
  std::cout<<'\n';
}
}
int main(int argc,char **argv){try {
  if(argc==2 && std::string(argv[1])=="--volume-probe") {
    unsigned admission{},n{};
    while(std::cin>>admission>>n) {
      std::vector<std::uint32_t> objects(n);for(auto &object:objects)std::cin>>object;
      SourceHsListBank bank;
      const auto list=source_hs_volume_objects({[&]{return bank.allocate();},[&](auto l,auto o){bank.append(l,o);}},
          {[&]{return admission==2;},[&]{return objects;},[](auto o){return (o&1u)==0;}});
      dump(list,bank);
    }
    return 0;
  }
  if(argc==2 && std::string(argv[1])=="--pool-probe") {
    SourceHsListBank bank;std::vector<std::uint32_t> handles;
    std::string line;
    while(std::getline(std::cin,line)) {
      std::istringstream in(line);char op{};in>>op;
      std::uint32_t result=SourceHsListBank::none;unsigned ordinal{},value{};
      if(op=='A'){result=bank.allocate();handles.push_back(result);}
      else if(op=='P'){in>>ordinal>>value;bank.append(handles.at(ordinal),value);}
      else if(op=='C'){in>>ordinal;result=bank.count(handles.at(ordinal));}
      else if(op=='G'){int index;in>>ordinal>>index;result=bank.get(handles.at(ordinal),std::int16_t(index));}
      else if(op=='R'){
        std::vector<std::uint32_t> roots;while(in>>ordinal)roots.push_back(handles.at(ordinal));bank.roots(roots);
      }else if(op=='D')bank.collect();
      else if(op=='N')result=bank.get(SourceHsListBank::none,0);
      else throw std::runtime_error("Invalid source list probe operation");
      dump(result,bank);
    }
    return 0;
  }
  SourceHsListBank bank;const auto list=bank.allocate();
  check(bank.headers().capacity()==48 && bank.references().capacity()==128,"Original list pool dimensions");
  bank.append(list,0xa1000000);bank.append(list,0xa2000007);
  check(bank.count(list)==2 && bank.get(list,0)==0xa2000007 && bank.get(list,-1)==0xa2000007 &&
        bank.get(list,1)==0xa1000000 && bank.get(list,2)==bank.none,"Original LIFO and signed-short list_get");
  check(bank.objects(list)==std::vector<std::uint32_t>{0xa2000007,0xa1000000},"Current object list enumeration");
  bank.roots(std::array{list});bank.collect();check(bank.count(list)==2,"Rooted list survived source GC");
  const auto checkpoint=bank;
  bank.roots({});bank.collect();check(bank.headers().count()==0 && bank.references().count()==0,"Source GC retired header and references");
  bank=checkpoint;check(bank.count(list)==2 && bank.references().count()==2,"Checkpoint retained list identities/links");
  const auto volume=source_hs_volume_objects({[&]{return bank.allocate();},[&](auto l,auto o){bank.append(l,o);}},
      {[]{return true;},[]{return std::vector<std::uint32_t>{7,0xffffffff,9,11};},[](auto o){return o!=9;}});
  check(bank.objects(volume)==std::vector<std::uint32_t>{11,7},"Volume preserves candidate order through source prepend");
  const auto empty=source_hs_volume_objects({[&]{return bank.allocate();},[&](auto l,auto o){bank.append(l,o);}},
      {[]{return false;},[]{throw std::runtime_error("Rejected volume queried candidates");return std::vector<std::uint32_t>{};},[](auto){return true;}});
  check(bank.count(empty)==0,"Unavailable volume retains a fresh empty list");
  SourceHsListBank exhausted;const auto many=exhausted.allocate();
  for(unsigned i=0;i<129;++i)exhausted.append(many,0xa1000000+i);
  check(exhausted.count(many)==129 && exhausted.references().count()==128 && exhausted.get(many,0)==0xa100007f,
        "Source count advances even when reference allocation fails");
  SourceHsListBank full;std::array<std::uint32_t,48> live{};
  for(auto &h:live)h=full.allocate();full.roots(live);
  check(full.allocate()==full.none,"Pinned source header pool must report exhaustion");
  full.roots(std::span(live).first(47));const auto replacement=full.allocate();
  check(replacement!=full.none && replacement!=live.back() && (replacement&65535)==47,"Allocation retry collects an unrooted header with a fresh salt");
  std::cout<<"source HS list bank lifecycle fixtures passed\n";
  return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
