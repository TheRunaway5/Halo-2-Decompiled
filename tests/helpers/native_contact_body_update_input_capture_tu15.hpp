#pragma once
#include "halo2/native_tu15_contact_body_update_input_observer.hpp"
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
namespace halo2::test {
class NativeContactBodyUpdateInputCaptureTu15 {
    std::filesystem::path directory_;bool captured_{};
    static void require(bool yes,const char*why){if(!yes)throw std::runtime_error(why);}
public:
    explicit NativeContactBodyUpdateInputCaptureTu15(std::filesystem::path path){
        const auto scratch=std::getenv("HALO2_TEST_SCRATCH");require(scratch&&*scratch,"Body update capture requires owned scratch");
        const auto owner=std::filesystem::weakly_canonical(scratch);directory_=std::filesystem::weakly_canonical(path);
        auto a=owner.begin(),b=directory_.begin();for(;a!=owner.end();++a,++b)require(b!=directory_.end()&&*a==*b,"Body update capture escapes owned scratch");
        require(b!=directory_.end(),"Body update capture requires child directory");
    }
    template<class Memory>void observe(const NativeTu15ContactBodyUpdateInput&input,const Memory&memory){
        if(captured_)return;
        require(input.preserved_x87_depth==0,"Body update BEFORE cannot capture unprovided live x87 values");
        std::uint64_t bytes{};unsigned count{};memory.visit_committed([&](auto,std::span<const std::byte>bank){bytes+=bank.size();++count;});
        require(count<=8192u&&bytes<=192u*1024u*1024u,"Body update BEFORE exceeds bounded banks");
        require(!std::filesystem::exists(directory_),"Body update BEFORE directory exists");std::filesystem::create_directories(directory_);
        nlohmann::json manifest{{"schema","native-tu15-contact-body-update-typed-before-v1"},{"reference","English TU1.5 only"},
          {"boundary","Actual native1D8A20 typed1D77D0 callback BEFORE original dispatch; no GP/SIMD ABI"},
          {"input",{{"owner",0x1d77d0u},{"manager",input.manager},{"row",input.row},{"skip_impulse",input.skip_impulse},
            {"reversed",input.reversed},{"component_datum",input.component_datum},{"body_slot",input.body_slot},
            {"preserved_x87_depth",input.preserved_x87_depth},{"floating_point",{{"x87_control_word",input.floating_point.control_word},{"x87_status",input.floating_point.status},{"mxcsr",input.floating_point.mxcsr}}}}},
          {"bank_bytes",bytes},{"regions",nlohmann::json::array()},{"guest_abi_fabricated",false},{"source_memory_mutated",false},{"physics_result_supplied",false}};
        memory.visit_committed([&](std::uint32_t address,std::span<const std::byte>bank){const auto file="before-"+std::to_string(address)+".bin";
          std::ofstream out(directory_/file,std::ios::binary);out.write(reinterpret_cast<const char*>(bank.data()),bank.size());require(bool(out),"Cannot write body update BEFORE bank");
          manifest["regions"].push_back({{"address",address},{"size",bank.size()},{"before_file",file}});});
        std::ofstream out(directory_/"capture-native-before.json");out<<manifest.dump(2)<<'\n';require(bool(out),"Cannot write body update BEFORE manifest");captured_=true;
    }
};
}
