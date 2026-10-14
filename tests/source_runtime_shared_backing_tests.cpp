#include "halo2/source_runtime_memory.hpp"
#include <array>
#include <iostream>
#include <stdexcept>

using namespace halo2;
namespace {
using Bank=SourceRuntimeMemory;
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
template<class F>void rejects(F &&f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,"Invalid shared backing was admitted");}
struct Allocation {
  std::shared_ptr<std::vector<std::byte>> bytes;
  std::shared_ptr<SourceRuntimeMemoryBacking> backing;
  explicit Allocation(std::size_t n,SourceRuntimeMemoryBacking::WriteGuard guard={})
      :bytes(std::make_shared<std::vector<std::byte>>(n,std::byte{0x37})),
       backing(std::make_shared<SourceRuntimeMemoryBacking>(bytes,std::span(*bytes),std::move(guard))){}
};
void lifetime(){
  Bank bank,other;Allocation allocation(64);std::weak_ptr<std::vector<std::byte>> lifetime=allocation.bytes;
  const auto pointer=reinterpret_cast<std::uintptr_t>(allocation.bytes->data());
  bank.mount(0x1000,allocation.backing,Bank::Access::ReadWrite,"physical owner");
  check(bank.native_address(0x1000,64).value==pointer,"Shared backing was copied");
  rejects([&]{other.mount(0x2000,allocation.backing,Bank::Access::ReadWrite,"second owner");});
  check(bank.has_backing(0x1000,allocation.backing),"Rejected mount detached the original region");
  Bank moved(std::move(bank));moved.swap_backing(other);
  check(other.native_address(0x1000,64).value==pointer,"Moving or swapping copied the retained allocation");
  allocation.bytes.reset();allocation.backing.reset();
  check(!lifetime.expired(),"Region lost its allocation lease");
  other.release(0x1000);check(lifetime.expired(),"Retired region leaked its allocation lease");
  Allocation reusable(32);bank.mount(0x3000,reusable.backing,Bank::Access::ReadOnly,"first owner");
  rejects([&]{bank.mutable_view(0x3000,1);});bank.release(0x3000);
  other.mount(0x4000,reusable.backing,Bank::Access::ReadWrite,"next owner");
  check(other.has_backing(0x4000,reusable.backing),"Released lease could not be transferred");
}
void guards(){
  Bank bank;Allocation left(16),right(16,[](auto,std::size_t n){if(n)throw std::runtime_error("platform read-only page");});
  bank.mount(0x1000,left.backing,Bank::Access::ReadWrite,"left owner");
  bank.mount(0x1010,right.backing,Bank::Access::ReadWrite,"right owner");
  const auto saved=bank.checkpoint();const std::array input{std::byte{1},std::byte{2}};
  rejects([&]{bank.write(0x100f,input);});
  rejects([&]{bank.mutable_view(0x1010,1);});
  rejects([&]{bank.validate_mutable_view(0x1010,1);});
  rejects([&]{bank.write(bank.native_address(0x1010,1),std::span(input).first(1));});
  check(bank.checkpoint()==saved,"Rejected shared-page write published a prefix");
  bank.write(0x1000,std::span(input).first(1));
  check((*left.bytes)[0]==input[0],"Source write missed the shared allocation");
  check(saved.regions[0].bytes[0]==std::byte{0x37},"Checkpoint aliases live shared pages");
}
void binding(){
  Bank bank;Allocation allocation(64);bank.mount(0x1000,*allocation.bytes,Bank::Access::ReadWrite,"physical owner");
  const auto saved=bank.checkpoint();const auto previous=bank.native_address(0x1000,64);
  const auto expected=saved.regions.front();
  auto bad=expected;bad.owner="wrong owner";rejects([&]{bank.bind_backing(bad,allocation.backing);});
  bad=expected;bad.access=Bank::Access::ReadOnly;rejects([&]{bank.bind_backing(bad,allocation.backing);});
  bad=expected;bad.bytes[0]=std::byte{0x81};rejects([&]{bank.bind_backing(bad,allocation.backing);});
  bad=expected;bad.bytes.pop_back();rejects([&]{bank.bind_backing(bad,allocation.backing);});
  Allocation wrong_bytes(64);(*wrong_bytes.bytes)[1]=std::byte{};
  rejects([&]{bank.bind_backing(expected,wrong_bytes.backing);});
  check(bank.native_address(0x1000,64)==previous && bank.checkpoint()==saved,"Rejected bind changed the staged bank");
  bank.bind_backing(expected,allocation.backing);bank.bind_backing(expected,allocation.backing);
  check(bank.has_backing(0x1000,allocation.backing) && bank.checkpoint()==saved,"Backing bind changed source state");
  check(bank.native_address(0x1000,64).value==reinterpret_cast<std::uintptr_t>(allocation.bytes->data()),"Bind retained the copied bank");
  rejects([&]{bank.view(previous,1);});
  Bank copy;copy.restore(saved);bank.write(0x1000,std::array{std::byte{0xf1}});
  check(copy.checkpoint()==saved,"Restored checkpoint aliases live physical storage");
}
void admission(){
  rejects([&]{SourceRuntimeMemoryBacking invalid({},std::span<std::byte>{});});
  Allocation allocation(64);Bank bank;
  rejects([&]{bank.mount(0x1000,allocation.backing,Bank::Access::ReadWrite,"");});
  bank.mount(0x1000,allocation.backing,Bank::Access::ReadWrite,"owner");
  auto alias=std::make_shared<SourceRuntimeMemoryBacking>(allocation.bytes,std::span(*allocation.bytes).subspan(16));
  rejects([&]{bank.mount(0x2000,alias,Bank::Access::ReadWrite,"native alias");});
  check(bank.checkpoint().regions.size()==1 && bank.has_backing(0x1000,allocation.backing),"Rejected native alias changed owner topology");
  Allocation overlap(16);rejects([&]{bank.mount(0x1030,overlap.backing,Bank::Access::ReadWrite,"source overlap");});
  Bank other;other.mount(0x5000,overlap.backing,Bank::Access::ReadWrite,"unclaimed lease");
  check(other.has_backing(0x5000,overlap.backing),"Rejected extent retained the publication claim");
}
}
int main()try{lifetime();guards();binding();admission();std::cout<<"PASS shared allocation lifetime, address identity, atomic guarded transfers, checkpoint isolation and staged binding admission\n";return 0;}
catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
