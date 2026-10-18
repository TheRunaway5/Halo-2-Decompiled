#include "halo2/model.hpp"
#include "halo2/original_data.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>

using namespace halo2;
using Json=nlohmann::json;
static std::array<std::uint32_t,3> words(Vec3 v) {
    return {std::bit_cast<std::uint32_t>(v.x),std::bit_cast<std::uint32_t>(v.y),std::bit_cast<std::uint32_t>(v.z)};
}
static std::uint64_t digest(const RenderModel &model) {
    std::uint64_t hash=14695981039346656037ull;
    const auto w=[&](std::uint32_t value){for(unsigned i=0;i<4;++i){hash^=(value>>(8*i))&255u;hash*=1099511628211ull;}};
    const auto xyz=[&](Vec3 v){for(auto x:words(v))w(x);};
    for(unsigned i=0;i<model.mesh.vertices.size();++i) {
        const auto &v=model.mesh.vertices[i];const auto &o=model.vertex_origins.at(i);
        xyz(v.position);xyz(v.source_local_position);xyz(v.normal);xyz(v.tangent);xyz(v.binormal);xyz(v.incident);xyz(v.color);
        w(std::bit_cast<std::uint32_t>(v.uv.x));w(std::bit_cast<std::uint32_t>(v.uv.y));
        w(v.material);w(v.source_section);w(v.source_part);w(v.source_part_type);w(v.source_part_flags);
        w(o.section);w(o.vertex);w(o.part);w(o.vertex_class);
        for(auto b:model.skin.at(i).bones)w(b);
        for(auto a:model.skin.at(i).weights)w(std::bit_cast<std::uint32_t>(a));
    }
    return hash;
}
int main(int argc,char **argv)try {
    if(argc!=4)throw FormatError("Supply original map, authenticated TU1.5 XBE, expected raw-stream fixture");
    const auto original=OriginalData::load_xbe(argv[2]);
    if(original->image!=OriginalData::Image::TitleUpdate15)throw FormatError("TU1.5 authority required");
    Cache cache(argv[1],std::filesystem::path(argv[1]).parent_path(),original);
    std::ifstream input(argv[3]);const auto expected=Json::parse(input);
    Json output;output["scope"]="Actual cache preview and all six source section LODs; CPU decode only, no GPU or original rendered-pixel equivalence";
    output["controls"]=Json::array();
    for(auto datum:{0xe23e00c9u,0xe24600d1u,0xe26000ebu,0xe1ee0079u,0xe21c00a7u}) {
        const auto model=cache.load_model(datum,false);
        output["controls"].push_back({{"datum",datum},{"name",model.name},{"vertices",model.mesh.vertices.size()},{"digest",digest(model)}});
    }
    const auto marine=std::ranges::find_if(cache.tags(),[](const auto &t){return t.group=="bipd" && t.name=="objects\\characters\\marine\\marine";});
    if(marine==cache.tags().end())throw FormatError("Actual compressed skinned Marine control unavailable");
    const auto skinned=cache.load_model(marine->datum,false);
    output["controls"].push_back({{"datum",marine->datum},{"name",skinned.name},{"vertices",skinned.mesh.vertices.size()},{"digest",digest(skinned)}});
    unsigned errors{},uv_errors{},identity_errors{},checked{};
    output["reach"]=Json::array();
    for(int mode=-1;mode<6;++mode) {
        const std::array sections{0u};
        const auto model=mode<0?cache.load_model(0xe26600f1u,false):
            cache.load_model_sections(0xe26600f1u,false,0,unsigned(mode),sections);
        if(model.mesh.vertices.size()!=12 || model.vertex_origins.size()!=12 || model.skin.size()!=12)
            throw FormatError("Reach authored four-triangle strip topology changed");
        unsigned mismatch{},uv_mismatch{},identity_mismatch{};
        for(unsigned i=0;i<12;++i) {
            const auto index=expected["expanded_indices"][i].get<unsigned>();
            const auto &v=model.mesh.vertices[i];const auto &o=model.vertex_origins[i];
            const auto position=expected["positions"][index].get<std::array<std::uint32_t,3>>();
            const auto uv=expected["uv"][index].get<std::array<std::uint32_t,2>>();
            mismatch+=words(v.position)!=position;
            mismatch+=words(v.source_local_position)!=position;
            uv_mismatch+=std::array{std::bit_cast<std::uint32_t>(v.uv.x),std::bit_cast<std::uint32_t>(v.uv.y)}!=uv;
            identity_mismatch+=o.section!=0 || o.part!=0 || o.vertex!=index || o.vertex_class!=1 ||
                model.skin[i].bones[0]!=0 || model.skin[i].weights[0]!=1.f;
            ++checked;
        }
        errors+=mismatch;uv_errors+=uv_mismatch;identity_errors+=identity_mismatch;
        output["reach"].push_back({{"mode",mode},{"position_errors",mismatch},{"packed_uv_errors",uv_mismatch},
            {"topology_skin_identity_errors",identity_mismatch},{"digest",digest(model)}});
    }
    output["expanded_vertices_checked"]=checked;output["position_errors"]=errors;
    output["packed_uv_errors"]=uv_errors;output["topology_skin_identity_errors"]=identity_errors;
    output["status"]=(errors||uv_errors||identity_errors)?"FAIL":"PASS";
    std::cout<<output.dump(2)<<'\n';return errors||uv_errors||identity_errors?1:0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 2;}
