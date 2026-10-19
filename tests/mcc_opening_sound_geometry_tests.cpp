#include "halo2/cache.hpp"
#include "halo2/mcc_runtime_delivery.hpp"
#include "halo2/mcc_sound_geometry.hpp"
#include "halo2/mcc_sound_runtime_delivery.hpp"
#include "halo2/source_runtime_resources.hpp"
#include "halo2/native_tu15_world_memory.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <set>

using namespace halo2;
namespace {
void check(bool value,const char* message){if(!value)throw FormatError(message);}
template<class F>void rejects(F f){try{f();}catch(const FormatError&){return;}throw FormatError("Unowned facial resource request admitted");}
std::vector<std::byte> decode(const std::string& text) {
    std::vector<std::byte> result;
    for(std::size_t n=0;n<text.size();n+=2)result.push_back(std::byte(std::stoul(text.substr(n,2),nullptr,16)));
    return result;
}
void word(std::vector<std::byte>& bytes,unsigned at,unsigned value) {
    for(unsigned n=0;n<4;++n)bytes.at(at+n)=std::byte(value>>(8*n));
}
}
int main(int argc,char**argv)try {
    if(argc!=3||!std::filesystem::is_regular_file(argv[1]))return 77;
    Cache cache(argv[1]);check(cache.is_mcc()&&cache.name()=="00a_introduction","Actual MCC intro required");
    std::ifstream fixture_file(argv[2]);const auto fixture=nlohmann::json::parse(fixture_file);
    auto cold=native_tu15_world_memory({0x60212000u,{},"declared facial-input test clock"});
    for(const auto& operand:fixture.at("speech_readonly_operands")) {
        const auto expected=decode(operand.at("bytes_hex"));const auto at=operand.at("address").get<unsigned>();
        check(cold.is_read_only(at,expected.size())&&std::ranges::equal(cold.view(at,expected.size()),expected),
            "Complete original speech/facial module has a missing or changed immutable input");
    }
    const auto& scratch=fixture.at("model_pose_scratch");
    const auto scratch_address=scratch.at("address").get<unsigned>(),scratch_bytes=scratch.at("bytes").get<unsigned>();
    check(!cold.is_read_only(scratch_address,scratch_bytes)&&
        std::ranges::all_of(cold.view(scratch_address,scratch_bytes),[](auto b){return b==std::byte{};}),
        "Original model-pose matrix scratch lacks its complete cold writable owner");
    const auto resources=plan_mcc_sound_geometry_resources(cache);
    SourceRuntimeResources io(cache);
    std::optional<MccSoundRuntimePlan> sound;
    const auto delivery=MccRuntimeCacheDelivery::plan(cache,0x90000000u,0x60000000u,
        [&](MccSoundRuntimePlan&& value){sound.emplace(std::move(value));});
    check(sound.has_value(),"Canonical metadata omitted its sound owner");
    const auto image=delivery.tag_image();
    const auto read=[&](unsigned address,unsigned bytes) {
        for(const auto& region:image.regions)if(address>=region.address&&
            std::uint64_t(address)-region.address+bytes<=region.bytes.size())
            return Bytes(region.bytes).slice(address-region.address,bytes);
        throw FormatError("Canonical facial descriptor has no native metadata allocation");
    };
    std::ifstream english(cache.shared_directory()/"sounds_en.dat",std::ios::binary);
    check(bool(english),"Actual English DAT absent");
    unsigned paired{};std::size_t payload_bytes{};std::set<unsigned> tokens;
    for(const auto& resource:resources) {
        check(resource.encoded_offset%512==0 && resource.encoded_offset>=cache.source_runtime_cache_bytes().size()&&
            tokens.insert(resource.encoded_offset).second,"Facial native token aliases source bytes or another resource");
        std::vector<std::byte> actual(resource.declared_bytes);
        english.seekg(resource.source_file_offset);english.read(reinterpret_cast<char*>(actual.data()),actual.size());
        check(bool(english),"Actual English facial payload truncated");
        check(io.load(resource.encoded_offset,resource.declared_bytes)==actual,"MCC facial payload bytes changed");
        check(io.load(resource.encoded_offset,resource.sector.size())==resource.sector,"Native facial sector differs from its real owned bytes");
        check(std::ranges::all_of(std::span(resource.sector).subspan(actual.size()),[](auto b){return b==std::byte{};}),
            "Facial sector consumes the following DAT payload");
        auto copy=io.load(resource.encoded_offset,resource.declared_bytes);copy[0]^=std::byte{1};
        check(io.load(resource.encoded_offset,resource.declared_bytes)==actual,"Caller mutation changed retained MCC facial ownership");
        rejects([&]{(void)io.load(resource.encoded_offset,resource.declared_bytes-1);});
        rejects([&]{(void)io.load(resource.encoded_offset+1,resource.declared_bytes);});
        const auto layout=std::ranges::find(sound->gestalts,resource.gestalt,&MccSoundRuntimeGestalt::owner);
        check(layout!=sound->gestalts.end(),"Facial owner lost its exact merged gestalt interval");
        const auto row=sound->blocks[10].address+(layout->first[10]+resource.index)*0x2cu+8u;
        const auto before=cache.read_tag_bytes(resource.source_descriptor,36);
        std::vector<std::byte> expected(before.begin(),before.end());
        word(expected,0,resource.encoded_offset);
        word(expected,20,delivery.runtime_address(Bytes(before).u32(20),Bytes(before).u32(16)*16u));
        check(std::ranges::equal(read(row,36),expected),"Facial projection changed another original resource-header field");
        if(resource.gestalt.value==fixture.at("gestalt")&&resource.index==fixture.at("index")) {
            check(resource.source_file_offset==fixture.at("mcc_english_offset")&&actual==decode(fixture.at("payload_hex")),
                "Whole actual MCC English facial block differs from the original reference");
            rejects([&]{(void)io.load(resource.source_file_offset,resource.declared_bytes);});++paired;
        }
        payload_bytes+=actual.size();
    }
    check(resources.size()>31&&paired==1,"Actual opening facial resource/reference cohort missing");
    std::cout<<"PASS "<<resources.size()<<" actual MCC facial resources, "<<payload_bytes
             <<" exact DAT bytes, original complete block, native header fields and bounded sector ownership\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
