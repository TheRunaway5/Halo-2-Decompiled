#include "halo2/tags/system.hpp"
#include "halo2/cache.hpp"
#include "halo2/source_mutable_tag_store.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <limits>

namespace {
using namespace halo2;
using Buffer = std::vector<std::byte>;
void check(bool ok, const char* why) { if (!ok) throw std::runtime_error(why); }
template<class F> void rejects(F action) {
    try { action(); } catch (const FormatError&) { return; }
    throw std::runtime_error("Invalid tag operation was accepted");
}
void word(Buffer& b, std::size_t at, std::uint32_t v) {
    for (unsigned i = 0; i < 4; ++i) b.at(at+i) = std::byte(v >> (8*i));
}
void string(Buffer& b, std::size_t at, std::string_view s) {
    for (auto c : s) b.at(at++) = std::byte(c);
}
Buffer fixture() {
    Buffer b(8192);
    word(b,0,0x68656164); word(b,4,8); word(b,8,b.size()); word(b,2044,0x666f6f74);
    string(b,288,"02.09.27.09809"); string(b,408,"tags");
    word(b,16,2048); word(b,20,2048); word(b,24,4096);
    word(b,2048,0x200020); word(b,2052,5); word(b,2056,0x20005c);
    word(b,2060,0xffffffffu); word(b,2064,0xffffffffu); word(b,2072,6); word(b,2076,0x74616773);
    const TagGroupDefinition groups[]{
        {tag_groups::antenna,{},{}}, {tag_groups::bitmap,{},{}},
        {tag_groups::item,tag_groups::object,{}}, {tag_groups::object,{},{}},
        {tag_groups::weapon,tag_groups::item,tag_groups::object}};
    for (std::size_t i=0;i<5;++i) {
        word(b,2080+i*12,groups[i].group.value); word(b,2084+i*12,groups[i].parent.value);
        word(b,2088+i*12,groups[i].grandparent.value);
    }
    const std::uint32_t rows[][4]{
        {tag_groups::weapon.value,0xe0000000u,0x100000,64},
        {tag_groups::bitmap.value,0xe1000001u,0x100100,64},
        {0xffffffffu,0xffffffffu,0,0},
        {tag_groups::item.value,0xe2000003u,0,0},
        {tag_groups::weapon.value,0xe3000004u,0x100100,0},
        {tag_groups::antenna.value,0xe4000005u,0x100200,32}};
    word(b,708,3000); word(b,712,256); word(b,716,3400);
    for (std::size_t i=0;i<6;++i) {
        for (unsigned j=0;j<4;++j) word(b,2140+i*16+j*4,rows[i][j]);
        word(b,3400+i*4,i*32); string(b,3000+i*32,i==0?"Weapons\\Example":"tag"+std::to_string(i));
    }
    word(b,4096,2); word(b,4100,0x100100); // block points into a different tag
    word(b,4104,4); word(b,4108,0x100120);
    word(b,4112,tag_groups::bitmap.value); word(b,4116,0xe1000001u);
    word(b,4120,tag_groups::bitmap.value); word(b,4124,0xffffffffu);
    word(b,4128,0xffffffffu); word(b,4132,0x100100);
    word(b,4136,4); word(b,4140,0xfffffffeu);
    word(b,4144,0); word(b,4148,0xffffffffu);
    word(b,4352,0xabcdef12); word(b,4356,0x1234abcd); word(b,4384,0xdecafbad);
    word(b,356,1);word(b,360,8);word(b,364,3600);word(b,368,3616);
    word(b,3600,0);string(b,3616,"example");
    word(b,4608,1);word(b,4612,0x100220);word(b,4640,0x13572468);
    return b;
}
void save(const std::filesystem::path& p, const Buffer& b) {
    std::ofstream f(p,std::ios::binary); f.write(reinterpret_cast<const char*>(b.data()),b.size());
    check(bool(f),"Could not save tag fixture");
}
}
int main() {
    const auto path=std::filesystem::temp_directory_path()/
        ("halo2-tags-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".map");
    try {
        auto bytes=fixture(); save(path,bytes);
        auto cache=std::make_shared<Cache>(path);
        TagSystem tags(cache);
        check(tag_group_catalog().size()==120,"Incomplete group catalog");
        for (const auto& row : tag_group_catalog()) {
            check(TagGroup::from_string(row.group.string())==row.group,"Group conversion changed bytes");
            check(tag_group_name(row.group)==row.name,"Group catalog name mismatch");
        }
        rejects([] { TagGroup::from_string("abc"); });
        check(tag_name_strip_path("weapons\\rifle\\rifle")=="rifle" &&
              tag_name_strip_path("rifle")=="rifle", "Tag basename path semantics");
        check(tags.groups().size()==5 && tags.list().size()==5,"Tag directory coverage");
        check(tags.list(tag_groups::none)==tags.list(),"Retail NONE iterator must select every group");
        const auto objects=tags.list(tag_groups::object);
        check(objects==std::vector<TagHandle>{{0xe0000000u},{0xe2000003u},{0xe3000004u}},"Inheritance or unloaded iteration changed");
        check(tags.list(tag_groups::bitmap)==std::vector<TagHandle>{{0xe1000001u}},"Exact group iteration");
        const auto* first_weapon=cache->query_tag({.group=tag_groups::weapon, .require_address=true});
        check(first_weapon && first_weapon->datum==0xe0000000u,"Directory query changed original first-match order");
        check(!cache->query_tag({.handle=TagHandle{0xe0000000u}, .group=tag_groups::object}),
              "Exact decoder query unexpectedly included a derived group");
        check(cache->find_tag({0xe0000000u},tag_groups::object)==first_weapon,
              "Inherited lookup lost the derived definition");
        check(!cache->query_tag({.handle=TagHandle{0xe0010000u}}),"Query accepted another salt");
        check(!cache->query_tag({.group=tag_groups::weapon, .name="WEAPONS\\EXAMPLE"}),
              "Exact decoder path query changed case semantics");
        check(cache->query_tag({.group=tag_groups::weapon, .name="Weapons\\Example"})==first_weapon,
              "Exact decoder path query lost the authored path");
        const auto aliases=cache->select_tags({.address=0x100100});
        check(std::distance(aliases.begin(),aliases.end())==2,"Address selection omitted a zero-sized alias");
        check(cache->query_tag({.group=tag_groups::weapon, .address=0x100100, .minimum_size=1})==nullptr,
              "A zero-sized alias became an owned payload");
        check(cache->query_tag({.contains=TagAddressRange{0x100100,4}})->datum==0xe1000001u,
              "Containing query changed directory-order ownership");
        check(!cache->query_tag({.contains=TagAddressRange{0x10013f,2}}),"Containing query escaped a payload");
        check(!cache->query_tag({.contains=TagAddressRange{0xffffffffu,2}}),"Containing query accepted address wrapping");
        check(cache->tag_at_index(2).handle().is_null(),"Sparse raw slot identity changed");
        rejects([&] { cache->tag_at_index(6); });
        check(Bytes(cache->read_tag_bytes(0x100100,4)).u32(0)==0xabcdef12,"Shared resident byte reader changed contents");
        const Bytes allocation(bytes);
        const TagAddressSpace binary(allocation,0xff000);
        const auto binary_block=binary.block(4096,4);
        check(binary_block.count==2 && binary_block.offset==4352,"Borrowed block reader changed pointer conversion");
        check(binary.block(4144,4).count==0,"Empty borrowed block dereferenced its unused pointer");
        rejects([&] { binary.block(4096,4,1); });
        rejects([&] { binary.block(4128,4); });
        rejects([&] { binary.block(4096,std::numeric_limits<std::size_t>::max()); });
        rejects([&] { binary.offset(0xfefff,4); });
        rejects([&] { binary.offset(0xff000+bytes.size()-1,2); });
        check(tags.find(tag_groups::weapon,"WEAPONS\\EXAMPLE")==TagHandle{0xe0000000u},"Case-insensitive path lookup");
        check(tags.find(tag_groups::object,"Weapons\\Example").is_null(),"Path lookup must use exact group");
        check(!tags.try_get({0xe2000003u}) && !tags.try_get({0xffffffffu}) &&
              !tags.try_get({0xe0010000u}) && !tags.try_get({0xe0008000u}),"Salt, null, signed index or residency check");
        check(!tags.try_get({0xe0000000u},tag_groups::bitmap),"Wrong group accepted");
        auto weapon=tags.get({0xe0000000u},tag_groups::object);
        check(weapon.info().type()==tag_groups::weapon && weapon.bytes().size()==64,"Inherited get");
        check(tags.get({0xe3000004u}).bytes().empty(),"Zero-size alias lost");
        auto block=weapon.block(0,4);
        check(block.size()==2 && block.element(0).u32(0)==0xabcdef12 &&
              block.element(1).u32(0)==0x1234abcd,"Cross-tag block read");
        rejects([&] { block.element(2); });
        rejects([&] { weapon.block(0,0); });
        rejects([&] { weapon.block(0,std::numeric_limits<std::size_t>::max()); });
        rejects([&] { weapon.block(32,4); });
        rejects([&] { weapon.data(32); });
        rejects([&] { weapon.data(40); });
        rejects([&] { weapon.read(64,1); });
        rejects([&] { tags.read(0x100fff,2); });
        rejects([&] { tags.read(0x100000,std::numeric_limits<std::size_t>::max()); });
        check(weapon.block(48,4).size()==0 && weapon.data(48).empty(),"Empty descriptors must ignore their pointer");
        check(Bytes(weapon.data(8)).u32(0)==0xdecafbad,"Data reference read");
        check(tags.resolve(weapon.reference(16))->info().datum==0xe1000001u,"Tag reference resolution");
        check(!tags.resolve(weapon.reference(24)),"Null reference resolution");
        check(tags.string_id(0x07000000)=="example","StringID length byte affected lookup");
        rejects([&] { tags.string_id(0x07010000); });
        check(tags.get({0xe4000005u}).block(0,4).element(0).u32(0)==0x13572468,
              "Structured block element lost its field interface");
        check(weapon.structure(16,8).reference(0).handle==TagHandle{0xe1000001u},"Embedded structure reference");
        rejects([&] { weapon.structure(60,8); });
        {
            SourceMutableTagStore mutable_tags(cache,{"weap"});
            check(mutable_tags.tag_count()==1,"Zero-size alias imported into mutable payloads");
            auto providers=mutable_tags.providers();
            auto field=providers.memory(0x100000,4);field[0]=std::byte{9};
            check(weapon.u32(0)==2 && Bytes(providers.memory(0x100000,4)).u32(0)==9,
                  "Mutable tag copy changed immutable source bytes");
        }
        rejects([&] { tags.resolve({tag_groups::weapon,{0xe1000001u}}); });
        rejects([&] { tags.load_bsp(0); });
        check(!tags.selected_bsp(),"Failed BSP load changed state");
        TagIterator iterator{0,{},tag_groups::bitmap};
        check(cache->next_tag(iterator)==TagHandle{0xe1000001u},"Iterator first result");
        check(cache->next_tag(iterator).is_null() && iterator.current==TagHandle{0xe1000001u},"Iterator exhausted state");
        std::weak_ptr<const Cache> lifetime=cache; cache.reset(); tags.close();
        rejects([&] { tags.list(); });
        check(!lifetime.expired() && block.element(1).u32(0)==0x1234abcd && weapon.bytes().size()==64,
              "Owning views did not retain backing after close");
        auto bad=bytes; word(bad,2076,0); save(path,bad); rejects([&] { Cache invalid(path); });
        bad=bytes; word(bad,2080+12,tag_groups::antenna.value); save(path,bad); rejects([&] { Cache invalid(path); });
        bad=bytes; word(bad,2144,0xe0000001u); save(path,bad); rejects([&] { Cache invalid(path); });
        bad=bytes; word(bad,2140,0x61626364); save(path,bad); rejects([&] { Cache invalid(path); });
        bad=bytes; word(bad,2084,0x61626364); save(path,bad); rejects([&] { Cache invalid(path); });
        std::filesystem::remove(path);
        std::cout<<"Tag handles, groups, inheritance, iteration, references, blocks, data, lifetime and malformed input passed\n";
    } catch (const std::exception& e) {
        std::filesystem::remove(path); std::cerr<<e.what()<<'\n'; return 1;
    }
}
