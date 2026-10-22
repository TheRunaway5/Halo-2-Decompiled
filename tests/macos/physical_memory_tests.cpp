#include "halo2/source_runtime_host_physical_memory.hpp"
#include "halo2/source_runtime_write_protection.hpp"
#include "native_protection.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>
#include <unistd.h>

using namespace halo2;
using halo2::macos_test::native_writable;
namespace {
void check(bool value,const char *message){if(!value)throw std::runtime_error(message);}
template<class F>void rejects(F &&f){bool failed=false;try{f();}catch(const std::exception &){failed=true;}
    check(failed,"Protected or malformed physical request was accepted");}
void write(SourceRuntimeMemory &bank,std::uint32_t p,std::byte value) {bank.write(p,std::array{value});}
void mixed_and_uniform_protection() {
    SourceRuntimeMemory bank;SourceRuntimeHostPhysicalMemory physical(bank);
    const auto base=physical.allocate({0x3000,0x61000,0x63fff,4096,4});
    check(base==0x80061000,"Source physical token range changed to host granularity");
    const auto native=bank.native_address(base,0x3000);
    auto *host=reinterpret_cast<void*>(native.value);
    check(native.value % ::sysconf(_SC_PAGESIZE)==0,"Physical backing is not a real native page mapping");
    write(bank,base+0xfff,std::byte{0xa1});write(bank,base+0x1fff,std::byte{0xb2});
    physical.protect(base+0xfff,2,2);
    check(physical.ownership_checkpoint().allocations[0].read_only==std::vector<std::uint8_t>({1,1,0}),
          "Unaligned protection request lost source page intersection semantics");
    std::size_t observed{};int owner;
    source_runtime_register_write_observer(bank,&owner,[&](auto,auto){++observed;});
    const std::array value{std::byte{0xc3}};
    rejects([&]{bank.write(base,value);});rejects([&]{bank.write(native,value);});
    rejects([&]{bank.mutable_view(base+0x1000,1);});
    rejects([&]{bank.mutable_view(NativeRuntimeAddress{native.value+0x1000},1);});
    rejects([&]{bank.validate_mutable_view(base+0x1000,1);});
    check(!observed,"Rejected physical writes reached mutation observers");
    write(bank,base+0x2000,std::byte{0xd4});
    check(bank.view(base+0x2000,1)[0]==std::byte{0xd4},"Read-only neighbor blocked a writable source page");
    const bool mixed_native=::sysconf(_SC_PAGESIZE)>0x1000;
    check(native_writable(host,0)==mixed_native && native_writable(host,0x2000),
          "Mixed source permissions changed the native granule policy");
    physical.protect(base,0x3000,2);
    check(!native_writable(host,0) && !native_writable(host,0x2000),
          "Uniform read-only source allocation is not protected in native memory");
    physical.protect(base+0x2000,0x1000,4);
    check(native_writable(host,0x2000),"Unprotecting source tail did not restore writable native storage");
    rejects([&]{bank.write(base,value);});
    physical.protect(base,0x1000,4);
    const auto previous=bank.view(base+0xfff,1)[0];
    const std::array crossing{std::byte{0x12},std::byte{0x34}};
    rejects([&]{bank.write(base+0xfff,crossing);});
    check(bank.view(base+0xfff,1)[0]==previous,"Rejected write changed its writable source prefix");
    check(bank.native_address(base,0x3000)==native,"Protection changed actual contiguous backing identity");
    source_runtime_unregister_write_observer(bank,&owner);
    physical.release(base);
    rejects([&]{bank.source_address(native,1);});
}
void multiple_native_granules_and_continuation() {
    SourceRuntimeMemory bank,replacement,target;
    SourceRuntimeHostPhysicalMemory physical(bank),resumed(target);
    const auto base=physical.allocate({0x5000,0x71000,0x75fff,4096,4});
    write(bank,base,std::byte{0x51});write(bank,base+0x4fff,std::byte{0x79});
    physical.protect(base,0x4000,2);
    const auto native=bank.native_address(base,0x5000);auto *host=reinterpret_cast<void*>(native.value);
    check(!native_writable(host,0) && native_writable(host,0x4000),"Protection leaked between native granules");
    physical.protect(base+0x4000,0x1000,2);
    check(!native_writable(host,0x4000),"Private native tail padding defeated full source protection");
    const auto saved=bank.checkpoint();
    const auto owner=physical.ownership_checkpoint();
    replacement.restore(saved);
    auto bad=owner;bad.allocations[0].read_only.back()=2;
    rejects([&]{resumed.prepare_ownership(bad,saved,replacement);});
    const auto before=target.checkpoint();
    {
        auto canceled=resumed.prepare_ownership(owner,saved,replacement);
        rejects([&]{replacement.mutable_view(base+0x4000,1);});
        check(target.checkpoint()==before,"Canceled physical preparation mutated live bank");
    }
    // A second preparation must reset the first staged mapping's native
    // protections before applying the incoming4KiB rows again.
    auto prepared=resumed.prepare_ownership(owner,saved,replacement);
    const auto staged_native=replacement.native_address(base,0x5000);
    check(!native_writable(reinterpret_cast<void*>(staged_native.value),0),"Staged native physical protection missing");
    check(!resumed.commit_ownership(std::move(prepared)),"Physical owner published before its exact byte bank");
    target.swap_backing(replacement);
    check(resumed.commit_ownership(std::move(prepared)),"Physical ownership publication failed");
    check(resumed.ownership_checkpoint()==owner && target.checkpoint()==saved,"Restored physical source state differs");
    rejects([&]{target.write(target.native_address(base+0x1000,1),std::array{std::byte{0x88}});});
    resumed.protect(base+0x1000,0x1000,4);
    write(target,base+0x1000,std::byte{0x91});
    rejects([&]{write(target,base,std::byte{0xff});});
    check(target.view(base,1)[0]==std::byte{0x51} && target.view(base+0x4fff,1)[0]==std::byte{0x79},
          "Source unprotection after continuation changed read-only neighbors");
    resumed.release(base);
}
void original_size_backing() {
    SourceRuntimeMemory bank;SourceRuntimeHostPhysicalMemory physical(bank);
    constexpr std::uint32_t bytes=0x30e4000;
    const auto base=physical.allocate({bytes,0x61000,0x61000+bytes-1,4096,4});
    check(base==0x80061000,"Original-size physical request changed its exact source token");
    const auto storage=bank.view(base,bytes);
    check(std::ranges::all_of(storage,[](auto byte){return byte==std::byte{};}),"Fresh original-size backing is not zero-filled");
    const auto native=bank.native_address(base,bytes);
    check(bank.source_address(NativeRuntimeAddress{native.value+bytes-1},1)==base+bytes-1,
          "Original-size native backing lost source/native contiguity");
    write(bank,base+bytes-1,std::byte{0x67});
    check(bank.view(base+bytes-1,1)[0]==std::byte{0x67},"Original-size physical backing last byte is inaccessible");
}
}
int main()try{
    mixed_and_uniform_protection();multiple_native_granules_and_continuation();original_size_backing();
    std::cout<<"PASS direct physical backing, source4KiB mixed protection/native aliases, native uniform protection, continuation and original-size allocation; host_page_bytes="
             <<::sysconf(_SC_PAGESIZE)<<'\n';return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
