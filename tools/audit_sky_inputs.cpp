// Explicit offline authority audit; original maps never enter normal gameplay.
#include "halo2/sky_input_identity.hpp"
#include "halo2/appearance.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include "halo2/model.hpp"
#include <fstream>
#include <iostream>
#include <map>
#include <set>
using namespace halo2;
namespace {
using namespace halo2::sky_audit;
json differences(const json& a,const json& b) {
    json out=json::object();for(auto i=a.begin();i!=a.end();++i)
        if(!b.contains(i.key()) || i.value()!=b.at(i.key()))out[i.key()]={{"original",i.value()},{"prepared",b.value(i.key(),json(nullptr))}};
    for(auto i=b.begin();i!=b.end();++i)if(!a.contains(i.key()))out[i.key()]={{"original",nullptr},{"prepared",i.value()}};
    return out;
}
std::set<std::pair<unsigned,int>> visible(const Cache& cache) {
    std::set<std::pair<unsigned,int>> out;
    for(unsigned bsp=0;bsp<cache.bsps().size();++bsp) {
        const auto a=cache.load_bsp_atmosphere(bsp);
        for(const auto& c:a.clusters) {
            int sky=c.visible_sky;if(sky<0 && (a.scenario_flags&2u) && !a.skies.empty())sky=0;
            if(sky>=0)out.emplace(bsp,sky);
        }
    }
    return out;
}
std::map<std::string,const Material*> used(const RenderModel& model) {
    std::set<unsigned> used;for(const auto& v:model.mesh.vertices)used.insert(v.material);
    std::map<std::string,const Material*> out;
    for(auto index:used) {
        const auto& v=model.mesh.materials.at(index);
        if(!out.emplace(v.name,&v).second)throw FormatError("Ambiguous authored sky material name");
    }
    return out;
}
void binary(const std::filesystem::path& path,const Texture& texture) {
    if(std::filesystem::exists(path))return;
    std::ofstream out(path,std::ios::binary);out.write(reinterpret_cast<const char*>(texture.pixels.data()),texture.pixels.size());
    if(!out)throw FormatError("Cannot export bounded offline correction input");
}
}
int main(int argc,char** argv)try {
    if(argc!=4 && argc!=5)throw FormatError("usage: audit_sky_inputs PROJECT PREPARED_ROOT REPORT [SCRATCH_EXPORT]");
    const std::filesystem::path root=argv[1],prepared=argv[2],output=argv[3],exports=argc==5?argv[4]:"";
    if(!exports.empty())std::filesystem::create_directories(exports);
    json report={{"schema","H2_NATIVE_SKY_INPUT_AUDIT_V1"},{"maps",json::array()},
        {"texture_pairs",0},{"material_pairs",0},{"sky_pairs",0},{"failures",0}};
    std::vector<std::filesystem::path> paths;
    for(const auto& directory:{root/"assets/disc/maps",root/"assets/map-pack/maps"})
        for(const auto& p:std::filesystem::directory_iterator(directory))
            if(p.is_regular_file() && p.path().extension()==".map")paths.push_back(p.path());
    std::sort(paths.begin(),paths.end());
    unsigned failures=0,material_pairs=0,texture_pairs=0,sky_pairs=0;
    for(const auto& original_path:paths) {
        const auto actual_path=prepared/"halo2/h2_maps_win64_dx11"/original_path.filename();
        if(!std::filesystem::exists(actual_path))continue;
        json map={{"map",original_path.filename().string()},{"skies",json::array()}};
        try {
            Cache original(original_path),actual(actual_path,actual_path.parent_path(),{},prepared);
            const auto original_visible=visible(original),actual_visible=visible(actual);
            if(original_visible!=actual_visible){map["visibility_mismatch"]=true;++failures;}
            for(const auto& [bsp,ordinal]:original_visible) {
                const auto a=original.load_sky(ordinal),b=actual.load_sky(ordinal);
                if(!a)continue;
                json row={{"bsp",bsp},{"ordinal",ordinal},{"materials",json::array()}};++sky_pairs;
                if(!b){row["missing_sky"]=true;++failures;map["skies"].push_back(row);continue;}
                row["definition_differences"]=differences(sky(*a,original),sky(*b,actual));
                if(!row["definition_differences"].empty())++failures;
                auto source=original.load_model(a->model,true),candidate=actual.load_model(b->model,true);
                prepare_forward_materials(original,source.mesh);prepare_forward_materials(actual,candidate.mesh);
                auto source_materials=used(source),candidate_materials=used(candidate);
                if(source_materials.size()!=candidate_materials.size()){row["material_count_mismatch"]=true;++failures;}
                for(const auto& [name,source_ptr]:source_materials) {
                    json m={{"name",name},{"textures",json::array()}};
                    const auto found=candidate_materials.find(name);
                    if(found==candidate_materials.end()){m["missing_material"]=true;++failures;row["materials"].push_back(m);continue;}
                    const auto& x=*source_ptr;const auto& y=*found->second;++material_pairs;
                    m["semantic_differences"]=differences(semantic(x,original),semantic(y,actual));
                    if(!m["semantic_differences"].empty())++failures;
                    const std::array<const Texture*,4> xt{&x.diffuse,&x.detail,&x.bump,&x.auxiliary},yt{&y.diffuse,&y.detail,&y.bump,&y.auxiliary};
                    for(unsigned slot=0;slot<4;++slot){++texture_pairs;if(!texture_equal(*xt[slot],*yt[slot])) {
                        ++failures;m["textures"].push_back({{"kind","bound"},{"slot",slot},{"original",texture(*xt[slot])},{"prepared",texture(*yt[slot])}});
                    }}
                    if(x.animated_textures.size()!=y.animated_textures.size())throw FormatError("Animated sky stage count differs");
                    for(unsigned stage=0;stage<4;++stage) {
                        if(x.animated_textures[stage].size()!=y.animated_textures[stage].size()) {
                            ++failures;m["animated_frame_count_differences"].push_back(stage);continue;
                        }
                        for(unsigned frame=0;frame<x.animated_textures[stage].size();++frame){++texture_pairs;
                            if(!texture_equal(x.animated_textures[stage][frame],y.animated_textures[stage][frame])) {
                                ++failures;m["textures"].push_back({{"kind","animated"},{"stage",stage},{"frame",frame},
                                    {"original",texture(x.animated_textures[stage][frame])},{"prepared",texture(y.animated_textures[stage][frame])}});
                            }
                        }
                    }
                    if(x.bitmaps.size()!=y.bitmaps.size()){++failures;m["bitmap_count_mismatch"]=true;}
                    for(unsigned slot=0;slot<std::min(x.bitmaps.size(),y.bitmaps.size());++slot) {
                        const auto& xb=x.bitmaps[slot];const auto& yb=y.bitmaps[slot];
                        const auto* x_tag=original.find_tag({xb.datum},tag_groups::bitmap);
                        const auto* y_tag=actual.find_tag({yb.datum},tag_groups::bitmap);
                        if(!x_tag && !y_tag)continue; // Identical typed NONE metadata remains compared above.
                        if(!x_tag || !y_tag){++failures;m["one_sided_invalid_bitmap"].push_back(slot);continue;}
                        const auto tx=original.load_bitmap(xb.datum,xb.index),ty=actual.load_bitmap(yb.datum,yb.index);++texture_pairs;
                        if(!texture_equal(tx,ty)) {
                            ++failures;json t={{"kind","authored"},{"slot",slot},{"frame",xb.index},
                                {"original_tag",tag_name(original,xb.datum)},{"prepared_tag",tag_name(actual,yb.datum)},
                                {"original_datum",xb.datum},{"prepared_datum",yb.datum},{"original",texture(tx)},{"prepared",texture(ty)}};
                            if(!exports.empty()) {
                                const auto target=sha256({tx.pixels.data(),tx.pixels.size()}),donor=sha256({ty.pixels.data(),ty.pixels.size()});
                                binary(exports/(target+".target"),tx);binary(exports/(donor+".donor"),ty);
                                t["target_file"]=target+".target";t["donor_file"]=donor+".donor";
                            }
                            m["textures"].push_back(t);
                        }
                    }
                    row["materials"].push_back(m);
                }
                map["skies"].push_back(row);
            }
        }catch(const std::exception& e){map["error"]=e.what();++failures;}
        if(!map["skies"].empty() || map.contains("error"))report["maps"].push_back(map);
        std::cout<<map["map"]<<": audited\n";
    }
    report["failures"]=failures;report["sky_pairs"]=sky_pairs;report["material_pairs"]=material_pairs;report["texture_pairs"]=texture_pairs;report["passed"]=failures==0;
    std::ofstream out(output);out<<report.dump(2)<<'\n';if(!out)throw FormatError("Cannot save sky input audit report");
    std::cout<<report["sky_pairs"]<<" sky pairs, "<<material_pairs<<" material pairs, "<<texture_pairs<<" complete texture pairs, "<<failures<<" failures\n";
    return failures?1:0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
