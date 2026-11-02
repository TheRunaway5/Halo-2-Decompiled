#pragma once
#include "halo2/cache.hpp"
#include "halo2/native_tu15_contact_agent_factory_input_observer.hpp"
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>

namespace halo2::test {
class NativeContactAgentFactoryInputCaptureTu15 {
    std::filesystem::path directory_;bool captured_{};std::uint32_t owner_{0x2f8030u};
    std::optional<std::uint32_t> excluded_class_query_;
    std::optional<std::uint32_t> required_class_query_;
    static void require(bool value,const char*why){if(!value)throw std::runtime_error(why);}
public:
    explicit NativeContactAgentFactoryInputCaptureTu15(std::filesystem::path path){
        const auto scratch=std::getenv("HALO2_TEST_SCRATCH");require(scratch&&*scratch,"Agent factory capture requires owned scratch");
        const auto owner=std::filesystem::weakly_canonical(scratch);directory_=std::filesystem::weakly_canonical(path);
        auto a=owner.begin(),b=directory_.begin();for(;a!=owner.end();++a,++b)
            require(b!=directory_.end()&&*a==*b,"Agent factory capture escapes owned scratch");
        require(b!=directory_.end(),"Agent factory capture needs its own child directory");
        if(const auto selected=std::getenv("HALO2_DESTRUCTION_AGENT_FACTORY_CAPTURE_OWNER")){
            char* end{};const auto value=std::strtoul(selected,&end,0);
            require(end&&end!=selected&&!*end&&(value==0x2f8030u||value==0x2f81f0u||value==0x2e4eb0u||value==0x2f1cc0u),"Unsupported factory INPUT capture owner");
            owner_=static_cast<std::uint32_t>(value);
        }
        if(const auto selected=std::getenv("HALO2_DESTRUCTION_AGENT_FACTORY_CAPTURE_EXCLUDE_CLASS_QUERY")){
            char* end{};const auto value=std::strtoul(selected,&end,0);
            require(owner_==0x2e4eb0u&&end&&end!=selected&&!*end&&value<=0xfffffffful,
                "Class-query capture selection requires an explicit convex factory owner");
            excluded_class_query_=static_cast<std::uint32_t>(value);
        }
        if(const auto selected=std::getenv("HALO2_TU15_AGENT_FACTORY_CAPTURE_REQUIRE_CLASS_QUERY")){
            char*end{};const auto value=std::strtoul(selected,&end,0);
            require(owner_==0x2e4eb0u&&end&&end!=selected&&!*end&&value<=0xfffffffful,
                "Required class-query capture needs an explicit convex factory owner");
            required_class_query_=static_cast<std::uint32_t>(value);
        }
    }
    template<class Memory>void observe(const NativeTu15ContactAgentFactoryInput& input,const Memory& memory){
        if(captured_||input.owner!=owner_)return;
        if(std::getenv("HALO2_TU15_AGENT_FACTORY_CAPTURE_ACTIVATION_ONLY")&&!input.activation_receiver)return;
        std::optional<std::uint32_t> second_shape,second_class_query;
        if(owner_==0x2e4eb0u){
            const auto word=[&](std::uint32_t at){return Bytes(memory.view(at,4)).u32(0);};
            second_shape=word(input.second);second_class_query=word(word(*second_shape)+0x14u);
            if(excluded_class_query_&&*excluded_class_query_==*second_class_query)return;
            if(required_class_query_&&*required_class_query_!=*second_class_query)return;
        }
        require(input.preserved_x87_depth==0,"Agent factory BEFORE needs explicit nonempty x87 operands");
        std::uint64_t bytes{};unsigned regions{};
        memory.visit_committed([&](auto,std::span<const std::byte>bank){bytes+=bank.size();++regions;});
        require(bytes<=192u*1024u*1024u&&regions<=8192u,"Agent factory BEFORE exceeds diagnostic storage bounds");
        require(!std::filesystem::exists(directory_),"Agent factory BEFORE already exists");std::filesystem::create_directories(directory_);
        nlohmann::json record{{"schema","native-tu15-contact-agent-factory-before-v1"},
            {"reference","English Xbox TU1.5 behavior; actual selected native factory input"},
            {"boundary","Actual typed factory BEFORE admission; no original GP/SIMD ABI captured"},
            {"input",{{"owner",input.owner},{"arguments",{input.first,input.second,input.context,input.manager}},
                {"preserved_x87_depth",input.preserved_x87_depth},
                {"floating_point",{{"x87_control_word",input.floating_point.control_word},
                    {"x87_status",input.floating_point.status},{"mxcsr",input.floating_point.mxcsr}}}}},
            {"bank_bytes",bytes},{"regions",nlohmann::json::array()},
            {"guest_abi_fabricated",false},{"source_memory_mutated",false},{"physics_result_supplied",false},
            {"expected_after_consumed",false}};
        if(second_class_query)record["actual_second_shape"]={{"identity",*second_shape},{"class_query_owner",*second_class_query}};
        if(input.activation_receiver)record["actual_activation_call"]={
            {"receiver",*input.activation_receiver},{"return_pop",input.activation_return_pop},
            {"source_call",0x28296fu},{"receiver_origin","actual context DATA, also the third cdecl argument"}};
        memory.visit_committed([&](std::uint32_t address,std::span<const std::byte>bank){
            const auto file="before-"+std::to_string(address)+".bin";std::ofstream out(directory_/file,std::ios::binary);
            out.write(reinterpret_cast<const char*>(bank.data()),bank.size());require(bool(out),"Cannot capture actual factory BEFORE bank");
            record["regions"].push_back({{"address",address},{"size",bank.size()},{"before_file",file}});
        });
        std::ofstream out(directory_/"capture-native-before.json");out<<record.dump(2)<<'\n';require(bool(out),"Cannot capture factory BEFORE manifest");captured_=true;
    }
};
} // namespace halo2::test
