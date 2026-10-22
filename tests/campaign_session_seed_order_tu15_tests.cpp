#include "halo2/cache.hpp"
#include "halo2/campaign_source_runtime.hpp"
#include "halo2/original_data.hpp"
#include "halo2/physics_live_world_tu15.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace {
void check(bool value,const char *message) {
    if(!value)throw std::runtime_error(message);
}
std::vector<std::byte> bytes(const std::string &text) {
    check(text.size()%2==0,"Odd original initialization bytes");
    std::vector<std::byte> result;
    for(std::size_t i=0;i<text.size();i+=2)result.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
    return result;
}
}

int main(int argc,char **argv) {
    nlohmann::json report{{"passed",false},{"runtime_results_supplied",false}};
    auto save=[&] {
        if(argc==6)std::ofstream(argv[5])<<report.dump(2)<<'\n';
    };
    try {
        check(argc==6,"Supply MCC map, maps directory, original reference, seed and report");
        const auto seed=std::uint32_t(std::stoul(argv[4],nullptr,0));
        report["seed"]=seed;
        nlohmann::json reference;std::ifstream(argv[3])>>reference;
        check(reference.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&
            reference.at("instruction_patches").empty()&&reference.at("supplied_dependency_returns").empty(),
            "Original initialization authority differs");
        const auto &cases=reference.at("cases");
        const auto expected=std::find_if(cases.begin(),cases.end(),[&](const auto& c){return c.at("seed")==seed;});
        check(expected!=cases.end(),"Original seed reference is absent");
        halo2::Cache cache(argv[1],argv[2],halo2::OriginalData::native_tu15());
        check(cache.is_mcc(),"Session initialization requires prepared MCC content");
        halo2::CampaignSourceRuntime runtime(cache,0,1,seed);
        const auto &memory=runtime.objects().tu15_world().memory();
        const auto noise=bytes(expected->at("noise_hex"));
        std::vector<std::byte> actual(noise.size());memory.read(0x505e90u,actual);
        if(actual!=noise) {
            const auto mismatch=std::mismatch(actual.begin(),actual.end(),noise.begin());
            report["first_noise_mismatch"]=std::size_t(mismatch.first-actual.begin());
            throw std::runtime_error("Admitted campaign seed does not produce original contrail noise");
        }
        const auto scalars=bytes(expected->at("scalar_hex"));actual.resize(scalars.size());
        memory.read(0x505e84u,actual);
        check(actual==scalars,"Original new-map scalar reset/derivation differs");
        report["noise_bytes_compared"]=noise.size();
        report["original_noise_sha256"]=expected->at("noise_sha256");
        report["scalar_bytes_compared"]=scalars.size();
        report["passed"]=true;
        save();
        std::cout<<"PASS original session-seeded noise and scalar initialization: "<<seed<<'\n';
        return 0;
    } catch(const std::exception &error) {
        report["error"]=error.what();save();std::cerr<<error.what()<<'\n';return 1;
    }
}
