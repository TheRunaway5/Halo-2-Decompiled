#include "halo2/scene.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

using namespace halo2;
namespace {
void require(bool value,const std::string &message) {if(!value)throw FormatError(message);}
template<class F>void rejects(F action,const char *message) {
    bool caught=false;try{action();}catch(const FormatError &){caught=true;}
    require(caught,message);
}
struct Authored {ObjectLightContactDefinition definition;bool available{};};
std::map<std::uint32_t,Authored> authored(const char *path,const Cache &cache) {
    std::ifstream input(path);require(bool(input),"original flag31 fixture missing");
    std::map<std::uint32_t,Authored> result;std::string line;unsigned rows{},actual_rows{};
    while(std::getline(input,line)) {
        if(line.empty() || line[0]=='#')continue;
        std::istringstream stream(line);std::string kind,label;std::array<std::uint32_t,13> w{};
        require(bool(stream>>kind>>label) && kind=="F","unknown original scene attribution fixture row");
        for(auto &v:w)require(bool(stream>>std::hex>>v),"short original scene attribution fixture row");
        std::string extra;require(!(stream>>extra),"trailing original scene attribution fixture row");++rows;
        if(!label.starts_with("actual_"))continue;
        require(w[9]<=1,"original scene attribution AL exceeds boolean width");
        const auto definition=cache.load_object_light_contact_definition(w[0]);
        require(definition && definition->datum==w[0] && definition->model==w[1] &&
            definition->render_model==w[2] && definition->collision==w[3] &&
            definition->prt_count==w[4] && definition->section_render_leaves_count==w[5],
            "actual scene attribution metadata differs from original "+label);
        const auto [entry,inserted]=result.emplace(w[0],Authored{*definition,bool(w[9])});
        require(inserted || entry->second.available==bool(w[9]),"original attribution depends on unrelated body flags");
        ++actual_rows;
    }
    require(input.eof() && rows==1818 && actual_rows==90 && result.size()==15,
        "original scene attribution fixture is incomplete");return result;
}
unsigned slot(std::uint32_t handle) {return handle&65535u;}
unsigned generation(std::uint32_t handle) {return handle>>16;}
void actual_lifetimes(const Cache &cache,const std::map<std::uint32_t,Authored> &expected) {
    const auto content=cache.load_content(0);unsigned calls{};
    const auto resolver=[&](std::uint32_t datum) {++calls;return cache.load_object_light_contact_definition(datum);};
    SceneObjects scene(content,0,{},resolver),plain(content,0);
    require(scene.objects().size()==plain.objects().size(),"birth projection changed actual startup allocations");
    unsigned live{},named{};int name=-1;std::uint32_t original=null_datum;
    for(std::size_t i=0;i<scene.objects().size();++i) {
        const auto &object=scene.objects()[i],&without=plain.objects()[i];
        require(object.handle==without.handle && object.placement==without.placement &&
            object.created_tick==without.created_tick && object.visible==without.visible,
            "birth resolver changed actual startup object identity/state");
        require(!without.render_attribution_at_birth,"no-resolver scene manufactured a known birth projection");
        if(object.handle==null_datum) {
            require(!object.render_attribution_at_birth,"startup destruction retained an attribution projection");continue;
        }
        const auto &placement=content.objects.at(object.placement);
        const auto found=expected.find(placement.definition);
        require(found!=expected.end() && object.render_attribution_at_birth==found->second.available,
            "actual startup birth differs from original BF9A0 predicate");++live;
        if(placement.name>=0) {++named;if(name<0){name=placement.name;original=object.handle;}}
    }
    require(live && named && calls>=live,"actual startup scene birth coverage is empty");
    const auto initial_calls=calls;const auto saved=scene;
    require(scene.create(name,false,31)==original && calls==initial_calls,
        "existing named generation was reconstructed/reclassified");
    const auto projection=scene.get(original).render_attribution_at_birth;
    scene.visible(original,false);scene.visible(original,true);scene.switch_bsp(0,true);
    require(scene.get(original).render_attribution_at_birth==projection && calls==initial_calls,
        "retained actual BSP/script visibility recomputed construction attribution");
    scene.destroy(original);
    require(scene.resolve(name)==null_datum && !scene.objects().at(slot(original)).render_attribution_at_birth,
        "destroy did not clear generation attribution");
    rejects([&]{scene.get(original);},"destroyed generation remained accessible");
    const auto next_salt=scene.object_lifetimes().pool().next_salt();
    const auto next=scene.create(name,false,37);
    require(next!=original && generation(next)==next_salt &&
        scene.get(next).created_tick==37 && scene.get(next).render_attribution_at_birth==projection && calls==initial_calls+1,
        "actual recreation lost generation/tick/birth classification");
    const auto before_restore=calls;scene.restore(saved);
    require(calls==before_restore && scene.resolve(name)==original &&
        scene.get(original).render_attribution_at_birth==projection,
        "checkpoint restore recomputed or lost actual birth projection");
    rejects([&]{scene.get(next);},"restore retained the later generation");
    scene.destroy_all();
    for(const auto &object:scene.objects())require(object.handle==null_datum && !object.render_attribution_at_birth,
        "destroy_all retained birth metadata");
    require(calls==before_restore,"destruction invoked the birth resolver");
    scene.restore(saved);require(calls==before_restore,"second restore recomputed attribution");
    plain.destroy(plain.resolve(name));const auto unknown=plain.create(name,false,41);
    require(!plain.get(unknown).render_attribution_at_birth,"no-resolver recreation manufactured attribution");
    std::cout<<live<<" actual startup generations / "<<named<<" named lifetimes checked\n";
}
LevelContent manual(const ObjectLightContactDefinition &definition) {
    LevelContent content;ScenarioObject object;object.type=6;object.placement=0;object.name=0;
    object.definition=definition.datum;object.flags=1;content.objects={object};
    content.object_names={{"explicit native lifecycle control",6,0}};return content;
}
ObjectLightContactDefinition positive(const ObjectLightContactDefinition &authored) {
    // Deliberately synthetic positive predicate inputs, not an altered stock
    // definition or a claim that any actual mainmenu owner has flag31 set.
    auto value=authored;value.model=0xe0000001;value.render_model=0xe0000002;
    value.collision=0xe0000003;value.prt_count=1;value.section_render_leaves_count=1;return value;
}
void retained_projection_controls(const ObjectLightContactDefinition &definition) {
    auto content=manual(definition);content.objects[0].bsp_policy=2;content.objects[0].manual_bsps=1;
    unsigned a_calls{},b_calls{};const auto accepted=positive(definition);
    auto rejected=accepted;rejected.prt_count=0;
    const auto a_resolver=[&](std::uint32_t) {++a_calls;return std::optional{accepted};};
    const auto b_resolver=[&](std::uint32_t) {++b_calls;return std::optional{rejected};};
    SceneObjects a(content,0,{},a_resolver),b(content,0,{},b_resolver),unknown(content,0);
    const auto first=a.create(0,false,7),other=b.create(0,false,8);
    require(a.get(first).render_attribution_at_birth==true && b.get(other).render_attribution_at_birth==false,
        "synthetic positive/rejected construction controls did not differ");
    a.switch_bsp(1,true);
    require(a.get(first).bsp_hidden && !a.get(first).visible && a.get(first).render_attribution_at_birth==true && a_calls==1,
        "hidden retained BSP owner lost or recomputed birth projection");
    a.switch_bsp(0,true);
    require(!a.get(first).bsp_hidden && a.get(first).visible && a_calls==1,
        "retained BSP reappearance constructed a new generation");
    b.restore(a);
    require(a_calls==1 && b_calls==1 && b.resolve(0)==first && b.get(first).render_attribution_at_birth==true,
        "restore invoked a resolver or retained destination birth metadata");
    b.destroy(first);const auto next=b.create(0,false,9);
    require(a_calls==2 && b_calls==1 && generation(next)==generation(first)+1 &&
        b.get(next).render_attribution_at_birth==true,"restore did not copy the saved resolver/generation");
    b.restore(unknown);const auto absent=b.create(0,false,10);
    require(!b.get(absent).render_attribution_at_birth && a_calls==2 && b_calls==1,
        "restore did not clear an absent saved resolver");
}
void failures(const ObjectLightContactDefinition &definition) {
    const auto content=manual(definition);unsigned calls{},mode{};
    const auto resolver=[&](std::uint32_t) -> std::optional<ObjectLightContactDefinition> {
        ++calls;if(mode==0)return {};
        if(mode==1){auto wrong=definition;wrong.datum^=1;return wrong;}
        if(mode==2)throw FormatError("explicit resolver failure control");
        return definition;
    };
    SceneObjects scene(content,0,{},resolver);
    for(mode=0;mode<3;++mode) {
        rejects([&]{scene.create(0);},"unresolved/wrong/failed metadata was published");
        require(scene.objects().empty() && scene.resolve(0)==null_datum,
            "failed metadata published a root or consumed a new slot");
    }
    mode=3;const auto first=scene.create(0,false,13);
    require(first==0xe26f0000 && calls==4,"failed metadata consumed an initial generation");
    scene.destroy(first);mode=0;rejects([&]{scene.create(0);},"reused slot accepted unresolved metadata");
    require(scene.objects().size()==1 && scene.objects()[0].handle==null_datum &&
        !scene.objects()[0].render_attribution_at_birth && scene.resolve(0)==null_datum,
        "failed reuse published or retained generation metadata");
    mode=3;const auto second=scene.create(0);
    require(second==0xe2700000,"failed metadata consumed a reused generation");
    mode=0;rejects([&]{scene.create(0,true);},"failed replacement accepted unresolved metadata");
    // Preserve existing replace semantics: the old generation is destroyed
    // before attempting its replacement, even when new metadata cannot resolve.
    require(scene.resolve(0)==null_datum && !scene.objects()[0].render_attribution_at_birth,
        "failed replacement restored a destroyed old generation");
    rejects([&]{scene.get(second);},"failed replacement retained the old generation");
    mode=3;require(scene.create(0)==0xe2710000,"failed replacement consumed a generation");
    auto invalid=content;invalid.objects[0].flags=0;invalid.objects[0].name=1;
    unsigned invalid_calls{};
    rejects([&]{SceneObjects bad(invalid,0,{},[&](std::uint32_t){++invalid_calls;return std::optional{definition};});},
        "invalid placement name was published");
    require(!invalid_calls,"invalid name invoked the metadata resolver before validation");
    invalid=content;invalid.objects[0].flags=0;auto duplicate=invalid.objects[0];duplicate.placement=1;
    invalid.objects.push_back(duplicate);invalid_calls=0;
    rejects([&]{SceneObjects bad(invalid,0,{},[&](std::uint32_t){++invalid_calls;return std::optional{definition};});},
        "duplicate live name was published");
    require(invalid_calls==1,"duplicate name invoked an extra metadata birth event");
}
void capacity(const ObjectLightContactDefinition &definition) {
    auto content=manual(definition);content.objects.clear();content.object_names.clear();
    for(unsigned i=0;i<2049;++i) {
        ScenarioObject object;object.type=6;object.placement=int(i);object.name=int(i);
        object.definition=definition.datum;object.flags=1;content.objects.push_back(object);
        content.object_names.push_back({"capacity control",6,int(i)});
    }
    unsigned calls{};SceneObjects scene(content,0,{},[&](std::uint32_t){++calls;return std::optional{definition};});
    for(unsigned i=0;i<2048;++i)require(scene.create(int(i))!=null_datum,"native capacity control filled early");
    const auto first=scene.resolve(0);
    rejects([&]{scene.create(2048);},"scene exceeded its existing native capacity");
    require(calls==2048 && scene.objects().size()==2048 && scene.resolve(2048)==null_datum &&
        scene.resolve(0)==first,"capacity failure invoked metadata or changed published roots");
    scene.destroy(first);const auto next_salt=scene.object_lifetimes().pool().next_salt();
    const auto reuse=scene.create(2048);
    require(slot(reuse)==slot(first) && generation(reuse)==next_salt && calls==2049 &&
        scene.get(reuse).render_attribution_at_birth.has_value(),"capacity reuse lost the new birth event");
}
}
int main(int argc,char **argv) {try {
    require(argc==3,"scene attribution test needs mainmenu.map and original flag31 TSV");
    for(unsigned i=1;i<3;++i)if(!std::filesystem::exists(argv[i]))return 77;
    const Cache cache(argv[1]);const auto expected=authored(argv[2],cache);
    actual_lifetimes(cache,expected);const auto &definition=expected.begin()->second.definition;
    retained_projection_controls(definition);failures(definition);capacity(definition);
    std::cout<<"15 original authored definitions; actual startup/create/destroy/reuse/restore, retained BSP, optional resolver, "
        <<"metadata/name/capacity failures and explicit synthetic positive controls; birth projection only, CPU only\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
