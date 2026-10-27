#include "halo2/path_tracing/sky_lighting.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {
unsigned checks{};
void require(bool value,const char* message) {++checks;if(!value)throw FormatError(message);}
bool near(Vec3 a,Vec3 b) {
    return std::max({std::abs(a.x-b.x),std::abs(a.y-b.y),std::abs(a.z-b.z)})<1.e-6f;
}
nlohmann::json vector(Vec3 v) {return nlohmann::json::array({v.x,v.y,v.z});}
}
int main(int argc,char** argv)try {
    if(argc!=6)throw FormatError("Expected original/MCC Lockout and Coagulation maps, then report path");
    for(unsigned i=1;i<=4;++i)if(!std::filesystem::exists(argv[i])) {
        std::cout<<"SKIP missing authored sky input: "<<argv[i]<<'\n';return 77;
    }
    nlohmann::json report{{"cohorts",nlohmann::json::array()}};
    for(unsigned cohort=1;cohort<=4;++cohort) {
        Cache cache(argv[cohort]);path_tracing::SkyLighting bank;
        require(!bank.select(cache,1000000) && bank.cached_skies()==0,
            "Inactive RT sky bank accessed source metadata or allocated storage");
        require(!cache.object_sky_lighting_available(0),
            "Actual map no longer exercises the independent RT/object-lighting admission path");
        bank.set_active(true);
        require(!bank.select(cache,-1) && bank.cached_skies()==0,
            "Absent selected sky allocated an RT definition");
        const auto* selected=bank.select(cache,0);
        const auto authored=cache.load_object_sky_lighting(0);
        require(selected && authored && selected->datum==authored->datum && selected->name==authored->name,
            "RT did not select the actual authored sky independently of object-lighting availability");
        require(bank.cached_skies()==1 && bank.select(cache,0)==selected,
            "Repeated selected-sky queries replaced or duplicated the resident definition");
        const auto frame=object_sky_light_frame(*selected);
        const auto exact=object_sky_light_frame(*authored);
        require(frame.test_collision && near(frame.incident,exact.incident) && near(frame.radiosity,exact.radiosity) &&
            near(selected->outdoor_ambient,authored->outdoor_ambient),"RT changed actual authored sky energy");
        const bool lockout=cohort==1 || cohort==3;
        require(near(frame.incident,lockout?Vec3{.6942720413f,.5825634003f,-.4226182699f}:
            Vec3{.8795877099f,.0769539475f,.4694715738f}),"Actual authored sun direction changed");
        require(near(frame.radiosity,lockout?Vec3{.5921570063f,.6117650270f,.7019609809f}:
            Vec3{.9490200281f,.9098039865f,.8745099902f}),"Actual authored sun radiosity changed");
        require(near(selected->outdoor_ambient,lockout?Vec3{}:Vec3{.7882350087f,.8352940083f,.8705880046f}),
            "Actual authored outdoor ambient changed");
        report["cohorts"].push_back({{"map",argv[cohort]},{"mcc",cache.is_mcc()},
            {"object_lighting_available",false},{"incident",vector(frame.incident)},
            {"radiosity",vector(frame.radiosity)},{"outdoor_ambient",vector(selected->outdoor_ambient)}});
        bank.set_active(false);
        require(bank.cached_skies()==0 && !bank.select(cache,1000000),
            "Disabling Path Tracing retained sky allocations or read inactive metadata");
        bank.set_active(true);
        require(bank.select(cache,0) && bank.cached_skies()==1,"Re-enabling Path Tracing did not restore authored sky energy");
    }
    {
        Cache lockout(argv[1]),coagulation(argv[2]);path_tracing::SkyLighting bank;bank.set_active(true);
        const auto* first=bank.select(lockout,0);require(first,"Missing Lockout sky for cache-change control");
        const auto first_name=first->name;
        const auto* second=bank.select(coagulation,0);
        require(second && second->name!=first_name && bank.cached_skies()==1 &&
            near(second->outdoor_ambient,{.7882350087f,.8352940083f,.8705880046f}),
            "Changing maps retained the previous map's selected RT sky");
    }
    report.update({{"passed",true},{"checks",checks},{"inactive_cache_empty",true},{"cache_change_clears_previous",true}});
    std::ofstream output(argv[5]);require(bool(output),"Cannot save selected-sky regression report");
    output<<report.dump(2)<<'\n';
    std::cout<<"PASS actual-source selected sky: cohorts=4 checks="<<checks<<'\n';return 0;
}catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
