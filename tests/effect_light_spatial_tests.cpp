#include "halo2/effect_light_spatial.hpp"
#include <fstream>
#include <iostream>
#include <memory>
using namespace halo2;
namespace {
void require(bool b,const std::string& m){if(!b)throw FormatError(m);}
}
int main(int argc,char**argv){try {
    require(argc==2,"Supply original effect-light spatial fixture");std::ifstream file(argv[1],std::ios::binary);
    require(bool(file),"Missing original light spatial fixture");
    const auto word=[&](){std::uint32_t v{};file.read(reinterpret_cast<char*>(&v),4);require(bool(file),"Truncated original spatial word");return v;};
    require(word()==0x31534c48,"Invalid original light spatial fixture");const auto cases=word();
    unsigned operations=0,compared=0;
    for(unsigned n=0;n<cases;++n) {
        const auto fill=word(),debug=word(),free_entries=word(),free_references=word(),count=word();
        require(free_entries<=2048 && free_references<=2048 && count==96,"Original light spatial scope changed");
        auto pool=std::make_unique<SourceEffectLightSpatialPool>(16);
        for(auto* p:{&pool->entries,&pool->references})for(auto& r:p->records)r.fill(fill);
        pool->activate(debug!=0);std::array<std::uint32_t,32> heads;heads.fill(0xffffffffu);
        for(unsigned i=0;i<2048-free_entries;++i)require(pool->entries.allocate()!=0xffffffffu,"Native light entry prefill failed");
        for(unsigned i=0;i<2048-free_references;++i)require(pool->references.allocate()!=0xffffffffu,"Native light reference prefill failed");
        const auto compare=[&](std::uint32_t got,const char* field){const auto expected=word();
            if(got!=expected)std::cerr<<"case="<<n<<" field="<<field<<" native="<<std::hex<<got<<" original="<<expected<<std::dec<<'\n';
            require(got==expected,"Original light spatial storage differs");++compared;};
        const auto summary=[&](){for(const auto* p:{&pool->entries,&pool->references})for(auto got:
            std::array{p->cursor,p->highwater,p->active_count,std::uint32_t(p->next_generation)})compare(got,"allocator");};
        summary();
        for(unsigned op=0;op<count;++op) {
            const auto deleting=word(),owner=word(),mask=word();require(owner<32,"Original light spatial owner exceeds fixture");
            const auto identity=0xe9810000u|owner;
            if(deleting)pool->erase(identity,heads[owner]);
            else {std::vector<std::int16_t> members;for(unsigned i=0;i<16;++i)if(mask&(1u<<i))members.push_back(std::int16_t(i));pool->insert(identity,heads[owner],members);}
            summary();for(auto got:pool->heads)compare(got,"cluster head");for(auto got:heads)compare(got,"owner head");++operations;
        }
        for(const auto* p:{&pool->entries,&pool->references}) {
            for(auto got:p->occupied)compare(got,"occupancy");
            for(const auto& r:p->records)for(unsigned i=0;i<3;++i)compare(r[i],"complete record");
        }
    }
    require(cases==16 && operations==1536 && compared>250000,"Original light spatial coverage missing");
    require(file.peek()==std::char_traits<char>::eof(),"Trailing original light spatial bytes");
    std::cout<<"PASS "<<cases<<" original cold/independent-pressure cases, "<<operations<<" ordered registration/removal operations, "<<compared<<" exact pool/queue words\n";
    return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
