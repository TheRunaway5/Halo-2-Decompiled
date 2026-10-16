#include "halo2/cache.hpp"
#include "halo2/source_tag_metadata.hpp"
#include "halo2/tags.hpp"
#include <algorithm>
#include <bit>
#include <chrono>
#include <fstream>
#include <iostream>

namespace {
using namespace halo2;
using Buffer=std::vector<std::byte>;
void check(bool value,const char* message) {if(!value)throw std::runtime_error(message);}
void word(Buffer& b,std::size_t at,std::uint32_t value) {
    for(unsigned i=0;i<4;++i)b.at(at+i)=std::byte(value>>(i*8));
}
void text(Buffer& b,std::size_t at,const std::string& value) {
    for(std::size_t i=0;i<value.size();++i)b.at(at+i)=std::byte(value[i]);
}
void save(const std::filesystem::path& path,const Buffer& bytes) {
    std::ofstream file(path,std::ios::binary);
    file.write(reinterpret_cast<const char*>(bytes.data()),std::streamsize(bytes.size()));
    check(bool(file),"Could not write fixture");
}
Buffer fixture(bool shared) {
    Buffer b(8192);
    word(b,0,0x68656164);word(b,4,8);word(b,8,std::uint32_t(b.size()));word(b,2044,0x666f6f74);
    word(b,16,2048);word(b,32,(shared?0x100000:0x200000)+2048-4096);
    text(b,300,"11081.07.04.30.0934.main");text(b,420,shared?"shared":"fixture");
    text(b,456,"scenarios\\fixture");
    word(b,2048+8,32);word(b,2048+12,shared?0xffffffffu:0xe0000001u);
    word(b,2048+16,0xffffffffu);word(b,2048+24,3);word(b,716,3);
    // A cache owns its authored group table even when only a subset is used.
    word(b,2048,128);word(b,2052,2);word(b,2076,0x74616773);
    word(b,2176,0x6269746d);word(b,2180,0xffffffffu);word(b,2184,0xffffffffu);
    word(b,2188,0x73636e72);word(b,2192,0xffffffffu);word(b,2196,0xffffffffu);
    word(b,720,3000);word(b,724,256);word(b,728,3300);
    text(b,3000,"scenarios\\fixture");text(b,3064,"textures\\original");
    word(b,3304,0);word(b,3308,64);
    word(b,2080+4,0xffffffffu); // Null slot must retain its low16 identity.
    word(b,2096,0x73636e72);word(b,2096+4,shared?0xffffffffu:0xe0000001u);
    word(b,2112,0x6269746d);word(b,2112+4,0xe1000002u);
    if(shared) {
        word(b,2112+8,0x100000);word(b,2112+12,192);
        word(b,4096+68,1);word(b,4096+72,0x100000+76);
    } else {
        word(b,2096+8,0x200000);word(b,2096+12,0x24c);
        word(b,4096+0x100,1);word(b,4096+0x104,0x200218);
        word(b,4096+0x218,std::bit_cast<std::uint32_t>(1.25f));
        word(b,4096+0x21c,std::bit_cast<std::uint32_t>(-.5f));
        word(b,4096+0x220,std::bit_cast<std::uint32_t>(9.f));
    }
    return b;
}
Buffer alias_fixture(bool shared) {
    auto b=fixture(shared);
    const auto original=Bytes(b).slice(2080,48);
    Buffer rows(original.begin(),original.end());
    std::copy(rows.begin(),rows.end(),b.begin()+2208);
    word(b,2056,160);word(b,2072,8);word(b,716,8);
    word(b,724,512);word(b,728,3700);word(b,3704,0);word(b,3708,64);
    const std::array addresses{0x100000u,0xffff0u,0x100004u,0x1000c0u,0u};
    for(unsigned i=0;i<addresses.size();++i) {
        const auto slot=i+3,at=2208+slot*16;
        word(b,at,tag_groups::bitmap.value);word(b,at+4,0xe2000003u+i*0x1000001u);
        word(b,at+8,shared?addresses[i]:0u);word(b,at+12,0);
        word(b,3700+slot*4,128+i*64);text(b,3000+128+i*64,"textures\\alias"+std::to_string(i));
    }
    return b;
}
template<class F> void rejects(F action,const char* message) {
    try {action();}catch(const FormatError&) {return;}
    throw std::runtime_error(message);
}
}

