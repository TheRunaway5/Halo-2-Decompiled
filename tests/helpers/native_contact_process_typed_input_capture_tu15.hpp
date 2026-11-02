#pragma once
#include "halo2/native_tu15_contact_process_typed_input_observer.hpp"
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
namespace halo2::test {
class NativeContactProcessTypedInputCaptureTu15 {
    std::filesystem::path directory_;bool captured_{};std::uint32_t owner_{};
    static void require(bool yes,const char*why){if(!yes)throw std::runtime_error(why);}
public:
    NativeContactProcessTypedInputCaptureTu15(std::filesystem::path path,std::uint32_t owner_filter):owner_(owner_filter){
        require(owner_==0x2e5320u||owner_==0x304350u,"Contact-process BEFORE requires an authenticated owner filter");
        const auto scratch=std::getenv("HALO2_TEST_SCRATCH");require(scratch&&*scratch,"Contact-process BEFORE requires owned scratch");
        const auto owner=std::filesystem::weakly_canonical(scratch);directory_=std::filesystem::weakly_canonical(path);
        auto a=owner.begin(),b=directory_.begin();for(;a!=owner.end();++a,++b)require(b!=directory_.end()&&*a==*b,"Contact-process BEFORE escapes owned scratch");
        require(b!=directory_.end(),"Contact-process BEFORE requires a child directory");
    }
    template<class Memory>void observe(const NativeTu15ContactProcessTypedInput&input,const Memory&memory){
        if(captured_||input.owner!=owner_)return;
        require(input.preserved_x87_depth==0,"Contact-process BEFORE requires provided empty x87 DATA");
        const std::array pointers{input.agent,input.first,input.second,input.configuration,input.output};
        require(std::all_of(pointers.begin(),pointers.end(),[](auto p){return p!=0;}),"Contact-process BEFORE requires actual nonnull formals");
        std::array<bool,5>found{};std::uint64_t bytes{};unsigned count{};
        memory.visit_committed([&](std::uint32_t address,std::span<const std::byte>bank){
            bytes+=bank.size();++count;require(count<=8192u&&bytes<=192u*1024u*1024u,"Contact-process BEFORE exceeds bounded banks");
            for(unsigned n=0;n<pointers.size();++n)if(pointers[n]>=address&&std::uint64_t(pointers[n]-address)<bank.size())found[n]=true;
        });
        require(std::all_of(found.begin(),found.end(),[](bool yes){return yes;}),"Contact-process BEFORE formals lack committed backing");
        require(!std::filesystem::exists(directory_),"Contact-process BEFORE directory already exists");std::filesystem::create_directories(directory_);
        nlohmann::json manifest{{"schema","native-tu15-contact-process-typed-before-v1"},{"reference","English TU1.5 only"},
          {"boundary","Actual native World typed contact-process owner BEFORE unchanged remaining-owner dispatch; no GP/SIMD ABI"},
          {"input",{{"owner",input.owner},{"agent",input.agent},{"first",input.first},{"second",input.second},{"configuration",input.configuration},{"output",input.output},
            {"preserved_x87_depth",input.preserved_x87_depth},{"floating_point",{{"x87_control_word",input.floating_point.control_word},{"x87_status",input.floating_point.status},{"mxcsr",input.floating_point.mxcsr}}}}},
          {"bank_bytes",bytes},{"regions",nlohmann::json::array()},{"guest_abi_fabricated",false},{"source_memory_mutated",false},{"physics_result_supplied",false}};
        std::uint64_t written_bytes{};unsigned written_count{};
        memory.visit_committed([&](std::uint32_t address,std::span<const std::byte>bank){
            written_bytes+=bank.size();++written_count;require(written_count<=8192u&&written_bytes<=192u*1024u*1024u,"Contact-process BEFORE changed beyond bounded banks");
            const auto file="before-"+std::to_string(address)+".bin";std::ofstream out(directory_/file,std::ios::binary);
            out.write(reinterpret_cast<const char*>(bank.data()),bank.size());require(bool(out),"Cannot write contact-process BEFORE bank");
            manifest["regions"].push_back({{"address",address},{"size",bank.size()},{"before_file",file}});
        });
        require(written_count==count&&written_bytes==bytes,"Contact-process BEFORE bank metadata changed during serialization");
        std::ofstream out(directory_/"capture-native-before.json");out<<manifest.dump(2)<<'\n';require(bool(out),"Cannot write contact-process BEFORE manifest");captured_=true;
    }
};
} // namespace halo2::test
