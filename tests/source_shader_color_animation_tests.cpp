#include "halo2/appearance.hpp"
#include "halo2/shader_animation.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace halo2;
namespace {
using Words=std::vector<std::uint32_t>;
void require(bool value,const char *message) {
    if(!value)throw std::runtime_error(message);
}
struct Input {
    Material material;
    ObjectColors colors;
    float seconds{};
};
Input decode(const Words &row) {
    unsigned index=0;
    const auto word=[&]{return row.at(index++);};
    const auto real=[&]{return std::bit_cast<float>(word());};
    Input result;
    auto &m=result.material;
    m.forward_pass=true;m.forward_pass_index=0;
    m.passes.emplace_back();
    auto &pass=m.passes[0];
    for(auto &v:pass.program.words)v=word();
    for(auto &bank:pass.constants)for(auto &vector:bank)for(auto &v:vector)v=real();
    for(auto &v:result.colors)v={real(),real(),real()};
    result.seconds=real();
    const bool fragmented=word();
    const auto selected=word(),offset=word();
    m.shader_switches[0]=selected;
    if(fragmented) {
        pass.fragments.push_back({0,{std::uint16_t(8<<9)}});
        pass.program.fragment_offsets[0]=offset;
    }
    const auto remaps=[&](std::vector<ShaderRemap>& output) {
        const auto count=word();
        for(unsigned i=0;i<count;++i) {
            const auto v=word();
            output.push_back({std::uint16_t(v),std::uint8_t(v>>16),std::uint8_t(v>>24)});
        }
    };
    remaps(pass.pixel_externs);
    remaps(pass.vertex_externs);
    auto &a=m.animation;
    std::vector<ShaderRemap> pixel,vertex;
    remaps(pixel);remaps(vertex);
    a.mappings=pixel;a.mappings.insert(a.mappings.end(),vertex.begin(),vertex.end());
    a.lods.push_back({1,{std::uint16_t(1<<9)}});
    a.layers.push_back({std::uint16_t(1<<9)});
    a.passes.push_back({std::uint16_t(1<<9)});
    a.implementations.push_back({{}, {std::uint16_t(pixel.size()<<9)},
        {std::uint16_t((vertex.size()<<9)|pixel.size())}});
    const auto functions=word();
    require(functions!=0,"shader color animation fixture has no function");
    for(unsigned i=0;i<functions;++i) {
        ShaderOverlay overlay;overlay.period=real();
        for(unsigned j=0;j<5;++j) {
            const auto v=word();
            for(unsigned k=0;k<4;++k)overlay.curve.push_back(std::byte(v>>(k*8)));
        }
        a.parameters.push_back({std::uint16_t((1<<9)|i)});
        a.references.push_back({std::int16_t(i),0});
        a.overlays.push_back(std::move(overlay));
    }
    a.tables=std::make_shared<ShaderFunctionTables>();
    require(index==row.size(),"shader color animation input has trailing words");
    m.forward_program=pass.program;m.program_constants=pass.constants;
    return result;
}
Words output(const Material& m) {
    Words result(m.forward_program.words.begin(),m.forward_program.words.end());
    for(const auto& bank:m.program_constants)for(const auto& vector:bank)
        for(const auto v:vector)result.push_back(std::bit_cast<std::uint32_t>(v));
    return result;
}
Words evaluate(const Words& row) {
    auto input=decode(row);
    apply_object_colors(input.material,input.colors);
    require(animate_forward_material(input.material,input.seconds,{}),"actual native animation path was not admitted");
    const auto expected=output(input.material);
    // Rebuilding the same draw from its retained object bank must not depend
    // on the previous frame's animated program words or vertex components.
    input.material.forward_program.words.fill(0xfeedface);
    for(auto& bank:input.material.program_constants)for(auto& v:bank)v.fill(-23);
    require(animate_forward_material(input.material,input.seconds,{}),"retained bank animation failed");
    require(output(input.material)==expected,"animation retained stale previous-frame bindings");
    return expected;
}
void controls(const Words& row,bool& missing_detected,bool& late_detected) {
    auto good=decode(row);apply_object_colors(good.material,good.colors);
    animate_forward_material(good.material,good.seconds,{});
    const auto expected=output(good.material);
    auto missing=decode(row);
    // Historical production regression: pixel object externs are bound once,
    // then the animation resets to the authored pass without retaining them.
    apply_object_colors(missing.material,missing.colors);
    missing.material.object_render_colors.reset();
    animate_forward_material(missing.material,missing.seconds,{});
    missing_detected|=output(missing.material)!=expected;
    auto late=good;
    reapply_object_colors(late.material);
    late_detected|=output(late.material)!=expected;
}
}
int main(int argc,char** argv) {try {
    require(argc==2,"shader color animation fixture required");
    const bool probe=std::string(argv[1])=="--probe";
    std::ifstream file;
    if(!probe){file.open(argv[1]);require(bool(file),"shader color animation fixture missing");}
    auto& stream=probe?std::cin:file;
    std::string line;unsigned cases=0;bool missing_detected=false,late_detected=false;
    while(std::getline(stream,line)) {
        if(line.empty()||line[0]=='#')continue;
        std::istringstream in(line);in>>std::hex;unsigned count;
        require(bool(in>>count),"shader color animation input count missing");
        Words row(count);for(auto& v:row)require(bool(in>>v),"shader color animation input truncated");
        const auto result=evaluate(row);
        if(probe) {for(const auto v:result)std::cout<<std::hex<<v<<' ';std::cout<<'\n';}
        else {
            for(const auto v:result) {unsigned expected;
                require(bool(in>>expected),"shader color animation output truncated");
                if(v!=expected)throw std::runtime_error("shader color animation mismatch case"+std::to_string(cases));
            }
            std::string extra;require(!(in>>extra),"shader color animation output has trailing words");
            controls(row,missing_detected,late_detected);
        }
        ++cases;
    }
    require(cases!=0,"shader color animation fixture empty");
    if(!probe) {
        require(missing_detected,"color-loss negative control did not differ");
        require(late_detected,"reversed binding-order negative control did not differ");
        std::cout<<cases<<" original object-color/animation binding cases matched; both wrong-order controls detected\n";
    }
    return 0;
} catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