int main() {
    using namespace halo2;
    const auto root=std::filesystem::temp_directory_path()/
        ("halo2-vista-cache-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    try {
        std::filesystem::create_directories(root/"maps");
        const auto local=fixture(false),shared=fixture(true);
        const auto path=root/"maps/fixture.map",shared_path=root/"maps/shared.map";
        save(path,local);save(shared_path,shared);
        {
            const Cache cache(path);
            check(cache.is_vista(),"Vista was not identified");
            check(cache.tags().size()==3 && cache.tags()[0].datum==0xffffffffu,"Unused directory slot was collapsed");
            check(cache.tags()[2].datum==0xe1000002u && cache.tags()[2].address==0x100000,"Shared datum ownership changed");
            const auto payload=cache.load_source_tag_metadata(0xe1000002u);
            check(std::ranges::equal(payload.bytes_at(0x100000,192),std::span(shared).subspan(4096,192)),"Shared payload changed");
            check(cache.spawns().size()==1 && cache.spawns()[0].position.x==1.25f &&
                  cache.spawns()[0].position.y==-.5f && cache.spawns()[0].position.z==9.f,"Vista spawn pointer was not retained");
            rejects([&]{(void)cache.load_source_tag_metadata(0xe2000002u);},"Different datum salt resolved");
            rejects([&]{(void)cache.load_bitmap(0xe1000002u,0);},"Missing Xbox donor silently used Vista artwork");
        }
        {
            const auto aliases=alias_fixture(false),donor=alias_fixture(true);
            save(path,aliases);save(shared_path,donor);
            auto owner=std::make_shared<const Cache>(path);TagSystem tags(owner);
            const auto alias=tags.get({0xe2000003u},tag_groups::bitmap);
            check(alias.source_address()==0x100000 && alias.bytes().empty(),
                  "Vista shared alias lost its source address or serialized empty extent");
            check(std::ranges::equal(owner->tag_payload({0xe2000003u}),std::span(donor).subspan(4096,192)),
                  "Vista shared alias does not resolve the original same-group payload");
            check(tags.resolve({tag_groups::bitmap,{0xe2000003u}})->info().datum==0xe2000003u,
                  "Vista shared alias reference lost its salted identity");
            check(tags.get({0xe3000004u}).bytes().empty() && tags.get({0xe5000006u}).bytes().empty(),
                  "Vista empty tag locations were discarded");
            check(tags.read_source(0x100008,8).size()==8,"Vista interior alias shadowed its containing allocation");
            rejects([&]{(void)tags.read_source(0xffff0,1);},"Vista empty tag acquired invented payload bytes");
            rejects([&]{(void)tags.read_source(0x1000c0,1);},"Vista end alias escaped its payload allocation");
            check(!tags.try_get({0xe6000007u}),"Vista alias without a source address became resident");
            auto malformed=donor;word(malformed,2208+3*16+4,0xe9000003u);save(shared_path,malformed);
            rejects([&]{Cache invalid(path);},"Vista zero-size alias admitted a different salt");
            save(path,local);save(shared_path,shared);
        }
        auto changed=shared;
        text(changed,3064,"textures\\different");save(shared_path,changed);
        rejects([&]{Cache cache(path);},"Mismatched shared tag path admitted");
        changed=shared;word(changed,2112+4,0xe2000002u);save(shared_path,changed);
        rejects([&]{Cache cache(path);},"Mismatched shared salt admitted");
        save(shared_path,shared);
        changed=shared;word(changed,2180,0x73636e72);save(shared_path,changed);
        rejects([&]{Cache cache(path);},"Mismatched shared group hierarchy admitted");
        save(shared_path,shared);
        changed=local;word(changed,32,0x100000+2048-4096);word(changed,2096+8,0x100000);save(path,changed);
        rejects([&]{Cache cache(path);},"Overlapping resident allocations admitted");
        changed=local;word(changed,3308,256);save(path,changed);
        rejects([&]{Cache cache(path);},"Out-of-range tag filename admitted");
        save(path,local);
        std::ofstream(root/"asset-donors.tsv")<<"H2_VISTA_DONORS_V1\nxbox\t../missing\n";
        rejects([&]{Cache cache(path);},"Incomplete configured donor admitted");
        std::filesystem::remove(root/"asset-donors.tsv");
        std::ifstream input(path,std::ios::binary);
        Buffer after(local.size());input.read(reinterpret_cast<char*>(after.data()),std::streamsize(after.size()));
        check(after==local,"Read-only cache adapter changed its input file");
        std::filesystem::remove_all(root);
        std::cout<<"Vista ownership, sparse slots, source bytes, bounds and donor rejection passed\n";
        return 0;
    } catch(const std::exception& error) {
        std::filesystem::remove_all(root);std::cerr<<error.what()<<'\n';return 1;
    }
}
