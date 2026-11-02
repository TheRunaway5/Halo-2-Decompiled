#pragma once
#include "halo2/native_tu15_pair_pool_release_input_observer.hpp"
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
namespace halo2::test {
class NativePairPoolReleaseInputCaptureTu15 {
    std::filesystem::path directory_;bool captured_{};
    static void require(bool yes,const char*why){if(!yes)throw std::runtime_error(why);}
public:
    explicit NativePairPoolReleaseInputCaptureTu15(std::filesystem::path path){
        const auto scratch=std::getenv("HALO2_TEST_SCRATCH");require(scratch&&*scratch,"Pair pool release capture requires owned scratch");
        const auto owner=std::filesystem::weakly_canonical(scratch);directory_=std::filesystem::weakly_canonical(path);
        auto a=owner.begin(),b=directory_.begin();for(;a!=owner.end();++a,++b)require(b!=directory_.end()&&*a==*b,"Pair pool release capture escapes owned scratch");
        require(b!=directory_.end(),"Pair pool release capture requires child directory");
    }
    template<class Memory>void observe(const NativeTu15PairPoolReleaseInput&input,const Memory&memory){
        if(captured_)return;
        require(input.preserved_x87_depth==0,"Pair pool release BEFORE cannot capture unprovided x87 values");
        require(input.allocator&&input.owner==0x232d20u,"Pair pool release BEFORE requires actual232D20 allocator owner");
        std::uint64_t bytes{};unsigned count{};
        memory.visit_committed([&](std::uint32_t address,std::span<const std::byte>bank){
            bytes+=bank.size();++count;require(count<=8192u&&bytes<=192u*1024u*1024u,"Pair pool release BEFORE exceeds bounded banks");
        });
        require(!std::filesystem::exists(directory_),"Pair pool release BEFORE directory exists");std::filesystem::create_directories(directory_);
        nlohmann::json manifest{{"schema","native-tu15-pair-pool-release-typed-before-v1"},{"reference","English TU1.5 only"},
          {"boundary","Actual native pair storage pool-release service BEFORE unchanged owner refusal; no GP/SIMD ABI"},
          {"input",{{"allocator",input.allocator},{"owner",input.owner},{"data",input.data},{"bytes",input.bytes},{"tag",input.tag},
            {"preserved_x87_depth",input.preserved_x87_depth},{"floating_point",{{"x87_control_word",input.floating_point.control_word},{"x87_status",input.floating_point.status},{"mxcsr",input.floating_point.mxcsr}}}}},
          {"bank_bytes",bytes},{"regions",nlohmann::json::array()},{"guest_abi_fabricated",false},{"source_memory_mutated",false},{"physics_result_supplied",false}};
        std::uint64_t written_bytes{};unsigned written_count{};
        memory.visit_committed([&](std::uint32_t address,std::span<const std::byte>bank){
            written_bytes+=bank.size();++written_count;require(written_count<=8192u&&written_bytes<=192u*1024u*1024u,"Pair pool release BEFORE changed beyond bounded banks");
            const auto file="before-"+std::to_string(address)+".bin";std::ofstream out(directory_/file,std::ios::binary);
            out.write(reinterpret_cast<const char*>(bank.data()),bank.size());require(bool(out),"Cannot write pair pool release BEFORE bank");
            manifest["regions"].push_back({{"address",address},{"size",bank.size()},{"before_file",file}});
        });
        require(written_bytes==bytes&&written_count==count,"Pair pool release BEFORE metadata changed during serialization");
        std::ofstream out(directory_/"capture-native-before.json");out<<manifest.dump(2)<<'\n';require(bool(out),"Cannot write pair pool release BEFORE manifest");captured_=true;
    }
};
} // namespace halo2::test
