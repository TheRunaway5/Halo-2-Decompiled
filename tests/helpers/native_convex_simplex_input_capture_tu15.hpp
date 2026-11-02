#pragma once
#include "halo2/native_tu15_convex_simplex_input_observer.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

namespace halo2::test {
class NativeConvexSimplexInputCaptureTu15 {
    std::filesystem::path directory_;bool captured_{};
    static void require(bool yes,const char*why){if(!yes)throw std::runtime_error(why);}
public:
    explicit NativeConvexSimplexInputCaptureTu15(const std::filesystem::path&path){
        const auto scratch=std::getenv("HALO2_TEST_SCRATCH");require(scratch&&*scratch,"Simplex BEFORE requires owned scratch");
        const auto owner=std::filesystem::weakly_canonical(scratch);directory_=std::filesystem::weakly_canonical(path);
        auto a=owner.begin(),b=directory_.begin();for(;a!=owner.end();++a,++b)require(b!=directory_.end()&&*a==*b,"Simplex BEFORE escapes owned scratch");
        require(b!=directory_.end(),"Simplex BEFORE requires a child directory");
    }
    void observe(const SourcePhysicsContactMemory&view,const NativeTu15ConvexSimplexSolverInput&input,
        const NativeTu15PhysicsFrameFp&fp,unsigned depth,const SourceRuntimeMemory&memory){
        if(captured_)return;
        require(depth==0,"Simplex BEFORE requires provided empty x87 inputs");
        // Verify that the exported World owns this solver's actual ordinary
        // inputs. No unprovided local row or output-plane byte is read here.
        for(auto [p,n]:{std::pair{input.work+12u,1u},std::pair{input.relative_transform,64u},std::pair{input.support_cache,12u}}){
            std::vector<std::byte>native(n);view.read(p,native);const auto source=memory.view(p,n);
            require(std::equal(native.begin(),native.end(),source.begin()),"Simplex typed inputs belong to another World");
        }
        std::uint64_t bytes{};unsigned count{};
        memory.visit_committed([&](auto,std::span<const std::byte>bank){bytes+=bank.size();++count;require(count<=8192u&&bytes<=256u*1024u*1024u,"Simplex BEFORE exceeds bounded banks");});
        require(!std::filesystem::exists(directory_),"Simplex BEFORE already exists");std::filesystem::create_directories(directory_);
        nlohmann::json manifest{{"schema","native-tu15-convex-simplex-typed-before-v1"},{"reference","English TU1.5 only"},
            {"boundary","Actual native30E930 BEFORE first solver read/write; ordinary typed DATA and FP only"},
            {"input",{{"owner",0x30e930u},{"work",input.work},{"first",input.first.identity},{"second",input.second.identity},
                {"relative_transform",input.relative_transform},{"support_cache",input.support_cache},{"plane",input.plane},
                {"preserved_x87_depth",depth},{"floating_point",{{"x87_control_word",fp.control_word},{"x87_status",fp.status},{"mxcsr",fp.mxcsr}}}}},
            {"regions",nlohmann::json::array()},{"bank_bytes",bytes},{"guest_abi_fabricated",false},
            {"source_memory_mutated",false},{"physics_result_supplied",false},{"expected_after_consumed",false}};
        std::uint64_t written{};unsigned written_count{};
        memory.visit_committed([&](std::uint32_t address,std::span<const std::byte>bank){
            written+=bank.size();++written_count;require(written<=bytes&&written_count<=count,"Simplex BEFORE topology changed");
            const auto name="before-"+std::to_string(address)+".bin";std::ofstream out(directory_/name,std::ios::binary);
            out.write(reinterpret_cast<const char*>(bank.data()),bank.size());require(bool(out),"Cannot write simplex BEFORE bank");
            manifest["regions"].push_back({{"address",address},{"size",bank.size()},{"before_file",name}});
        });
        require(written==bytes&&written_count==count,"Simplex BEFORE topology changed");
        std::ofstream out(directory_/"capture-native-before.json");out<<manifest.dump(2)<<'\n';require(bool(out),"Cannot write simplex BEFORE manifest");captured_=true;
    }
};
} // namespace halo2::test
