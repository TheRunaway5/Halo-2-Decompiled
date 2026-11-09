#include "halo2/source_animation_cache_io_tu15.hpp"
#include "halo2/source_runtime_cache_tags.hpp"
#include <iostream>
#include <map>
#include <tuple>
using namespace halo2;
namespace {
void check(bool value){if(!value)throw std::runtime_error("Animation I/O factoring changed its source policy");}
struct Owner:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::byte> bytes;
    mutable std::vector<std::tuple<char,std::uint32_t,std::vector<std::byte>>> trace;
    unsigned fail{};mutable unsigned calls{};std::vector<std::array<std::uint32_t,3>> jobs;
    void hit()const{if(++calls==fail)throw std::runtime_error("Declared provider failure");}
    void read(std::uint32_t at,std::span<std::byte> out)const override{hit();for(unsigned n=0;n<out.size();++n)out[n]=bytes.at(at+n);trace.emplace_back('r',at,std::vector(out.begin(),out.end()));}
    void write(std::uint32_t at,std::span<const std::byte> data)override{hit();trace.emplace_back('w',at,std::vector(data.begin(),data.end()));for(unsigned n=0;n<data.size();++n)bytes[at+n]=data[n];}
    std::vector<std::byte> view(std::uint32_t at,unsigned size)const{hit();std::vector<std::byte> value;for(unsigned n=0;n<size;++n)value.push_back(bytes.at(at+n));trace.emplace_back('v',at,value);return value;}
    std::uint32_t word(std::uint32_t at)const{std::array<std::byte,4> value;read(at,value);return Bytes(value).u32(0);}
    void put(std::uint32_t at,std::uint32_t value){for(unsigned n=0;n<4;++n)bytes[at+n]=std::byte(value>>(n*8));}
};
struct BackedServices {
    Owner&owner;SourceRuntimeCacheAnimationImage resource;
    SourceRuntimeCacheAnimationImage inspect(std::uint32_t descriptor){owner.hit();owner.trace.emplace_back('i',descriptor,std::vector<std::byte>{});return resource;}
    std::vector<std::byte> load(std::uint32_t encoded,unsigned size){owner.hit();auto out=owner.view(encoded,size);owner.trace.emplace_back('l',encoded,out);return out;}
    std::uint32_t retain(std::uint32_t descriptor,std::uint32_t destination,unsigned size){owner.hit();owner.jobs.push_back({descriptor,destination,size});owner.trace.emplace_back('j',descriptor,std::vector<std::byte>{});return 0x10000u+owner.jobs.size();}
};
// Verbatim ordering of the previous World boundary, retained only as this
// factoring regression. The declared backed services own bytes and job IDs;
// this is not a claim about Xbox asynchronous scheduling or asset coverage.
void previous(Owner&m,SourcePhysicsWorldRuntimeAbi&a,BackedServices&resources,std::thread::id main_thread){
    const auto sp=a.gp[4],tracking=a.gp[7];
    const auto descriptor=m.word(tracking+0xcu),size=m.word(sp+4),destination=m.word(sp+8);
    const auto resource=resources.inspect(descriptor);
    if(std::this_thread::get_id()!=main_thread||m.word(sp)!=0x12613eu||m.word(sp+12)!=0||m.word(sp+16)!=tracking+4u||m.word(sp+20)!=6u||
        !std::ranges::equal(m.view(descriptor,12),std::span(resource.descriptor).first(12))||a.gp[1]!=m.word(descriptor+8u)||size!=((resource.bytes.size()+511u)&~511u))throw FormatError("Previous animation policy refused");
    auto bytes=resources.load(a.gp[1],size);m.write(destination,bytes);
    const auto job=resources.retain(descriptor,destination,size);const std::array done{std::byte{1}};m.write(tracking+4u,done);
    a.gp[0]=job;a.gp[4]+=28;
}
}
int main()try{
    unsigned comparisons{},returns{},failures{},refusals{};
    for(unsigned variant=0;variant<10;++variant)for(unsigned alignment=0;alignment<16;++alignment){
        Owner input;SourcePhysicsWorldRuntimeAbi incoming{};incoming.gp={1,0x4000,3,4,0x2000+alignment,6,7,0x1000};incoming.eflags=0xcd7;
        const auto sp=incoming.gp[4];input.put(0x100c,0x3000);input.put(0x3000,0x11223344);input.put(0x3004,777);input.put(0x3008,0x4000);
        input.put(sp,0x12613e);input.put(sp+4,1024);input.put(sp+8,0x5000);input.put(sp+12,0);input.put(sp+16,0x1004);input.put(sp+20,6);
        for(unsigned n=0;n<1024;++n){input.bytes[0x4000+n]=std::byte((n*13+variant)&255);input.bytes[0x5000+n]=std::byte{0xcc};}
        SourceRuntimeCacheAnimationImage resource{};for(unsigned n=0;n<12;++n)resource.descriptor[n]=input.bytes.at(0x3000+n);resource.bytes.resize(777);
        if(variant==1)input.put(sp,0xdeadbeef);if(variant==2)input.put(sp+12,1);if(variant==3)input.put(sp+16,0x1005);
        if(variant==4)input.put(sp+20,5);if(variant==5)resource.descriptor[0]^=std::byte{1};if(variant==6)incoming.gp[1]++;
        if(variant==7)input.put(sp+4,512);if(variant==8)input.put(sp+8,0x3000);
        const auto executor=variant==9?std::thread::id{}:std::this_thread::get_id();
        // Each source read, cache lookup, payload transfer, retained request,
        // and completion publication can fail at its own real boundary.
        for(unsigned fail=0;fail<23;++fail){
            Owner before=input,after=input;before.fail=after.fail=fail;BackedServices old{before,resource},now{after,resource};auto a=incoming,b=incoming;bool old_failed{},new_failed{};
            try{previous(before,a,old,executor);}catch(const std::exception&){old_failed=true;}
            try{const SourcePhysicsContactRuntimeCall call{0x126139u,0x1a3b70u,false};check(source_try_animation_cache_io_tu15(after,after,call,b,executor,
                [&](auto descriptor){return now.inspect(descriptor);},[&](auto descriptor){return after.view(descriptor,12);},
                [&](const SourceAnimationCacheReadTu15&r){return now.load(r.encoded_offset,r.bytes);},[&](const SourceAnimationCacheReadTu15&r){return now.retain(r.descriptor,r.destination,r.bytes);}));}catch(const std::exception&){new_failed=true;}
            check(old_failed==new_failed&&a.gp==b.gp&&a.eflags==b.eflags&&before.bytes==after.bytes&&before.jobs==after.jobs&&before.trace==after.trace);
            ++comparisons;if(new_failed)++failures;else ++returns;
        }
        const auto a=incoming;Owner unowned=input;BackedServices service{unowned,resource};auto b=a;
        const SourcePhysicsContactRuntimeCall unrelated{0x12613au,0x1a3b70u,false};
        check(!source_try_animation_cache_io_tu15(unowned,unowned,unrelated,b,executor,[&](auto d){return service.inspect(d);},[&](auto d){return unowned.view(d,12);},[&](const auto&r){return service.load(r.encoded_offset,r.bytes);},[&](const auto&r){return service.retain(r.descriptor,r.destination,r.bytes);}));
        check(b.gp==a.gp&&unowned.trace.empty()&&unowned.jobs.empty());++refusals;
    }
    std::cout<<"{\"passed\":true,\"comparisons\":"<<comparisons<<",\"returns\":"<<returns<<",\"failure_or_validation_boundaries\":"<<failures<<",\"unowned_refusals\":"<<refusals<<",\"declared_backed_service_scope\":true}\n";
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
