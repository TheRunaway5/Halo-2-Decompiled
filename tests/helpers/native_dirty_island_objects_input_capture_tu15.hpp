#pragma once
#include "halo2/native_tu15_dirty_island_objects_input_observer.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <map>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
namespace halo2::test {
struct NativeDirtyIslandObjectsInputCaptureActivityTu15 {
    std::uint64_t attempts{},pending_skips{},reentrant_skips{},unavailable_banks{},
        unsupported_depths{},no_dirty_island{},captures{};
};
class NativeDirtyIslandObjectsInputCaptureTu15 {
    std::filesystem::path directory_;NativeDirtyIslandObjectsInputCaptureActivityTu15 activity_;
    static void require(bool yes,const char*why){if(!yes)throw std::runtime_error(why);}
public:
    explicit NativeDirtyIslandObjectsInputCaptureTu15(std::filesystem::path path){
        const auto scratch=std::getenv("HALO2_TEST_SCRATCH");require(scratch&&*scratch,"Dirty island capture requires owned scratch");
        const auto owner=std::filesystem::weakly_canonical(scratch);directory_=std::filesystem::weakly_canonical(path);
        auto a=owner.begin(),b=directory_.begin();for(;a!=owner.end();++a,++b)require(b!=directory_.end()&&*a==*b,"Dirty island capture escapes owned scratch");
        require(b!=directory_.end(),"Dirty island capture requires child directory");
    }
    const auto&activity()const{return activity_;}
    template<class Memory>void observe(const NativeTu15DirtyIslandObjectsInput&input,const Memory&memory){
        if(activity_.captures)return;++activity_.attempts;
        if(input.preserved_x87_depth){++activity_.unsupported_depths;return;}
        // visit_committed may lend a temporary buffer for one callback only.
        // Copy each requested scalar while that callback owns the bytes.
        const auto word=[&](std::uint32_t at,unsigned width=4)->std::optional<std::uint32_t>{
            std::optional<std::uint32_t>result;
            memory.visit_committed([&](std::uint32_t address,std::span<const std::byte>bank){
                if(result||at<address||std::uint64_t(at-address)+width>bank.size())return;
                std::uint32_t value{};for(unsigned n=0;n<width;++n)value|=std::to_integer<unsigned>(bank[at-address+n])<<(n*8u);
                result=value;
            });
            return result;
        };
        const auto pending=word(0x56dfccu,1),busy=word(0x48b873u,1),world=word(0x52be48u);
        if(!pending||!busy||!world||!*world){++activity_.unavailable_banks;return;}
        if(!*pending){++activity_.pending_skips;return;}if(*busy){++activity_.reentrant_skips;return;}
        std::uint32_t chosen{},chosen_row{},chosen_cohort{},chosen_bodies{};bool incomplete=false;
        for(unsigned cohort=0;cohort<2&&!chosen;++cohort){
            const auto array=word(*world+(cohort?0x14u:8u)),count=word(*world+(cohort?0x18u:0xcu));
            if(!count){incomplete=true;continue;}if(std::bit_cast<std::int32_t>(*count)<=0)continue;
            if(!array||*count>4096u){incomplete=true;continue;}
            for(std::uint32_t row=0;row<*count;++row){
                const auto island=word(*array+row*4u);if(!island||!*island){incomplete=true;continue;}
                const auto dirty=word(*island+0xaeu,1),bodies=word(*island+0x40u);
                if(!dirty||!bodies){incomplete=true;continue;}
                if(*dirty){chosen=*island;chosen_row=row;chosen_cohort=cohort;chosen_bodies=*bodies;break;}
            }
        }
        if(!chosen){if(incomplete)++activity_.unavailable_banks;else ++activity_.no_dirty_island;return;}
        std::uint64_t bytes{};unsigned count{};
        memory.visit_committed([&](auto,std::span<const std::byte>bank){bytes+=bank.size();++count;
            require(count<=8192u&&bytes<=192u*1024u*1024u,"Dirty island BEFORE exceeds bounded banks");});
        require(!std::filesystem::exists(directory_),"Dirty island BEFORE directory exists");std::filesystem::create_directories(directory_);
        nlohmann::json manifest{{"schema","native-tu15-dirty-island-objects-typed-before-v1"},{"reference","English TU1.5 only"},
          {"boundary","Actual native frame post_step zero-formal282C10 BEFORE original dispatch; no GP/SIMD ABI"},
          {"input",{{"owner",0x282c10u},{"preserved_x87_depth",input.preserved_x87_depth},{"floating_point",{{"x87_control_word",input.floating_point.control_word},{"x87_status",input.floating_point.status},{"mxcsr",input.floating_point.mxcsr}}}}},
          {"readonly_selection",{{"world",*world},{"pending",*pending},{"busy",*busy},{"cohort",chosen_cohort},{"row",chosen_row},{"island",chosen},{"body_count",chosen_bodies}}},
          {"bank_bytes",bytes},{"regions",nlohmann::json::array()},{"guest_abi_fabricated",false},{"source_memory_mutated",false},{"physics_result_supplied",false}};
        std::uint64_t written_bytes{};unsigned written_count{};
        memory.visit_committed([&](std::uint32_t address,std::span<const std::byte>bank){
            written_bytes+=bank.size();++written_count;
            require(written_count<=8192u&&written_bytes<=192u*1024u*1024u,"Dirty island BEFORE changed beyond bounded banks");
            const auto file="before-"+std::to_string(address)+".bin";
            std::ofstream out(directory_/file,std::ios::binary);out.write(reinterpret_cast<const char*>(bank.data()),bank.size());require(bool(out),"Cannot write dirty island BEFORE bank");
            manifest["regions"].push_back({{"address",address},{"size",bank.size()},{"before_file",file}});
        });
        require(written_bytes==bytes&&written_count==count,"Dirty island BEFORE bank metadata changed during serialization");
        std::ofstream out(directory_/"capture-native-before.json");out<<manifest.dump(2)<<'\n';require(bool(out),"Cannot write dirty island BEFORE manifest");++activity_.captures;
    }
};
} // namespace halo2::test
