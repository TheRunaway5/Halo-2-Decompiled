#include "halo2/pstencil/texture_accumulation_2003.hpp"
#include "../scalar_combiners.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <random>
using namespace halo2;
namespace {
void require(bool v,const char* why){if(!v)throw std::runtime_error(why);}
}
int main(int argc,char** argv)try {
    require(argc==3,"Supply original constructor and current authored texture bindings");
    nlohmann::json original,current;std::ifstream(argv[1])>>original;std::ifstream(argv[2])>>current;
    require(original["original_sha256"]=="5a9edb4c874a782594c86020db49b0086f44ae38f53ff2276b3192a2c2383d18","Unexpected primary image");
    const auto program=pstencil::texture_accumulation_2003();unsigned matches{};
    for(const auto& row:original["fixtures"])if(row["builtin"]==3 && row["global_48f494"]==1 && row["global_48f498"]==1) {
        require(row["program_words"].get<std::array<std::uint32_t,60>>()==program.words,"Texture program differs from unchanged original output");++matches;
    }
    require(matches==1,"Missing original default constructor");
    std::mt19937 gen(0x200395);std::uniform_real_distribution<float> uniform(0,1);
    unsigned cases{};float max_error{};
    for(const auto& row:current) {
        const auto authored=row["program"].get<std::array<std::uint32_t,60>>();
        for(unsigned n=0;n<2048;++n) {
            std::array<ShaderVector,4> samples;ShaderVector v0,v1;
            for(auto& s:samples)for(auto& c:s)c=uniform(gen);
            for(auto* v:{&v0,&v1})for(auto& c:*v)c=uniform(gen);
            const auto stages=authored[54];for(unsigned s=0;s<4;++s)if(!((stages>>(s*5))&31u))samples[s]={};
            const auto before=halo2_test::scalar_combiners(authored,samples,v0,v1);
            auto neutral=samples;for(unsigned c=0;c<3;++c){neutral[0][c]=1;neutral[1][c]=.5f;}
            const auto color=halo2_test::scalar_combiners(authored,neutral,v0,v1);
            auto adapted=samples;adapted[3]={1,1,1,1};
            const auto result=halo2_test::scalar_combiners(program.words,adapted,color,{0,0,0,1});
            for(unsigned c=0;c<3;++c) {
                const float error=std::abs(result[c]-before[c]);max_error=std::max(max_error,error);
                if(error>2.e-5f){std::cerr<<row["shader"]<<" sample "<<n<<" channel "<<c<<" "<<before[c]<<" -> "<<result[c]<<'\n';throw std::runtime_error("Current color factor adaptation changed base/detail RGB");}
            }
            // Literal original3 with the completed mask must own target alpha;
            // it is intentionally not replaced by the current target alpha.
            require(std::isfinite(result[3]) && result[3]>=0 && result[3]<=1,"Nonfinite original texture alpha");
            ++cases;
        }
    }
    require(cases==8192,"Incomplete current armor color-factor coverage");
    std::cout<<"1 original texture program, "<<cases<<" exact current-color/original-accumulation cases; maximum RGB error "<<max_error<<'\n';return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
