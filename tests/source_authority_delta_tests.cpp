#include "halo2/source_authority_delta.hpp"
#include <openssl/sha.h>
#include <zlib.h>
#include <algorithm>
#include <future>
#include <iostream>
#include <random>
#include <stdexcept>
#include <cstdlib>
#include <new>

// Cap this one hostile request so the regression is safe before remediation.
thread_local bool guard_authority_allocation=false,large_authority_allocation=false;
void *operator new(std::size_t size){
    if(guard_authority_allocation&&size>64ull*1024*1024){large_authority_allocation=true;throw std::bad_alloc();}
    if(auto *p=std::malloc(size?size:1))return p;throw std::bad_alloc();
}
void operator delete(void *p)noexcept{std::free(p);}
void operator delete(void *p,std::size_t)noexcept{std::free(p);}

using namespace halo2;
namespace {
using Bytes=std::vector<std::byte>;
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
template<class F>void rejects(F f){bool caught=false;try{f();}catch(const std::exception &){caught=true;}check(caught,"Malformed delta or wrong base accepted");}
void put(Bytes &p,std::uint64_t v,unsigned n){for(unsigned i=0;i<n;++i)p.push_back(std::byte(v>>(8*i)));}
void set(Bytes &p,unsigned at,std::uint64_t v,unsigned n=8){for(unsigned i=0;i<n;++i)p[at+i]=std::byte(v>>(8*i));}
std::uint64_t get(const Bytes &p,unsigned at){std::uint64_t v=0;for(unsigned i=0;i<8;++i)v|=std::to_integer<std::uint64_t>(p[at+i])<<(8*i);return v;}
Bytes packed(std::span<const std::byte> raw){Bytes result(compressBound(raw.size()));uLongf size=result.size();
    check(compress2(reinterpret_cast<Bytef *>(result.data()),&size,reinterpret_cast<const Bytef *>(raw.data()),raw.size(),Z_BEST_SPEED)==Z_OK,"Fixture compression failed");
    result.resize(size);return result;}
void digest_into(Bytes &p,unsigned at,std::span<const std::byte> raw){check(SHA256(reinterpret_cast<const unsigned char *>(raw.data()),raw.size(),reinterpret_cast<unsigned char *>(p.data()+at)),"Fixture hash failed");}
Bytes archive(const Bytes &raw){Bytes p;for(char c:std::string("H2AUTH01"))p.push_back(std::byte(c));put(p,1,4);put(p,0,4);
    auto payload=packed(raw);put(p,raw.size(),8);put(p,payload.size(),8);p.resize(64);digest_into(p,32,raw);p.insert(p.end(),payload.begin(),payload.end());return p;}
template<class F>Bytes mutate_patch(Bytes p,F f){const auto size=get(p,32);Bytes raw(size);uLongf actual=size;uLong consumed=p.size()-144;
    check(uncompress2(reinterpret_cast<Bytef *>(raw.data()),&actual,reinterpret_cast<const Bytef *>(p.data()+144),&consumed)==Z_OK,"Fixture patch expansion failed");
    f(raw);auto compressed=packed(raw);p.resize(144);set(p,32,raw.size());set(p,40,compressed.size());digest_into(p,112,raw);
    p.insert(p.end(),compressed.begin(),compressed.end());return p;}
void run(){
    auto bomb=archive(Bytes(1,std::byte{}));set(bomb,16,1024ull*1024*1024);
    guard_authority_allocation=true;
    rejects([&]{AuthorityArchiveImage::read(bomb);});
    guard_authority_allocation=false;
    check(!large_authority_allocation,"Host archive requested gigabyte storage before rejecting its declaration");
    // Invalid declarations below the decoded cap must also fail before a
    // disproportionate request. A tight cumulative budget includes the base,
    // patch and replacement together rather than granting each its own limit.
    auto high_ratio=archive(Bytes(1,std::byte{}));set(high_ratio,16,64ull*1024*1024);
    rejects([&]{AuthorityArchiveImage::read(high_ratio,source_authority_network_delta_limits());});
    std::mt19937 random(0x1500);Bytes raw(2*1024*1024);for(unsigned i=0;i<1024*1024;++i)raw[i]=std::byte(random());
    const auto base=AuthorityArchiveImage::read(archive(raw));auto target=raw;
    for(unsigned i:{23u,5013u,700115u})target[i]^=std::byte{0x15};
    target.insert(target.begin()+10103,137,std::byte{0x42});target.erase(target.begin()+500000,target.begin()+500092);
    std::copy_n(raw.begin()+200000,4096,target.begin()+800000);
    const auto next=AuthorityArchiveImage::read(archive(target));const auto delta=encode_authority_delta(base,next);
    const auto restored=apply_authority_delta(base,delta);
    check(restored.digest()==next.digest()&&restored.decoded_size()==target.size()&&restored.encode()==archive(target),"Insert/delete/moved/repeated regions did not reconstruct complete canonical bytes");
    check(delta.size()<20000,"Delta resent unchanged source regions instead of copying exact base ranges");
    const auto identical=encode_authority_delta(base,base);check(identical.size()<256,"Unchanged complete image did not reduce to one copy");
    auto cumulative=source_authority_network_delta_limits();cumulative.working_bytes=base.decoded_size();
    rejects([&]{apply_authority_delta(base,identical,cumulative);});
    check(apply_authority_delta(base,identical).encode()==base.encode(),"Unchanged archive differs");
    const auto other=AuthorityArchiveImage::read(archive(Bytes(2000,std::byte{7})));
    rejects([&]{apply_authority_delta(other,delta);});
    for(unsigned at:{0u,8u,16u,24u,32u,40u,48u,80u,112u,144u}){auto bad=delta;bad[at]^=std::byte{1};rejects([&]{apply_authority_delta(base,bad);});}
    auto trailing=delta;trailing.push_back(std::byte{});rejects([&]{apply_authority_delta(base,trailing);});
    rejects([&]{apply_authority_delta(base,std::span(delta).first(delta.size()-1));});
    auto limits=AuthorityDeltaLimits{};limits.decoded_bytes=base.decoded_size()-1;rejects([&]{apply_authority_delta(base,delta,limits);});
    limits={};limits.patch_bytes=8;rejects([&]{apply_authority_delta(base,delta,limits);});
    limits={};limits.commands=1;rejects([&]{apply_authority_delta(base,delta,limits);});
    limits={};limits.encoded_bytes=144;rejects([&]{encode_authority_delta(base,next,limits);});
    // Valid checksums cannot grant copy ranges, output extents, or opcodes.
    for(auto bad:{mutate_patch(identical,[](Bytes &p){p[8]=std::byte{2};}),
                  mutate_patch(identical,[](Bytes &p){set(p,9,UINT64_MAX);}),
                  mutate_patch(identical,[](Bytes &p){set(p,17,UINT64_MAX);}),
                  mutate_patch(identical,[](Bytes &p){set(p,0,UINT64_MAX);}),
                  mutate_patch(identical,[](Bytes &p){p.push_back(std::byte{});})})
        rejects([&]{apply_authority_delta(base,bad);});
    auto invalid_source=archive(raw);invalid_source[32]^=std::byte{1};rejects([&]{AuthorityArchiveImage::read(invalid_source);});
    rejects([&]{AuthorityArchiveImage::read(std::span(invalid_source).first(63));});
    // Each concurrent worker keeps an immutable base alive while independently
    // reconstructing and encoding its own complete target. No shared mutation.
    std::vector<std::future<void>> work;for(unsigned i=0;i<4;++i)work.push_back(std::async(std::launch::async,[base,next,delta]{
        auto result=apply_authority_delta(base,delta);check(result.digest()==next.digest(),"Concurrent reconstruction changed the cut");
        check(AuthorityArchiveImage::read(result.encode()).digest()==next.digest(),"Concurrent archive encoding changed state");}));
    for(auto &worker:work)worker.get();
    // Network ceiling is usable for highly compressible native-memory banks.
    // The unchanged delta exercises a maximum-size output with a tiny patch.
    {
        const auto bounded=source_authority_network_delta_limits();
        const auto bytes=archive(Bytes(bounded.decoded_bytes,std::byte{}));
        const auto maximum=AuthorityArchiveImage::read(bytes,bounded);
        const auto copy=encode_authority_delta(maximum,maximum,bounded);
        check(apply_authority_delta(maximum,copy,bounded).digest()==maximum.digest(),"Maximum network delta failed");
        auto too_large=bytes;set(too_large,16,bounded.decoded_bytes+1);
        for(unsigned attempt=0;attempt<16;++attempt)rejects([&]{AuthorityArchiveImage::read(too_large,bounded);});
        auto too_large_patch=copy;set(too_large_patch,32,bounded.patch_bytes+1);
        rejects([&]{apply_authority_delta(maximum,too_large_patch,bounded);});
        std::cout<<"Network maximum image: "<<maximum.decoded_size()<<" decoded / "<<bytes.size()
            <<" encoded bytes; unchanged delta: "<<copy.size()<<" bytes; decode/patch reservation "<<bounded.working_bytes<<" bytes\n";
    }
    // Tiny states, complete replacement and growing/shrinking cuts exercise
    // the literal-only path independently of the copy index.
    for(unsigned n:{1u,63u,64u,65u,1023u,4097u}){Bytes value(n);for(auto &v:value)v=std::byte(random());
        auto image=AuthorityArchiveImage::read(archive(value));auto d=encode_authority_delta(base,image);
        check(apply_authority_delta(base,d).encode()==archive(value),"Literal-only complete replacement differs");}
    std::cout<<"PASS complete canonical archive delta insert/delete/move/repeated regions: "<<next.encode().size()<<" full bytes -> "<<delta.size()
        <<" delta bytes; exact base/target checksums, malformed checksummed instructions, budgets, tiny cuts and concurrent immutable reconstruction\n";
}
}
int main()try{run();return 0;}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
