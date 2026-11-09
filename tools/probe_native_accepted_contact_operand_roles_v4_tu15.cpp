#include "halo2/native_tu15_accepted_contact_role_memory.hpp"
#include <iostream>
namespace {
using namespace halo2;
using Role=NativeTu15AcceptedPrivateRole;
constexpr std::uint32_t p=0x600e0000u,event=p+0x5030u;
void check(bool passed,const char*reason){if(!passed)throw std::runtime_error(reason);}
struct Backing final:SourcePhysicsContactMemory {
    mutable unsigned reads{};unsigned writes{};
    void read(std::uint32_t,std::span<std::byte>out)const override{++reads;std::fill(out.begin(),out.end(),std::byte{0x69});}
    void write(std::uint32_t,std::span<const std::byte>)override{++writes;}
};
// Delegates the real private-definition/extent semantics. Counters distinguish
// metadata queries from actual DATA reads, including reads inside the sidecar.
struct CountedView final:SourcePhysicsContactMemory,NativeTu15RetainedStoreObservationDomain,NativeTu15RetainedStoreObserverProvider {
    NativeTu15ContactCandidatePrivateData&memory;mutable unsigned reads{},metadata{};unsigned writes{};
    SourcePhysicsContactRecordObserver observer=[](const auto&){};
    explicit CountedView(NativeTu15ContactCandidatePrivateData&m):memory(m){}
    void read(std::uint32_t p,std::span<std::byte>out)const override{++reads;memory.read(p,out);}
    void read_vector(std::uint32_t p,std::span<std::byte,16>out)const override{++reads;memory.read_vector(p,out);}
    void write(std::uint32_t p,std::span<const std::byte>in)override{++writes;memory.write(p,in);}
    bool observes_retained_store(std::uint32_t pc,std::uint32_t p,std::size_t n)const override{++metadata;return memory.observes_retained_store(pc,p,n);}
    void require_retained_extent(std::uint32_t p,std::size_t n)const override{++metadata;memory.require_retained_extent(p,n);}
    bool data_is_defined(std::uint32_t p,std::size_t n)const override{return memory.data_is_defined(p,n);}
    bool withhold_defined_data(std::uint32_t p,std::size_t n)override{return memory.withhold_defined_data(p,n);}
    bool copy_defined_data(std::uint32_t pc,std::uint32_t dst,std::uint32_t src,std::size_t n,const SourcePhysicsContactRecordObserver&o)override{return memory.copy_defined_data(pc,dst,src,n,o);}
    const SourcePhysicsContactRecordObserver&retained_store_observer()const noexcept override{return observer;}
};
}
int main()try{
    using namespace halo2;
    const std::array<NativeTu15ContactPrivateRange,15>fields{{
        {p+0x6100u,4u},{p+0x6104u,4u},{p+0x6108u,4u},{p+0x610cu,4u},
        {p+0x6110u,2u},{p+0x6112u,2u},{p+0x6114u,4u},{p+0x6118u,4u},
        {p+0x611cu,4u},{p+0x6120u,4u},{p+0x6124u,4u},{p+0x6128u,4u},
        {p+0x6130u,64u},{p+0x6170u,16u},{p+0x6180u,16u}}};
    const std::array<NativeTu15ContactPrivateRange,1>owned{{{p,0x8000u}}};Backing backing;
    NativeTu15ContactCandidatePrivateData data(backing,owned,{});CountedView current(data);
    std::array<std::byte,64>payload;payload.fill(std::byte{0xa5});
    for(auto field:fields)data.write(field.first,std::span(payload).first(field.bytes));
    data.write(event,std::span(payload).first(2));data.write(event+4u,std::span(payload).first(32));
    data.write(p+0x7000u,std::span(payload).first(16)); // Defined, different owner.
    std::uint64_t refusals{};accepted_contact_detail::RoleMemory view(current,current,fields,event,refusals);
    unsigned controls{};const auto deny=[&](auto action,const char*reason){const auto reads=current.reads;const auto writes=current.writes;const auto prior=refusals;bool stopped{};try{action();}catch(const std::runtime_error&){stopped=true;}
        check(stopped&&refusals==prior+1u&&current.reads==reads&&current.writes==writes,reason);++controls;};
    for(unsigned i=0;i<fields.size();++i){const auto&field=fields[i];
        deny([&]{std::array<std::byte,1>b;view.read(field.first,b);},"Defined private alias accepted as a retained read");
        deny([&]{(void)view.observes_retained_store(0x1d76b6u,field.first,1u);},"Retained row writer aliased private DATA before preimage check");
        std::array<std::byte,64>b{};view.read_accepted_private(static_cast<Role>(i),field.first,std::span(b).first(field.bytes));
        check(std::equal(b.begin(),b.begin()+field.bytes,payload.begin()),"Explicit typed private role was lost");++controls;
    }
    deny([&]{std::array<std::byte,16>b;view.read_vector(p+0x7000u,b);},"Another initialized private owner was admitted as retained");
    deny([&]{view.require_retained_extent(p-4u,8u);},"Crossing retained extent was admitted");
    deny([&]{view.require_retained_extent(0xfffffffcu,8u);},"Wrapping retained extent was admitted");
    deny([&]{std::array<std::byte,4>b;view.read_accepted_private(Role::event,event,b);},"Event padding was read");
    deny([&]{std::array<std::byte,4>b;view.read_accepted_private(Role::request,fields[0].first,b);},"Request role accepted forged scalar address");
    deny([&]{std::array<std::byte,4>b;view.read_accepted_private(Role::first_weight,fields[1].first,b);},"Typed scalar role accepted another scalar");
    deny([&]{(void)view.observes_retained_store(0x281787u,fields[1].first,4u);},"Weight writer accepted another scalar");
    deny([&]{(void)view.observes_retained_store(0x20d8b0u,fields[12].first,4u);},"Request writer accepted wrong width");
    deny([&]{(void)view.observes_retained_store(0x20d8b6u,fields[12].first,4u);},"Request writer accepted wrong offset");
    deny([&]{(void)view.copy_defined_data(0x2e2e43u,p+0x9000u,fields[0].first,4u);},"Private DATA escaped through array copy");
    deny([&]{(void)view.copy_defined_data(0x2e2e43u,fields[0].first,p+0x9000u,4u);},"Retained copy targeted private DATA");
    check(!view.observes_retained_store(0x281787u,fields[0].first,4u)&&!view.observes_retained_store(0x20d8b0u,fields[12].first,1u),"Private source stores acquired a preimage");++controls;
    check(view.observes_retained_store(0x1d76b6u,p+0x9000u,4u),"Retained SourcePC observation was dropped");++controls;
    check(&view.retained_store_observer()==&current.observer,"Observer identity was not forwarded");++controls;
    check(native_tu15_read_accepted_private_word(view,Role::event,event,2u)==0xa5a5u,"Provided event ID changed");++controls;
    check(view.withhold_defined_data(fields[0].first,1u)&&!view.data_is_defined(fields[0].first,4u),"Private lifetime invalidation not delegated");++controls;
    bool undefined{};try{(void)native_tu15_read_accepted_private_word(view,Role::first_weight,fields[0].first);}catch(const std::runtime_error&){undefined=true;}
    check(undefined,"Typed private read bypassed the underlying definition guard");++controls;
    std::array<std::byte,4>b{};view.read(p+0x9000u,b);check(backing.reads==1u&&backing.writes==0u&&b[0]==std::byte{0x69},"Retained memory was not delegated once");++controls;
    std::cout<<"{\"passed\":true,\"controls\":"<<controls<<",\"role_refusals\":"<<refusals<<",\"backing_reads\":"<<backing.reads<<",\"backing_writes\":"<<backing.writes<<"}\n";
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
