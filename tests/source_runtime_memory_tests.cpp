#include "halo2/object_body_heap.hpp"
#include <algorithm>
#include "halo2/source_runtime_memory.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include <array>
#include <cerrno>
#include <cfenv>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#if defined(__unix__)
#include <sys/mman.h>
#endif

using halo2::SourceRuntimeMemory;
namespace {
void check(bool value, const char *message) {
  if (!value)
    throw std::runtime_error(message);
}
template <class F> void fails(F &&f) {
  bool failed = false;
  try {
    f();
  } catch (const std::exception &) {
    failed = true;
  }
  check(failed, "Missing backing/invalid ownership must fail");
}
std::uint32_t word(const SourceRuntimeMemory &m, std::uint32_t p) {
  const auto b = m.view(p, 4);
  std::uint32_t v = 0;
  for (unsigned i = 0; i < 4; ++i)
    v |= std::to_integer<unsigned>(b[i]) << (8 * i);
  return v;
}

void topology_gap_queries() {
  using A = SourceRuntimeMemory::Access;
  constexpr std::uint32_t base = 0x80000000u;
  SourceRuntimeMemory bank;
  std::array<bool,512> occupied{};
  for (const auto [offset, size] :
       {std::pair{0u,7u}, {16u,31u}, {63u,1u}, {128u,111u}, {400u,112u}}) {
    bank.mount(base+offset,std::vector<std::byte>(size),A::ReadOnly,"gap oracle owner");
    std::fill(occupied.begin()+offset,occupied.begin()+offset+size,true);
  }
  unsigned calls{};int guard;
  halo2::source_runtime_register_topology_guard(bank,&guard,[&](auto,auto){++calls;});
  halo2::source_runtime_register_write_guard(bank,&guard,[&](auto,auto){++calls;});
  for (unsigned alignment : {1u,2u,4u,8u,16u,32u,64u})
    for (unsigned first : {0u,1u,8u,17u,48u,64u,127u,239u,399u,511u})
      for (unsigned size=1;size<=80;++size) {
        std::optional<std::uint32_t> expected;
        for (unsigned at=first;at+size<=occupied.size();++at) {
          if ((base+at)%alignment) continue;
          if (std::none_of(occupied.begin()+at,occupied.begin()+at+size,[](bool v){return v;})) {
            expected=base+at;break;
          }
        }
        check(bank.find_unowned_range(base+first,std::uint64_t(base)+512,size,alignment)==expected,
            "Topology gap differs from independent byte-occupancy first fit");
      }
  check(calls==0,"Read-only gap query invoked mutable ownership policy");
  halo2::source_runtime_unregister_topology_guard(bank,&guard);
  halo2::source_runtime_unregister_write_guard(bank,&guard);
  SourceRuntimeMemory reserved;
  reserved.reserve(base,8192,4096,"uncommitted pages remain occupied");
  check(reserved.find_unowned_range(base,std::uint64_t(base)+16384,4096,4096)==base+8192,
        "Gap query treated uncommitted reservation pages as free");
  reserved.release(base);
  check(reserved.find_unowned_range(base,std::uint64_t(base)+16384,4096,4096)==base,
        "Gap query retained a released owner");
  SourceRuntimeMemory edge;
  check(edge.find_unowned_range(0xffffffffu,0x100000000ull,1,1)==0xffffffffu &&
        !edge.find_unowned_range(0xffffffffu,0x100000000ull,2,1) &&
        !edge.find_unowned_range(0xffffffffu,0x100000000ull,1,2) &&
        edge.find_unowned_range(1,0x100000000ull,1,0x80000000u)==0x80000000u,
        "Gap query alignment/wrap boundary differs");
  fails([&]{edge.find_unowned_range(base,base,1,1);});
  fails([&]{edge.find_unowned_range(base,0x100000001ull,1,1);});
  fails([&]{edge.find_unowned_range(base,0x100000000ull,0,1);});
  fails([&]{edge.find_unowned_range(base,0x100000000ull,1,0);});
  fails([&]{edge.find_unowned_range(base,0x100000000ull,1,3);});
#if defined(__unix__)
  // A topology query must never sample or snapshot the payload. The actual
  // host lease is above4GiB and deliberately unreadable to make that observable.
  constexpr std::size_t unreadable_bytes=2*1024*1024;
  void* mapped=mmap(nullptr,unreadable_bytes,PROT_NONE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
  check(mapped!=MAP_FAILED,"Cannot allocate protected gap-query control");
  auto lifetime=std::shared_ptr<void>(mapped,[](void* p){munmap(p,unreadable_bytes);});
  if constexpr(sizeof(void*)>4)
    check(reinterpret_cast<std::uintptr_t>(mapped)>0xffffffffull,
          "Protected native gap-query control must exercise a host address above4GiB");
  auto backing=std::make_shared<halo2::SourceRuntimeMemoryBacking>(lifetime,
      std::span(static_cast<std::byte*>(mapped),unreadable_bytes));
  SourceRuntimeMemory protected_bank;
  protected_bank.mount(base,backing,A::ReadOnly,"unreadable actual native64 payload");
  check(protected_bank.find_unowned_range(base,std::uint64_t(base)+unreadable_bytes+4096,4096,4096)==base+unreadable_bytes,
        "Gap query sampled opaque backing or failed its full native extent");
#endif
}
void guarded_topology() {
  using A = SourceRuntimeMemory::Access;
  SourceRuntimeMemory left, right;
  const std::array left_bytes{std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}};
  const std::array right_bytes{std::byte{5}, std::byte{6}, std::byte{7}, std::byte{8}};
  left.mount(0x4000, left_bytes, A::ReadWrite, "left retained owner");
  right.mount(0x4000, right_bytes, A::ReadWrite, "right retained owner");
  struct Watch {
    SourceRuntimeMemory &memory;
    unsigned notifications{};
    bool deny{};
    explicit Watch(SourceRuntimeMemory &memory) : memory(memory) {
      halo2::source_runtime_register_topology_observer(memory, this,
          [this](auto, auto) noexcept { ++notifications; });
      halo2::source_runtime_register_topology_guard(memory, this, [this](auto, auto) {
        if (deny) throw std::runtime_error("actual render borrow retains this backing");
      });
    }
    ~Watch() {
      halo2::source_runtime_unregister_topology_guard(memory, this);
      halo2::source_runtime_unregister_topology_observer(memory, this);
    }
  } left_watch(left), right_watch(right);
  const auto left_before = left.checkpoint(), right_before = right.checkpoint();
  const auto left_native = left.native_address(0x4000, 4);
  const auto right_native = right.native_address(0x4000, 4);
  for (auto *blocked : {&left_watch, &right_watch}) {
    blocked->deny = true;
    fails([&] { left.swap_backing(right); });
    fails([&] { right.swap_backing(left); });
    fails([&] { left = std::move(right); });
    fails([&] { right = std::move(left); });
    auto &owner = blocked->memory;
    fails([&] { SourceRuntimeMemory stolen(std::move(owner)); });
    fails([&] { owner.restore(left_before); });
    fails([&] { owner.mount(0x5000, left_bytes, A::ReadWrite, "denied new backing"); });
    check(left.checkpoint() == left_before && right.checkpoint() == right_before &&
          left.native_address(0x4000, 4) == left_native &&
          right.native_address(0x4000, 4) == right_native,
          "Denied backing changes must preserve both real owners and native addresses");
    check(left_watch.notifications == 0 && right_watch.notifications == 0,
          "Denied backing changes must not retire receipts through observers");
    blocked->deny = false;
  }
  left.swap_backing(right);
  check(left_watch.notifications == 1 && right_watch.notifications == 1 &&
        left.native_address(0x4000, 4) == right_native &&
        right.native_address(0x4000, 4) == left_native,
        "Accepted swap must notify both owners and preserve moved native allocations");
  SourceRuntimeMemory moved(std::move(left));
  check(left_watch.notifications == 2 && moved.native_address(0x4000, 4) == right_native,
        "Accepted move must retire the source receipt and retain its real allocation");
  left.restore(left_before);
  check(left_watch.notifications == 3 && left.checkpoint() == left_before,
        "Accepted restore must retain its owner observers and restore exact source bytes");
}
void full_page_topology() {
  using A=SourceRuntimeMemory::Access;
  SourceRuntimeMemory bank,other;
  std::vector<std::byte> bytes(0x4203,std::byte{0x37});
  bank.mount(0x3fefff,bytes,A::ReadWrite,"unaligned region crosses source-index block");
  bank.mount(0xffffe000,std::span(bytes).first(0x2000),A::ReadOnly,"final source pages");
  std::array<std::byte,8> sample;
  for(auto address:{0x3fefffu,0x3ff000u,0x400000u,0x4031fau,0xfffffff8u}) {
    bank.read(address,sample);
    check(std::ranges::all_of(sample,[](auto b){return b==std::byte{0x37};}),
          "Full or partial source page resolved a different region");
  }
  sample.fill(std::byte{0x7a});
  fails([&]{bank.read(0x4031ff,sample);});
  check(sample.front()==std::byte{0x7a},"Page lookup published a rejected read prefix");
  const auto saved=bank.checkpoint();
  bank.release(0x3fefff);
  fails([&]{bank.read(0x400000,sample);});
  bytes.assign(bytes.size(),std::byte{0x52});
  bank.mount(0x3fefff,bytes,A::ReadWrite,"replacement with same source pages");
  bank.read(0x400000,sample);
  check(sample.front()==std::byte{0x52},"Page lookup retained a released node");
  other.mount(0x3fefff,bytes,A::ReadWrite,"different bank with same addresses");
  bank.restore(saved);
  bank.swap_backing(other);
  other.read(0x400000,sample);
  check(sample.front()==std::byte{0x37},"Swap retained a different bank's page index");
  SourceRuntimeMemory moved(std::move(other));
  fails([&]{other.read(0x400000,sample);});
  moved.read(0x400000,sample);
  check(sample.front()==std::byte{0x37},"Move lost the actual source page owner");
  bank=std::move(moved);
  fails([&]{moved.read(0x400000,sample);});
  bank.read(0x400000,sample);
  check(sample.front()==std::byte{0x37},"Move assignment retained discarded page nodes");
}
void native_read_view_queries() {
  using A=SourceRuntimeMemory::Access;
  const auto enabled=[] {
    for(const auto *name:{"HALO2_REFERENCE_SMALL_MEMORY_TRANSFERS","HALO2_REFERENCE_SCALAR_VECTOR_READS"})
      if(const auto *value=std::getenv(name);value && *value)return false;
    return true;
  }();
  SourceRuntimeMemory bank;
  const std::vector<std::byte> bytes(0x230,std::byte{0x53});
  bank.mount(0x3fff03,bytes,A::ReadOnly,"native query partial/full source pages");
  bank.mount(0x1000,std::span(bytes).first(16),A::ReadWrite,"first adjacent native owner");
  bank.mount(0x1010,std::span(bytes).first(16),A::ReadOnly,"second adjacent native owner");
  bank.mount(0xfffffff0,std::span(bytes).first(16),A::ReadOnly,"final native read extent");
  bank.reserve(0x600000,0x4000,0x1000,"native read committed pages");
  const std::vector<std::byte> page(0x1000,std::byte{0x37});
  bank.commit(0x600000,page);
  struct Watch {
    SourceRuntimeMemory &bank;unsigned calls{};
    explicit Watch(SourceRuntimeMemory &bank):bank(bank) {
      auto observer=[this](auto,auto){++calls;};
      halo2::source_runtime_register_write_guard(bank,this,observer);
      halo2::source_runtime_register_write_observer(bank,this,observer);
      halo2::source_runtime_register_topology_guard(bank,this,observer);
      halo2::source_runtime_register_topology_observer(bank,this,observer);
    }
    ~Watch() {
      halo2::source_runtime_unregister_write_guard(bank,this);
      halo2::source_runtime_unregister_write_observer(bank,this);
      halo2::source_runtime_unregister_topology_guard(bank,this);
      halo2::source_runtime_unregister_topology_observer(bank,this);
    }
  } watch(bank);
  struct Generic final:halo2::SourcePhysicsContactMemory {
    mutable unsigned calls{};
    void read(std::uint32_t,std::span<std::byte>)const override {++calls;throw std::runtime_error("Unexpected custom read");}
    void write(std::uint32_t,std::span<const std::byte>)override {++calls;throw std::runtime_error("Unexpected custom write");}
  } generic;
  struct Restore {
    std::fenv_t environment{};int error=errno;
    Restore() {check(std::feholdexcept(&environment)==0,"Cannot hold native-view FP fixture");}
    ~Restore() {std::fesetenv(&environment);errno=error;}
  } restore;
  struct Query {std::uint32_t address;std::size_t bytes;bool complete;};
  const std::array queries{
    Query{0x3fff03,0x230,true},Query{0x400000,16,true},Query{0x400123,16,true},
    Query{0x3fff02,1,false},Query{0x400124,16,false},Query{0x1000,16,true},
    Query{0x1008,16,false},Query{0xfffffff0,16,true},Query{0xfffffff8,16,false},
    Query{0x600ff0,16,true},Query{0x600ff8,16,false},Query{0x601000,16,false},
    Query{0x600000,std::numeric_limits<std::size_t>::max(),false}};
  static_assert(noexcept(bank.try_read_view(0,1)));
  static_assert(noexcept(generic.try_read_view(0,1)));
  for(const auto rounding:{FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO}) {
    check(std::fesetround(rounding)==0 && std::feclearexcept(FE_ALL_EXCEPT)==0 &&
          std::feraiseexcept(FE_DIVBYZERO|FE_INEXACT)==0,"Cannot set native-view FP fixture");
    for(const auto query:queries) {
      std::fenv_t before{},after{};
      check(std::fegetenv(&before)==0,"Cannot capture native-view FP input");
      errno=E2BIG;
      const auto borrowed=bank.try_read_view(query.address,query.bytes);
      const auto after_errno=errno;
      check(std::fegetenv(&after)==0,"Cannot capture native-view FP output");
      check(after_errno==E2BIG && std::memcmp(&before,&after,sizeof(before))==0,
            "Native read admission changed full FP environment or errno");
      check(bool(borrowed)==(enabled && query.complete),"Native read admitted a partial/invalid region or lost a complete one");
      if(borrowed)check(borrowed->size()==query.bytes && borrowed->data()==bank.view(query.address,query.bytes).data(),
                        "Native read returned a different source span");
    }
    std::fenv_t before{},after{};
    check(std::fegetenv(&before)==0,"Cannot capture custom native-view FP input");
    errno=E2BIG;
    const auto borrowed=generic.try_read_view(0x1000,16);
    const auto after_errno=errno;
    check(std::fegetenv(&after)==0,"Cannot capture custom native-view FP output");
    check(!borrowed && generic.calls==0 && after_errno==E2BIG &&
          std::memcmp(&before,&after,sizeof(before))==0,"Custom memory did not refuse native read without side effects");
  }
  check(watch.calls==0,"Native read admission invoked an ownership guard or observer");
  bank.commit(0x601000,page);
  check(bool(bank.try_read_view(0x600ff8,16))==enabled,"Native read failed to requalify a newly committed page");
  bank.release(0x3fff03);
  check(!bank.try_read_view(0x400000,16),"Native read retained a released page-index owner");
  bank.mount(0x3fff03,bytes,A::ReadOnly,"replacement native read region");
  check(bool(bank.try_read_view(0x400000,16))==enabled,"Native read failed to requalify replacement source backing");
#if defined(__unix__)
  // Admission must inspect metadata only, including on the indexed full-page
  // path. Sampling even one backing byte would fault on this real lease.
  constexpr std::size_t inaccessible_bytes=8192;
  void *mapped=mmap(nullptr,inaccessible_bytes,PROT_NONE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
  check(mapped!=MAP_FAILED,"Cannot allocate unreadable native-view lease");
  auto lifetime=std::shared_ptr<void>(mapped,[](void *p){munmap(p,inaccessible_bytes);});
  unsigned platform_guards{};
  auto backing=std::make_shared<halo2::SourceRuntimeMemoryBacking>(lifetime,
      std::span(static_cast<std::byte*>(mapped),inaccessible_bytes),
      [&](auto,auto){++platform_guards;throw std::runtime_error("Native read invoked platform write guard");});
  SourceRuntimeMemory opaque;
  opaque.mount(0x900000,backing,A::ReadWrite,"unreadable native-view lease");
  const auto borrowed=opaque.try_read_view(0x90000d,0x1230);
  check(bool(borrowed)==enabled && platform_guards==0,"Native view sampled backing or invoked its platform guard");
  if(borrowed)check(borrowed->data()==static_cast<const std::byte*>(mapped)+13 && borrowed->size()==0x1230,
                    "Native read capability lost its actual host extent");
#endif
}
} // namespace
int main() {
  try {
    topology_gap_queries();
    guarded_topology();
    full_page_topology();
    native_read_view_queries();
    SourceRuntimeMemory m;
    using A = SourceRuntimeMemory::Access;
    const std::array first{std::byte{1}, std::byte{2}, std::byte{3},
                           std::byte{4}};
    m.mount(0x1000, first, A::ReadWrite, "component bank");
    auto live = m.mutable_view(0x1000, 4);
    const auto native=m.native_address(0x1000,4);
    check(native.value>0xffffffffu && m.source_address(native,4)==0x1000 &&
          m.view(native,4).data()==live.data(),
          "Live runtime backing must retain its actual 64-bit host address");
    SourceRuntimeMemory unrelated;
    unrelated.mount(0x1000,first,A::ReadWrite,"another world");
    fails([&]{unrelated.view(native,1);});
    fails([&]{m.view(halo2::NativeRuntimeAddress{0x1000},1);});
    fails([&]{m.view(halo2::NativeRuntimeAddress{~std::uint64_t{0}},2);});
    m.mount(0x1004, first, A::ReadWrite, "body bank");
    check(live.data() == m.mutable_view(0x1000, 4).data(),
          "Mounting a bank must preserve live views");
    check(native==m.native_address(0x1000,4),"Mount changed native address identity");
    fails([&]{m.view(native+3,2);});
    m.write(native+1,m.view(native,3));
    check(m.view(native,4)[3]==std::byte{3},"Native aliased stores changed source bytes");
    m.write(native,first);
    std::array<std::byte, 8> read{};
    m.read(0x1000, read);
    check(read[0] == first[0] && read[7] == first[3],
          "Adjacent regions retain exact bytes");
    m.write(0x1001, m.view(0x1000, 3));
    check(m.view(0x1000, 4)[3] == std::byte{3},
          "Aliased stores preserve their complete input");
    m.mount(0x1008, first, A::ReadOnly, "authored metadata");
    const auto before = std::vector<std::byte>(m.view(0x1004, 4).begin(),
                                               m.view(0x1004, 4).end());
    fails([&] { m.write(0x1007, first); });
    check(std::ranges::equal(m.view(0x1004, 4), before),
          "A rejected cross-region write must not mutate preceding bytes");
    fails([&] { m.mutable_view(0x1008, 1); });
    const auto readonly=m.native_address(0x1008,4);
    fails([&]{m.write(readonly,first);});
    fails([&]{m.mutable_view(readonly,1);});
    read.fill(std::byte{0x7a});
    fails([&]{m.read(native+3,read);});
    check(read[0]==std::byte{0x7a},"Rejected native read published a prefix");
    fails([&] { m.view(0x1003, 2); });
    fails([&] { m.mount(0x1003, first, A::ReadWrite, "overlap"); });
    read.fill(std::byte{0x7a});
    fails([&] { m.read(0x1009, read); });
    check(read[0] == std::byte{0x7a},
          "A missing read must leave its output untouched");
    const auto released=m.native_address(0x1004,4);
    m.release(0x1004);
    fails([&]{m.view(released,1);});
    fails([&] { m.read(0x1003, std::span(read).first(2)); });
    fails([&] { m.release(0x1004); });
    m.mount(0x1004, first, A::ReadWrite, "new allocation");
    m.mount(0xfffffffc, first, A::ReadWrite, "last32-bit word");
    fails([&] { m.read(0xffffffff, std::span(read).first(2)); });
    check(word(m, 0xfffffffc) == 0x04030201,
          "The final valid address must remain usable");
    fails([&] {
      m.mount(0xfffffffe, first, A::ReadWrite, "wrapped allocation");
    });
    // A reservation owns its holes as well as its committed pages. Source
    // reads, writes and views must all stop before touching an uncommitted page.
    m.reserve(0x100000, 0x4000, 0x1000, "retained reservation");
    fails([&] { m.view(0x100000, 1); });
    fails([&] { m.mutable_view(0x100000, 1); });
    fails([&] { m.read(0x100000, read); });
    fails([&] { m.write(0x100000, first); });
    fails([&] { m.mount(0x101000, first, A::ReadWrite, "reserved overlap"); });
    std::vector<std::byte> page(0x1000, std::byte{0x49});
    m.commit(0x100000, page);
    const auto native_page=m.native_address(0x100000,0x1000);
    fails([&]{m.native_address(0x101000,1);});
    fails([&]{m.view(native_page+0xffc,8);});
    fails([&]{m.mutable_view(native_page+0x1000,1);});
    auto committed_view = m.mutable_view(0x100000, 0x1000);
    committed_view[0] = std::byte{0x68};
    page[0] = std::byte{0x77};
    m.commit(0x100000, page);
    check(m.view(0x100000, 1)[0] == std::byte{0x68},
          "Recommit must preserve source writes");
    const auto edge_view = m.view(0x100ffc, 4);
    const std::vector<std::byte> edge(edge_view.begin(), edge_view.end());
    fails([&] { m.write(0x100ffe, first); });
    check(std::ranges::equal(m.view(0x100ffc, 4), edge),
          "Cross-page rejection must preserve committed bytes");
    read.fill(std::byte{0x7a});
    fails([&] { m.read(0x100ffc, read); });
    check(read[0] == std::byte{0x7a}, "Cross-page read must not publish a prefix");
    fails([&] { m.view(0x100ffc, 8); });
    fails([&] { m.mutable_view(0x100ffc, 8); });
    m.commit(0x101000, page);
    check(committed_view.data() == m.view(0x100000, 0x2000).data(),
          "New commitment must preserve earlier views and reservation identity");
    m.commit(0x103000, page);
    const auto saved=m.checkpoint();
    SourceRuntimeMemory restored;restored.restore(saved);
    const auto restored_page=restored.native_address(0x100000,1);
    check(restored_page!=native_page && restored.source_address(restored_page,1)==0x100000,
          "Restored world did not rebind native addresses to its own allocations");
    fails([&]{restored.view(native_page,1);});
    check(restored.checkpoint()==saved,"Whole-bank checkpoint lost bytes/topology/access/owners");
    fails([&]{restored.view(0x102000,1);});
    fails([&]{restored.mutable_view(0x1008,1);});
    restored.commit(0x102000,page);m.commit(0x102000,page);
    restored.mutable_view(0x103010,1)[0]=std::byte{0x82};
    m.mutable_view(0x103010,1)[0]=std::byte{0x82};
    check(restored.checkpoint()==m.checkpoint(),"Restored reservation changed subsequent commits/writes");
    auto invalid=saved;invalid.regions.back().address=0x1001;
    const auto retained=restored.checkpoint();
    auto retained_view=restored.mutable_view(0x100000,1);
    fails([&]{restored.restore(invalid);});
    check(restored.checkpoint()==retained && retained_view.data()==restored.view(0x100000,1).data(),
        "Rejected checkpoint import mutated backing or invalidated views");
    invalid=saved;invalid.version=2;fails([&]{restored.restore(invalid);});
    invalid=saved;invalid.regions.back().access=static_cast<A>(99);fails([&]{restored.restore(invalid);});
    invalid=saved;
    auto reserved=std::find_if(invalid.regions.begin(),invalid.regions.end(),[](const auto&r){return r.page_bytes!=0;});
    reserved->committed[0]=2;fails([&]{restored.restore(invalid);});
    reserved->committed.pop_back();fails([&]{restored.restore(invalid);});
    invalid=saved;invalid.regions.front().bytes.clear();fails([&]{restored.restore(invalid);});
    invalid=saved;invalid.regions.back().bytes.push_back(std::byte{});fails([&]{restored.restore(invalid);});
    // Continue the original hole checks from the saved commitment topology.
    m.restore(saved);
    fails([&]{m.view(native_page,1);});
    const auto moved_address=unrelated.native_address(0x1000,4);
    SourceRuntimeMemory moved=std::move(unrelated);
    check(moved.native_address(0x1000,4)==moved_address &&
          moved.source_address(moved_address,4)==0x1000,
          "Moving a memory owner lost its native allocation index");
    fails([&] { m.view(0x101ffc, 0x1008); });
    fails([&] { m.commit(0x103800, page); });
    fails([&] { m.commit(0x104000, page); });
    fails([&] { m.commit(0x1000, page); });
    m.release(0x100000);
    fails([&] { m.view(0x103000, 1); });
    // Bind the production memory adapter to the actual source body-heap owners.
    // Backing/size are declared host allocation inputs; constructors perform
    // all initialization, and allocation/resize are never fabricated successes.
    std::vector<std::byte> heap(0x250, std::byte{0xa7});
    m.mount(0x2000, heap, A::ReadWrite, "object body heap");
    const std::array name{std::byte{'o'}, std::byte{'b'}, std::byte{'j'},
                          std::byte{}};
    m.mount(0x3000, name, A::ReadOnly, "heap name");
    const std::array<std::byte, 8> pointers{};
    m.mount(0x3100, pointers, A::ReadWrite, "pointer cells");
    halo2::SourceObjectBodyHeapServices s;
    s.read = [&](auto p, auto n) { return m.view(p, n); };
    s.memory = [&](auto p, auto n) { return m.mutable_view(p, n); };
    halo2::initialize_source_object_body_heap(0x2000, 0x200, 0x3000, 0, s);
    check(halo2::allocate_source_object_body(0x2000, 0x3100, 32, 0, 0, s),
          "Real body allocation");
    const auto body = word(m, 0x3100);
    check(m.view(body, 32)[0] == std::byte{0xa7},
          "The heap must preserve unconstructed payload");
    m.mutable_view(body, 32)[0] = std::byte{0x6b};
    check(halo2::allocate_source_object_body(0x2000, 0x3104, 32, 0, 0, s),
          "Adjacent body allocation");
    const std::array<std::byte, 4> resize{};
    m.mount(0x3200, resize, A::ReadWrite, "resize frame");
    check(
        halo2::resize_source_object_body(0x2000, 0x3100, 96, 0, 0, {0x3200}, s),
        "Source relocation");
    check(
        word(m, 0x3100) != body &&
            m.view(word(m, 0x3100), 96)[0] == std::byte{0x6b},
        "Source relocation retains payload and updates the real pointer cell");
    halo2::free_source_object_body(0x2000, 0x3100, s);
    // Resume actual heap owners against a separately imported bank. Matching
    // future allocation addresses/bytes detect missing allocator continuation.
    SourceRuntimeMemory heap_replica;heap_replica.restore(m.checkpoint());
    auto replica_services=s;
    replica_services.read=[&](auto p,auto n){return heap_replica.view(p,n);};
    replica_services.memory=[&](auto p,auto n){return heap_replica.mutable_view(p,n);};
    check(halo2::allocate_source_object_body(0x2000,0x3100,48,0,0,s) &&
        halo2::allocate_source_object_body(0x2000,0x3100,48,0,0,replica_services),
        "Heap continuation allocation failed");
    check(heap_replica.checkpoint()==m.checkpoint(),
        "Heap continuation changed allocator links, pointer cells or payload bytes");
    std::cout << "Native64-bit addresses, cross-world ownership, retained32-bit regions, aliases, permissions, lifetime and "
                 "source heap relocation and whole-bank checkpoint continuation pass\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
