#include "halo2/source_model_shader.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace halo2;
namespace {
using Words=std::vector<std::uint32_t>;
void require(bool condition,const char *message){if(!condition)throw std::runtime_error(message);}
Words evaluate(const Words &row) {
    unsigned at=0;const auto word=[&]{return row.at(at++);};
    const auto real=[&]{return std::bit_cast<float>(word());};
    SourceModelShaderView view;
    for(auto &f:view.camera.inverse_view_z)f=real();view.camera.viewport_height_metric=real();
    const Vec3 center{real(),real(),real()};const auto radius=real();
    view.object_coverage_multiplier=real();view.part_shader_multiplier=real();
    const auto top=std::bit_cast<std::int16_t>(std::uint16_t(word()));
    const auto bottom=std::bit_cast<std::int16_t>(std::uint16_t(word()));
    const bool present=word(),auxiliary=word();
    view.shader=source_shader_lod_view(top,bottom,present,auxiliary);
    view.view_modifier=word();const auto scale=real();
    SourceShaderLodDefinition definition;definition.bias=std::uint16_t(word());
    const auto count=word();for(unsigned i=0;i<count;++i)definition.thresholds.push_back(real());
    require(at==row.size(),"model shader input has trailing fields");
    const auto collector=source_model_shader_collector(center,radius,view);
    const auto part=source_model_shader_part(collector,scale,definition,view);
    return {std::bit_cast<std::uint32_t>(collector.coverage),collector.coverage_byte,
        std::bit_cast<std::uint32_t>(part.metric),std::bit_cast<std::uint32_t>(part.choice.index),
        part.choice.accepted};
}
void boundaries() {
    SourceModelShaderView view{{{0,0,-1,0},240},source_shader_lod_view(0,480,false,false),1,.85f,false};
    const auto root=source_model_shader_collector({0,0,-.1f},1,view);
    const SourceShaderLodDefinition material{0,{.5f,.1f,0}};
    const auto high=source_model_shader_part(root,1,material,view);
    require(root.coverage_byte==255 && high.choice.index==0,"ordinary owner coverage did not saturate");
    // The same root collector must be supplied to every rendered attachment.
    // A different rendered-child sphere is intentionally a different input.
    const auto unrelated_child=source_model_shader_collector({0,0,-100},.1f,view);
    require(source_model_shader_part(unrelated_child,1,material,view).choice.index!=high.choice.index,
        "model shader test cannot distinguish owner coverage from child-bound substitution");
    view.shader=source_shader_lod_view(0,2160,false,false);
    require(source_model_shader_part(root,1,material,view).choice.index==1,
        "first-person shader selection was silently forced to highest LOD");
    require(source_model_shader_part(root,1,{5,{.5f,.1f,0}},view).choice.index==0,
        "authored bias5 scan bypass changed");
    const auto empty=source_model_shader_part(root,1,{0,{}},view);
    require(empty.choice.index==-1 && empty.choice.accepted,"literal empty shader return changed");
}
}
int main(int argc,char **argv){try {
    require(argc==2,"model shader fixture required");const bool probe=std::string(argv[1])=="--probe";
    std::ifstream file;if(!probe){file.open(argv[1]);require(bool(file),"model shader fixture missing");}
    auto &stream=probe?std::cin:file;std::string line;unsigned cases=0;
    while(std::getline(stream,line)) {
        if(line.empty() || line[0]=='#')continue;
        std::istringstream input(line);input>>std::hex;unsigned count;
        require(bool(input>>count),"model shader count missing");Words row(count);
        for(auto &v:row)require(bool(input>>v),"model shader input truncated");
        const auto result=evaluate(row);
        if(probe){for(auto v:result)std::cout<<std::hex<<v<<' ';std::cout<<'\n';}
        else {for(auto v:result){unsigned expected;require(bool(input>>expected),"model shader output missing");
                if(v!=expected)throw std::runtime_error("model shader mismatch case"+std::to_string(cases));}
            std::string extra;require(!(input>>extra),"model shader fixture trailing output");}
        ++cases;
    }
    require(cases!=0,"model shader fixture empty");
    if(!probe){boundaries();std::cout<<cases<<" original model shader collector/part cases matched\n";}
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
