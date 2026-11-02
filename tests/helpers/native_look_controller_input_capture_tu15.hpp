#pragma once
#include "halo2/native_tu15_look_controller_input_observer.hpp"
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
namespace halo2::test {
// One genuinely positive current control record, captured just before the
// existing18A300 call. Skipping NONE is only a read-only diagnostic filter.
class NativeLookControllerInputCaptureTu15 {
    std::filesystem::path directory_;bool complete_{};
    std::uint64_t skipped_inactive_{};
    static void require(bool yes,const char*why){if(!yes)throw std::runtime_error(why);}
    static std::uint32_t word(const SourcePhysicsContactMemory&memory,std::uint32_t p){
        std::array<std::byte,4>b{};memory.read(p,b);std::uint32_t n{};
        for(unsigned k=0;k<4;++k)n|=std::to_integer<unsigned>(b[k])<<(k*8u);return n;
    }
public:
    explicit NativeLookControllerInputCaptureTu15(std::filesystem::path path){
        const auto scratch=std::getenv("HALO2_TEST_SCRATCH");
        require(scratch&&*scratch,"Look BEFORE capture requires owned scratch");
        const auto owner=std::filesystem::weakly_canonical(scratch);
        directory_=std::filesystem::weakly_canonical(path);
        auto a=owner.begin(),b=directory_.begin();
        for(;a!=owner.end();++a,++b)
            require(b!=directory_.end()&&*a==*b,"Look capture escapes owned scratch");
        require(b!=directory_.end(),"Look capture requires its own child directory");
    }
    bool complete()const noexcept{return complete_;}
    template<class Memory>void observe(const NativeTu15LookControllerInput&input,
        const Memory&memory){
        if(complete_)return;
        require(input.preserved_x87_depth==0,"Look BEFORE cannot capture unprovided live x87 values");
        const auto controls=word(memory,0x4f9b54u);
        const auto record=controls+input.control_index*0x94u+0x14u;
        const auto unit=word(memory,record);
        if(unit==0xffffffffu){++skipped_inactive_;return;}
        std::uint64_t bytes{};unsigned count{};
        memory.visit_committed([&](auto,std::span<const std::byte>bank){bytes+=bank.size();++count;});
        require(count<=8192u&&bytes<=192u*1024u*1024u,"Look BEFORE exceeds bounded banks");
        require(!std::filesystem::exists(directory_),"Look BEFORE directory already exists");
        std::filesystem::create_directories(directory_);
        nlohmann::json manifest{{"schema","tu15-look-controller-typed-before-v1"},
          {"reference","English TU1.5 only"},
          {"boundary","Actual prepare_player_look typed18A300 formals BEFORE existing run_actual; no captured guest register ABI"},
          {"input",{{"owner",0x18a300u},{"control_index",input.control_index},
            {"yaw_delta_bits",input.yaw_delta_bits},{"pitch_delta_bits",input.pitch_delta_bits},
            {"floating_point",{{"x87_control_word",input.floating_point.control_word},
              {"x87_status",input.floating_point.status},{"mxcsr",input.floating_point.mxcsr}}},
            {"preserved_x87_depth",input.preserved_x87_depth}}},
          {"caller",{{"player",input.caller_player},{"unit",input.caller_unit}}},
          {"actual_control_record",record},{"actual_record_unit",unit},
          {"record_matches_caller_unit",unit==input.caller_unit},
          {"skipped_none_inputs",skipped_inactive_},{"bank_bytes",bytes},
          {"regions",nlohmann::json::array()},{"guest_abi_fabricated",false},
          {"source_memory_mutated",false},{"physics_result_supplied",false},
          {"native_controller_admission_added",false}};
        memory.visit_committed([&](std::uint32_t address,std::span<const std::byte>bank){
            const auto name="before-"+std::to_string(address)+".bin";
            std::ofstream out(directory_/name,std::ios::binary);
            out.write(reinterpret_cast<const char*>(bank.data()),bank.size());
            require(bool(out),"Cannot retain actual look BEFORE bank");
            manifest["regions"].push_back({{"address",address},{"size",bank.size()},{"before_file",name}});
        });
        std::ofstream out(directory_/"capture-native-before.json");out<<manifest.dump(2)<<'\n';
        require(bool(out),"Cannot retain actual look BEFORE manifest");complete_=true;
    }
};
} // namespace halo2::test
