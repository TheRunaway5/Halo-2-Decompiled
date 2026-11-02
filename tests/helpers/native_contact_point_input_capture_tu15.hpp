#pragma once
#include "halo2/native_tu15_contact_point_input_observer.hpp"
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
namespace halo2::test {
class NativeContactPointInputCaptureTu15 {
    std::filesystem::path directory_;bool captured_{};
    static void require(bool yes,const char*why){if(!yes)throw std::runtime_error(why);}
public:
    explicit NativeContactPointInputCaptureTu15(std::filesystem::path path){
        const auto scratch=std::getenv("HALO2_TEST_SCRATCH");require(scratch&&*scratch,"Point input capture requires owned scratch");
        const auto owner=std::filesystem::weakly_canonical(scratch);directory_=std::filesystem::weakly_canonical(path);auto a=owner.begin(),b=directory_.begin();
        for(;a!=owner.end();++a,++b)require(b!=directory_.end()&&*a==*b,"Point input capture escapes owned scratch");require(b!=directory_.end(),"Point input capture requires a child directory");
    }
    template<class Memory>void observe(const NativeTu15ContactPointRemovalInput&input,const Memory&memory){
        if(captured_)return;
        require(input.owner==0x322f70u,"Point BEFORE requires actual322F70 owner");
        require(input.preserved_x87_depth==0,"Point BEFORE needs explicit nonempty x87 values");
        std::uint64_t bytes{};unsigned count{};memory.visit_committed([&](auto,std::span<const std::byte>bank){bytes+=bank.size();++count;});
        require(count<=8192u&&bytes<=192u*1024u*1024u,"Point input exceeds bounded committed bank profile");require(!std::filesystem::exists(directory_),"Point BEFORE directory already exists");std::filesystem::create_directories(directory_);
        nlohmann::json manifest{{"schema","native-tu15-contact-point-typed-before-v1"},{"reference","English TU1.5 only"},{"boundary","Actual selected contact-manager virtual8 point-removal322F70 input BEFORE genuine child call; not captured original GP/SIMD ABI"},
          {"input",{{"manager",input.manager},{"point",input.point},{"owner",input.owner},{"preserved_x87_depth",input.preserved_x87_depth},{"floating_point",{{"x87_control_word",input.floating_point.control_word},{"x87_status",input.floating_point.status},{"mxcsr",input.floating_point.mxcsr}}}}},
          {"bank_bytes",bytes},{"regions",nlohmann::json::array()},{"guest_abi_fabricated",false},{"source_memory_mutated",false},{"physics_result_supplied",false}};
        memory.visit_committed([&](std::uint32_t address,std::span<const std::byte>bank){const auto file="before-"+std::to_string(address)+".bin";std::ofstream out(directory_/file,std::ios::binary);out.write(reinterpret_cast<const char*>(bank.data()),bank.size());require(bool(out),"Cannot retain actual point BEFORE bank");manifest["regions"].push_back({{"address",address},{"size",bank.size()},{"before_file",file}});});
        std::ofstream out(directory_/"capture-native-before.json");out<<manifest.dump(2)<<'\n';require(bool(out),"Cannot retain point BEFORE manifest");captured_=true;
    }
};
} // namespace halo2::test
