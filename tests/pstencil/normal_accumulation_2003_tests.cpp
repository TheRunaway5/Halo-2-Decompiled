#include "halo2/pstencil/normal_accumulation_2003.hpp"
#include "halo2/pstencil/reference_light_materials.hpp"
#include "../scalar_combiners.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;
namespace {
void require(bool value,const char* why){if(!value)throw std::runtime_error(why);}
void sampler(const nlohmann::json& selection,unsigned stage,const ShaderTextureState& state,unsigned& fields) {
    const std::array values{state.address_u,state.address_v,state.address_w,state.mag_filter,
        state.min_filter,state.mip_filter,state.lod_bias,state.max_mip,state.anisotropy,
        state.color_key_op,state.color_sign,state.alpha_kill,state.border_color,state.key_color};
    for(const auto& entry:selection["states"])if(entry["texture stage index*"]==stage) {
        const unsigned index=entry["D3D state*"];
        require(index<values.size(),"Unknown source sampler field");
        require(values[index]==entry["data*"],"Normal path sampler differs from original resident binding");++fields;
    }
}
}
int main(int argc,char** argv)try {
    require(argc==3,"Supply original accumulation constructor and resident binding references");
    nlohmann::json construction,bindings;
    std::ifstream(argv[1])>>construction;std::ifstream(argv[2])>>bindings;
    require(construction["original_sha256"]=="5a9edb4c874a782594c86020db49b0086f44ae38f53ff2276b3192a2c2383d18",
        "Unexpected original executable fixture");
    require(construction["instruction_patches"].empty() && construction["replaced_services"].empty(),
        "Original program construction was patched");
    unsigned programs{},materials{},fields{};
    const auto program=pstencil::normal_accumulation_2003();
    for(const auto& row:construction["fixtures"])if(row["builtin"]==1) {
        require(program.words==row["program_words"].get<std::array<std::uint32_t,60>>(),
            "Normal program differs from unchanged original builtin1 output");++programs;
    }
    require(programs==1,"Fixed original normal constructor fixture missing");
    for(const auto& row:bindings["shaders"]) {
        const auto name=row["shader"]["name"].get<std::string>();
        const auto* material=pstencil::reference_light_material(name);
        bool producer=false,consumer=false;
        for(const auto& selection:row["selections"])if(selection["lighting_model"]==0) {
            if(selection["layer"]==2) {
                require(material,"Authenticated normal producer absent from admitted material set");
                require(selection["builtin"]==1,"Normal producer has another builtin program");
                sampler(selection,0,pstencil::normal_bump_sampler_2003(material->utility_builtin!=10),fields);
                sampler(selection,3,pstencil::normalization_cube_sampler_2003(),fields);
                require(selection["textures"][3]["override"]==1,"Normal producer cube binding changed");
                producer=true;
            }
            if(material && selection["layer"]==16) {
                sampler(selection,0,pstencil::normal_target_sampler_2003(),fields);
                sampler(selection,1,pstencil::normalization_cube_sampler_2003(),fields);
                require(selection["textures"][0]["override"]==2,"Normal utility no longer consumes target9");
                consumer=true;
            }
        }
        require(bool(material)==producer,"Material normal-path admission differs from primary resident layer2 set");
        if(producer){require(consumer,"Normal producer lacks its matching consumer");++materials;}
    }
    require(materials==16 && fields>=570,"Normal producer/consumer sampler coverage incomplete");
    // Independent scalar evaluation checks the source final-combiner ownership.
    for(unsigned n=0;n<256;++n) {
        const float alpha=float(n)/255.f;
        const ShaderVector cube{float((n*37)%256)/255.f,float((n*11)%256)/255.f,float((n*83)%256)/255.f,.25f};
        const std::array<ShaderVector,4> samples{ShaderVector{.3f,.4f,.5f,alpha},ShaderVector{},ShaderVector{},cube};
        const auto output=halo2_test::scalar_combiners(program.words,samples,{0,0,0,1},{0,0,0,1});
        require(output[0]==cube[0] && output[1]==cube[1] && output[2]==cube[2] && output[3]==alpha,
            "Normal target does not retain normalization cube RGB and original bump alpha");
    }
    std::cout<<programs<<" original 60-word normal program; "<<materials<<" admitted source materials; "
        <<fields<<" original sampler fields; 256 independent RGB/alpha ownership cases\n";return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
