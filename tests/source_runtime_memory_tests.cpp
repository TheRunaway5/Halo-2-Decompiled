#include "halo2/object_body_heap.hpp"
#include <algorithm>
#include "halo2/source_runtime_memory.hpp"
#include <array>
#include <iostream>
#include <stdexcept>

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
} // namespace
int main() {
  try {
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
