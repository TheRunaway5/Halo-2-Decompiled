#include "halo2/cache.hpp"
#include "halo2/mcc_native_shaders.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool value,const char* why){if(!value)throw FormatError(why);}
std::vector<std::byte> hex(std::string_view value) {
    check(value.size()%2==0,"Fixture hex has an odd extent");std::vector<std::byte> out;
    for(unsigned i=0;i<value.size();i+=2)out.push_back(std::byte(std::stoul(std::string(value.substr(i,2)),nullptr,16)));
    return out;
}
Tag tag(const Json& value){return {value.at("group"),value.at("name"),value.at("datum"),value.at("address"),value.at("size")};}
struct Fixture {
    Tag shader,stem;std::string payload_name;std::vector<std::byte> payload;
    std::map<std::uint32_t,Tag> names;std::map<std::uint32_t,unsigned> frames;
    std::vector<std::pair<std::uint32_t,std::vector<std::byte>>> regions;
    explicit Fixture(const Json& root):shader(tag(root.at("source").at("shader"))),
        stem(tag(root.at("source").at("native_stem"))),payload_name(root.at("native_payload_name")),
        payload(hex(root.at("native_payload_hex").get<std::string>())) {
        const auto& source=root.at("source");
        for(const auto& value:source.at("namespace")){auto t=tag(value);names.emplace(t.datum,std::move(t));}
        for(const auto& [key,value]:source.at("bitmap_frame_counts").items())frames.emplace(std::stoul(key),value.get<unsigned>());
        for(const auto& value:source.at("regions"))regions.emplace_back(value.at(0),hex(value.at(1).get<std::string>()));
    }
    std::span<const std::byte> read(std::uint32_t at,std::size_t n)const {
        for(const auto& [base,bytes]:regions)if(at>=base && std::uint64_t(at)-base<=bytes.size() && n<=bytes.size()-(at-base))
            return std::span(bytes).subspan(at-base,n);
        throw FormatError("Glass fixture read escaped source allocation");
    }
    void mutate(std::uint32_t at) {
        bool found{};for(auto& [base,bytes]:regions)if(at>=base && std::uint64_t(at)-base<bytes.size()) {
            bytes[at-base]^=std::byte{1};found=true;
        }
        check(found,"Glass mutation escaped its fixture");
    }
    auto run(bool opaque=false)const {
        const MccShaderRead source=[&](auto at,auto n){return read(at,n);};
        const MccShaderFind find=[&](auto handle)->const Tag*{const auto at=names.find(handle);return at==names.end()?nullptr:&at->second;};
        const MccShaderBitmapFrame frame=[&](auto handle,auto index){const auto at=frames.find(handle);return at!=frames.end() && index<at->second;};
        return opaque?project_mcc_unanimated_tex_bump(shader,stem,payload_name,payload,source,find,frame):
            project_mcc_transparent_two_alpha_env_illum(shader,stem,payload_name,payload,source,find,frame);
    }
};
}
int main(int argc,char** argv)try {
    check(argc==2,"Paired glass metadata fixture required");Json root;std::ifstream(argv[1])>>root;
    Fixture fixture(root);const auto before=fixture.regions;const auto result=fixture.run();
    check(bool(result),"Authenticated complete transparent glass tuple was rejected");
    check(!fixture.run(true),"Historical opaque projector admitted a transparent material");
    const Bytes output(result->bytes);const auto pp=output.u32(36);
    check(output.u32(32)==1,"MCC triple postprocess bank was not projected to original shape");
    unsigned banks{};
    for(const auto& [field_text,expected]:root.at("original").at("banks").items()) {
        const auto field=unsigned(std::stoul(field_text)),count=expected.at("count").get<unsigned>(),stride=expected.at("stride").get<unsigned>();
        check(output.u32(pp+field)==count,"Projected glass bank count differs from original");
        const auto actual=output.slice(output.u32(pp+field+4),std::size_t(count)*stride);
        // Bitmap salted handles deliberately stay with the actual MCC owner;
        // each remaining authored byte must match the independently read Xbox.
        if(field!=4)check(std::ranges::equal(actual,hex(expected.at("hex").get<std::string>())),
            "Projected glass cooked bank differs from independently decoded original");
        else {
            const auto source_images=hex(root.at("source").at("banks").at("4").at("hex").get<std::string>());
            check(std::ranges::equal(actual,source_images),"Projection borrowed/replaced an authored MCC bitmap reference");
        }
        ++banks;
    }
    const Bytes source_root(fixture.read(fixture.shader.address,84));
    check(std::ranges::equal(output.slice(output.u32(16),80),fixture.read(source_root.u32(16),80)),
        "Projection changed authored runtime metadata");
    check(std::ranges::equal(output.slice(output.u32(48),output.u32(44)*8),fixture.read(source_root.u32(48),source_root.u32(44)*8)),
        "Projection changed actual bitmap predictions");
    std::set<unsigned> fields;
    for(const auto& [field,target]:result->pointers)
        check(fields.insert(field).second && target<result->bytes.size() && output.u32(field)==target,
            "Projected glass pointer map lost its owned relative target");
    check(fixture.regions==before,"Projection mutated shared source metadata");
    unsigned negatives{};
    const auto rejects=[&](auto change){auto bad=fixture;change(bad);check(!bad.run(),"Changed/unproved transparent tuple was admitted");++negatives;};
    for(unsigned field:{0u,4u,12u,20u,24u,28u,32u,40u,44u,52u,56u,60u,64u,68u,72u,76u,80u})
        rejects([&](auto& bad){bad.mutate(bad.shader.address+field);});
    const auto source_pp=source_root.u32(36);const Bytes definition(fixture.read(source_pp,124));
    for(unsigned field:{4u,12u,20u,28u,36u,44u,52u,60u,68u,76u,84u,92u,100u,108u,116u,120u})
        rejects([&](auto& bad){bad.mutate(source_pp+field);});
    for(unsigned field:{12u,20u,28u,36u,44u,52u,92u,100u,108u})
        rejects([&](auto& bad){bad.mutate(definition.u32(field+4));});
    for(unsigned index=0;index<5;++index) {
        const auto at=definition.u32(8)+index*12;
        rejects([&](auto& bad){bad.mutate(at);});
        rejects([&](auto& bad){bad.mutate(at+4);});
        rejects([&](auto& bad){bad.mutate(at+8);});
        const auto handle=Bytes(fixture.read(at,4)).u32(0);
        rejects([&](auto& bad){bad.names.at(handle).name+="_unproved";});
        rejects([&](auto& bad){bad.frames.at(handle)=0;});
        rejects([&](auto& bad){bad.mutate(source_root.u32(48)+index*8);});
    }
    for(unsigned field:{0u,4u,8u,12u,44u,48u,52u,56u,60u,64u,68u,72u,76u})
        rejects([&](auto& bad){bad.mutate(source_root.u32(16)+field);});
    rejects([](auto& bad){bad.payload.back()^=std::byte{1};});
    rejects([](auto& bad){bad.payload_name="unverified.bin";});
    rejects([](auto& bad){bad.stem.name="unverified\\template";});
    std::cout<<"PASS paired original glass banks="<<banks<<", rejection controls="<<negatives
        <<"; exact source-owned bitmaps/runtime/predictions, independently proved original pixel ordering and isolated opaque API\n";
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
