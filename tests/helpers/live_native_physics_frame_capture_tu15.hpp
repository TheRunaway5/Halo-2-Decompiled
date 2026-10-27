#pragma once
#include "halo2/native_tu15_physics_frame_observation.hpp"
#include "halo2/source_runtime_memory.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <nlohmann/json.hpp>

namespace halo2::test {
// An opt-in, read-only capture for independently replaying the whole composed
// frame. No original root register postimage is manufactured for a void native
// API. Every persistent byte, FP policy/status and live x87 value is compared.
class LiveNativePhysicsFrameCaptureTu15 {
    using Json=nlohmann::json;
    std::filesystem::path directory_;
    Json record_;
    bool active_{},complete_{};
    static void require(bool ok,const char *why){if(!ok)throw FormatError(why);}
    static std::string hex(std::span<const std::byte> bytes) {
        constexpr char digits[]="0123456789abcdef";std::string result;
        for(auto b:bytes){const auto n=std::to_integer<unsigned>(b);result+=digits[n>>4];result+=digits[n&15];}return result;
    }
    static Json values(const SourcePhysicsContactRuntimeAbi &state) {
        static_assert(std::numeric_limits<long double>::digits==64);
        Json out=Json::array();for(unsigned n=0;n<state.x87_depth;++n)
            out.push_back(hex(std::as_bytes(std::span(&state.x87[n],1)).first(10)));return out;
    }
    static void write(const std::filesystem::path &path,std::span<const std::byte> bytes) {
        std::ofstream file(path,std::ios::binary);file.write(reinterpret_cast<const char *>(bytes.data()),bytes.size());
        require(bool(file),"Cannot write native physics frame diagnostic bank");
    }
public:
    explicit LiveNativePhysicsFrameCaptureTu15(std::filesystem::path path):directory_(std::move(path)){}
    void observe(const NativeTu15PhysicsFrameObservationEvent &event,const SourceRuntimeMemory &memory) {
        if(complete_)return;
        if(event.entry) {
            require(!active_,"Nested native physics frame diagnostic");const auto &state=*event.entry;
            require(state.gp[4]==0x600f0000u,"Native frame capture requires its true standalone world boundary");
            std::filesystem::create_directories(directory_);
            record_={{"entry",0x1cc8d0u},{"return",Bytes(memory.view(state.gp[4],4)).u32(0)},
                {"before",{{"gp",state.gp},{"xmm",state.xmm},{"eflags",state.eflags},{"fs_base",state.fs_base},
                    {"x87_values",values(state)},{"x87_status",event.floating_point.status},
                    {"x87_control_word",event.floating_point.control_word},{"mxcsr",event.floating_point.mxcsr}}},
                {"regions",Json::array()},{"reference","English TU1.5 only"},
                {"comparison_contract","native_tu15_void_world_frame_v1"},
                {"private_returned_stack",{{"first",0x600e0000u},{"end",state.gp[4]}}},
                {"boundary","Actual composed native void world frame; persistent memory and FP/live x87 outputs; no manufactured GPR/SSE register postimage"}};
            memory.visit_committed([&](auto address,auto bytes){
                const auto file="before-"+std::to_string(address)+".bin";write(directory_/file,bytes);
                record_["regions"].push_back({{"address",address},{"size",bytes.size()},{"before_file",file}});
            });active_=true;return;
        }
        if(!active_)return;
        require(event.residual_live_values.x87_depth==0&&!event.residual_live_values.active_mmx,
            "Native void world capture requires its actual balanced empty x87 return");
        Json topology=Json::array();memory.visit_committed([&](auto address,auto bytes){topology.push_back({address,bytes.size()});});
        Json prior=Json::array();for(const auto &region:record_["regions"])prior.push_back({region.at("address"),region.at("size")});
        require(topology==prior,"Composed native frame added backing; original replay requires a complete input topology");
        record_["after"]={{"x87_values",values(event.residual_live_values)},
            {"x87_status",event.floating_point.status},{"x87_control_word",event.floating_point.control_word},
            {"mxcsr",event.floating_point.mxcsr}};
        for(auto &region:record_["regions"]) {
            const auto address=region.at("address").get<std::uint32_t>();const auto size=region.at("size").get<std::size_t>();
            const auto current=memory.view(address,size);std::ifstream before(directory_/region.at("before_file").get<std::string>(),std::ios::binary);
            std::vector<std::byte> chunk(std::min<std::size_t>(size,65536));bool changed{};
            for(std::size_t offset=0;offset<size;offset+=chunk.size()) {
                const auto count=std::min(chunk.size(),size-offset);before.read(reinterpret_cast<char *>(chunk.data()),count);
                require(bool(before),"Cannot read native frame diagnostic input");
                if(std::memcmp(chunk.data(),current.data()+offset,count)){changed=true;break;}
            }
            if(changed){const auto file="after-"+std::to_string(address)+".bin";write(directory_/file,current);region["after_file"]=file;}
        }
        std::ofstream file(directory_/"capture.json");file<<record_.dump(2)<<'\n';
        require(bool(file),"Cannot write native frame semantic capture");complete_=true;active_=false;
    }
};
} // namespace halo2::test
