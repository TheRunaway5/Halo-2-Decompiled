#pragma once
#include "halo2/native_tu15_camera_query_input_observer.hpp"
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
namespace halo2::test {
class NativeCameraQueryInputCaptureTu15 {
    std::filesystem::path directory_;bool captured_{};
    static void require(bool b,const char*s){if(!b)throw std::runtime_error(s);}
public:
    explicit NativeCameraQueryInputCaptureTu15(std::filesystem::path path){
        const auto scratch=std::getenv("HALO2_TEST_SCRATCH");require(scratch&&*scratch,"Camera query BEFORE requires owned scratch");const auto owner=std::filesystem::weakly_canonical(scratch);directory_=std::filesystem::weakly_canonical(path);auto a=owner.begin(),b=directory_.begin();for(;a!=owner.end();++a,++b)require(b!=directory_.end()&&*a==*b,"Camera query BEFORE escapes owned scratch");require(b!=directory_.end(),"Camera query BEFORE needs own child");
    }
    template<class Memory>void observe(const NativeTu15CameraQueryInput&input,const Memory&memory){
        if(captured_)return;require(input.owner==0x16c620u,"Camera query BEFORE requires actual owner");require(input.preserved_x87_depth==0,"Camera query BEFORE cannot invent live x87 values");require(input.origin&&input.displacement&&input.result,"Camera query BEFORE needs actual DATA pointers");
        std::uint64_t bytes{};unsigned count{};memory.visit_committed([&](auto,std::span<const std::byte>b){bytes+=b.size();++count;require(count<=8192u&&bytes<=192u*1024u*1024u,"Camera query BEFORE exceeds bounded banks");});require(!std::filesystem::exists(directory_),"Camera query BEFORE exists");std::filesystem::create_directories(directory_);
        nlohmann::json manifest{{"schema","native-tu15-camera-query-typed-before-v1"},{"reference","English TU1.5 only"},{"boundary","Actual native camera16C620 typed geometric-query service BEFORE unchanged refusal; no captured guest ABI"},{"input",{{"owner",input.owner},{"mask",input.mask},{"origin",input.origin},{"displacement",input.displacement},{"ignore_first",input.ignore_first},{"ignore_second",input.ignore_second},{"result",input.result},{"source_call_site",input.source_call_site},{"preserved_x87_depth",input.preserved_x87_depth},{"floating_point",{{"x87_control_word",input.floating_point.control_word},{"x87_status",input.floating_point.status},{"mxcsr",input.floating_point.mxcsr}}}}},{"bank_bytes",bytes},{"regions",nlohmann::json::array()},{"guest_abi_fabricated",false},{"source_memory_mutated",false},{"physics_result_supplied",false}};
        std::uint64_t written{};unsigned n{};memory.visit_committed([&](std::uint32_t address,std::span<const std::byte>b){written+=b.size();++n;require(n<=8192u&&written<=192u*1024u*1024u,"Camera query BEFORE changed beyond bounds");const auto name="before-"+std::to_string(address)+".bin";std::ofstream out(directory_/name,std::ios::binary);out.write(reinterpret_cast<const char*>(b.data()),b.size());require(bool(out),"Cannot retain camera query bank");manifest["regions"].push_back({{"address",address},{"size",b.size()},{"before_file",name}});});require(written==bytes&&n==count,"Camera query BEFORE regions changed");std::ofstream out(directory_/"capture-native-before.json");out<<manifest.dump(2)<<'\n';require(bool(out),"Cannot retain camera query manifest");captured_=true;
    }
};
}
