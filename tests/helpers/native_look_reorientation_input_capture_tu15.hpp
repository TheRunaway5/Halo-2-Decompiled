#pragma once
#include "halo2/native_tu15_look_reorientation_input_observer.hpp"
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
namespace halo2::test {
class NativeLookReorientationInputCaptureTu15 {
    std::filesystem::path directory_;bool captured_{};
    static void require(bool yes,const char*why){if(!yes)throw std::runtime_error(why);}
public:
    explicit NativeLookReorientationInputCaptureTu15(std::filesystem::path path){
        const auto scratch=std::getenv("HALO2_TEST_SCRATCH");require(scratch&&*scratch,"Look reorientation capture requires owned scratch");
        const auto owner=std::filesystem::weakly_canonical(scratch);directory_=std::filesystem::weakly_canonical(path);
        auto a=owner.begin(),b=directory_.begin();for(;a!=owner.end();++a,++b)require(b!=directory_.end()&&*a==*b,"Look reorientation capture escapes owned scratch");
        require(b!=directory_.end(),"Look reorientation capture requires child directory");
    }
    template<class Memory>void observe(const NativeTu15LookReorientationInput&input,const Memory&memory){
        if(captured_)return;
        require(input.preserved_x87_depth==0,"Look reorientation BEFORE cannot capture unprovided x87 values");
        require(input.forward,"Look reorientation BEFORE requires actual nonnull formals");
        std::uint64_t bytes{};unsigned count{};std::array<std::uint32_t,3>words{};std::array<bool,3>found{};
        memory.visit_committed([&](std::uint32_t address,std::span<const std::byte>bank){
            bytes+=bank.size();++count;require(count<=8192u&&bytes<=192u*1024u*1024u,"Look reorientation BEFORE exceeds bounded banks");
            for(unsigned lane=0;lane<3;++lane){const auto at=input.forward+lane*4u;
                if(at<address||std::uint64_t(at-address)+4u>bank.size())continue;
                for(unsigned n=0;n<4;++n)words[lane]|=std::to_integer<unsigned>(bank[at-address+n])<<(n*8u);found[lane]=true;
            }
        });
        require(std::all_of(found.begin(),found.end(),[](bool yes){return yes;})&&words==input.forward_words,"Look reorientation BEFORE direction words differ from actual forward DATA");
        require(!std::filesystem::exists(directory_),"Look reorientation BEFORE directory exists");std::filesystem::create_directories(directory_);
        nlohmann::json manifest{{"schema","native-tu15-look-reorientation-typed-before-v1"},{"reference","English TU1.5 only"},
          {"boundary","Actual Host reorient_retained_input typed18A2A0 BEFORE unchanged original call; no GP/SIMD ABI"},
          {"input",{{"owner",0x18a2a0u},{"control_index",input.control_index},{"forward",input.forward},{"forward_words",input.forward_words},{"caller_player",input.caller_player},{"caller_unit",input.caller_unit},
            {"preserved_x87_depth",input.preserved_x87_depth},{"floating_point",{{"x87_control_word",input.floating_point.control_word},{"x87_status",input.floating_point.status},{"mxcsr",input.floating_point.mxcsr}}}}},
          {"bank_bytes",bytes},{"regions",nlohmann::json::array()},{"guest_abi_fabricated",false},{"source_memory_mutated",false},{"physics_result_supplied",false}};
        std::uint64_t written_bytes{};unsigned written_count{};
        memory.visit_committed([&](std::uint32_t address,std::span<const std::byte>bank){
            written_bytes+=bank.size();++written_count;require(written_count<=8192u&&written_bytes<=192u*1024u*1024u,"Look reorientation BEFORE changed beyond bounded banks");
            const auto file="before-"+std::to_string(address)+".bin";std::ofstream out(directory_/file,std::ios::binary);
            out.write(reinterpret_cast<const char*>(bank.data()),bank.size());require(bool(out),"Cannot write look reorientation BEFORE bank");
            manifest["regions"].push_back({{"address",address},{"size",bank.size()},{"before_file",file}});
        });
        require(written_bytes==bytes&&written_count==count,"Look reorientation BEFORE metadata changed during serialization");
        std::ofstream out(directory_/"capture-native-before.json");out<<manifest.dump(2)<<'\n';require(bool(out),"Cannot write look reorientation BEFORE manifest");captured_=true;
    }
};
} // namespace halo2::test
