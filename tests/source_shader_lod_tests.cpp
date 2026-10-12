#include "halo2/source_shader_lod.hpp"
#include "halo2/math.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace halo2;
namespace {
using Words=std::vector<std::uint32_t>;
void require(bool value,const char *message){if(!value)throw std::runtime_error(message);}
Words evaluate(const Words &row) {
    unsigned at=0;const auto word=[&]{return row.at(at++);};
    const auto real=[&]{return std::bit_cast<float>(word());};
    const auto short_word=[&]{return std::bit_cast<std::int16_t>(std::uint16_t(word()));};
    const auto mode=word();SourceShaderLodView view;Words result;float metric{};unsigned bias{};bool modifier{};
    if(mode==3)result.push_back(std::bit_cast<std::uint32_t>(source_shader_lod_player_multiplier(word())));
    else if(mode==4)result.push_back(source_shader_coverage_byte(real()));
    else if(mode==8)result.push_back(source_shader_instance_alpha(real()));
    else if(mode==9) {
        const auto pattern=source_shader_instance_stipple(std::uint8_t(word()));
        result.push_back(pattern.level);result.insert(result.end(),pattern.rows.begin(),pattern.rows.end());
    }
    else if(mode==10) {
        const auto layer=word();const auto type=std::bit_cast<std::int32_t>(word());
        result.push_back(source_shader_layer_stipple(layer,type));
    }
    else if(mode==11)result.push_back(std::uint32_t(source_shader_gameplay_frame_type));
    else if(mode==5 || mode==6 || mode==7) {
        SourceShaderCoverageCamera camera;for(auto &f:camera.inverse_view_z)f=real();camera.viewport_height_metric=real();
        float coverage;
        if(mode==5){std::array<float,6> bounds;for(auto &f:bounds)f=real();coverage=source_shader_cluster_coverage(bounds,camera);}
        else{const Vec3 center{real(),real(),real()};const auto radius=real();
            if(mode==6)coverage=source_shader_instance_coverage(center,radius,camera);
            else coverage=source_shader_object_coverage(center,radius,real(),camera);}
        result.push_back(std::bit_cast<std::uint32_t>(coverage));result.push_back(source_shader_coverage_byte(coverage));
    }
    else if(mode==0 || mode==2) {
        const auto top=short_word(),bottom=short_word();const bool present=word(),auxiliary=word();
        view=source_shader_lod_view(top,bottom,present,auxiliary);
        for(auto f:view.multipliers)result.push_back(std::bit_cast<std::uint32_t>(f));
        if(mode==2) {
            const auto value=std::uint8_t(word());const auto scale=real(),players=real();
            metric=source_shader_part_metric(value,scale,players);result.push_back(std::bit_cast<std::uint32_t>(metric));
            bias=word();modifier=word();
        }
    }else if(mode==1) {
        bias=word();modifier=word();metric=real();for(auto &f:view.multipliers)f=real();
    }else throw std::runtime_error("unknown source shader LOD fixture mode");
    if(mode==1 || mode==2) {
        const auto count=word();std::vector<float> thresholds;for(unsigned i=0;i<count;++i)thresholds.push_back(real());
        const auto choice=source_shader_lod_choice(thresholds,bias,metric,view,modifier);
        result.push_back(std::bit_cast<std::uint32_t>(choice.index));result.push_back(choice.accepted);
    }
    require(at==row.size(),"source shader LOD input has trailing fields");return result;
}
void boundaries() {
    const auto view=source_shader_lod_view(0,480,false,false);
    const std::array<float,3> thresholds{1,.5f,0};
    require(source_shader_lod_choice(thresholds,0,240,view,false).index==1,
        "source shader LOD threshold equality changed");
    require(source_shader_lod_choice(thresholds,5,0,view,false).index==0,
        "source shader Never bias incorrectly scanned thresholds");
    const auto empty=source_shader_lod_choice({},0,0,view,false);
    require(empty.index==-1 && empty.accepted,"empty original shader LOD return changed");
    bool rejected=false;try{source_shader_lod_choice(thresholds,7,0,view,false);}catch(const std::invalid_argument &){rejected=true;}
    require(rejected,"out-of-range shader bias silently read unrelated table memory");
    const auto cinematic=source_shader_lod_view(0,480,true,true);
    require(cinematic.multipliers[0]==view.multipliers[0] && cinematic.multipliers[6]==view.multipliers[6]*2,
        "cinematic auxiliary changed ordinary shader bias0");
    const auto visible=source_shader_instance_stipple(255),hidden=source_shader_instance_stipple(0);
    for(unsigned row=0;row<32;++row)
        require(visible.rows[row]==0xffffffffu && hidden.rows[row]==0,
            "source instance stipple endpoint changed");
    require(source_shader_layer_stipple(15,source_shader_gameplay_frame_type) &&
        !source_shader_layer_stipple(8,source_shader_gameplay_frame_type) &&
        !source_shader_layer_stipple(15,4),"source layer stipple enable leaked across passes");
    rejected=false;try{source_shader_layer_stipple(25,source_shader_gameplay_frame_type);}catch(const std::invalid_argument &){rejected=true;}
    require(rejected,"source layer table read beyond literal25 records");
}
}
int main(int argc,char **argv){try {
    require(argc==2,"source shader LOD fixture required");const bool probe=std::string(argv[1])=="--probe";
    std::ifstream file;if(!probe){file.open(argv[1]);require(bool(file),"source shader LOD fixture missing");}
    auto &stream=probe?std::cin:file;std::string line;unsigned count=0;
    while(std::getline(stream,line)) {
        if(line.empty() || line[0]=='#')continue;
        std::istringstream input(line);input>>std::hex;
        unsigned n;require(bool(input>>n),"source shader LOD input count missing");Words row(n);
        for(auto &v:row)require(bool(input>>v),"source shader LOD input truncated");
        const auto output=evaluate(row);
        if(probe){for(auto v:output)std::cout<<std::hex<<v<<' ';std::cout<<'\n';}
        else {
            for(auto v:output){unsigned expected;require(bool(input>>expected),"source shader LOD expected output missing");
                if(v!=expected)throw std::runtime_error("source shader LOD mismatch case"+std::to_string(count));}
            std::string extra;require(!(input>>extra),"source shader LOD output has trailing fields");
        }
        ++count;
    }
    require(count!=0,"source shader LOD fixture empty");
    if(!probe){boundaries();std::cout<<count<<" original shader LOD view/choice/part metric cases matched\n";}
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
