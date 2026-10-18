#include "halo2/cache.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include <algorithm>
#include <iostream>
#include <thread>

namespace {
using namespace halo2;
void require(bool condition,const char* message){if(!condition)throw FormatError(message);}
template<class F> void rejects(F&& action) {
    try {action();}catch(const FormatError&){return;}
    throw FormatError("Resident cache ownership test accepted a foreign or incomplete address");
}
}
int main(int argc,char** argv)try {
    using namespace halo2;
    if(argc!=3 || !*argv[1] || !std::filesystem::is_regular_file(argv[1]))return 77;
    const Cache cache(argv[1]);if(!cache.is_mcc())return 77;
    std::filesystem::create_directories(argv[2]);
    PhysicsLiveWorldTu15 world(cache,argv[2]);auto& memory=world.memory();
    const auto word=[&](std::uint32_t p){return Bytes(memory.view(p,4)).u32(0);};
    const auto table=word(0x4f0414);
    unsigned roots{},banks{};
    for(const auto& tag:cache.select_tags({.group=tag_groups::bitmap,.require_address=true})) {
        require(tag.size>=0x4c,"Bitmap root lacks its complete MCC descriptor");
        const auto native=world.resident_cache_address(cache,tag.address,tag.size);
        require(native.value>UINT32_MAX,"Resident cache lookup narrowed its actual host pointer");
        const auto logical=memory.source_address(native,tag.size);
        const auto entry=table+tag.handle().index()*16;
        require(word(entry)==tag_groups::bitmap.value && word(entry+4)==tag.datum &&
            word(entry+8)==logical && word(entry+12)==tag.size,"Resident bitmap mapping differs from its typed directory");
        const Bytes root(cache.tag_payload(tag.handle()));const auto count=root.u32(0x44),first=root.u32(0x48);
        if(count) {
            const auto extent=std::size_t(count)*0xa8;
            const auto bank=world.resident_cache_address(cache,first,extent);
            require(memory.source_address(bank,extent)==word(logical+0x48),"Resident bitmap bank differs from its exact source allocation");
            require(std::ranges::equal(memory.view(bank,extent),cache.read_tag_bytes(first,extent)),
                "Resident bitmap bytes differ from their source preimage");
            ++banks;
        }
        rejects([&]{(void)world.resident_cache_address(cache,tag.address,0);});
        rejects([&]{(void)world.resident_cache_address(cache,tag.address+tag.size-1,2);});
        ++roots;
    }
    require(roots && banks,"Owned map supplied no resident bitmap owners");
    const Cache foreign(argv[1]);const auto* first=cache.query_tag({.group=tag_groups::bitmap,.require_address=true});
    require(first,"Bitmap directory unexpectedly disappeared");
    rejects([&]{(void)world.resident_cache_address(foreign,first->address,first->size);});
    rejects([&]{(void)world.resident_cache_address(cache,0xffffffffu,2);});
    bool rejected_thread{};
    std::thread other([&]{try {(void)world.resident_cache_address(cache,first->address,first->size);}
        catch(const FormatError&){rejected_thread=true;}});other.join();
    require(rejected_thread,"Resident cache lookup accepted a foreign executor");
    // Real Lockout banks with identical24-byte headers must retain distinct
    // immutable mappings even if writable runtime descriptors are redirected.
    const Tag *headlight{},*generic{};
    for(const auto& tag:cache.tags()) {
        if(tag.group!="bitm")continue;
        if(tag.name=="effects\\bitmaps\\lens_flares\\headlight_lens_flare")headlight=&tag;
        if(tag.name=="effects\\bitmaps\\lens_flares\\flares_generic")generic=&tag;
    }
    bool matching_prefix_control{};
    if(headlight && generic) {
        const Bytes a(cache.tag_payload(headlight->handle())),b(cache.tag_payload(generic->handle()));
        if(a.u32(0x44) && b.u32(0x44) && a.u32(0x48)!=b.u32(0x48) &&
           std::ranges::equal(cache.read_tag_bytes(a.u32(0x48),24),cache.read_tag_bytes(b.u32(0x48),24))) {
            const auto extent=std::size_t(a.u32(0x44))*0xa8;
            const auto correct=memory.source_address(world.resident_cache_address(cache,a.u32(0x48),extent),extent);
            const auto wrong=memory.source_address(world.resident_cache_address(cache,b.u32(0x48),0xa8),0xa8);
            const auto root=memory.source_address(world.resident_cache_address(cache,headlight->address,headlight->size),headlight->size);
            std::array<std::byte,4> redirect{};for(unsigned n=0;n<4;++n)redirect[n]=std::byte(wrong>>(8*n));
            memory.write(root+0x48,redirect);
            require(word(root+0x48)==wrong && correct!=wrong &&
                memory.source_address(world.resident_cache_address(cache,a.u32(0x48),extent),extent)==correct,
                "Matching-prefix redirect changed the retained admitted relocation");
            for(unsigned n=0;n<4;++n)redirect[n]=std::byte(correct>>(8*n));memory.write(root+0x48,redirect);
            matching_prefix_control=true;
        }
    }
    std::cout<<"PASS resident Cache ownership: roots="<<roots<<" banks="<<banks
        <<" matching_prefix_control="<<matching_prefix_control<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
