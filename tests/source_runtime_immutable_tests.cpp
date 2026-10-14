#include "halo2/source_runtime_memory.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>

namespace { bool forbid_allocation=false; }
void *operator new(std::size_t bytes){if(forbid_allocation)throw std::bad_alloc();if(auto p=std::malloc(bytes?bytes:1))return p;throw std::bad_alloc();}
void *operator new[](std::size_t bytes){return ::operator new(bytes);}
void operator delete(void *p)noexcept{std::free(p);}
void operator delete[](void *p)noexcept{std::free(p);}
void operator delete(void *p,std::size_t)noexcept{std::free(p);}
void operator delete[](void *p,std::size_t)noexcept{std::free(p);}
using halo2::SourceRuntimeMemory;
namespace {
using Bank=SourceRuntimeMemory;
void check(bool b,const char *message){if(!b)throw std::runtime_error(message);}
bool legacy_matches(const Bank::Checkpoint &live,const Bank::Checkpoint &incoming){
  if(incoming.version!=1)return false;
  std::vector<const Bank::CheckpointRegion*> a,b;
  for(const auto &r:live.regions)if(r.access==Bank::Access::ReadOnly)a.push_back(&r);
  for(const auto &r:incoming.regions)if(r.access==Bank::Access::ReadOnly)b.push_back(&r);
  if(a.size()!=b.size())return false;
  for(std::size_t i=0;i<a.size();++i)if(*a[i]!=*b[i])return false;
  return true;
}
bool compare(const Bank &bank,const Bank::Checkpoint &incoming){
#if defined(HALO2_TEST_LEGACY_IMMUTABLE_COMPARISON)
  return legacy_matches(bank.checkpoint(),incoming);
#else
  return bank.matches_read_only_checkpoint(incoming);
#endif
}
void guarded_check(const Bank &bank,const Bank::Checkpoint &incoming,bool expected){
  bool result;
  forbid_allocation=true;
  try{result=compare(bank,incoming);}catch(...){forbid_allocation=false;throw;}
  forbid_allocation=false;
  check(result==expected,"Immutable comparison changed the original byte/metadata decision");
}
}
int main()try{
  Bank bank;const std::array bytes{std::byte{0x27},std::byte{0x85},std::byte{0xa6},std::byte{0x19}};
  bank.mount(0x1000,bytes,Bank::Access::ReadOnly,"first immutable image");
  bank.mount(0x2000,bytes,Bank::Access::ReadWrite,"mutable game state");
  bank.mount(0x3000,bytes,Bank::Access::ReadOnly,"second immutable image");
  bank.reserve(0x4000,0x2000,0x1000,"mutable reserved state");
  const auto current=bank.checkpoint();std::vector<Bank::Checkpoint> cases{current};
  auto add=[&](auto change){auto copy=current;change(copy);cases.push_back(std::move(copy));};
  add([](auto &c){c.version=2;});
  add([](auto &c){++c.regions[0].address;});
  add([](auto &c){c.regions[0].owner="different source owner";});
  add([](auto &c){c.regions[0].access=Bank::Access::ReadWrite;});
  add([](auto &c){c.regions[0].page_bytes=4096;});
  add([](auto &c){c.regions[0].committed.push_back(1);});
  add([](auto &c){c.regions[0].bytes[0]^=std::byte{0xff};});
  add([](auto &c){c.regions[2].bytes[3]^=std::byte{0xff};});
  add([](auto &c){c.regions[0].bytes.pop_back();});
  add([](auto &c){c.regions[0].bytes.push_back(std::byte{});});
  add([](auto &c){c.regions.erase(c.regions.begin());});
  add([](auto &c){c.regions.push_back(c.regions[0]);});
  add([](auto &c){std::swap(c.regions[0],c.regions[2]);});
  add([](auto &c){c.regions[1].bytes[0]^=std::byte{0xff};});
  add([](auto &c){c.regions[1].owner="other mutable owner";});
  add([](auto &c){c.regions[3].bytes[0x1007]=std::byte{0x92};});
  add([](auto &c){c.regions[1].access=Bank::Access::ReadOnly;});
  for(const auto &c:cases)guarded_check(bank,c,legacy_matches(current,c));
  Bank empty;Bank::Checkpoint empty_state;guarded_check(empty,empty_state,true);guarded_check(empty,current,false);
  auto mutable_only=current;std::erase_if(mutable_only.regions,[](const auto &r){return r.access==Bank::Access::ReadOnly;});
  guarded_check(empty,mutable_only,true);guarded_check(bank,mutable_only,false);
  check(bank.checkpoint()==current,"Read-only comparison mutated live memory");
  std::cout<<"PASS "<<cases.size()+4<<" exact legacy immutable decisions with all heap allocation forbidden\n";return 0;
}catch(const std::bad_alloc &){forbid_allocation=false;std::cerr<<"FAIL immutable validation allocates a temporary snapshot or list\n";return 1;}
catch(const std::exception &e){forbid_allocation=false;std::cerr<<e.what()<<'\n';return 1;}
