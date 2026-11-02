#pragma once
#include "halo2/native_tu15_pair_deletion_child_input_observer.hpp"
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
namespace halo2::test {
class NativePairDeletionChildInputCaptureTu15 {
    std::filesystem::path directory_;bool captured_{};
    static void require(bool yes,const char*why){if(!yes)throw std::runtime_error(why);}
public:
    explicit NativePairDeletionChildInputCaptureTu15(std::filesystem::path path){
        const auto scratch=std::getenv("HALO2_TEST_SCRATCH");require(scratch&&*scratch,"Pair deletion child capture requires owned scratch");
        const auto owner=std::filesystem::weakly_canonical(scratch);directory_=std::filesystem::weakly_canonical(path);
        auto a=owner.begin(),b=directory_.begin();for(;a!=owner.end();++a,++b)require(b!=directory_.end()&&*a==*b,"Pair child capture escapes owned scratch");
        require(b!=directory_.end(),"Pair child capture requires child directory");
    }
    template<class Memory>void observe(const NativeTu15PairDeletionChildInput&input,const Memory&memory){
        if(captured_||input.owner!=0x2f81b0u)return;
        require(input.preserved_x87_depth==0,"Pair child BEFORE cannot capture unprovided live x87 values");
        std::uint64_t bytes{};unsigned count{};memory.visit_committed([&](auto,std::span<const std::byte>bank){bytes+=bank.size();++count;});
        require(count<=8192u&&bytes<=192u*1024u*1024u,"Pair child BEFORE exceeds bounded banks");
        require(!std::filesystem::exists(directory_),"Pair child BEFORE directory exists");std::filesystem::create_directories(directory_);
        nlohmann::json receiver=nullptr;if(input.receiver)receiver=*input.receiver;
        nlohmann::json manifest{{"schema","native-tu15-pair-deletion-child-typed-before-v1"},{"reference","English TU1.5 only"},
          {"boundary","Actual typed nested owner BEFORE native pair-deletion refusal; no original GP/SIMD ABI"},
          {"input",{{"owner",input.owner},{"receiver",receiver},{"arguments",input.arguments},{"return_pop",input.return_pop},
            {"preserved_x87_depth",input.preserved_x87_depth},{"floating_point",{{"x87_control_word",input.floating_point.control_word},{"x87_status",input.floating_point.status},{"mxcsr",input.floating_point.mxcsr}}}}},
          {"bank_bytes",bytes},{"regions",nlohmann::json::array()},{"guest_abi_fabricated",false},{"source_memory_mutated",false},{"physics_result_supplied",false}};
        memory.visit_committed([&](std::uint32_t address,std::span<const std::byte>bank){const auto file="before-"+std::to_string(address)+".bin";
          std::ofstream out(directory_/file,std::ios::binary);out.write(reinterpret_cast<const char*>(bank.data()),bank.size());require(bool(out),"Cannot write pair child BEFORE bank");
          manifest["regions"].push_back({{"address",address},{"size",bank.size()},{"before_file",file}});});
        std::ofstream out(directory_/"capture-native-before.json");out<<manifest.dump(2)<<'\n';require(bool(out),"Cannot write pair child BEFORE manifest");captured_=true;
    }
};
}
