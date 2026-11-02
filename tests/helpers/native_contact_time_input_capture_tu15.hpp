#pragma once
#include "halo2/native_tu15_contact_time_input_observer.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
namespace halo2::test {
// Diagnostic only: retain one real eligible native3205B0 BEFORE input. The
// manifest has typed inputs, never a fabricated original entry ABI/postimage.
class NativeContactTimeInputCaptureTu15 {
    using Json=nlohmann::json;
    std::filesystem::path directory_;bool complete_{};
    static void require(bool yes,const char*why){if(!yes)throw std::runtime_error(why);}
    static std::optional<std::uint32_t>word(const SourceRuntimeMemory&memory,std::uint32_t p,unsigned width=4){
        const auto bytes=memory.try_read_view(p,width);if(!bytes)return{};std::uint32_t value{};
        for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>((*bytes)[n])<<(n*8u);return value;
    }
    static std::optional<Json>body(const SourceRuntimeMemory&memory,std::uint32_t payload){
        if(payload<16u)return{};const auto indirect=word(memory,payload+0x10u);if(!indirect||!*indirect)return{};
        const auto identity=*indirect,entity=payload-16u;const auto disabled=word(memory,identity+0x40u,1);
        if(!disabled||*disabled)return{};const auto motion=word(memory,identity+0x3cu);if(!motion||!*motion)return{};
        const auto table=word(memory,*motion);if(!table||!*table)return{};const auto owner=word(memory,*table+0x18u);if(!owner)return{};
        // Authenticated TU1.5 motion type getters. Unknown owners remain
        // ineligible; no original callback is run to manufacture admission.
        unsigned kind{};switch(*owner){case 0x31d520u:kind=2;break;case 0x31e2f0u:kind=3;break;case 0x31dd70u:kind=5;break;case 0x31edf0u:return{};case 0x31ede0u:kind=7;break;default:return{};}
        const auto count=word(memory,identity+0x34u),properties=word(memory,identity+0x30u);if(!count||!properties||*count>64u)return{};
        std::optional<std::uint32_t>datum,slot;Json actual_properties=Json::array();
        for(unsigned n=0;n<*count;++n){const auto key=word(memory,*properties+n*8u),value=word(memory,*properties+n*8u+4u);if(!key||!value)return{};actual_properties.push_back({{"key",*key},{"value",*value}});if(*key==0x2001u&&!datum)datum=*value;if(*key==0x2002u&&!slot)slot=*value;}
        if(!datum||*datum==0xffffffffu)return{};
        const auto entity_type=word(memory,entity+0x18u);
        return Json{{"payload",payload},{"entity",entity},{"entity_type",entity_type?Json(*entity_type):Json(nullptr)},{"body",identity},{"payload_plus10_body",*indirect},{"motion",*motion},{"motion_getter",*owner},{"motion_kind",kind},{"body_40",*disabled},{"component_datum",*datum},{"body_slot",slot?Json(*slot):Json(nullptr)},{"actual_properties",actual_properties}};
    }
    static void write(const std::filesystem::path&path,std::span<const std::byte>bytes){std::ofstream out(path,std::ios::binary);out.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());require(bool(out),"Cannot retain actual native contact-time BEFORE bank");}
public:
    explicit NativeContactTimeInputCaptureTu15(std::filesystem::path path):directory_(std::move(path)){
        const auto scratch=std::getenv("HALO2_TEST_SCRATCH");require(scratch&&*scratch,"Native contact-time capture requires owned test scratch");
        const auto owner=std::filesystem::weakly_canonical(scratch),target=std::filesystem::weakly_canonical(directory_);auto a=owner.begin(),b=target.begin();
        for(;a!=owner.end();++a,++b)require(b!=target.end()&&*a==*b,"Native contact-time capture escapes its owned test scratch");
        require(b!=target.end(),"Native contact-time capture requires its own child directory");directory_=target;
    }
    bool complete()const noexcept{return complete_;}
    void observe(const NativeTu15ContactTimeInput&input,const SourceRuntimeMemory&memory){
        if(complete_)return;const auto first_payload=word(memory,input.entry.identity),second_payload=word(memory,input.entry.identity+4u);
        if(!first_payload||!second_payload||*first_payload==*second_payload)return;
        const auto first=body(memory,*first_payload),second=body(memory,*second_payload);if(!first||!second)return;if(first->at("body")==second->at("body"))return;
        require(!std::filesystem::exists(directory_),"Native contact-time BEFORE destination already exists");std::filesystem::create_directories(directory_);
        Json record{{"schema","native-tu15-contact-time-typed-before-v1"},{"reference","English TU1.5 only"},{"boundary","Read-only actual native3205B0 typed input before first kernel access"},{"input",{{"island",input.island.identity},{"configuration",input.configuration.identity},{"entry",input.entry.identity},{"output",input.output.identity},{"time_bits",input.time_bits},{"floating_point",{{"x87_control_word",input.floating_point.control_word},{"x87_status",input.floating_point.status},{"mxcsr",input.floating_point.mxcsr}}},{"preserved_x87_depth",input.preserved_x87_depth}}},{"pair",{{"first",*first},{"second",*second}}},{"regions",Json::array()},{"captured_after_consumed",false},{"guest_abi_fabricated",false},{"source_memory_mutated",false},{"physics_result_supplied",false}};
        std::uint64_t bytes{};
        memory.visit_committed([&](std::uint32_t address,std::span<const std::byte>bank){const auto file="before-"+std::to_string(address)+".bin";write(directory_/file,bank);bytes+=bank.size();record["regions"].push_back({{"address",address},{"size",bank.size()},{"before_file",file}});});record["bank_bytes"]=bytes;
        std::ofstream out(directory_/"capture-native-before.json");out<<record.dump(2)<<'\n';require(bool(out),"Cannot retain typed native contact-time BEFORE manifest");complete_=true;
    }
};
} // namespace halo2::test
