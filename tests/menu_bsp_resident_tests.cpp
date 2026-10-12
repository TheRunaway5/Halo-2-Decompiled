#include "halo2/source_bsp_resident_plan.hpp"
#include "halo2/source_transparent_parts.hpp"
#include <algorithm>
#include <bit>
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <tuple>

using namespace halo2;
namespace {
using Key=std::pair<int,unsigned>;
void require(bool value,const std::string &message) {if(!value)throw FormatError(message);}
struct Expected {
    int owner{};unsigned section{},part{};std::uint32_t shader{};
    unsigned coverage{},alpha{};std::uint32_t metric{},lod{};bool accepted{};
};
struct Case {
    int tick{};SourceBspShaderLodAuxiliary auxiliary;
    std::uint32_t controlled_shader=0xffffffffu;std::uint64_t controlled_offset{};
    std::array<std::uint32_t,48> bank{};
    std::array<std::uint32_t,7> shader_view{};
    std::vector<Expected> parts;
};
std::vector<Case> fixture(const std::filesystem::path &path) {
    std::ifstream input(path);require(bool(input),"original mainmenu resident fixture missing");
    std::vector<Case> cases;
    for(std::string line;std::getline(input,line);) {
        if(line.empty() || line[0]=='#')continue;
        std::istringstream row(line);char kind{};unsigned index{};row>>kind>>index;
        if(kind=='V') {
            require(index==cases.size(),"original mainmenu resident view order changed");Case value;
            row>>value.tick>>value.auxiliary.present>>value.auxiliary.byte5
               >>value.controlled_shader>>value.controlled_offset;
            for(auto &word:value.bank)row>>word;
            for(auto &word:value.shader_view)row>>word;
            require(bool(row),"original mainmenu resident view truncated");cases.push_back(value);
        } else if(kind=='P') {
            require(index<cases.size(),"original mainmenu resident part precedes its view");Expected value;
            row>>value.owner>>value.section>>value.part>>value.shader>>value.coverage
               >>value.alpha>>value.metric>>value.lod>>value.accepted;
            require(bool(row),"original mainmenu resident part truncated");cases[index].parts.push_back(value);
        } else throw FormatError("unknown original mainmenu resident fixture row");
        std::string extra;require(!(row>>extra),"original mainmenu resident fixture has extra fields");
    }
    require(cases.size()==12,"original mainmenu resident view census changed");return cases;
}
struct PrivateCache {
    std::filesystem::path folder,path;
    PrivateCache(const std::filesystem::path &source,const Case &control) {
        folder=std::filesystem::temp_directory_path()/
            ("halo2-menu-bsp-bias6-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        require(std::filesystem::create_directory(folder),"could not create private bias6 cache folder");
        try {
            path=folder/"mainmenu.map";std::filesystem::copy_file(source,path);
            std::fstream file(path,std::ios::binary|std::ios::in|std::ios::out);
            require(bool(file),"could not open private bias6 control cache");
            file.seekg(std::streamoff(control.controlled_offset));std::array<char,2> bias{};
            file.read(bias.data(),bias.size());require(bool(file) && bias==std::array<char,2>{},
                "private control no longer addresses an authored bias0 shader");
            file.seekp(std::streamoff(control.controlled_offset));bias[0]=6;
            file.write(bias.data(),bias.size());require(bool(file),"private bias6 control write failed");
        } catch(...) {std::filesystem::remove_all(folder);throw;}
    }
    ~PrivateCache() {std::error_code error;std::filesystem::remove_all(folder,error);}
};
const Material &lighting(const Material &root) {return root.environment_pass?*root.environment_pass:root;}
bool layer3(const Material &m) {
    return m.selected_shader_lod<m.lods.size() &&
        bool(m.lods[m.selected_shader_lod].available_layers&(1u<<3));
}
void assert_same_selection(const SourceBspResidentViewPlan &a,const SourceBspResidentViewPlan &b) {
    require(a.bindings.size()==b.bindings.size() && a.selected_parts.size()==b.selected_parts.size(),
        "existing default resident selection changed its catalogue or accepted count");
    for(unsigned i=0;i<a.bindings.size();++i)
        require(a.bindings[i].material==b.bindings[i].material && a.bindings[i].stipple==b.bindings[i].stipple,
            "existing default resident selection changed a binding");
    for(unsigned i=0;i<a.selected_parts.size();++i) {
        const auto &x=a.selected_parts[i],&y=b.selected_parts[i];
        require(std::tie(x.geometry,x.section,x.part,x.shader_lod,x.material,x.coverage_byte,x.instance_alpha)==
                std::tie(y.geometry,y.section,y.part,y.shader_lod,y.material,y.coverage_byte,y.instance_alpha),
            "existing default resident selection changed ordered source fields");
    }
}
struct Totals {unsigned views{},records{},accepted{},lower{},suppressed{},cinematic_changes{},materials{};};
void exercise(const Cache &cache,const std::vector<Case> &cases,bool controlled,Totals &total) {
    auto mesh=cache.load_bsp(0);const auto raw_roots=mesh.materials;const auto roots=unsigned(raw_roots.size());
    const auto parts=cache.load_source_transparent_bsp_parts(0);
    const auto geometry=cache.load_source_bsp_admission(0);
    require(geometry.clusters.size()==70 && geometry.instances.size()==104,"mainmenu owner census changed");
    SourceBspAdmission admission;
    for(unsigned i=0;i<geometry.clusters.size();++i)admission.clusters.push_back({i});
    for(unsigned i=0;i<geometry.instances.size();++i)admission.instances.push_back({i});
    // Explicit all-owner admission verifies the selection consumer. It does
    // not replace the separate original camera/portal visibility producer.
    SourceBspResidentPlan plan(cache,mesh,0,SourceBspResidentLighting::classification3);
    unsigned root_lighting{},lower_lighting{};
    for(unsigned i=0;i<mesh.materials.size();++i) {
        const auto &m=mesh.materials[i];
        if(!layer3(m)) {
            if(i<roots)require(m.forward_program.words==raw_roots[i].forward_program.words &&
                m.forward_pass_index==raw_roots[i].forward_pass_index,
                "classification3 rewrote a caller-owned nonlighting root");
            continue;
        }
        const auto &bound=lighting(m);
        require(bound.forward_pass && bound.passes.at(bound.forward_pass_index).implementation==3 &&
            !bound.static_lightmap_pass && !bound.baked_lightmap_texture,
            "title resident lighting selected a static or live-lighting implementation");
        require(bound.selected_shader_lod==m.selected_shader_lod && bound.source_shader_datum==m.source_shader_datum,
            "title resident lighting lost source shader/LOD identity");
        if(i<roots)++root_lighting;else ++lower_lighting;
    }
    require(root_lighting==179 && lower_lighting==190,"mainmenu classification3 LOD coverage changed");
    total.materials+=root_lighting+lower_lighting;
    std::map<Key,unsigned> catalogue;
    for(unsigned i=0;i<plan.parts().size();++i)catalogue.emplace(Key{plan.parts()[i].original.geometry,plan.parts()[i].original.part},i);
    std::map<Key,unsigned> prior_control;
    for(const auto &test:cases)if((test.controlled_shader!=0xffffffffu)==controlled) {
        const auto bank=std::bit_cast<SourceViewProjectionBank>(test.bank);
        require(std::bit_cast<std::array<std::uint32_t,7>>(source_shader_lod_view(0,480,
            test.auxiliary.present,test.auxiliary.byte5).multipliers)==test.shader_view,
            "resident view inputs differ from complete original1BEB0");
        const SourceBspShaderView view{bank,admission,0,480,1,false};
        const auto selected=plan.select(view,test.auxiliary);
        std::map<Key,const SourceBspShaderPart*> actual;
        for(const auto &part:selected.selected_parts)
            require(actual.emplace(Key{part.geometry,part.part},&part).second,"resident emitted duplicate all-owner part");
        unsigned expected_accepted{};
        for(const auto &want:test.parts) {
            ++total.records;const Key key{want.owner,want.part};
            const auto &section=want.owner>=0?parts.clusters.at(unsigned(want.owner)):
                parts.instance_definitions.at(want.section);
            const auto &literal=section.parts.at(want.part);
            require(literal.shader==want.shader,"resident original part shader differs from fixture");
            const auto scale=source_transparent_cluster_geometry(literal).lod_scale;
            require(std::bit_cast<std::uint32_t>(source_shader_part_metric(std::uint8_t(want.coverage),scale,
                source_shader_lod_player_multiplier(1)))==want.metric,"resident metric differs from original41040 multiplies");
            if(!catalogue.contains(key))continue; // No emitted source triangles.
            const auto index=catalogue.at(key);
            const auto &decision=selected.decisions.at(index);
            if(!want.accepted) {
                require(!actual.contains(key) && !selected.bindings.at(index).material &&
                    decision.state==SourceBspPartDecision::suppressed &&
                    decision.suppression==(want.alpha?SourceBspPartSuppression::rejected_lod:SourceBspPartSuppression::zero_alpha),
                    "resident retained an original alpha-zero/rejected part");++total.suppressed;continue;
            }
            ++expected_accepted;require(actual.contains(key),"resident omitted an original accepted part");
            const auto &got=*actual.at(key);
            require(got.section==want.section && got.shader_lod==want.lod &&
                got.coverage_byte==want.coverage && got.instance_alpha==want.alpha,
                "resident selection differs from original instructions at tick"+std::to_string(test.tick)+
                " owner"+std::to_string(want.owner)+" part"+std::to_string(want.part));
            const auto &material=mesh.materials.at(got.material);
            require(material.source_shader_datum==want.shader && material.selected_shader_lod==want.lod &&
                selected.bindings.at(index).material==got.material && decision.selection &&
                decision.selection->shader_lod==want.lod,
                "resident selection bound a different source shader/LOD/material");
            if(controlled) {
                if(test.auxiliary.present && test.auxiliary.byte5 && prior_control.contains(key))
                    total.cinematic_changes+=prior_control.at(key)!=got.shader_lod;
                else prior_control[key]=got.shader_lod;
            }
            ++total.accepted;total.lower+=want.lod>0;
        }
        require(expected_accepted==actual.size(),"resident selected an unauthored extra part");
        if(!controlled && !test.auxiliary.byte5)assert_same_selection(selected,plan.select(view));
        ++total.views;
    }
    if(!controlled) {
        // Keep the existing default constructor/select contracts independently
        // observable on an actual source part, without another full atlas.
        const auto first=std::find_if(mesh.vertices.begin(),mesh.vertices.end(),[&](const auto &v){return layer3(raw_roots.at(v.material));});
        require(first!=mesh.vertices.end(),"mainmenu has no static-default control part");
        Mesh small;small.materials.push_back(raw_roots.at(first->material));small.vertices.push_back(*first);small.vertices[0].material=0;
        const auto original_program=small.materials[0].forward_program.words;
        SourceBspResidentPlan defaults(cache,small,0);
        require(small.materials[0].forward_program.words==original_program,"default resident constructor rewrote its caller's root");
        const auto bank=std::bit_cast<SourceViewProjectionBank>(cases[0].bank);
        const SourceBspShaderView view{bank,admission,0,480,1,false};
        assert_same_selection(defaults.select(view),defaults.select(view,{false,false}));
        bool rejected=false;
        try {SourceBspResidentPlan bad(cache,small,0,SourceBspResidentLighting(99));}
        catch(const FormatError &) {rejected=true;}
        require(rejected,"unsupported resident lighting policy was silently selected");
    }
}
}
int main(int argc,char **argv) {
    try {
        require(argc==3,"menu BSP resident test requires original cache and original fixture");
        if(!std::filesystem::exists(argv[1]))return 77;
        const auto cases=fixture(argv[2]);Totals total;
        {const Cache cache(argv[1]);exercise(cache,cases,false,total);}
        const auto control=std::find_if(cases.begin(),cases.end(),[](const auto &c){return c.controlled_shader!=0xffffffffu;});
        require(control!=cases.end(),"private cinematic forwarding control missing");
        const PrivateCache copy(argv[1],*control);
        const Cache controlled(copy.path,std::filesystem::path(argv[1]).parent_path());
        require(controlled.load_source_shader_lod_definition(control->controlled_shader).bias==6,
            "private cinematic control failed to change its isolated source input");
        exercise(controlled,cases,true,total);
        require(total.views==12 && total.records==5052 && total.lower && total.suppressed && total.cinematic_changes,
            "resident original fixture lacks selected lower LOD/alpha/cinematic forwarding controls");
        std::cout<<total.views<<" original views, "<<total.records<<" authored part records, "<<total.accepted
            <<" accepted, "<<total.lower<<" lower LOD, "<<total.suppressed<<" suppressed, "<<total.cinematic_changes
            <<" private bias6 cinematic changes; "<<total.materials<<" classification3 material banks; default preserved\n";
        return 0;
    } catch(const std::exception &error) {std::cerr<<error.what()<<'\n';return 1;}
}
