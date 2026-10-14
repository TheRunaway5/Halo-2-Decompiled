#include "halo2/source_runtime_host_physical_memory.hpp"
#include "halo2/source_runtime_host_virtual_memory.hpp"
#include "halo2/object_body_heap.hpp"
#include "halo2/physics_pool_host_storage.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>

using namespace halo2;
namespace {
using Bank=SourceRuntimeMemory;
void check(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
template<class F> void rejects(F &&f) {
  bool rejected=false;try{f();}catch(const std::exception &){rejected=true;}
  check(rejected,"Invalid continuation was accepted");
}
void marker(Bank &bank,std::uint32_t address,std::byte value) {
  bank.write(address,std::array{value});
}
SourceObjectBodyHeapServices heap_services(Bank &bank) {
  SourceObjectBodyHeapServices services;
  services.read=[&bank](auto p,auto n){return bank.view(p,n);};
  services.memory=[&bank](auto p,auto n){return bank.mutable_view(p,n);};
  return services;
}
auto &region(Bank::Checkpoint &bank,std::uint32_t address) {
  const auto found=std::ranges::find(bank.regions,address,&Bank::CheckpointRegion::address);
  if(found==bank.regions.end())throw std::runtime_error("Test bank owner missing");
  return *found;
}
void put(Bank &bank,std::uint32_t address,std::uint32_t word) {
  std::array<std::byte,4> bytes;
  for(unsigned i=0;i<4;++i)bytes[i]=std::byte(word>>(i*8));
  bank.write(address,bytes);
}
std::uint32_t word(Bank &bank,std::uint32_t address) {
  std::uint32_t value=0;const auto bytes=bank.view(address,4);
  for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[i])<<(i*8);
  return value;
}
std::uint32_t pool_allocate(Bank &bank,SourcePhysicsPoolHostStorage &pool,std::uint32_t bytes) {
  SourcePhysicsWorldRuntimeAbi abi{};abi.gp[4]=0x4000;
  const std::array<std::uint32_t,6> frame{0x33b3c5,0x4100,0,0x4104,0x101000,4};
  for(unsigned i=0;i<frame.size();++i)put(bank,0x4000+i*4,frame[i]);
  put(bank,0x4100,0);put(bank,0x4104,bytes);
  SourcePhysicsContactRuntimeCall call{};call.instruction=0x33786f;call.target=0x800000b8;
  check(pool.try_call(bank,call,abi) && abi.gp[0]==0 && abi.gp[4]==0x4018,"Native pool allocation ABI failed");
  return word(bank,0x4100);
}
void pool_release(Bank &bank,SourcePhysicsPoolHostStorage &pool,std::uint32_t base) {
  SourcePhysicsWorldRuntimeAbi abi{};abi.gp[4]=0x4000;
  const std::array<std::uint32_t,4> frame{0x33b401,0x4100,0x4104,0x8000};
  for(unsigned i=0;i<frame.size();++i)put(bank,0x4000+i*4,frame[i]);
  put(bank,0x4100,base);put(bank,0x4104,0);
  SourcePhysicsContactRuntimeCall call{};call.instruction=0x3378ab;call.target=0x800000c7;
  check(pool.try_call(bank,call,abi) && abi.gp[0]==0 && abi.gp[4]==0x4010,"Native pool release ABI failed");
}
void pool_continuation() {
  Bank source,target;
  source.mount(0x4000,std::vector<std::byte>(0x200),Bank::Access::ReadWrite,"native pool caller frame");
  target.mount(0x4000,std::vector<std::byte>(0x200),Bank::Access::ReadWrite,"native pool caller frame");
  std::uint32_t kept{},scratch_base{};
  std::vector<SourcePhysicsPoolResourceEvent> original_events,resumed_events;
  {
    SourcePhysicsPoolHostStorage pool(source,0x20000000,[&](const auto &e){original_events.push_back(e);});
    SourcePhysicsPoolHostStorage resumed(target,0x50000000,[&](const auto &e){resumed_events.push_back(e);});
    kept=pool_allocate(source,pool,0x1800);marker(source,kept+0x10,std::byte{0xad});
    const auto retired=pool_allocate(source,pool,0x3000);pool_release(source,pool,retired);
    (void)pool_allocate(target,resumed,0x1000);
    SourceRuntimeHostStorage scratch(source,0x60000000);
    scratch_base=scratch.allocate({0,0,0x1234,0x101000,4}).base;
    marker(source,scratch_base,std::byte{0xc7}); // A different owner shares the raw label.
    const auto saved=source.checkpoint(),old=target.checkpoint();
    const auto ownership=pool.ownership_checkpoint(),old_owner=resumed.ownership_checkpoint();
    {
      auto stage=resumed.prepare_ownership(ownership,saved);
      check(!pool.commit_ownership(std::move(stage)),"Foreign pool owner accepted prepared registry");
      check(target.checkpoint()==old,"Pool staging mutated the current bank");
    }
    auto bad=ownership;bad.version=2;rejects([&]{resumed.prepare_ownership(bad,saved);});
    bad=ownership;bad.cursor.version=2;rejects([&]{resumed.prepare_ownership(bad,saved);});
    bad=ownership;bad.cursor.next=kept+0x1000;rejects([&]{resumed.prepare_ownership(bad,saved);});
    bad=ownership;bad.allocations.at(kept)=0x1000;rejects([&]{resumed.prepare_ownership(bad,saved);});
    bad=ownership;bad.allocations.emplace(kept+0x1000,0x1000);rejects([&]{resumed.prepare_ownership(bad,saved);});
    auto invalid=saved;region(invalid,kept).owner="different owner";
    rejects([&]{resumed.prepare_ownership(ownership,invalid);});
    check(target.checkpoint()==old && resumed.ownership_checkpoint()==old_owner,"Rejected pool import changed current owner");
    auto stage=resumed.prepare_ownership(ownership,saved);
    target.restore(saved);check(resumed.commit_ownership(std::move(stage)),"Pool registry publication failed");
    check(resumed.ownership_checkpoint()==ownership && resumed.live_allocations()==1,"Imported pool registry differs");
    original_events.clear();resumed_events.clear();
    const auto next=pool_allocate(source,pool,0x2500);
    check(pool_allocate(target,resumed,0x2500)==next && next==ownership.cursor.next,"Resumed pool reused a retired token");
    pool_release(source,pool,kept);pool_release(target,resumed,kept);
    rejects([&]{pool_release(target,resumed,kept);});
    // Rejected double-release writes only its caller frame; perform the same
    // rejected request on the uninterrupted owner before whole-bank comparison.
    rejects([&]{pool_release(source,pool,kept);});
    check(source.checkpoint()==target.checkpoint() && pool.ownership_checkpoint()==resumed.ownership_checkpoint(),
          "Future pool allocation/release diverged after import");
    check(original_events.size()==resumed_events.size(),"Local pool observer was replaced or replayed");
    for(std::size_t i=0;i<original_events.size();++i)
      check(original_events[i].allocation==resumed_events[i].allocation &&
          original_events[i].request==resumed_events[i].request && original_events[i].result==resumed_events[i].result,
          "Resumed pool observer reported a different native resource request");
    // Restore the empty pool's advanced cursor too.
    pool_release(source,pool,next);pool_release(target,resumed,next);
    const auto empty_bank=source.checkpoint();
    auto empty=resumed.prepare_ownership(pool.ownership_checkpoint(),empty_bank);
    target.restore(empty_bank);check(resumed.commit_ownership(std::move(empty)),"Empty pool cursor restore failed");
    check(pool_allocate(source,pool,0x1000)==pool_allocate(target,resumed,0x1000),"Empty pool import reset its cursor");
  }
  rejects([&]{target.view(kept,1);});
  check(target.view(scratch_base,1)[0]==std::byte{0xc7},"Pool retirement released another raw owner");
}
}
int main() try {
#if !defined(__linux__)
  return 77;
#endif
  Bank source,target;
  const std::array name{std::byte{'o'},std::byte{'b'},std::byte{'j'},std::byte{}};
  source.mount(0x1000,name,Bank::Access::ReadOnly,"immutable heap name");
  source.mount(0x2000,std::array<std::byte,12>{},Bank::Access::ReadWrite,"heap pointer cells");
  std::uint32_t imported_base{},physical_base{};
  {
    SourceRuntimeHostVirtualMemory vm(source),other_vm(target,0x30000000);
    SourceRuntimeHostPhysicalMemory physical(source),other_physical(target);
    const auto reserve=vm.allocate({0,0,0x5000,0x2000,4});imported_base=reserve.base;
    vm.allocate({reserve.base,0,0x1000,0x1000,4});
    vm.allocate({reserve.base+0x3000,0,0x1000,0x1000,4});
    marker(source,reserve.base+0x3010,std::byte{0x5e});
    // Preserve a retired allocation's cursor gap as well as active regions.
    const auto retired=vm.allocate({0,0,0x2000,0x2000,4});
    vm.release(retired.base,retired.bytes,0x8000);
    const auto extra=vm.allocate({0,0,0x2000,0x2000,4});
    vm.allocate({extra.base,0,0x2000,0x1000,4});
    auto services=heap_services(source);
    initialize_source_object_body_heap(reserve.base,0x800,0x1000,0,services);
    check(allocate_source_object_body(reserve.base,0x2000,64,0,0,services),"Source heap setup failed");
    physical_base=physical.allocate({0x3000,0x20000,0x2ffff,0x1000,4});
    marker(source,physical_base+0x1fff,std::byte{0x9a});
    physical.protect(physical_base+0xfff,2,2); // Two intersecting pages.
    const auto second=physical.allocate({0x1000,0x20000,0x2ffff,0x1000,4});
    marker(source,second,std::byte{0x81});
    const auto old_virtual=other_vm.allocate({0,0,0x1000,0x2000,4});
    other_vm.allocate({old_virtual.base,0,0x1000,0x1000,4});
    const auto old_physical=other_physical.allocate({0x1000,0x70000,0x70fff,0x1000,4});
    other_physical.protect(old_physical,1,2);
    const auto old_bank=target.checkpoint();
    const auto old_vm=other_vm.ownership_checkpoint(),saved_vm=vm.ownership_checkpoint();
    const auto old_physical_owner=other_physical.ownership_checkpoint(),saved_physical=physical.ownership_checkpoint();
    const auto saved=source.checkpoint();
    const auto old_address=target.native_address(old_virtual.base,1);
    // Cancellation, wrong-owner commits and moves must not publish a prefix.
    {
      auto staged_vm=other_vm.prepare_ownership(saved_vm,saved);
      auto staged_physical=other_physical.prepare_ownership(saved_physical,saved);
      check(!vm.commit_ownership(std::move(staged_vm)),"Foreign virtual owner accepted staged resources");
      check(!physical.commit_ownership(std::move(staged_physical)),"Foreign physical owner accepted staged resources");
      auto moved=std::move(staged_vm);
      check(moved.for_owner(other_vm) && !staged_vm.for_owner(other_vm),"Prepared ownership move lost identity");
      check(target.checkpoint()==old_bank,"Staging changed the target byte bank");
    }
    check(other_vm.ownership_checkpoint()==old_vm && other_physical.ownership_checkpoint()==old_physical_owner,
          "Cancelled staging changed resource ownership");
    // Validate malformed topology before publishing any incoming resource.
    auto bad_vm=saved_vm;bad_vm.version=2;rejects([&]{other_vm.prepare_ownership(bad_vm,saved);});
    bad_vm=saved_vm;bad_vm.next=reserve.base;rejects([&]{other_vm.prepare_ownership(bad_vm,saved);});
    bad_vm=saved_vm;bad_vm.reservations[0].committed[0]=2;rejects([&]{other_vm.prepare_ownership(bad_vm,saved);});
    bad_vm=saved_vm;bad_vm.reservations[0].committed.pop_back();rejects([&]{other_vm.prepare_ownership(bad_vm,saved);});
    bad_vm=saved_vm;bad_vm.reservations[1]=bad_vm.reservations[0];rejects([&]{other_vm.prepare_ownership(bad_vm,saved);});
    bad_vm=saved_vm;bad_vm.reservations.pop_back();rejects([&]{other_vm.prepare_ownership(bad_vm,saved);});
    auto bad_bank=saved;region(bad_bank,reserve.base).committed[1]=1;
    rejects([&]{other_vm.prepare_ownership(saved_vm,bad_bank);});
    bad_bank=saved;region(bad_bank,reserve.base).owner="foreign reservation";
    rejects([&]{other_vm.prepare_ownership(saved_vm,bad_bank);});
    auto bad_physical=saved_physical;bad_physical.version=2;
    rejects([&]{other_physical.prepare_ownership(bad_physical,saved);});
    bad_physical=saved_physical;bad_physical.allocations[0].read_only[0]=2;
    rejects([&]{other_physical.prepare_ownership(bad_physical,saved);});
    bad_physical=saved_physical;bad_physical.allocations[0].read_only.pop_back();
    rejects([&]{other_physical.prepare_ownership(bad_physical,saved);});
    bad_physical=saved_physical;bad_physical.allocations[1]=bad_physical.allocations[0];
    rejects([&]{other_physical.prepare_ownership(bad_physical,saved);});
    bad_physical=saved_physical;bad_physical.allocations.pop_back();
    rejects([&]{other_physical.prepare_ownership(bad_physical,saved);});
    bad_bank=saved;region(bad_bank,physical_base).access=Bank::Access::ReadOnly;
    rejects([&]{other_physical.prepare_ownership(saved_physical,bad_bank);});
    check(target.checkpoint()==old_bank && target.native_address(old_virtual.base,1)==old_address &&
        other_vm.ownership_checkpoint()==old_vm && other_physical.ownership_checkpoint()==old_physical_owner,
        "Rejected staging changed current resources or invalidated views");
    auto staged_vm=other_vm.prepare_ownership(saved_vm,saved);
    auto staged_physical=other_physical.prepare_ownership(saved_physical,saved);
    target.restore(saved);
    check(other_vm.commit_ownership(std::move(staged_vm)) &&
          other_physical.commit_ownership(std::move(staged_physical)),"Admitted owner publication failed");
    check(target.checkpoint()==saved && other_vm.ownership_checkpoint()==saved_vm &&
        other_physical.ownership_checkpoint()==saved_physical,"Resource import differs from uninterrupted owner");
    rejects([&]{target.view(old_address,1);});
    rejects([&]{target.view(old_virtual.base,1);});
    rejects([&]{target.view(old_physical,1);});
    rejects([&]{target.view(reserve.base+0x1000,1);});
    rejects([&]{marker(target,physical_base+0x1000,std::byte{1});});
    rejects([&]{target.mutable_view(physical_base+0xfff,2);});
    // Compare future actual source heap operations, commitment, first-fit
    // physical allocation and monotonic VM allocation after ownership import.
    auto resumed_services=heap_services(target);
    check(allocate_source_object_body(reserve.base,0x2004,128,0,0,services) &&
        allocate_source_object_body(reserve.base,0x2004,128,0,0,resumed_services),"Resumed source heap allocation failed");
    check(resize_source_object_body(reserve.base,0x2000,192,0,0,{0x2008},services) &&
        resize_source_object_body(reserve.base,0x2000,192,0,0,{0x2008},resumed_services),"Resumed source heap relocation failed");
    free_source_object_body(reserve.base,0x2004,services);
    free_source_object_body(reserve.base,0x2004,resumed_services);
    for(auto *owner:{&vm,&other_vm})owner->allocate({reserve.base,0,0x4000,0x1000,4});
    for(auto *owner:{&physical,&other_physical})owner->protect(physical_base+1,0x2000,4);
    for(auto *bank:{&source,&target}) {
      marker(*bank,reserve.base+0x1010,std::byte{0x35});
      marker(*bank,physical_base+0x1010,std::byte{0x48});
    }
    const auto next_vm=vm.allocate({0,0,0x1000,0x2000,4});
    check(other_vm.allocate({0,0,0x1000,0x2000,4}).base==next_vm.base && next_vm.base==saved_vm.next,
          "Imported VM cursor reused a retired token or diverged");
    check(physical.allocate({0x1000,0x20000,0x2ffff,0x1000,4})==
          other_physical.allocate({0x1000,0x20000,0x2ffff,0x1000,4}),"Imported physical first-fit topology diverged");
    check(source.checkpoint()==target.checkpoint() && vm.ownership_checkpoint()==other_vm.ownership_checkpoint() &&
          physical.ownership_checkpoint()==other_physical.ownership_checkpoint(),"Future source/resource execution diverged");
    // Repeated handoff restores active owners, rather than stacking old maps.
    for(int repeat=0;repeat<3;++repeat) {
      const auto copy=source.checkpoint();
      auto v=other_vm.prepare_ownership(vm.ownership_checkpoint(),copy);
      auto p=other_physical.prepare_ownership(physical.ownership_checkpoint(),copy);
      target.restore(copy);
      check(other_vm.commit_ownership(std::move(v)) && other_physical.commit_ownership(std::move(p)),"Repeated handoff failed");
    }
  }
  rejects([&]{target.view(imported_base,1);});
  rejects([&]{target.view(physical_base,1);});
  check(target.view(0x1000,4)[0]==name[0],"Resource retirement released an unrelated owner");
  pool_continuation();
  std::cout<<"PASS staged native VM/physical/pool import, rejection/cancellation, protections, real source heap continuation, future allocation/release and repeated retirement\n";
  return 0;
} catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
