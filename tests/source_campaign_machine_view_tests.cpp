#include "halo2/source_machine_scene.hpp"
#include "halo2/source_campaign_machine_view.hpp"
#include "halo2/content.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <thread>

namespace {
using namespace halo2;unsigned checks{},negatives{};
void check(bool value,const char*message){++checks;if(!value)throw FormatError(message);}
template<class F>void reject(F&&call,const char*message){bool rejected{};try{call();}catch(const FormatError&){rejected=true;}check(rejected,message);++negatives;}
}
int main(int argc,char**argv)try {
    if(argc!=2&&argc!=3)throw FormatError("Supply actual MCC Armory map and optional owned report path");
    if(!*argv[1]||!std::filesystem::is_regular_file(argv[1]))return 77;
    const Cache cache(argv[1],std::filesystem::path(argv[1]).parent_path());
    check(cache.is_mcc(),"Campaign machine test requires actual MCC runtime inputs");
    const auto content=cache.load_content(0,false);Mesh mesh;
    // Controlled namespace prefixes test transfer arithmetic, not shader data.
    constexpr unsigned before=3,after=7;mesh.materials.resize(before);
    const auto owner=SourceMachineScene::campaign_resources(cache,content.objects,0,mesh);
    const auto target=std::ranges::find_if(cache.tags(),[](const auto&tag){return tag.group=="mach"&&
        tag.name=="scenarios\\objects\\solo\\spacestation\\ss_exterior_gun\\ss_exterior_gun_hinge\\ss_exterior_gun_hinge";});
    check(target!=cache.tags().end(),"Actual Armory gun-hinge definition is absent");
    check(owner->owns_definition(target->datum),"Actual authored Armory PRT resource owner did not prepare");
    check(!owner->owns_definition(0xffffffffu),"Unknown definition acquired a campaign machine owner");
    check(owner->stats().prt_materials>0&&owner->stats().union_section_banks>0,"Actual PRT stream/section banks are absent");
    const auto material_count=mesh.materials.size();
    std::vector<std::uint32_t> identities;std::vector<unsigned> banks;
    for(unsigned n=before;n<material_count;++n){identities.push_back(mesh.materials[n].source_shader_datum);
        const auto&pass=mesh.materials[n].environment_pass;
        banks.push_back(pass&&pass->vertex_bank?unsigned(pass->vertex_bank->constants.size()):0);}
    owner->begin_campaign_frame(1);
    reject([&]{owner->begin_campaign_frame(1);},"Repeated enclosing epoch was accepted");
    reject([&]{owner->begin_campaign_frame(0);},"Cold epoch was accepted as completed frame");
    reject([&]{owner->update_pose(0,0,mesh);},"Resource-only machine started a second pose/controller");
    reject([&]{owner->begin_source_frame();},"Resource-only machine used a legacy private view slot");
    reject([&]{(void)owner->fork();},"Resource-only machine created an effects/private-controller fork");
    reject([&]{std::vector<Vertex>vertices;std::vector<ObjectLightingBinding>light;owner->append(vertices,light);},
        "Resource-only machine submitted a private/default pose");
    bool foreign_rejected{};std::thread foreign([&]{try{(void)owner->owns_definition(target->datum);}catch(const FormatError&){foreign_rejected=true;}});foreign.join();
    check(foreign_rejected,"Campaign machine resource crossed its executor");
    reject([&]{owner->rebase_campaign_materials(before,0xfffffff0u);},"Overflowing material namespace transfer was accepted");
    check(owner->owns_definition(target->datum)&&mesh.materials.size()==material_count,"Rejected rebase changed the resource owner");
    std::vector<Material> transferred(after);transferred.insert(transferred.end(),
        std::make_move_iterator(mesh.materials.begin()+before),std::make_move_iterator(mesh.materials.end()));
    mesh.materials=std::move(transferred);owner->rebase_campaign_materials(before,after);
    check(owner->owns_definition(target->datum),"Transferred actual PRT resource owner became unavailable");
    for(unsigned n=0;n<identities.size();++n){const auto&material=mesh.materials[after+n];const auto&pass=material.environment_pass;
        check(material.source_shader_datum==identities[n],"Material transfer changed an authored shader");
        check((pass&&pass->vertex_bank?unsigned(pass->vertex_bank->constants.size()):0)==banks[n],"Material transfer replaced a real resource bank");}
    reject([&]{owner->rebase_campaign_materials(before,after);},"Repeated old namespace transfer was accepted");
    owner->reset_lifetimes();owner->begin_campaign_frame(1);
    check(owner->owns_definition(target->datum),"Frame retirement discarded authored resources");
    nlohmann::json report={{"passed",true},{"checks",checks},{"negatives",negatives},
        {"actual_definition",target->datum},{"resource_materials",material_count-before},
        {"prepared_models",owner->stats().render_models},{"prt_materials",owner->stats().prt_materials},
        {"section_banks",owner->stats().union_section_banks},{"deferred_owners",owner->stats().deferred_resources},
        {"scope","Actual MCC cold resources/rebase/executor/frame/controller guards; genuine World packet extraction is validated separately"},
        {"fake_coefficients_or_source_outcomes",false}};
    const auto scratch=std::getenv("HALO2_TEST_SCRATCH");
    if(argc==2&&!scratch)throw FormatError("Campaign machine regression requires its owned test scratch");
    const auto report_path=argc==3?std::filesystem::path(argv[2]):std::filesystem::path(scratch)/"campaign-machine-resources.json";
    std::ofstream out(report_path);out<<report.dump(2)<<'\n';check(bool(out),"Owned cold-resource report failed");
    std::cout<<"PASS "<<checks<<" campaign machine cold/rebase/frame checks; genuine PRT resource owner\n";return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}
