#include "halo2/source_runtime_host_physical_memory.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <optional>

using namespace halo2;
namespace {
using Bank=SourceRuntimeMemory;
constexpr auto data_base=0x20000000u;
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
template<class F>void rejects(F &&f){bool rejected=false;try{f();}catch(const std::exception &){rejected=true;}check(rejected,"Invalid checkpoint operation was admitted");}
std::byte sample(std::uint32_t n){auto value=n*0x9e3779b9u;value^=value>>16;return std::byte(value>>24);}
void initialize(Bank &bank,std::uint32_t bytes){
  bank.reserve(data_base,bytes,4096,"nonuniform live source bank");std::array<std::byte,4096> page;
  for(std::uint32_t offset=0;offset<bytes;offset+=page.size()){
    for(unsigned i=0;i<page.size();++i)page[i]=sample(offset+i);
    bank.commit(data_base+offset,page);
  }
}
void verify(std::span<const std::byte> bytes){
  for(std::uint32_t i=0;i<bytes.size();++i)check(bytes[i]==sample(i),"Complete nonuniform snapshot bytes differ");
}
void checkpoint(){
  Bank bank;initialize(bank,20u*1024u*1024u);SourceRuntimeHostPhysicalMemory physical(bank);
  constexpr auto extent=0x30e4000u;
  const auto token=physical.allocate({extent,0x61000u,0x61000u+extent-1,4096,4});
  bank.write(token+4097,std::array{std::byte{0x85}});bank.write(token+extent-1,std::array{std::byte{0xa7}});
  std::cout<<"READY 20 MiB live nonuniform bytes and original-size actual physical allocation\n";
  const auto saved=bank.checkpoint();
  check(saved.regions.size()==2,"Checkpoint lost an owner");verify(saved.regions[0].bytes);
  const auto &bytes=saved.regions[1].bytes;check(bytes.size()==extent,"Physical snapshot extent differs");
  for(std::uint32_t i=0;i<extent;++i)check(bytes[i]==(i==4097?std::byte{0x85}:i==extent-1?std::byte{0xa7}:std::byte{}),"Complete physical snapshot bytes differ");
  bank.write(token+4097,std::array{std::byte{0x19}});
  check(bytes[4097]==std::byte{0x85},"Snapshot aliases live physical writes");
  std::cout<<"PASS complete large live-bank/physical checkpoint with independent bytes\n";
}
Bank::Checkpoint incoming(std::uint32_t size){
  std::vector<std::byte> input(size);for(std::uint32_t i=0;i<size;++i)input[i]=sample(i);
  Bank::Checkpoint saved;saved.regions.push_back({data_base,Bank::Access::ReadWrite,"complete incoming snapshot",0,std::move(input),{}});
  return saved;
}
void restore(){
  auto saved=incoming(48u*1024u*1024u);std::cout<<"READY complete 48 MiB nonuniform incoming checkpoint\n";
  Bank restored;restored.restore(saved);verify(restored.view(data_base,saved.regions[0].bytes.size()));
  saved.regions[0].bytes[7]^=std::byte{0xff};check(restored.view(data_base+7,1)[0]==sample(7),"Restored bank aliases checkpoint writes");
  restored.write(data_base+8,std::array{std::byte{0x94}});check(saved.regions[0].bytes[8]==sample(8),"Checkpoint aliases restored bank writes");
  check(restored.native_address(data_base,1).value>0xffffffffu,"Restored native address was truncated");
  std::cout<<"PASS complete large restore, native address and two-way isolation\n";
}
void copy(){
  auto saved=incoming(40u*1024u*1024u);auto *borrowed=saved.regions[0].bytes.data();
  std::cout<<"READY complete 40 MiB nonuniform checkpoint with borrowed mutable bytes\n";
  auto clone=saved;verify(clone.regions[0].bytes);borrowed[17]^=std::byte{0xff};
  check(clone.regions[0].bytes[17]==sample(17),"Checkpoint copy aliases a pre-existing mutable borrower");
  clone.regions[0].bytes[18]^=std::byte{0xff};check(saved.regions[0].bytes[18]==sample(18),"Checkpoint copies share mutable bytes");
  std::cout<<"PASS complete large checkpoint copy and pre-existing borrower isolation\n";
}
void contracts(){
  Bank bank;const std::array initial{std::byte{1},std::byte{2},std::byte{3},std::byte{4}};
  bank.mount(0x1000,initial,Bank::Access::ReadOnly,"immutable image");
  bank.reserve(0x2000,0x3000,0x1000,"reservation with holes");std::array<std::byte,4096> page{};page[7]=std::byte{0x87};
  bank.commit(0x2000,page);bank.commit(0x4000,page);auto saved=bank.checkpoint();
  saved.regions[1].bytes[0x1008]=std::byte{0x62}; // Preserve inaccessible hole bytes too.
  Bank restored;restored.restore(saved);check(restored.checkpoint()==saved,"Snapshot lost complete bytes, holes or metadata");
  rejects([&]{restored.view(0x3000,1);});rejects([&]{restored.mutable_view(0x1000,1);});
  const auto before=restored.checkpoint();const auto native=restored.native_address(0x2000,1);
  auto invalid=saved;invalid.regions[1].committed[0]=2;rejects([&]{restored.restore(invalid);});
  check(restored.checkpoint()==before && restored.native_address(0x2000,1)==native,"Rejected snapshot publication changed current memory");
  invalid=saved;invalid.regions[1].bytes.pop_back();rejects([&]{restored.restore(invalid);});
  invalid=saved;invalid.regions[0].bytes.clear();rejects([&]{restored.restore(invalid);});
  restored.commit(0x3000,page);check(restored.view(0x3000,4096)[7]==page[7],"Commit after restore differs");
  auto moved=std::move(saved);check(moved.regions.size()==2,"Checkpoint move lost regions");
  std::cout<<"PASS version-1 topology/access, exact hole bytes, atomic rejection and future commitment\n";
}

void adoption(){
  Bank source;initialize(source,42u*1024u*1024u);SourceRuntimeHostPhysicalMemory physical(source);
  constexpr auto extent=0x30e4000u,used=10u*1024u*1024u;
  const auto token=physical.allocate({extent,0x61000u,0x61000u+extent-1,4096,4});
  std::array<std::byte,4096> page;
  for(std::uint32_t offset=0;offset<used;offset+=page.size()){
    for(unsigned i=0;i<page.size();++i)page[i]=sample(offset+i);
    source.write(token+offset,page);
  }
  physical.protect(token,4096,2);const auto saved=source.checkpoint();const auto owner=physical.ownership_checkpoint();
  Bank target,replacement;SourceRuntimeHostPhysicalMemory resumed(target);replacement.restore(saved);
  const auto native=replacement.native_address(token,extent);
  std::cout<<"READY 52 MiB actual nonuniform live bytes and independent restored physical snapshot\n";
  auto prepared=resumed.prepare_ownership(owner,saved,replacement);
  check(replacement.native_address(token,extent)==native,"Physical preparation copied the restored snapshot pages");
  rejects([&]{replacement.mutable_view(token,1);});
  target.swap_backing(replacement);check(resumed.commit_ownership(std::move(prepared)),"Physical snapshot adoption failed publication");
  verify(target.view(token,used));check(target.view(token+extent-1,1)[0]==std::byte{},"Unused physical page bytes changed");
  resumed.protect(token,4096,4);target.write(token+17,std::array{std::byte{0x97}});
  check(source.view(token+17,1)[0]==sample(17) && saved.regions[1].bytes[17]==sample(17),"Adopted physical writes reached source or snapshot");
  std::cout<<"PASS aligned snapshot physical adoption, protection, publication and complete bytes without another resident copy\n";
}

void storage(){
  using Buffer=decltype(Bank::CheckpointRegion{}.bytes);
  constexpr auto boundary=1024u*1024u;
  Buffer bytes(std::vector<std::byte>(boundary-1,std::byte{0x22}));
  bytes.push_back(std::byte{0x33});check(bytes.size()==boundary && bytes[boundary-1]==std::byte{0x33},"Growth across storage boundary differs");
  bytes.pop_back();bytes.push_back(std::byte{0x44});
  check(bytes.size()==boundary && bytes[boundary-1]==std::byte{0x44},"Mapped buffer regrowth differs");
  auto *borrowed=bytes.data();auto moved=std::move(bytes);
  check(moved.data()==borrowed && moved.size()==boundary,"Byte-buffer move invalidated a retained borrower");
  moved=moved;check(moved.data()==borrowed,"Self-assignment invalidated a retained borrower");
  auto copy=moved;borrowed[17]=std::byte{0x95};check(copy[17]==std::byte{0x22},"Copy shares an earlier mutable borrower");
  copy.clear();check(copy.empty(),"Byte-buffer clear differs");
#if defined(__linux__)
  auto saved=incoming(2u*1024u*1024u);Bank target;const std::array marker{std::byte{0x71}};
  target.mount(0x1000,marker,Bank::Access::ReadWrite,"existing target");
  const auto before=target.checkpoint();const auto native=target.native_address(0x1000,1);
  const auto old=std::getenv("XDG_CACHE_HOME");const std::optional<std::string> old_cache=old?std::optional<std::string>(old):std::nullopt;
  struct RestoreEnvironment {std::optional<std::string> previous;~RestoreEnvironment(){if(previous)::setenv("XDG_CACHE_HOME",previous->c_str(),1);else ::unsetenv("XDG_CACHE_HOME");}} environment{old_cache};
  check(::setenv("XDG_CACHE_HOME","/dev/null",1)==0,"Cannot select unavailable snapshot backing");
  rejects([&]{target.restore(saved);});
  check(target.checkpoint()==before && target.native_address(0x1000,1)==native,"Unavailable backing changed current memory or views");
#endif
  std::cout<<"PASS storage-boundary growth, pointer-stable moves/self-assignment and atomic storage failure\n";
}
}
int main(int argc,char **argv)try{
  std::cout<<std::unitbuf;check(argc==2,"Select checkpoint, restore, copy or contracts");const std::string_view mode=argv[1];
  if(mode=="checkpoint")checkpoint();else if(mode=="restore")restore();else if(mode=="copy")copy();else if(mode=="storage")storage();else if(mode=="adoption")adoption();else{check(mode=="contracts","Unknown snapshot case");contracts();}return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
