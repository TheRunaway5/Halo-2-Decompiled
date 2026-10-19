#include "halo2/native_tu15_world_memory.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <ranges>

using namespace halo2;
namespace {
void check(bool condition,const char *message) {
    if(!condition)throw std::runtime_error(message);
}
std::vector<std::byte> decode(const std::string &hex) {
    check(hex.size()%2==0,"Malformed original weapon-table bytes");
    std::vector<std::byte> result;
    for(std::size_t i=0;i<hex.size();i+=2)result.push_back(std::byte(std::stoul(hex.substr(i,2),nullptr,16)));
    return result;
}
std::uint32_t word(const SourceRuntimeMemory &memory,std::uint32_t address) {
    const auto bytes=memory.view(address,4);std::uint32_t result{};
    for(unsigned i=0;i<4;++i)result|=std::to_integer<unsigned>(bytes[i])<<(i*8);
    return result;
}
}
int main(int argc,char **argv)try {
    check(argc==2,"Supply offline original weapon-table/selector fixture");
    std::ifstream input(argv[1]);check(bool(input),"Missing original weapon-table fixture");
    const auto fixture=nlohmann::json::parse(input);
    check(fixture.at("schema")=="H2_TU15_WEAPON_RUNTIME_TABLES_V1" &&
        fixture.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
        fixture.at("owner")==0x1030f0u,"Weapon fixture lost its complete original owner/authority");
    auto memory=native_tu15_world_memory({0x60212000u,
        {std::byte{0x78},std::byte{0x56},std::byte{0x34},std::byte{0x12}},
        "weapon table test retained native clock"});
    const auto snapshot=memory.checkpoint();
    unsigned words{},selectors{},new_words{};
    for(const auto &table:fixture.at("tables")) {
        const auto address=table.at("address").get<std::uint32_t>();
        const auto count=table.at("count").get<unsigned>();
        const auto expected=decode(table.at("original_hex"));
        check(expected.size()==count*4 && table.at("width")==4 && table.at("section").at("read_only")==true,
            "Original bounded weapon table extent/access differs");
        std::vector<std::byte> actual(expected.size());memory.read(address,actual);
        check(actual==expected,"Cold native weapon table differs from original TU1.5 bytes");
        for(unsigned i=0;i<count;++i) {
            const auto at=address+i*4;
            check(memory.is_read_only(at,4),"Original weapon table became writable");
            unsigned owners{};
            for(const auto &region:snapshot.regions)
                if(at>=region.address && std::uint64_t(at)+4<=std::uint64_t(region.address)+region.bytes.size()) {
                    ++owners;
                    if(address==0x104684u || address==0x104694u) {
                        check(region.owner=="native TU1.5 whole1030F0 bounded weapon dispatch target",
                            "New weapon word lost its named immutable owner");
                        ++new_words;
                    }
                }
            check(owners==1,"Weapon table word acquired a duplicate or absent storage owner");
            auto changed=std::array{std::byte{0xff},std::byte{0xff},std::byte{0xff},std::byte{0xff}};
            bool rejected{};try{memory.write(at,changed);}catch(const std::exception&){rejected=true;}
            check(rejected && word(memory,at)==table.at("entries").at(i),
                "Native weapon dispatch word accepted a write or changed afterward");
            ++words;
        }
        for(const auto &selector:table.at("selectors")) {
            const auto index=selector.at("index").get<std::int32_t>();
            const auto target=index>=0 && std::uint32_t(index)<count?
                word(memory,address+std::uint32_t(index)*4):table.at("default_target").get<std::uint32_t>();
            check(target==selector.at("target"),"Cold table target differs from untouched original selector execution");
            ++selectors;
        }
    }
    check(words==26 && new_words==8 && selectors==51,"Incomplete whole-owner weapon table/selector coverage");
    unsigned method_words{};
    for(const auto &table:fixture.at("method_tables")) {
        const auto address=table.at("address").get<std::uint32_t>();
        const auto count=table.at("count").get<unsigned>();
        const auto expected=decode(table.at("original_hex"));
        check(expected.size()==count*4 && table.at("next_boundary")==address+expected.size() &&
            table.at("section").at("read_only")==true && !table.at("original_installers").empty(),
            "Contact interface lost its original installer, boundary or access");
        std::vector<std::byte> actual(expected.size());memory.read(address,actual);
        check(actual==expected,"Cold contact interface differs from original TU1.5 method identities");
        for(unsigned i=0;i<count;++i) {
            const auto at=address+i*4;unsigned owners{};
            for(const auto &region:snapshot.regions)
                if(at>=region.address && std::uint64_t(at)+4<=std::uint64_t(region.address)+region.bytes.size()) {
                    ++owners;
                    // Whole185111's independently published two-method query
                    // can own these same original words before the charge
                    // interface declaration shares their immutable backing.
                    const bool shared_query=(at==0x45f8c0u || at==0x45f8c4u) &&
                        region.owner=="native TU1.5 authored 185111 physical stack query identities";
                    check(region.owner=="native TU1.5 authored charge projectile contact interface identity" || shared_query,
                        "Contact method lost its canonical immutable owner");
                }
            check(owners==1 && memory.is_read_only(at,4),"Contact method has overlapping or writable ownership");
            bool rejected{};try {memory.write(at,std::array<std::byte,4>{});}
            catch(const std::exception&){rejected=true;}
            check(rejected && word(memory,at)==table.at("entries").at(i),
                "Contact method accepted a write or changed its original target");
            ++method_words;
        }
    }
    check(method_words==10,"Incomplete authored contact interfaces");
    std::cout<<"PASS cold factory original weapon tables:26 words/104 bytes,8 newly owned words,51 original guarded selectors; all immutable and uniquely owned\n";
    std::cout<<"PASS actual charge projectile contact interfaces:3 complete tables/10 original methods/40 immutable bytes\n";
    std::cout<<"LIMIT original cmp/ja/jmp oracle uses declared selector registers; MCC charge composition is a separate native run\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
