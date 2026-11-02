// Exercise the shared-lock dispatch branch deterministically, without a
// scheduling race with a real publisher. This TU owns the actual registry.
#include "../src/runtime/source_runtime_memory.cpp"
#include <iostream>

int main()try {
    using namespace halo2;
    SourceRuntimeMemory bank;
    bank.mount(0x10000,std::vector<std::byte>(4096),SourceRuntimeMemory::Access::ReadWrite,"recursive admission");
    const std::array<SourceRuntimeNativeWriteRange,1> ranges{{{0x10100,4}}};
    const std::array<std::byte,4> word{};
    int owner;unsigned calls{};
    const std::array<const void*,1> allowed{{&owner}};
    auto callback=[&](auto,auto) {
        ++calls;
        SourceRuntimeNativeWriteLease lease(bank,ranges,allowed);
        if(lease||lease.refusal()!=SourceRuntimeNativeWriteRefusal::Recursion)
            throw std::runtime_error("Shared-lock callback admitted or misattributed a native lease");
        if(!dispatch_slot(native_write_guards()).depth)
            throw std::runtime_error("Shared-lock callback lost dispatch depth");
    };
    // The writer flag selects the exact slow branch that a quiescing foreign
    // publisher selects. No policy is changed while the callback is running.
    auto dispatch=[&] {
        auto &registry=native_write_guards();
        registry.writer.store(true,std::memory_order_seq_cst);
        try{bank.write(0x10100,word);}
        catch(...){registry.writer.store(false,std::memory_order_seq_cst);throw;}
        registry.writer.store(false,std::memory_order_seq_cst);
        const auto &slot=dispatch_slot(registry);
        if(slot.depth||slot.active.load())throw std::runtime_error("Slow dispatch leaked its slot");
    };
    source_runtime_register_write_guard(bank,&owner,callback);dispatch();
    source_runtime_unregister_write_guard(bank,&owner);
    source_runtime_register_write_observer(bank,&owner,callback);dispatch();
    source_runtime_unregister_write_observer(bank,&owner);
    if(calls!=2)throw std::runtime_error("Shared-lock guard/observer branches were not both exercised");
    SourceRuntimeNativeWriteLease after(bank,ranges);
    if(!after)throw std::runtime_error("Slow dispatch prevented later ordinary admission");
    std::cout<<"PASS shared-lock guard and observer recursive refusal, slot cleanup, subsequent admission\n";
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
