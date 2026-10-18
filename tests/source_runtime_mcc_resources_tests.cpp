#include "halo2/source_runtime_resources.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <zlib.h>

namespace {
using namespace halo2;
using Buffer=std::vector<std::byte>;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void word(Buffer& bytes,std::size_t offset,std::uint32_t value) {
    for(unsigned n=0;n<4;++n)bytes.at(offset+n)=std::byte(value>>(8*n));
}
template<class F>void rejects(F&& operation) {
    try{operation();}catch(const FormatError&){return;}
    throw std::runtime_error("Malformed MCC resource request was admitted");
}
void save(const std::filesystem::path& path,const Buffer& bytes) {
    std::ofstream out(path,std::ios::binary);
    out.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
    check(bool(out),"Cannot write MCC resource fixture");
}
Buffer logical(unsigned seed) {
    Buffer bytes(8192);
    word(bytes,0,0x68656164);word(bytes,4,13);word(bytes,8,bytes.size());
    word(bytes,16,1024);word(bytes,32,2);word(bytes,36,1600);word(bytes,40,64);word(bytes,44,1536);
    word(bytes,720,1024+0x100000-2048);
    word(bytes,1024,0x401000+32);word(bytes,1028,1);word(bytes,1032,0x401000+44);
    word(bytes,1036,0xffffffffu);word(bytes,1040,0xffffffffu);word(bytes,1048,2);word(bytes,1052,0x74616773);
    word(bytes,1056,tag_groups::bitmap.value);word(bytes,1060,0xffffffffu);word(bytes,1064,0xffffffffu);
    word(bytes,1072,0xffffffffu);
    word(bytes,1084,tag_groups::bitmap.value);word(bytes,1088,0xe1000001u);
    word(bytes,1092,0x100000);word(bytes,1096,128);
    constexpr std::string_view name="textures\\resource_fixture";
    for(unsigned n=0;n<name.size();++n)bytes[1600+n]=std::byte(name[n]);
    for(unsigned n=0;n<32;++n)bytes[4096+n]=std::byte(seed+n);
    return bytes;
}
Buffer compressed(const Buffer& source) {
    Buffer bytes(source.begin(),source.begin()+896);
    word(bytes,28,1);word(bytes,776,source.size());word(bytes,780,904);word(bytes,784,896);word(bytes,788,1);
    uLongf extent=compressBound(source.size()-896);Buffer payload(extent);
    check(compress2(reinterpret_cast<Bytef*>(payload.data()),&extent,
        reinterpret_cast<const Bytef*>(source.data()+896),source.size()-896,Z_BEST_SPEED)==Z_OK,
        "Cannot compress MCC resource fixture");
    bytes.resize(904);word(bytes,896,extent);word(bytes,900,904);
    bytes.insert(bytes.end(),payload.begin(),payload.begin()+extent);return bytes;
}
}
int main(int argc,char** argv)try {
    using namespace halo2;
    struct Directory {
        std::filesystem::path path;
        ~Directory(){std::error_code error;std::filesystem::remove_all(path,error);}
    } directory{std::filesystem::temp_directory_path()/
        ("halo2-mcc-resources-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))};
    std::filesystem::create_directory(directory.path);
    const auto local=logical(17);
    const std::array shared{logical(41),logical(73),logical(105)};
    const std::array names{"mainmenu.map","shared.map","single_player_shared.map"};
    save(directory.path/"fixture.map",compressed(local));
    for(unsigned n=0;n<3;++n)save(directory.path/names[n],compressed(shared[n]));
    check(std::filesystem::file_size(directory.path/"shared.map")<4096,
        "Compressed resource fixture does not distinguish stored and logical offsets");
    Cache cache(directory.path/"fixture.map");SourceRuntimeResources resources(cache);
    const auto identity=resources.identity();
    for(unsigned n=0;n<4;++n) {
        const auto result=resources.load((n<<30)|4096u,32);
        const auto& expected=n?shared[n-1]:local;
        check(std::ranges::equal(result,std::span(expected).subspan(4096,32)),
            "MCC source resource used compressed file offsets or wrong cache selector");
    }
    for(unsigned n=0;n<3;++n) {
        save(directory.path/"replacement",Buffer(16));
        std::filesystem::rename(directory.path/"replacement",directory.path/names[n]);
        check(std::ranges::equal(resources.load(((n+1)<<30)|4096u,32),std::span(shared[n]).subspan(4096,32)),
            "Bound MCC resource reopened a replaced shared path");
    }
    check(resources.identity()==identity,"Bound resource identity changed with external filenames");
    rejects([&]{SourceRuntimeResources malformed(cache);});
    for(const auto request:std::array<std::pair<std::uint32_t,std::uint32_t>,5>{{
        {4096,0},{4096,128u*1024*1024+1},{8190,4},{0x80001ffeu,4},{8192,1}}})
        rejects([&]{(void)resources.load(request.first,request.second);});
    if(argc==2) {
        Cache actual(argv[1]);
        const Tag* selected{};std::uint32_t descriptor{};
        for(const auto& tag:actual.select_tags({.group=tag_groups::model_animation_graph,.require_address=true})) {
            const Bytes root(actual.read_tag_bytes(tag.address,0xb4));
            if(root.u32(0xac)){selected=&tag;descriptor=root.u32(0xb0);break;}
        }
        check(actual.is_mcc()&&selected,"Actual MCC cache has no normalized animation descriptor");
        constexpr std::uint32_t runtime_descriptor=0x8a000000u;unsigned calls{};
        SourceRuntimeResources translated(actual,[&](std::uint32_t address,std::size_t size) {
            check(address==runtime_descriptor&&size==20,"Descriptor translation received an untyped domain");
            ++calls;return descriptor;
        });
        const auto expected=actual.load_source_runtime_animation_image(descriptor);
        const auto image=translated.animation(runtime_descriptor);
        check(calls==1&&image.descriptor==expected.descriptor&&image.bytes==expected.bytes,
            "Canonical animation descriptor did not retain exact source resource ownership");
        const Bytes fields(image.descriptor);const auto size=fields.u32(4),offset=fields.u32(8);
        if(size) {
            const auto padded=(size+511u)&~511u;
            check(translated.load(offset,padded)==actual.load_source_runtime_resource(offset,padded),
                "Generated animation sector delivery changed normalized bytes");
        }
    }else check(argc==1,"Supply only an optional actual MCC map");
    std::cout<<"PASS MCC decoded local/shared resources, pinned ownership, bounds and typed descriptor translation\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
