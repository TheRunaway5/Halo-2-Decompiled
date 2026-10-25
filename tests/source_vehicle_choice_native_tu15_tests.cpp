#include "gameplay/objects/source_vehicle_choice_native_tu15_internal.hpp"
#include "vehicle_choice_literal_routes.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>

using namespace halo2;
using namespace halo2::physics_world_runtime_detail;
namespace {
using Json=nlohmann::json;
void check(bool value,const char* text){if(!value)throw std::runtime_error(text);}
struct Memory final:SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::byte> bytes;
    mutable Json reads=Json::array();Json writes=Json::array();std::uint32_t pc{};
    static std::string hex(std::span<const std::byte> values) {
        constexpr char digits[]="0123456789abcdef";std::string out;
        for(auto byte:values){const auto value=std::to_integer<unsigned>(byte);out+=digits[value>>4];out+=digits[value&15];}
        return out;
    }
    void patch(std::uint32_t at,const std::string& raw) {
        for(std::size_t i=0;i<raw.size()/2;++i)bytes[at+std::uint32_t(i)]=std::byte(std::stoul(raw.substr(i*2,2),nullptr,16));
    }
    void read(std::uint32_t at,std::span<std::byte> out)const override {
        for(std::size_t i=0;i<out.size();++i)out[i]=bytes.at(at+std::uint32_t(i));
        reads.push_back(Json::array({pc,at,hex(out)}));
    }
    void write(std::uint32_t at,std::span<const std::byte> data)override {
        for(std::size_t i=0;i<data.size();++i)bytes.at(at+std::uint32_t(i))=data[i];
        writes.push_back(Json::array({pc,at,hex(data)}));
    }
};
}

// Xbox string IDs remain literal in this component input. Production resolves
// these two compiled symbols through the authenticated MCC string owner.
namespace halo2 {
std::uint32_t source_vehicle_compiled_symbol_tu15(const SourcePhysicsContactMemory&,
    std::uint32_t,std::uint32_t literal){return literal;}
}

int main(int argc,char** argv)try {
    check(argc==2,"Supply the independent whole-original vehicle-choice fixture");
    std::ifstream stream(argv[1]);const auto fixture=Json::parse(stream);
    check(fixture.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Vehicle-choice authority changed");
    std::size_t cases{},comparisons{},draws{};
    for(const auto& row:fixture.at("cases")) {
        std::optional<std::map<std::uint32_t,std::byte>> native_memory;
        for(unsigned route=0;route<2;++route)try {
            Memory memory;
            for(const auto& bank:fixture.at("component_banks")) {
                const auto at=bank[0].get<std::uint32_t>(),size=bank[1].get<unsigned>();
                for(unsigned i=0;i<size;++i)memory.bytes.emplace(at+i,std::byte(bank[2].get<unsigned>()));
            }
            for(const auto& bank:fixture.at("read_only_banks"))memory.patch(bank[0],bank[1]);
            for(const auto& patch:row.at("patches"))memory.patch(patch[0],patch[1]);
            SourcePhysicsContactRuntimeAbi abi{};abi.gp=row.at("initial_gp").get<decltype(abi.gp)>();
            abi.eflags=row.at("initial_flags");abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
            Json trace=Json::array();SourcePhysicsWorldRuntimeServices services{};
            services.instruction=[&](std::uint32_t pc){memory.pc=pc;trace.push_back(pc);};
            const O observer{}; // Memory records actual stores; no trace-preimage reads.
            WorldState state(memory,observer,services,SourcePhysicsContactImage::TitleUpdate15);state.load(abi);
            const auto calls=vehicle_choice_native_calls.load(),random=vehicle_choice_native_random_draws.load();
            const auto flying_calls=vehicle_choice_native_flying_calls.load();
            const auto expected_flying=std::count(row.at("trace").begin(),row.at("trace").end(),Json(0x15cd30));
            const auto family=row.at("family")=="light"?VehicleChoiceFamilyTu15::light:
                row.at("family")=="heavy"?VehicleChoiceFamilyTu15::heavy:
                row.at("family")=="flying"?VehicleChoiceFamilyTu15::flying:VehicleChoiceFamilyTu15::turret;
            if(route==0)vehicle_choice_native_tu15(state,family,services.instruction);
            else vehicle_choice_literal_routes::run(row.at("entry"),state);
            abi=state.save();
            check(trace==row.at("trace"),"Vehicle-choice instruction sequence differs");
            if(memory.reads!=row.at("reads")) {
                const auto& expected=row.at("reads");std::size_t i=0;
                while(i<memory.reads.size() && i<expected.size() && memory.reads[i]==expected[i])++i;
                std::cerr<<"read="<<i<<" native="<<(i<memory.reads.size()?memory.reads[i]:Json())
                    <<" original="<<(i<expected.size()?expected[i]:Json())<<' ';
                throw std::runtime_error("Vehicle-choice ordered reads differ");
            }
            check(memory.writes==row.at("stores"),"Vehicle-choice ordered stores differ");
            check(abi.gp==row.at("final_gp").get<decltype(abi.gp)>(),"Vehicle-choice full GP ABI differs");
            check(abi.eflags==row.at("final_flags"),"Vehicle-choice integer flags differ");
            check(abi.xmm==decltype(abi.xmm){} && abi.x87_depth==0 && abi.x87_status==0 && !abi.active_mmx &&
                abi.floating_point->x87_control_word==0x27f && abi.floating_point->mxcsr==0x1f80,"Vehicle-choice FP state changed");
            if(route==0) {
                check(vehicle_choice_native_calls.load()>calls,"Native vehicle-choice owner was not selected");
                check(vehicle_choice_native_random_draws.load()-random==row.at("rng_draws").get<unsigned>(),"Native Random draw count differs");
                check(vehicle_choice_native_flying_calls.load()-flying_calls==expected_flying,"Native flying call count differs");
                native_memory=memory.bytes;draws+=row.at("rng_draws").get<unsigned>();
            } else {
#ifdef HALO2_VEHICLE_CHOICE_INSTALLED
                check(vehicle_choice_native_calls.load()>calls &&
                    vehicle_choice_native_random_draws.load()-random==row.at("rng_draws").get<unsigned>(),"Installed vehicle selector did not route through native owner");
                check(vehicle_choice_native_flying_calls.load()-flying_calls==expected_flying,"Installed flying call count differs");
#else
                check(vehicle_choice_native_calls.load()==calls && vehicle_choice_native_random_draws.load()==random,"Literal control entered native replacement");
                check(vehicle_choice_native_flying_calls.load()==flying_calls,"Literal control entered native flying replacement");
#endif
                check(memory.bytes==*native_memory,"Native and literal complete memory banks differ");
            }
            ++comparisons;
        }catch(const std::exception&){std::cerr<<"case="<<cases<<" family="<<row.at("family")<<" route="<<route<<' ';throw;}
        ++cases;
    }
    std::cout<<"PASS original vehicle selectors cases="<<cases<<" comparisons="<<comparisons<<" native_random_draws="<<draws<<'\n';
    return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
