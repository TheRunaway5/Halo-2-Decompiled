#include "halo2/mcc_ordinary_pointer.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <iostream>
#include <map>
#include <stdexcept>

namespace {
using namespace halo2;
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void word(std::vector<std::byte>& out,unsigned at,std::uint32_t value) {
    Bytes(out).require(at,4);for(unsigned n=0;n<4;++n)out[at+n]=std::byte(value>>(8*n));
}
struct Owners {
    std::map<std::uint32_t,std::vector<std::byte>> banks;
    std::span<const std::byte> read(std::uint32_t address,std::size_t bytes)const {
        for(const auto& [base,bank]:banks)if(address>=base && std::uint64_t(address)-base<=bank.size() && bytes<=bank.size()-(address-base))
            return std::span(bank).subspan(address-base,bytes);
        throw FormatError("Synthetic metadata reference has no declared owner");
    }
    auto fields(const char* group,std::uint32_t root=0x10000)const {
        return ordinary_mcc_pointer_fields(TagGroup::from_string(group),root,
            [&](auto address,auto bytes){return read(address,bytes);});
    }
};
template<class F> void rejected(const F& f,const char* message) {
    bool failed=false;try{f();}catch(const FormatError&){failed=true;}require(failed,message);
}
bool has(std::span<const MccOrdinaryPointerField> rows,unsigned word_address,unsigned pointer,unsigned bytes) {
    return std::ranges::any_of(rows,[&](auto r){return r.field_address==word_address && r.address==pointer && r.bytes==bytes;});
}
}
int main(int argc,char**argv)try {
    using namespace halo2;
    Owners weapon;
    weapon.banks[0x10000].resize(0x31c);
    weapon.banks[0x20000].resize(0x20);
    weapon.banks[0x30000]={std::byte{1},std::byte{2},std::byte{3}};
    word(weapon.banks[0x10000],0x64,1);word(weapon.banks[0x10000],0x68,0x20000);
    word(weapon.banks[0x20000],0x14,3);word(weapon.banks[0x20000],0x18,0x30000);
    // A scalar/tag datum that happens to equal an owner address is preserved.
    word(weapon.banks[0x10000],0x18,0x30000);
    const auto original=weapon.banks;
    const auto w=weapon.fields("weap");require(w.has_value() && w->size()==2,"Nested weapon descriptors missing");
    require(has(*w,0x10068,0x20000,0x20) && has(*w,0x20018,0x30000,3),"Absolute external child field identities differ");
    require(weapon.banks==original,"Visitor changed authored metadata bytes");
    word(weapon.banks[0x10000],0x5c,0);word(weapon.banks[0x10000],0x60,0x30000);
    require(has(*weapon.fields("weap"),0x10060,0x30000,0),"Empty descriptor retained donor address without relocation");
    word(weapon.banks[0x10000],0x64,0x80000000u);
    rejected([&]{weapon.fields("weap");},"Excessive block count admitted");
    word(weapon.banks[0x10000],0x64,1);word(weapon.banks[0x10000],0x68,0);
    rejected([&]{weapon.fields("weap");},"Nonempty null block admitted");
    word(weapon.banks[0x10000],0x68,0x20000);
    weapon.banks[0x40000].resize(0x5c);weapon.banks[0x50000].resize(0xc);
    weapon.banks[0x60000].resize(0xec);weapon.banks[0x70000].resize(0x34);
    word(weapon.banks[0x10000],0x2c0,1);word(weapon.banks[0x10000],0x2c4,0x40000);
    word(weapon.banks[0x40000],0x54,1);word(weapon.banks[0x40000],0x58,0x50000);
    word(weapon.banks[0x10000],0x2d0,1);word(weapon.banks[0x10000],0x2d4,0x60000);
    word(weapon.banks[0x60000],0xe4,1);word(weapon.banks[0x60000],0xe8,0x70000);
    const auto firing=weapon.fields("weap");
    require(has(*firing,0x10000+0x2c4,0x40000,0x5c) && has(*firing,0x40058,0x50000,0xc) &&
        has(*firing,0x10000+0x2d4,0x60000,0xec) && has(*firing,0x600e8,0x70000,0x34),
        "Weapon magazine/effect child graph differs");

    Owners bitmap;bitmap.banks[0x10000].resize(0x50);bitmap.banks[0x40000].resize(0xa8);
    word(bitmap.banks[0x10000],0x44,1);word(bitmap.banks[0x10000],0x48,0x40000);
    require(has(*bitmap.fields("bitm"),0x10048,0x40000,0xa8),"MCC bitmap stride differs");
    // Check the high half: a native host pointer must never be truncated.
    word(bitmap.banks[0x40000],0x34,1);
    rejected([&]{bitmap.fields("bitm");},"Bound 64-bit runtime pointer admitted");

    for(const auto [group,descriptor,stride,count]:{
        std::tuple("bipd",0x284u,0x80u,2u),std::tuple("bipd",0x28cu,0x50u,2u),
        std::tuple("crea",0xf4u,0x80u,2u),std::tuple("crea",0xfcu,0x50u,2u),
        std::tuple("vehi",0x2f8u,0x2a0u,1u)}) {
        Owners unit;unit.banks[0x10000].resize(0x314);unit.banks[0x50000].resize(stride*count);
        word(unit.banks[0x10000],descriptor,count);word(unit.banks[0x10000],descriptor+4,0x50000);
        require(has(*unit.fields(group),0x10000+descriptor+4,0x50000,stride*count),"Normalized unit shape stride regressed to donor layout");
    }

    Owners scenario;scenario.banks[0x10000].resize(0x3e0);scenario.banks[0x60000].resize(0x14);
    word(scenario.banks[0x10000],0x238,1);word(scenario.banks[0x10000],0x23c,0x60000);
    word(scenario.banks[0x60000],0xc,0x30000);
    const auto s=scenario.fields("scnr");require(s && s->size()==1 && has(*s,0x1023c,0x60000,0x14),"Relative script string offset treated as address");
    word(scenario.banks[0x10000],0x180,1);
    rejected([&]{scenario.fields("scnr");},"Unknown nonempty pathfinding owner admitted");

    Owners bsp;bsp.banks[0x10000].resize(0x23c);bsp.banks[0x20000].resize(0xc8);
    bsp.banks[0x30000].resize(0xa0);bsp.banks[0x40000].resize(3);
    word(bsp.banks[0x10000],0x138,1);word(bsp.banks[0x10000],0x13c,0x20000);
    word(bsp.banks[0x20000],0xb0,1);word(bsp.banks[0x20000],0xb4,0x30000);
    word(bsp.banks[0x30000],0x64,3);word(bsp.banks[0x30000],0x68,0x40000);
    // The unused reservation tail is outside the normalized original record.
    // The donor MOPP field location must not be walked after normalization.
    word(bsp.banks[0x30000],0x98,0xdeadbeef);word(bsp.banks[0x30000],0x9c,0x12345678);
    const auto b=bsp.fields("sbsp");
    require(b && b->size()==3 && has(*b,0x1013c,0x20000,0xc8) &&
        has(*b,0x200b4,0x30000,0xa0) && has(*b,0x30068,0x40000,3),
        "Normalized BSP physics retained donor MOPP pointer offset");
    word(bsp.banks[0x10000],0,1);
    rejected([&]{bsp.fields("sbsp");},"Undeclared BSP import block admitted");
    word(bsp.banks[0x10000],0,0);
    bsp.banks[0x50000].resize(65537*8);
    word(bsp.banks[0x10000],0x4c,65537);word(bsp.banks[0x10000],0x50,0x50000);
    require(has(*bsp.fields("sbsp"),0x10050,0x50000,65537*8),
        "32-bit surface-reference count narrowed to a tag datum index");

    Owners globals;globals.banks[0x10000].resize(0x284);globals.banks[0x70000].resize(16);globals.banks[0x80000].resize(5);
    word(globals.banks[0x10000],0x188,0x70000);word(globals.banks[0x10000],0x18c,0x80000);
    word(globals.banks[0x10000],0x190,2);word(globals.banks[0x10000],0x194,5);
    const auto g=globals.fields("matg");require(g && g->size()==2 && has(*g,0x10188,0x70000,16) && has(*g,0x1018c,0x80000,5),"Locale pointer extents differ");
    word(globals.banks[0x10000],0x188,0);word(globals.banks[0x10000],0x18c,0);
    require(globals.fields("matg")->empty(),"Unloaded authored locale counts treated as resident pointer owners");
    // Actual MCC has 0xcdcdcd00 in this runtime flag/padding word. It is
    // separate from the pointer fields and must not be reinterpreted as a
    // four-byte authored boolean or an address.
    word(globals.banks[0x10000],0x1a0,0xcdcdcd00);
    require(globals.fields("matg")->empty(),"Locale runtime padding interpreted as resident activation");
    globals.banks[0x90000].resize(0x108);
    word(globals.banks[0x10000],0x108,1);word(globals.banks[0x10000],0x10c,0x90000);
    word(globals.banks[0x90000],0x58,1);word(globals.banks[0x90000],0x5c,0xa0000);
    rejected([&]{globals.fields("matg");},"Unverified rasterizer bank bypassed ordinary traversal");
    const auto native_root=ordinary_mcc_pointer_fields(tag_groups::globals,0x10000,
        [&](auto at,auto size){return globals.read(at,size);},MccOrdinaryPointerFlavor::Mcc,
        [](auto at,auto size){return at==0x90000 && size==0x108;});
    require(native_root && native_root->size()==1 && has(*native_root,0x1010c,0x90000,0x108),
        "Closed native root skipped its parent relocation");
    globals.banks[0xa0000].resize(8);
    word(globals.banks[0x10000],0xd0,1);word(globals.banks[0x10000],0xd4,0xa0000);
    word(globals.banks[0xa0000],0,1);word(globals.banks[0xa0000],4,0xb0000);
    rejected([&]{ordinary_mcc_pointer_fields(tag_groups::globals,0x10000,
        [&](auto at,auto size){return globals.read(at,size);},MccOrdinaryPointerFlavor::Mcc,
        [](auto,auto){return true;});},"Closed rasterizer predicate bypassed unrelated gameplay definitions");
    require(!globals.fields("zzzz"),"Unknown group claimed complete");
    bool read_native=false;
    const auto native_shader=ordinary_mcc_pointer_fields(TagGroup{"shad"},0x10000,
        [&](auto,auto){read_native=true;return std::span<const std::byte>{};},MccOrdinaryPointerFlavor::Xbox);
    require(!native_shader && !read_native,"Native shader parsed through generic editor layout");

    std::size_t actual_tags=0,actual_fields=0;
    if(argc>1) {
        Cache cache(argv[1]);
        auto owners=cache.tags();
        owners.insert(owners.end(),cache.retained_tag_allocations().begin(),cache.retained_tag_allocations().end());
        for(const auto& tag:owners)if(tag.address && tag.size) {
            const auto flavor=cache.is_preserved_native_shader(tag)?MccOrdinaryPointerFlavor::Xbox:MccOrdinaryPointerFlavor::Mcc;
            const auto payload=cache.read_tag_bytes(tag.address,tag.size);
            const std::vector<std::byte> before(payload.begin(),payload.end());
            try {
                if(auto fields=ordinary_mcc_pointer_fields(tag.type(),tag.address,
                    [&](auto at,auto size){return cache.read_tag_bytes(at,size);},flavor,
                    [&](auto at,auto size){return std::ranges::any_of(cache.preserved_native_pointer_maps(),
                        [&](const auto& map){return map.owner==tag.handle() && map.address==at && map.bytes>=size;});})) {
                    ++actual_tags;actual_fields+=fields->size();
                }
            }catch(const FormatError&e){throw FormatError(tag.group+" "+tag.name+": "+e.what());}
            require(std::ranges::equal(before,cache.read_tag_bytes(tag.address,tag.size)),
                "Actual visitor changed an authored metadata allocation");
        }
    }
    std::cout<<"ordinary metadata pointer tests passed; actual tags="<<actual_tags<<" fields="<<actual_fields<<'\n';
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
