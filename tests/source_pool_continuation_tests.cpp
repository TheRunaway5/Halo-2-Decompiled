#include "halo2/native_object_lifetimes.hpp"
#include "halo2/raw_datum_pool.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <algorithm>
#include <array>
#include <iostream>

using namespace halo2;
namespace {
void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
template<class F>void rejects(F &&f){bool threw=false;try{f();}catch(const std::exception &){threw=true;}check(threw,"Invalid source continuation was accepted");}
constexpr std::uint32_t pool_address=0x10000000u,name_address=0x20000000u;
void put(SourceRuntimeMemory &memory,std::uint32_t address,std::uint32_t value,unsigned size=4) {
    std::array<std::byte,4> bytes{};for(unsigned i=0;i<size;++i)bytes[i]=std::byte(value>>(8*i));
    memory.write(address,std::span(bytes).first(size));
}
void initialize(SourceRuntimeMemory &memory) {
    constexpr auto capacity=0x800,stride=12;
    memory.mount(pool_address,std::vector<std::byte>(0x4c+capacity*stride+((capacity+31)/32)*4+16),
        SourceRuntimeMemory::Access::ReadWrite,"controlled raw pool constructor input");
    std::array<std::byte,32> name{};constexpr std::string_view text="object";
    for(unsigned i=0;i<text.size();++i)name[i]=std::byte(text[i]);
    memory.mount(name_address,name,SourceRuntimeMemory::Access::ReadOnly,"controlled pool name");
    SourceRawDatumPoolServices storage{[&](auto p,auto n){return memory.mutable_view(p,n);},
        [&](auto p,auto n){return memory.view(p,n);}};
    source_raw_datum_initialize_inplace(pool_address,name_address,capacity,stride,0,0,storage);
    source_raw_datum_make_valid(pool_address,storage);
}
// This focused fixture exercises the continuation seam with declared body
// inputs. Actual complete TU1.5 births are exercised by the physics test.
std::uint32_t birth(SourceRuntimeMemory &memory,NativeObjectLifetimes &lifetimes,std::uint32_t definition) {
    DatumPool pool(memory,pool_address);const auto handle=pool.allocate();check(handle!=DatumPool::none,"Pool full");
    const auto header=pool.lookup(handle);header[3]=0;header[6]=4;
    const auto body=0x40000000u+(handle&65535u)*0x100u;
    memory.mount(body,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"declared constructed body input");
    put(memory,body,definition);for(unsigned i=0;i<4;++i)header[8+i]=std::uint8_t(body>>(8*i));
    lifetimes.observe_source_birth(handle);return handle;
}
void retire(SourceRuntimeMemory &memory,NativeObjectLifetimes &lifetimes,std::uint32_t handle) {
    memory.release(0x40000000u+(handle&65535u)*0x100u);
    DatumPool(memory,pool_address).release_unchecked(handle);lifetimes.observe_source_retirement(handle);
}
void same(const DatumPool &a,const DatumPool &b) {
    check(a.capacity()==b.capacity() && a.stride()==b.stride() && a.cursor()==b.cursor() &&
        a.high_water()==b.high_water() && a.count()==b.count() && a.next_salt()==b.next_salt() &&
        std::ranges::equal(a.elements(),b.elements()) && std::ranges::equal(a.bitmap(),b.bitmap()),
        "Imported source allocator topology/bytes diverged");
}
}
int main() try {
    SourceRuntimeMemory source,target;initialize(source);initialize(target);
    DatumPool source_pool(source,pool_address),target_pool(target,pool_address);
    NativeObjectLifetimes source_lifetimes(source,pool_address),target_lifetimes(target,pool_address);
    NativeObjectLifetimes owned;
    rejects([&]{owned.source_authority_checkpoint();});
    rejects([&]{DatumPool copy(source_pool);}); // Ordinary copying still cannot clone a source view.
    const auto departed=birth(source,source_lifetimes,17);retire(source,source_lifetimes,departed);
    // Requested-slot and automatic births consume salts independently of rank.
    {DatumPool raw(source,pool_address);const auto sparse=raw.allocate_at(25),next=raw.allocate();
        raw.release_unchecked(next);raw.release_unchecked(sparse);}
    const auto current=birth(source,source_lifetimes,19);
    check((current&65535u)==(departed&65535u) && current!=departed,"Full-salt reuse fixture absent");
    const auto saved=source_lifetimes.source_authority_checkpoint();
    const auto saved_pool=source_pool.source_binding();
    SourceRuntimeMemory replacement;replacement.restore(source.checkpoint());
    const auto original=target.checkpoint();const auto old_address=target_pool.address();
    {
        auto cancelled=target_lifetimes.prepare_source_authority(saved,replacement);
        check(cancelled.for_owner(target_lifetimes) && !source_lifetimes.commit_source_authority(std::move(cancelled)),
            "Foreign source lifetime publication accepted");
        check(!target_lifetimes.commit_source_authority(std::move(cancelled)),"Unpublished replacement accepted");
    }
    check(target.checkpoint()==original && target_pool.address()==old_address,"Cancellation changed target backing/view");
    auto bad=saved;bad.version=2;rejects([&]{target_lifetimes.prepare_source_authority(bad,replacement);});
    bad=saved;bad.events.front().ordinal=2;rejects([&]{target_lifetimes.prepare_source_authority(bad,replacement);});
    bad=saved;bad.events.front().kind=static_cast<NativeObjectLifetimeEventKind>(99);
    rejects([&]{target_lifetimes.prepare_source_authority(bad,replacement);});
    bad=saved;bad.owners.at(current).definition=20;rejects([&]{target_lifetimes.prepare_source_authority(bad,replacement);});
    bad=saved;bad.events.back().owner.definition=20;bad.owners.at(current).definition=20;
    rejects([&]{target_lifetimes.prepare_source_authority(bad,replacement);}); // Consistent sidecars, wrong body.
    bad=saved;bad.owners.clear();rejects([&]{target_lifetimes.prepare_source_authority(bad,replacement);});
    auto damaged_pool=saved_pool;damaged_pool.name[0]=std::byte{'x'};
    rejects([&]{target_pool.prepare_source_view(damaged_pool,replacement);});
    damaged_pool=saved_pool;damaged_pool.version=2;rejects([&]{target_pool.prepare_source_view(damaged_pool,replacement);});
    rejects([&]{target_pool.prepare_source_view(saved_pool,target);});
    auto invalid=source.checkpoint();invalid.regions.front().access=SourceRuntimeMemory::Access::ReadOnly;
    SourceRuntimeMemory readonly;readonly.restore(invalid);
    rejects([&]{target_pool.prepare_source_view(saved_pool,readonly);});
    SourceRuntimeMemory missing;missing.restore(source.checkpoint());put(missing,pool_address+0x48,0xfffffff0u);
    rejects([&]{target_pool.prepare_source_view(saved_pool,missing);});
    missing.restore(source.checkpoint());
    const auto bitmap_bytes=missing.view(pool_address+0x48,4);std::uint32_t bitmap=0;
    for(unsigned i=0;i<4;++i)bitmap|=std::to_integer<std::uint32_t>(bitmap_bytes[i])<<(8*i);
    put(missing,bitmap,0);rejects([&]{target_pool.prepare_source_view(saved_pool,missing);});
    missing.restore(source.checkpoint());put(missing,pool_address+0x3c,0);
    rejects([&]{target_pool.prepare_source_view(saved_pool,missing);});
    put(missing,pool_address+0x48,0);put(missing,pool_address+0x34,UINT32_MAX);
    rejects([&]{target_pool.prepare_source_view(saved_pool,missing);});
    check(target.checkpoint()==original,"Rejected import changed current source bank");
    auto prepared=target_lifetimes.prepare_source_authority(saved,replacement);
    auto player_view=target_pool.prepare_source_view(saved_pool,replacement);
    check(!target_pool.source_view_ready(player_view),"Premature source view ready");
    SourceRuntimeMemory wrong;wrong.restore(source.checkpoint());target.swap_backing(wrong);
    check(!target_pool.commit_source_view(std::move(player_view)) &&
        !target_lifetimes.commit_source_authority(std::move(prepared)),"Equal independently restored backing accepted");
    target.swap_backing(wrong);target.swap_backing(replacement);
    rejects([&]{target_pool.capacity();}); // Old native views have actually expired.
    check(target_pool.source_view_ready(player_view) && target_pool.commit_source_view(std::move(player_view)) &&
        target_lifetimes.commit_source_authority(std::move(prepared)),"Exact replacement binding failed");
    check(!target_pool.commit_source_view(std::move(player_view)) &&
        !target_lifetimes.commit_source_authority(std::move(prepared)),"Consumed preparation accepted twice");
    check(target_lifetimes.source_authority_checkpoint()==saved && target_lifetimes.contains(current) &&
        !target_lifetimes.contains(departed),"Retained sidecars or full salts changed");
    same(source_pool,target_pool);
    retire(source,source_lifetimes,current);retire(target,target_lifetimes,current);
    const auto future_source=birth(source,source_lifetimes,29),future_target=birth(target,target_lifetimes,29);
    check(future_source==future_target && source_lifetimes.source_authority_checkpoint()==target_lifetimes.source_authority_checkpoint(),
        "Future allocation/retirement order diverged");
    same(source_pool,target_pool);check(source.checkpoint()==target.checkpoint(),"Future complete raw banks diverged");
    source_pool.lookup(future_source)[11]^=0x55;
    check(!std::ranges::equal(source_pool.lookup(future_source),target_pool.lookup(future_target)),"Independent source view aliases old host");
    std::cout<<"PASS staged source pool/lifetime continuation: exact backing, cancellation, malformed imports, full salts, allocation topology, retained ordered sidecars and subsequent allocation/retirement\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
