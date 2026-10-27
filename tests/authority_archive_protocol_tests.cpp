#include "halo2/authority_archive_protocol.hpp"
#include <algorithm>
#include <atomic>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
using namespace halo2;
namespace {
using Bytes=std::vector<std::byte>;
void check(bool v,const char *why){if(!v)throw std::runtime_error(why);}
void put(Bytes &b,std::uint64_t v,unsigned n){for(unsigned i=0;i<n;++i)b.push_back(std::byte(v>>(i*8)));}
void octets(Bytes &b,std::string_view text){for(unsigned char c:text)b.push_back(std::byte(c));}
constexpr AuthorityArchiveDecodeLimits limits{1<<20,32<<20,32<<20,1'000'000,128};
// Test-only independent stored-block zlib fixture, with a declared placeholder
// SHA value. Native checksum admission is tested by source_authority_schema3;
// this test deliberately exercises framing/grammar before native admission.
Bytes frame(const Bytes &raw){
    Bytes compressed{std::byte{0x78},std::byte{1}};
    for(std::size_t at=0;at<raw.size();){
        const auto n=std::min<std::size_t>(65535,raw.size()-at);
        put(compressed,at+n==raw.size()?1:0,1);put(compressed,n,2);put(compressed,65535-n,2);
        compressed.insert(compressed.end(),raw.begin()+at,raw.begin()+at+n);at+=n;
    }
    std::uint32_t a=1,b=0;for(auto value:raw){a=(a+std::to_integer<unsigned>(value))%65521;b=(b+a)%65521;}
    const auto adler=(b<<16)|a;for(unsigned i=0;i<4;++i)put(compressed,adler>>(24-8*i),1);
    Bytes out;octets(out,"H2AUTH01");put(out,1,4);put(out,0,4);put(out,raw.size(),8);put(out,compressed.size(),8);
    for(unsigned i=0;i<32;++i)put(out,0x15,1);out.insert(out.end(),compressed.begin(),compressed.end());return out;
}
Bytes seed(){
    Bytes raw;put(raw,13,1);put(raw,15,4);
    for(unsigned tag=0;tag<15;++tag){
        put(raw,1,2);put(raw,'a'+tag,1);put(raw,tag,1);
        if(tag==0)put(raw,1,1);
        else if(tag==1||tag==2||tag==4||tag==14)put(raw,0x8000000000000001ull,8);
        else if(tag==3)put(raw,0x7fc00001,4);
        else if(tag==5||tag==6){put(raw,3,8);put(raw,0x80ff00,3);}
        else if(tag>=7&&tag<=10){put(raw,1,8);put(raw,0,1);put(raw,1,1);if(tag==9){put(raw,0,1);put(raw,0,1);}}
        else if(tag==11){put(raw,1,1);put(raw,0,1);put(raw,0,1);}
        else if(tag==12)put(raw,0,1);
        else if(tag==13)put(raw,0,4);
    }
    return raw;
}
struct Expected {unsigned tag{};std::uint64_t bits{},count{};std::size_t next{};Bytes name,data;};
// Test-only recursive extraction of the original named grammar. Ordinary
// mutation seeds are shallow; the hostile deep case never enters this walker.
struct Reference {
    const Bytes &raw;std::size_t at{};std::vector<Expected> nodes;
    std::uint64_t get(unsigned n){check(at<=raw.size()&&n<=raw.size()-at,"reference truncation");
        std::uint64_t v{};for(unsigned i=0;i<n;++i)v|=std::uint64_t(std::to_integer<unsigned>(raw[at++]))<<(8*i);return v;}
    Bytes bytes(std::uint64_t n){check(at<=raw.size()&&n<=raw.size()-at,"reference range");
        Bytes b(raw.begin()+at,raw.begin()+at+std::size_t(n));at+=std::size_t(n);return b;}
    void value(Bytes name={},unsigned depth=0){
        check(depth<128,"reference depth");const auto id=nodes.size();Expected n;n.name=std::move(name);n.tag=get(1);
        std::uint64_t children{};
        switch(n.tag){
        case 0:n.bits=get(1);check(n.bits<=1,"bool");break;
        case 1:case 2:case 4:case 14:n.bits=get(8);break;
        case 3:n.bits=get(4);break;
        case 5:case 6:n.count=get(8);n.data=bytes(n.count);break;
        case 7:case 8:case 9:case 10:n.count=get(8);check(n.count<=raw.size()-at,"count");children=n.count*(n.tag==9?2:1);break;
        case 11:n.bits=get(1);check(n.bits<=1,"optional");children=n.bits;break;
        case 12:check(get(1)==0,"pointer");break;
        case 13:n.count=get(4);check(n.count<=raw.size()-at,"record");children=n.count;break;
        default:throw std::runtime_error("tag");}
        nodes.push_back(n);
        for(std::uint64_t i=0;i<children;++i){Bytes field;if(n.tag==13)field=bytes(get(2));value(std::move(field),depth+1);}
        nodes[id].next=nodes.size();
    }
};
bool same(const ParsedAuthorityArchive &parsed,const std::vector<Expected> &expected){
    if(parsed.size()!=expected.size())return false;
    for(unsigned i=0;i<expected.size();++i){const auto n=parsed.node(i);const auto &e=expected[i];
        if(n.tag!=e.tag||n.bits!=e.bits||n.count!=e.count||n.next!=e.next||
            !std::ranges::equal(parsed.name(i),e.name)||!std::ranges::equal(parsed.data(i),e.data))return false;}
    return true;
}
}
int main()try{
    std::size_t compared{};
    const auto compare=[&](const Bytes &raw){
        Reference ref{raw};bool accepted=true;
        try{ref.value();check(ref.at==raw.size()&&ref.nodes.front().tag==13,"root/trailing");}catch(const std::exception&){accepted=false;}
        const auto parsed=parse_authority_archive(frame(raw),limits);
        check(bool(parsed)==accepted,"Rust/C++ authority grammar acceptance differs");
        if(parsed)check(same(*parsed,ref.nodes),"Rust/C++ authority values differ");++compared;
    };
    const auto original=seed();compare(original);
    std::array<std::byte,32> hash;hash.fill(std::byte{0x15});
    auto canonical=parse_authority_archive_raw(original,hash,limits);
    Reference reference{original};reference.value();
    check(canonical&&same(*canonical,reference.nodes),"Raw canonical image changed archive grammar");
    check(canonical->expected_hash()==hash,"Raw image lost its independent digest");
    auto tight=limits;tight.working_bytes=original.size();
    check(!parse_authority_archive_raw(original,hash,tight),"Raw parser skipped aggregate backing/metadata budget");
    for(unsigned n=1;n<original.size();++n)compare(Bytes(original.begin(),original.begin()+n));
    for(unsigned at=0;at<original.size();++at)for(unsigned byte=0;byte<256;++byte){auto changed=original;changed[at]=std::byte(byte);compare(changed);}
    auto trailing=original;trailing.push_back(std::byte{});compare(trailing);
    auto packet=frame(original);auto owned=parse_authority_archive(packet,limits);check(bool(owned),"Stored archive fixture rejected");
    packet.clear();packet.shrink_to_fit();check(std::ranges::equal(owned->raw(),original),"Archive backing borrowed retired packet");
    for(auto b:owned->expected_hash())check(b==std::byte{0x15},"Expected checksum bits changed");
    for(unsigned at:{0u,8u,12u,16u,24u,64u}){auto bad=frame(original);bad[at]^=std::byte{0x80};check(!parse_authority_archive(bad,limits),"Bad envelope/stream accepted");}
    packet=frame(original);packet.push_back(std::byte{});check(!parse_authority_archive(packet,limits),"Trailing stream accepted");
    Bytes deep;put(deep,13,1);put(deep,1,4);put(deep,0,2);
    for(unsigned n=0;n<100'000;++n){put(deep,11,1);put(deep,1,1);}put(deep,0,1);put(deep,1,1);
    auto nested=parse_authority_archive(frame(deep),limits);check(bool(nested),"Valid deep flat archive rejected");
    check(nested->stats().deepest==100'001&&nested->size()==100'002,"Deep tree lost nodes");nested.reset();
    auto budget=limits;budget.allocated_bytes=37;budget.container_elements=37;
    auto bounded=parse_authority_archive(frame(original),budget);check(bool(bounded),"Claim fixture rejected");
    std::vector<std::jthread> workers;for(unsigned i=0;i<4;++i)workers.emplace_back([&]{for(unsigned j=0;j<100;++j)bounded->claim(1,1,true);});workers.clear();
    check(bounded->stats().native_claimed==37&&!bounded->claim(UINT64_MAX,2,true),"Concurrent native reservation budget bypassed");
    budget=limits;budget.decoded_bytes=1;check(!parse_authority_archive(frame(original),budget),"Decoded expansion budget bypassed");
    std::cout<<"PASS "<<compared<<" named archive grammar/value comparisons, owned backing, exact scalar bits, 100000 nested values with flat teardown and synchronized native reservation limits; native checksum/schema/import acceptance remains separate.\n";
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
