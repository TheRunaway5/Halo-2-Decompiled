#pragma once
#include "halo2/native_tu15_pair_consumer_input_observer.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
namespace halo2::test {
// One authentic root BEFORE bankset, followed by bounded typed service INPUT
// windows. No GP/SIMD ABI, return frame, postimage or physics result is supplied.
class NativePairConsumerInputCaptureTu15 {
    using Json=nlohmann::json;
    std::filesystem::path directory_;bool complete_{};unsigned service_records_{},dropped_{};std::size_t service_bytes_{};
    static void require(bool yes,const char*why){if(!yes)throw std::runtime_error(why);}
    static std::optional<std::vector<std::byte>>read(const SourceRuntimeMemory&memory,std::uint32_t address,std::uint32_t size){
        std::optional<std::vector<std::byte>>result;
        memory.visit_committed([&](std::uint32_t first,std::span<const std::byte>bank){
            if(address>=first && std::uint64_t(address)+size<=std::uint64_t(first)+bank.size())
                result=std::vector<std::byte>(bank.begin()+(address-first),bank.begin()+(address-first)+size);
        });return result;
    }
    static std::optional<std::uint32_t>word(const SourceRuntimeMemory&memory,std::uint32_t address){
        const auto b=read(memory,address,4);if(!b)return{};std::uint32_t result{};
        for(unsigned n=0;n<4;++n)result|=std::to_integer<unsigned>((*b)[n])<<(n*8u);return result;
    }
    static std::string hex(std::span<const std::byte>bytes){
        constexpr char digits[]="0123456789abcdef";std::string result;result.reserve(bytes.size()*2);
        for(auto b:bytes){const auto value=std::to_integer<unsigned>(b);result+=digits[value>>4];result+=digits[value&15];}return result;
    }
    static void write(const std::filesystem::path&path,std::span<const std::byte>bytes){
        std::ofstream out(path,std::ios::binary);out.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());require(bool(out),"Cannot retain actual pair consumer BEFORE bank");
    }
    static Json typed(const NativeTu15PairConsumerInput&i){
        return {{"phase",unsigned(i.phase)},{"source_call_site",i.source_call_site},
            {"island",i.island.identity},{"first",{{"data",i.first.data},{"count",i.first.count}}},
            {"second",{{"data",i.second.data},{"count",i.second.count}}},
            {"temporary_header",i.scratch.temporary.identity},
            {"merge_scratch",{{"first_end",i.scratch.merge.first_end},{"second_end",i.scratch.merge.second_end},
              {"interior_decision",i.scratch.merge.interior_decision},{"tail_decision",i.scratch.merge.tail_decision}}},
            {"allocator",i.allocator},{"owner",i.owner},{"data",i.data},{"bytes",i.bytes},{"tag",i.tag},
            {"floating_point",{{"x87_control_word",i.floating_point.control_word},{"x87_status",i.floating_point.status},{"mxcsr",i.floating_point.mxcsr}}},
            {"preserved_x87_depth",i.preserved_x87_depth}};
    }
public:
    explicit NativePairConsumerInputCaptureTu15(std::filesystem::path directory):directory_(std::move(directory)){
        const auto scratch=std::getenv("HALO2_TEST_SCRATCH");require(scratch&&*scratch,"Pair input capture requires owned test scratch");
        const auto owner=std::filesystem::weakly_canonical(scratch),target=std::filesystem::weakly_canonical(directory_);auto a=owner.begin(),b=target.begin();
        for(;a!=owner.end();++a,++b)require(b!=target.end()&&*a==*b,"Pair input capture escapes its owned scratch");
        require(b!=target.end(),"Pair input capture requires its own child directory");directory_=target;
    }
    bool complete()const noexcept{return complete_;}
    Json status()const{return{{"root_before_captured",complete_},{"service_records",service_records_},{"service_bytes",service_bytes_},{"service_records_dropped",dropped_}};}
    void observe(const NativeTu15PairConsumerInput&input,const SourceRuntimeMemory&memory){
        if(input.phase==NativeTu15PairConsumerInputPhase::Entry){
            if(complete_||(!input.first.count&&!input.second.count))return;
            std::uint64_t total{};unsigned count{};
            memory.visit_committed([&](std::uint32_t,std::span<const std::byte>bank){total+=bank.size();++count;});
            require(total<=512u*1024u*1024u&&count<=8192u,"Actual pair input banks exceed diagnostic bound");
            require(!std::filesystem::exists(directory_),"Pair input BEFORE directory already exists");std::filesystem::create_directories(directory_);
            Json record{{"schema","native-tu15-pair-consumer-typed-before-v1"},{"reference","English TU1.5 only"},
                {"boundary","Actual first nonempty native321510 typed input before first kernel access"},
                {"input",typed(input)},{"regions",Json::array()},{"bank_bytes",total},
                {"captured_after_consumed",false},{"guest_abi_fabricated",false},{"source_memory_mutated",false},{"physics_result_supplied",false}};
            memory.visit_committed([&](std::uint32_t address,std::span<const std::byte>bank){const auto file="before-"+std::to_string(address)+".bin";write(directory_/file,bank);record["regions"].push_back({{"address",address},{"size",bank.size()},{"before_file",file}});});
            std::ofstream out(directory_/"capture-native-before.json");out<<record.dump(2)<<'\n';require(bool(out),"Cannot retain actual pair typed BEFORE manifest");complete_=true;return;
        }
        if(!complete_)return;if(service_records_>=4096u){++dropped_;return;}
        auto row=typed(input);row["boundary"]="Actual typed service BEFORE input; not a root expected postimage";
        Json windows=Json::array();
        const auto window=[&](const char*name,std::uint32_t address,std::uint32_t size){const auto bytes=read(memory,address,size);windows.push_back({{"name",name},{"address",address},{"size",size},{"bytes",bytes?Json(hex(*bytes)):Json()}});};
        window("allocator",input.allocator,0x40u);const auto table=word(memory,input.allocator);if(table)window("allocator_table",*table,0x2cu);
        window("temporary_header",input.scratch.temporary.identity,20);window("island_pair_state",input.island.identity+0x68u,0x40u);
        const auto world=word(memory,input.island.identity+0x20u);if(world)window("world_pair_owners",*world+0xccu,0x70u);
        if(input.first.count<=4096u)window("first_pairs",input.first.data,input.first.count*8u);
        if(input.second.count<=4096u)window("second_pairs",input.second.data,input.second.count*8u);
        row["windows"]=windows;row["guest_abi_fabricated"]=false;row["physics_result_supplied"]=false;
        const auto line=row.dump();if(service_bytes_+line.size()+1u>4u*1024u*1024u){++dropped_;return;}
        std::ofstream out(directory_/"typed-service-before.jsonl",std::ios::app);out<<line<<'\n';require(bool(out),"Cannot retain typed pair service input");++service_records_;service_bytes_+=line.size()+1u;
    }
};
} // namespace halo2::test
