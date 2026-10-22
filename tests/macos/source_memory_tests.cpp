#include "halo2/source_runtime_host_storage.hpp"
#include "halo2/source_runtime_host_virtual_memory.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include "halo2/physics_pool_host_storage.hpp"
#include "runtime/source_runtime_host_pages.hpp"
#include "native_protection.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
#include <unistd.h>

using namespace halo2;
namespace {
void check(bool value,const char *message) { if(!value)throw std::runtime_error(message); }
template<class F> void rejects(F &&f) {
    bool failed=false;
    try {f();}catch(const std::exception &){failed=true;}
    check(failed,"Inaccessible source bytes or malformed ownership was accepted");
}
void put(SourceRuntimeMemory &bank,std::uint32_t address,std::byte value) {
    bank.write(address,std::array{value});
}
using halo2::macos_test::native_writable;
void native_granule_rollback() {
    namespace pages=source_runtime_detail::host_pages;
    struct Mapping {
        void *base=pages::reserve(0x5000);
        ~Mapping(){pages::release(base,0x5000);}
    } mapping;
    check(!native_writable(mapping.base,0) && !native_writable(mapping.base,0x4000),
          "Reserved native pages are writable before commitment");
    pages::make_writable(mapping.base,0,0x1000);
    static_cast<std::byte*>(mapping.base)[0]=std::byte{0x4a};
    std::vector<bool> committed(5,false);committed[0]=true;
    pages::make_writable(mapping.base,0x1000,0x1000);
    pages::rollback_commitment(mapping.base,0x1000,0x1000,committed);
    check(native_writable(mapping.base,0),"Rollback revoked a retained neighboring native commitment");
    check(native_writable(mapping.base,0x1000)==(::sysconf(_SC_PAGESIZE)>0x1000),
          "Rollback did not retain exactly the necessary host granule");
    check(static_cast<std::byte*>(mapping.base)[0]==std::byte{0x4a},"Rollback changed retained native page bytes");
    pages::make_writable(mapping.base,0x3000,0x2000);
    pages::rollback_commitment(mapping.base,0x3000,0x2000,committed);
    check(native_writable(mapping.base,0) && !native_writable(mapping.base,0x4000),
          "Rollback retained an unrelated native granule or revoked the old one");
}
void small_reservations_and_holes() {
    SourceRuntimeMemory bank;
    SourceRuntimeHostVirtualMemory vm(bank,0x20001000);
    const auto small=vm.allocate({0,0,0x1000,0x2000,4});
    check(small.base==0x20001000 && small.bytes==0x1000,"Host rounding widened source reservation");
    vm.allocate({small.base,0,0x1000,0x1000,4});
    put(bank,small.base+0xfff,std::byte{0xa7});
    rejects([&]{bank.view(small.base+0x1000,1);});
    vm.release(small.base,small.bytes,0x8000);
    const auto region=vm.allocate({0,0,0x5000,0x2000,4});
    check(region.base==small.base+0x1000 && region.bytes==0x5000,"Source token cursor changed to native granularity");
    vm.allocate({region.base+0x1000,0,0x1000,0x1000,4});
    put(bank,region.base+0x1010,std::byte{0x63});
    vm.allocate({region.base+0x3000,0,0x1000,0x1000,4});
    put(bank,region.base+0x3010,std::byte{0x95});
    rejects([&]{bank.view(region.base,1);});
    rejects([&]{bank.view(region.base+0x2000,1);});
    rejects([&]{bank.native_address(region.base+0x2000,1);});
    const auto native_base=bank.native_address(region.base,0);
    rejects([&]{bank.view(NativeRuntimeAddress{native_base.value+0x2000},1);});
    vm.allocate({region.base+0x4000,0,0x1000,0x1000,4});
    check(bank.view(region.base+0x4000,0x1000).back()==std::byte{},"Tail source page is not zero-filled");
    vm.allocate({region.base,0,0x5000,0x1000,4});
    check(bank.view(region.base+0x1010,1)[0]==std::byte{0x63} &&
          bank.view(region.base+0x3010,1)[0]==std::byte{0x95},"Recommit lost existing source writes");
    check(vm.ownership_checkpoint().reservations[0].committed==std::vector<std::uint8_t>(5,1),
          "Source commitment bits changed to native page granularity");
}
void rejected_commitment() {
    SourceRuntimeMemory bank;
    SourceRuntimeHostVirtualMemory vm(bank);
    const auto region=vm.allocate({0,0,0x5000,0x2000,4});
    vm.allocate({region.base,0,0x1000,0x1000,4});
    put(bank,region.base,std::byte{0xc7});
    const auto before=bank.checkpoint();
    const auto owner=vm.ownership_checkpoint();
    bool denied=false;
    source_runtime_register_topology_guard(bank,&denied,[&](auto address,auto) {
        if(address==region.base+0x1000){denied=true;throw std::runtime_error("retained source borrower");}
    });
    rejects([&]{vm.allocate({region.base+0x1000,0,0x1000,0x1000,4});});
    source_runtime_unregister_topology_guard(bank,&denied);
    check(denied && bank.checkpoint()==before && vm.ownership_checkpoint()==owner,
          "Rejected source commitment changed bank or resource ownership");
    rejects([&]{bank.view(region.base+0x1000,1);});
    check(bank.view(region.base,1)[0]==std::byte{0xc7},"Rejected commitment damaged neighboring source bytes");
    vm.allocate({region.base+0x2000,0,0x1000,0x1000,4});
    check(bank.view(region.base+0x2000,1)[0]==std::byte{},"Commit after rejection cannot use the retained native granule");
}
void virtual_continuation() {
    SourceRuntimeMemory source,target,replacement;
    SourceRuntimeHostVirtualMemory original(source,0x30001000),resumed(target,0x60001000);
    const auto region=original.allocate({0,0,0x5000,0x2000,4});
    original.allocate({region.base+0x1000,0,0x1000,0x1000,4});
    original.allocate({region.base+0x4000,0,0x1000,0x1000,4});
    put(source,region.base+0x1010,std::byte{0x49});
    put(source,region.base+0x4010,std::byte{0x87});
    const auto saved=source.checkpoint();
    const auto ownership=original.ownership_checkpoint();
    replacement.restore(saved);
    auto invalid=ownership;invalid.reservations[0].committed[0]=2;
    rejects([&]{resumed.prepare_ownership(invalid,saved,replacement);});
    auto staged=resumed.prepare_ownership(ownership,saved,replacement);
    check(!resumed.commit_ownership(std::move(staged)),"VM ownership published before its exact byte bank");
    target.swap_backing(replacement);
    check(resumed.commit_ownership(std::move(staged)),"VM ownership did not follow byte bank publication");
    check(resumed.ownership_checkpoint()==ownership && target.checkpoint()==saved,"VM continuation changed source ownership or bytes");
    rejects([&]{target.view(region.base+0x2000,1);});
    resumed.allocate({region.base+0x2000,0,0x1000,0x1000,4});
    check(target.view(region.base+0x2000,1)[0]==std::byte{} &&
          target.view(region.base+0x1010,1)[0]==std::byte{0x49} &&
          target.view(region.base+0x4010,1)[0]==std::byte{0x87},"Commit after continuation changed restored source bytes");
}
void raw_source_storage() {
    SourceRuntimeMemory bank;
    SourceRuntimeHostStorage raw(bank,0x40001000);
    const auto first=raw.allocate({0,0,0x1001,0x101000,4});
    check(first.base==0x40001000 && first.bytes==0x2000,"Raw allocation exposes native padding or alignment to source");
    check(bank.view(first.base,first.bytes).back()==std::byte{},"Raw source allocation is not zero-filled");
    put(bank,first.base+0x1fff,std::byte{0xb4});
    const auto second=raw.allocate({0,0,1,0x1000,4});
    check(second.base==first.base+0x2000 && second.bytes==0x1000,"Raw source cursor changed to16KiB units");
    rejects([&]{bank.view(second.base+second.bytes,1);});
    check(bank.view(first.base+0x1fff,1)[0]==std::byte{0xb4},"Next raw mapping replaced previous source bytes");
    const auto cursor=raw.cursor_checkpoint();
    rejects([&]{raw.allocate({first.base,0,0x1000,0x1000,4});});
    check(raw.cursor_checkpoint()==cursor,"Rejected raw allocation advanced source cursor");
}
void pool_storage_continuation() {
    const auto write_word=[](SourceRuntimeMemory &bank,std::uint32_t p,std::uint32_t value) {
        std::array<std::byte,4> bytes;
        for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(i*8));
        bank.write(p,bytes);
    };
    const auto read_word=[](SourceRuntimeMemory &bank,std::uint32_t p) {
        std::uint32_t result{};const auto bytes=bank.view(p,4);
        for(unsigned i=0;i<4;++i)result|=std::to_integer<std::uint32_t>(bytes[i])<<(i*8);
        return result;
    };
    const auto allocate=[&](SourceRuntimeMemory &bank,SourcePhysicsPoolHostStorage &pool,std::uint32_t bytes) {
        const std::array<std::uint32_t,6> frame{0x33b3c5,0x4100,0,0x4104,0x101000,4};
        for(unsigned i=0;i<frame.size();++i)write_word(bank,0x4000+i*4,frame[i]);
        write_word(bank,0x4100,0);write_word(bank,0x4104,bytes);
        SourcePhysicsWorldRuntimeAbi abi{};abi.gp[4]=0x4000;
        SourcePhysicsContactRuntimeCall call{};call.instruction=0x33786f;call.target=0x800000b8;
        check(pool.try_call(bank,call,abi) && abi.gp[0]==0 && abi.gp[4]==0x4018,
              "Pool allocation did not retain its source call/stack ABI");
        check(read_word(bank,0x4104)==0x2000,"Pool source byte output widened to host granularity");
        return read_word(bank,0x4100);
    };
    SourceRuntimeMemory bank,target;
    bank.mount(0x4000,std::vector<std::byte>(0x200),SourceRuntimeMemory::Access::ReadWrite,"pool caller");
    SourcePhysicsPoolHostStorage pool(bank,0x50001000),resumed(target,0x70001000);
    const auto first=allocate(bank,pool,0x1800);
    check(first==0x50001000,"Pool token alignment differs from source pages");
    put(bank,first+0x1010,std::byte{0xd5});
    const auto saved=bank.checkpoint();const auto owner=pool.ownership_checkpoint();
    auto prepared=resumed.prepare_ownership(owner,saved);
    target.restore(saved);
    check(resumed.commit_ownership(std::move(prepared)),"Pool ownership import failed");
    check(target.view(first+0x1010,1)[0]==std::byte{0xd5},"Pool ownership import changed live bytes");
    const auto next=allocate(bank,pool,0x1800),continued=allocate(target,resumed,0x1800);
    check(next==owner.cursor.next && continued==next &&
          pool.ownership_checkpoint().allocations==resumed.ownership_checkpoint().allocations,
          "Restored pool cursor/registry changed future source allocation");
}
}
int main() {
    try {
        native_granule_rollback();small_reservations_and_holes();rejected_commitment();
        virtual_continuation();raw_source_storage();pool_storage_continuation();
        std::cout<<"PASS source4KiB reservation/holes/recommit/rejection/continuation/raw/pool storage and native-granule rollback; host_page_bytes="
                 <<::sysconf(_SC_PAGESIZE)<<'\n';
    } catch(const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}
}
