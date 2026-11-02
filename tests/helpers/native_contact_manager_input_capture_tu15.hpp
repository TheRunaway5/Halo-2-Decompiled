#pragma once
#include "halo2/native_tu15_contact_manager_input_observer.hpp"
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <optional>
namespace halo2::test {
class NativeContactManagerInputCaptureTu15 {
    std::filesystem::path directory_;bool captured_{};
    static void require(bool yes,const char*why){if(!yes)throw std::runtime_error(why);}
public:
    explicit NativeContactManagerInputCaptureTu15(std::filesystem::path path){
        const auto scratch=std::getenv("HALO2_TEST_SCRATCH");require(scratch&&*scratch,"Manager input capture requires owned scratch");
        const auto owner=std::filesystem::weakly_canonical(scratch);directory_=std::filesystem::weakly_canonical(path);auto a=owner.begin(),b=directory_.begin();
        for(;a!=owner.end();++a,++b)require(b!=directory_.end()&&*a==*b,"Manager input capture escapes owned scratch");require(b!=directory_.end(),"Manager input capture requires a child directory");
    }
    template<class Memory>void observe(const NativeTu15ContactManagerCleanupInput&input,const Memory&memory){
        if(captured_)return;
        if(std::getenv("HALO2_TU15_CONTACT_MANAGER_CAPTURE_OWNED8")) {
            const auto word=[&](std::uint32_t address){
                std::optional<std::uint32_t>value;
                memory.visit_committed([&](std::uint32_t first,std::span<const std::byte>bank){
                    if(address>=first&&std::uint64_t(address)+4u<=std::uint64_t(first)+bank.size()){
                        std::uint32_t raw{};for(unsigned n=0;n<4;++n)raw|=std::to_integer<unsigned>(bank[address-first+n])<<(n*8u);value=raw;
                    }
                });return value;
            };
            const auto cap98=word(input.manager+0x10u+0x98u),cap3c=word(input.manager+0x10u+0x3cu);
            if(!cap98||!cap3c||*cap98!=8u||*cap3c!=8u)return;
        }
        require(input.preserved_x87_depth==0,"Manager BEFORE needs explicit nonempty x87 values");
        std::uint64_t bytes{};unsigned count{};memory.visit_committed([&](auto,std::span<const std::byte>bank){bytes+=bank.size();++count;});
        require(count<=8192u&&bytes<=192u*1024u*1024u,"Manager input exceeds bounded committed bank profile");require(!std::filesystem::exists(directory_),"Manager BEFORE directory already exists");std::filesystem::create_directories(directory_);
        nlohmann::json manifest{{"schema","native-tu15-contact-manager-typed-before-v1"},{"reference","English TU1.5 only"},{"boundary","Actual selected native3232B0 manager cleanup input BEFORE admission guard; not captured original GP/SIMD ABI"},
          {"input",{{"manager",input.manager},{"disposing",input.disposing},{"owner",0x3232b0u},{"preserved_x87_depth",input.preserved_x87_depth},{"floating_point",{{"x87_control_word",input.floating_point.control_word},{"x87_status",input.floating_point.status},{"mxcsr",input.floating_point.mxcsr}}}}},
          {"bank_bytes",bytes},{"regions",nlohmann::json::array()},{"guest_abi_fabricated",false},{"source_memory_mutated",false},{"physics_result_supplied",false}};
        memory.visit_committed([&](std::uint32_t address,std::span<const std::byte>bank){const auto file="before-"+std::to_string(address)+".bin";std::ofstream out(directory_/file,std::ios::binary);out.write(reinterpret_cast<const char*>(bank.data()),bank.size());require(bool(out),"Cannot retain actual manager BEFORE bank");manifest["regions"].push_back({{"address",address},{"size",bank.size()},{"before_file",file}});});
        std::ofstream out(directory_/"capture-native-before.json");out<<manifest.dump(2)<<'\n';require(bool(out),"Cannot retain manager BEFORE manifest");captured_=true;
    }
};
} //namespace halo2::test
