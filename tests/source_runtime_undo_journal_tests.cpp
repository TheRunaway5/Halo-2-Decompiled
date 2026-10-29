#include "halo2/source_runtime_undo_journal.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <new>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace {
thread_local bool measure_allocations{};
thread_local std::size_t allocations{};
thread_local int allocation_budget=-1;
}
void* operator new(std::size_t bytes) {
    if(allocation_budget==0)throw std::bad_alloc();
    if(allocation_budget>0)--allocation_budget;
    if(auto* result=std::malloc(bytes?bytes:1)) {
        if(measure_allocations)++allocations;
        return result;
    }
    throw std::bad_alloc();
}
void operator delete(void* address)noexcept {std::free(address);}
void operator delete(void* address,std::size_t)noexcept {std::free(address);}

namespace {
using namespace halo2;
using Storage=SourceRuntimeUndoJournal::Storage;
using Bytes=std::vector<std::byte>;
struct Event {
    std::uint32_t address;Bytes before;
    bool operator==(const Event&)const=default;
};
void need(bool value,const char* message) {
    if(!value)throw std::runtime_error(message);
}
void reject_topology(std::uint32_t,std::size_t) {throw std::runtime_error("original topology rejection");}
// Frozen pre-change journal. Compare every observed rollback as well as the
// final bytes, so overlapping stores cannot pass by restoring only a final
// checkpoint or by coalescing away observable intermediate restore values.
struct OriginalJournal {
    SourceRuntimeMemory& bank;
    std::vector<std::pair<std::uint32_t,Bytes>> undo;
    explicit OriginalJournal(SourceRuntimeMemory& value):bank(value) {
        source_runtime_register_topology_guard(bank,this,reject_topology);
        try {
            source_runtime_register_write_observer(bank,this,[this](auto at,auto size) {
                const auto original=bank.view(at,size);
                undo.emplace_back(at,Bytes(original.begin(),original.end()));
            });
        } catch(...) {source_runtime_unregister_topology_guard(bank,this);throw;}
    }
    ~OriginalJournal() {
        source_runtime_unregister_write_observer(bank,this);
        try {for(auto at=undo.rbegin();at!=undo.rend();++at)bank.write(at->first,at->second);}
        catch(...) {std::terminate();}
        source_runtime_unregister_topology_guard(bank,this);
    }
};
void mount(SourceRuntimeMemory& bank) {
    Bytes initial(0x4000);
    for(unsigned i=0;i<initial.size();++i)initial[i]=std::byte((i*37u+11u)&255u);
    bank.mount(0x10000u,initial,SourceRuntimeMemory::Access::ReadWrite,"complete undo data owner");
    bank.mount(0x30000u,std::span(initial).first(4),SourceRuntimeMemory::Access::ReadWrite,"first adjacent scalar");
    bank.mount(0x30004u,std::span(initial).subspan(4,4),SourceRuntimeMemory::Access::ReadWrite,"second adjacent scalar");
}
template<class Journal>void with_journal(SourceRuntimeMemory& bank,Storage storage,auto action) {
    if constexpr(std::is_same_v<Journal,OriginalJournal>) {OriginalJournal journal(bank);action();}
    else {SourceRuntimeUndoJournal journal(bank,storage,reject_topology);action();}
}
template<class Journal>std::vector<Event> exercise(Storage storage,bool exception) {
    SourceRuntimeMemory bank;mount(bank);const auto initial=bank.checkpoint();
    std::vector<Event> events;
    source_runtime_register_write_observer(bank,&events,[&](auto at,auto size) {
        const auto before=bank.view(at,size);events.push_back({at,{before.begin(),before.end()}});
    });
    unsigned seed=0x137a249u;
    const auto next=[&]{seed=seed*1664525u+1013904223u;return seed;};
    bool caught{};
    try {with_journal<Journal>(bank,storage,[&] {
        for(unsigned i=0;i<500;++i) {
            const auto size=std::array{1u,2u,4u,8u,16u,64u,512u}[i%7];
            const auto at=0x10000u+next()%(0x4000u-size);
            Bytes values(size);for(auto& byte:values)byte=std::byte(next()>>24);
            if(i%5==0) {
                const auto input=bank.view(at-std::min(7u,at-0x10000u),size);
                const Bytes expected(input.begin(),input.end());
                bank.write(at,input);
                need(std::ranges::equal(bank.view(at,size),expected),"Alias write changed its actual source input");
            } else if(i%3==0) {
                auto output=bank.mutable_view(at,size);std::ranges::copy(values,output.begin());
                need(std::ranges::equal(bank.view(at,size),values),"Mutable borrow did not retain current writes");
            } else {
                bank.write(at,values);
                need(std::ranges::equal(bank.view(at,size),values),"Undo capture changed a current write");
            }
        }
        const auto outer=bank.checkpoint();
        with_journal<Journal>(bank,storage,[&] {
            bank.write(0x10000u,std::array{std::byte{0x81},std::byte{0x82}});
            bank.write(0x10001u,std::array{std::byte{0x83},std::byte{0x84}});
        });
        need(bank.checkpoint()==outer,"Nested rollback did not restore its current outer inputs");
        for(auto action:{0u,1u,2u}) {
            bool rejected{};
            try {
                if(action==0)bank.release(0x10000u);
                else if(action==1)bank.mount(0x50000u,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"forbidden owner");
                else bank.write(0x30000u,std::array<std::byte,8>{}); // Original view cannot span separate owners.
            }catch(const std::exception&){rejected=true;}
            need(rejected,"Journal admitted topology or a cross-owner captured view");
        }
        if(exception)throw std::runtime_error("declared translated-owner stop");
    });}catch(const std::runtime_error& error) {
        need(exception && std::string_view(error.what())=="declared translated-owner stop","Unexpected journal failure");caught=true;
    }
    need(caught==exception && bank.checkpoint()==initial,"Journal failed complete successful/exception rollback");
    // This legitimate write is observable after retirement and is not undone.
    bank.write(0x10000u,std::array{std::byte{0x5a}});
    need(bank.view(0x10000u,1)[0]==std::byte{0x5a},"Journal callback survived its owner lifetime");
    source_runtime_unregister_write_observer(bank,&events);
    return events;
}
void rejected_write() {
    for(auto storage:{Storage::packed,Storage::store_vectors}) {
        SourceRuntimeMemory bank;mount(bank);const auto before=bank.checkpoint();
        bool deny=true;
        source_runtime_register_write_guard(bank,&deny,[&](auto,auto) {
            if(deny)throw std::runtime_error("declared write guard stop");
        });
        {
            SourceRuntimeUndoJournal journal(bank,storage,reject_topology);
            bool rejected{};try{bank.write(0x10000u,std::array<std::byte,4>{});}
            catch(const std::runtime_error&){rejected=true;}
            need(rejected && bank.checkpoint()==before,"Denied write partially captured or changed memory");
            deny=false;bank.write(0x10000u,std::array<std::byte,4>{});
        }
        source_runtime_unregister_write_guard(bank,&deny);
        need(bank.checkpoint()==before,"Later accepted write lost its initial rollback bytes");
    }
}
template<class Journal>void rejected_capture(Storage storage) {
    for(int budget:{0,1}) {
        SourceRuntimeMemory bank;mount(bank);const auto before=bank.checkpoint();
        with_journal<Journal>(bank,storage,[&] {
            bank.write(0x10000u,std::array{std::byte{0x41},std::byte{0x42},std::byte{0x43},std::byte{0x44}});
            const auto prefix=bank.checkpoint();bool rejected{};
            struct Budget {
                explicit Budget(int count){allocation_budget=count;}
                ~Budget(){allocation_budget=-1;}
            };
            try {
                Budget limited(budget);
                bank.write(0x10001u,std::array<std::byte,4>{});
            }catch(const std::bad_alloc&){rejected=true;}
            need(rejected && bank.checkpoint()==prefix,"Failed capture allocated or changed the source write prefix");
            // Continue in the same scope after a failed byte/record growth.
            bank.write(0x10002u,std::array{std::byte{0x51},std::byte{0x52},std::byte{0x53},std::byte{0x54}});
        });
        need(bank.checkpoint()==before,"Failed capture damaged an earlier or subsequent undo record");
    }
}
template<class Journal>std::pair<std::size_t,double> benchmark(Storage storage) {
    SourceRuntimeMemory bank;mount(bank);const auto before=bank.checkpoint();
    const std::array<std::byte,4> value{std::byte{1},std::byte{2},std::byte{3},std::byte{4}};
    const auto run=[&] {with_journal<Journal>(bank,storage,[&] {
        for(unsigned i=0;i<4096;++i)bank.write(0x10000u+(i%1024u)*4u,value);
    });};
    run();allocations=0;const auto begin=std::clock();measure_allocations=true;
    for(unsigned i=0;i<24;++i)run();
    measure_allocations=false;const auto elapsed=double(std::clock()-begin)*1000./CLOCKS_PER_SEC/24;
    need(bank.checkpoint()==before,"Measured undo route failed exact source restoration");
    return {allocations/24,elapsed};
}
}
int main()try {
    for(bool exception:{false,true}) {
        const auto reference=exercise<OriginalJournal>(Storage::store_vectors,exception);
        for(auto storage:{Storage::packed,Storage::store_vectors})
            need(exercise<SourceRuntimeUndoJournal>(storage,exception)==reference,
                "Packed journal changed an original observed store/restore byte prefix");
    }
    rejected_write();
    rejected_capture<OriginalJournal>(Storage::store_vectors);
    rejected_capture<SourceRuntimeUndoJournal>(Storage::packed);
    rejected_capture<SourceRuntimeUndoJournal>(Storage::store_vectors);
    const auto reference=benchmark<OriginalJournal>(Storage::store_vectors);
    const auto packed=benchmark<SourceRuntimeUndoJournal>(Storage::packed);
    need(packed.first*8<reference.first,"Packed journal retained per-store heap allocation");
    std::cout<<"PASS exact original undo ordering/bytes, overlaps, alias reads, mutable borrows, nested scopes, "
        "topology/guard rejection and exception rollback\n";
    std::cout<<"4096-store component allocation/CPU control: original_allocations="<<reference.first
        <<" packed_allocations="<<packed.first<<" original_cpu_ms="<<reference.second
        <<" packed_cpu_ms="<<packed.second<<"; runtime performance unqualified\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
