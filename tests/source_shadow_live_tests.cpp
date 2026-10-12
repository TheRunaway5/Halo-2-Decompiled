#include "halo2/source_shadow_submission.hpp"
#include "halo2/source_model_fade.hpp"
#include "halo2/source_local_player_priority.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
namespace {
using namespace halo2;using Words=std::vector<std::uint32_t>;
void require(bool b,const char*m){if(!b)throw FormatError(m);}
float f(unsigned v){return std::bit_cast<float>(v);}
unsigned b(float v){return std::bit_cast<unsigned>(v);}
struct ModelFadeCall {
    SourceModelFadeInput input;
    std::vector<SourceModelFadeOwner> owners;
    std::vector<SourceModelFadeDefinition> definitions;
    std::vector<SourceModelFadeModel> models;
};
ModelFadeCall read_model_fade_call(std::span<const std::uint32_t> words){
    std::size_t at=0;auto n=[&]{require(at<words.size(),"truncated live model fade call");return words[at++];};
    require(n()==1,"live model fade input version");const auto no=n(),nd=n(),nm=n();
    require(no<=64&&nd<=64&&nm<=64,"live model fade bank count");ModelFadeCall call;
    call.input.owner=n();const auto modifier=n();require(modifier<=255,"live model fade modifier");
    call.input.view_modifier=std::uint8_t(modifier);call.input.draw_metric=f(n());
    for(unsigned i=0;i<no;++i){SourceModelFadeOwner o;o.identity=n();o.flags=n();o.parent=n();o.definition=n();o.radius=f(n());call.owners.push_back(o);}
    for(unsigned i=0;i<nd;++i){SourceModelFadeDefinition d;d.identity=n();d.model=n();call.definitions.push_back(d);}
    for(unsigned i=0;i<nm;++i){SourceModelFadeModel m;m.identity=n();m.fade_end=f(n());m.fade_start=f(n());
        for(auto&v:m.render_cutoffs)v=f(n());const auto count=n();require(count<=65535,"live model fade LOD count");m.render_lod_count=std::uint16_t(count);call.models.push_back(m);}
    require(at==words.size(),"trailing live model fade call words");return call;
}
struct PriorityCall {
    std::uint32_t owner{};
    std::int32_t slot{};
    std::uint8_t special{};
    std::array<SourcePriorityObserver,4> observers;
    std::array<std::optional<std::uint32_t>,4> players;
    std::vector<SourcePriorityOwner> owners;
};
PriorityCall read_priority_call(std::span<const std::uint32_t> words){
    std::size_t at=0;auto n=[&]{require(at<words.size(),"truncated live priority call");return words[at++];};
    require(n()==1,"live priority input version");const auto count=n();require(count<=64,"live priority owner count");PriorityCall call;
    call.owner=n();call.slot=std::bit_cast<std::int32_t>(n());const auto special=n();require(special<=255,"live priority special byte");call.special=std::uint8_t(special);
    for(auto &observer:call.observers){observer.callback=n();observer.scalar_bits=n();}
    for(auto &player:call.players)player=n();
    for(unsigned i=0;i<count;++i){SourcePriorityOwner owner;owner.identity=n();owner.parent=n();const auto kind=n();require(kind<=255,"live priority header kind");owner.header_kind=std::uint8_t(kind);owner.player=n();call.owners.push_back(owner);}
    require(at==words.size(),"trailing live priority call words");return call;
}
Words evaluate(const Words&w,bool wrong_fade_metric=false,bool wrong_priority_player=false){
    unsigned at=0;auto n=[&]{return w.at(at++);};auto fl=[&]{return f(n());};
    require(n()==3,"live shadow fixture version");SourceShadowCandidateContext context;
    context.mode=n();const unsigned owner_count=n(),collector_count=n();
    context.clock={std::bit_cast<int>(n()),fl()};for(auto&v:context.depth_plane)v=fl();context.pixel_scale=fl();
    std::vector<SourceShadowOwnerState> owners;std::map<unsigned,unsigned> observed_priorities,computed_priorities,observed_fades;
    for(unsigned i=0;i<owner_count;++i){SourceShadowOwnerState o;
        o.identity=n();o.parent=n();o.sibling=n();o.child=n();o.object_flags=n();o.definition_mode=std::uint16_t(n());
        o.object_type=std::uint8_t(n());o.header_flags=std::uint8_t(n());o.header_kind=std::uint8_t(n());o.lod_flags_10a=std::uint8_t(n());o.death_tick=std::bit_cast<int>(n());
        o.sphere={{fl(),fl(),fl()},fl()};const auto has_camo=n();const auto camo=fl();if(has_camo)o.camouflage=camo;
        const auto has_fade=n(),fade=n();require(has_fade<=1&&fade<=255,"live observed model fade byte");
        if(has_fade)observed_fades.emplace(o.identity,fade);
        const auto priority=n();if(priority!=0xffffffffu){require(priority<=1,"live observed priority byte");observed_priorities.emplace(o.identity,priority);}owners.push_back(o);
    }
    std::vector<SourceShadowCollectorEntry> collector;for(unsigned i=0;i<collector_count;++i)collector.push_back({n(),std::uint16_t(n())});
    std::array<std::uint8_t,32> scratch{};for(auto&v:scratch)v=std::uint8_t(n());context.retained_fourth_flag_bytes=scratch;
    SourceShadowSelectionCache cache;cache.count=n();require(cache.count<=32,"live source selection count");
    for(unsigned i=0;i<cache.count;++i){auto&e=cache.entries[i];e.candidate={n(),n(),std::bit_cast<int>(n()),fl(),fl()};e.slope=fl();e.fade=fl();e.last_seen_frame=std::bit_cast<int>(n());e.created_frame=std::bit_cast<int>(n());}
    const auto frame=std::bit_cast<int>(n());const bool bypass=n()!=0;const auto root=n();const Vec3 direction{fl(),fl(),fl()};const auto opacity=fl(),requested_fade=fl();const bool depth_feature=n()!=0;
    const auto fade_count=n();require(fade_count==observed_fades.size()&&fade_count!=0,"live model fade call coverage");
    Words fade_outputs{fade_count};std::vector<unsigned> reconstructed;
    for(unsigned i=0;i<fade_count;++i){const auto count=n();require(count<=w.size()-at,"truncated live fade input bank");
        auto call=read_model_fade_call(std::span<const std::uint32_t>(w).subspan(at,count));at+=count;
        const auto first=n(),second=n();require(first<=255&&second<=255,"live observed model fade outputs");
        const auto owner=std::find_if(owners.begin(),owners.end(),[&](const auto&o){return o.identity==call.input.owner;});
        require(owner!=owners.end(),"live model fade owner absent");
        require(std::find(reconstructed.begin(),reconstructed.end(),call.input.owner)==reconstructed.end(),"duplicate live model fade call");
        const auto current=std::find_if(call.owners.begin(),call.owners.end(),[&](const auto&o){return o.identity==call.input.owner;});
        require(current!=call.owners.end()&&current->flags==owner->object_flags&&current->parent==owner->parent&&current->radius&&b(*current->radius)==b(owner->sphere.radius),"live model fade owner fields disagree");
        const auto observed=observed_fades.find(call.input.owner);require(observed!=observed_fades.end()&&observed->second==second,"live fade observations disagree");
        // Controlled negative input: replacing the observed Warthog metric
        // with0 must fail its independently captured255/0 output assertion.
        if(wrong_fade_metric&&i==0)call.input.draw_metric=0.0f;
        const auto value=source_model_fade(call.owners,call.definitions,call.models,call.input);
        require(value==SourceModelFadeBytes{std::uint8_t(first),std::uint8_t(second)},"live observed model fade differs");
        owner->model_fade=value.second;reconstructed.push_back(call.input.owner);
        fade_outputs.insert(fade_outputs.end(),{call.input.owner,unsigned(value.first),unsigned(value.second)});
    }
    require(std::find(reconstructed.begin(),reconstructed.end(),root)!=reconstructed.end(),"live root fade producer absent");
    const auto priority_count=n();require(priority_count==observed_priorities.size()&&priority_count!=0,"live priority call coverage");
    Words priority_outputs{priority_count};
    for(unsigned i=0;i<priority_count;++i){const auto count=n();require(count<=w.size()-at,"truncated live priority input bank");
        auto call=read_priority_call(std::span<const std::uint32_t>(w).subspan(at,count));at+=count;const auto observed=n();require(observed<=1,"live original priority AL");
        const auto prior=observed_priorities.find(call.owner);require(prior!=observed_priorities.end()&&prior->second==observed,"live priority observations disagree");
        for(const auto &record:call.owners){const auto existing=std::find_if(owners.begin(),owners.end(),[&](const auto &value){return value.identity==record.identity;});
            require(existing!=owners.end()&&record.parent==existing->parent&&record.header_kind==existing->header_kind,"live priority ownership fields disagree");}
        if(wrong_priority_player&&i==0){require(call.slot>=0&&call.slot<4&&call.players[unsigned(call.slot)],"live priority negative control slot");call.players[unsigned(call.slot)]=*call.players[unsigned(call.slot)]^1u;}
        const auto value=source_local_player_priority(call.owners,call.owner,{call.slot,call.observers,call.players,call.special});
        require(value==observed,"live observed local-player priority differs");
        require(computed_priorities.emplace(call.owner,value).second,"duplicate live priority call");priority_outputs.insert(priority_outputs.end(),{call.owner,unsigned(value)});
    }
    context.priority=[&](unsigned id){const auto p=computed_priorities.find(id);require(p!=computed_priorities.end(),"unreconstructed live shadow priority demanded");return std::uint8_t(p->second);};
    require(at==w.size(),"trailing live shadow fixture words");
    Words out=std::move(fade_outputs);out.insert(out.end(),priority_outputs.begin(),priority_outputs.end());auto put_candidate=[&](const auto&r){out.insert(out.end(),{r.flags,r.identity,std::bit_cast<unsigned>(r.owner),b(r.requested_fade),b(r.coverage)});};
    const auto candidates=source_shadow_candidates(owners,collector,context);out.push_back(unsigned(candidates.size()));for(const auto&r:candidates)put_candidate(r);
    const auto selected=source_shadow_select(cache,candidates,frame,4,f(0x3ba3d70a),bypass);out.push_back(unsigned(selected.size()));for(const auto&r:selected)put_candidate(r);
    out.push_back(cache.count);for(unsigned i=0;i<cache.count;++i){const auto&e=cache.entries[i];put_candidate(e.candidate);out.insert(out.end(),{b(e.slope),b(e.fade),std::bit_cast<unsigned>(e.last_seen_frame),std::bit_cast<unsigned>(e.created_frame)});}
    const auto fade=source_shadow_owner_fade(owners,root,context.clock);out.insert(out.end(),{unsigned(fade.eligible),b(fade.fade),unsigned(fade.local_shadow),unsigned(source_shadow_owner_blocked(owners,root))});
    const auto hierarchy=source_shadow_owner_hierarchy(owners,root);out.push_back(unsigned(hierarchy.size()));for(const auto&s:hierarchy)out.insert(out.end(),{b(s.center.x),b(s.center.y),b(s.center.z),b(s.radius)});
    const auto owner=std::find_if(owners.begin(),owners.end(),[&](const auto&o){return o.identity==root;});require(owner!=owners.end(),"live root absent");
    const auto shadow=source_shadow_frame(owner->sphere,hierarchy,direction,opacity,requested_fade*fade.fade);out.push_back(unsigned(bool(shadow)));if(shadow){
        for(auto v:shadow->source_values())out.push_back(b(v));
        const auto uniforms=source_shadow_uniforms(*shadow,depth_feature);
        for(const auto&v:uniforms.projection)for(auto scalar:v)out.push_back(b(scalar));
        for(const auto&v:uniforms.constants)for(auto scalar:v)out.push_back(b(scalar));
    }
    return out;
}
}
int main(int argc,char**argv){try{
    const bool probe=argc==2&&std::string_view(argv[1])=="--probe";require(argc==2,"live shadow fixture required");std::ifstream file;if(!probe){file.open(argv[1]);require(bool(file),"live shadow fixture missing");}
    auto&stream=probe?std::cin:file;std::string line;unsigned cases=0,words=0;
    while(std::getline(stream,line)){if(line.empty()||line[0]=='#')continue;const auto sep=line.find('|');std::istringstream input(line.substr(0,sep));Words row;unsigned v;while(input>>std::hex>>v)row.push_back(v);const auto actual=evaluate(row);
        if(probe){for(auto word:actual)std::cout<<std::hex<<word<<' ';std::cout<<'\n';}
        else {require(sep!=std::string::npos,"live shadow expected words missing");Words expected;std::istringstream golden(line.substr(sep+1));while(golden>>std::hex>>v)expected.push_back(v);if(actual!=expected){unsigned first=0;while(first<actual.size()&&first<expected.size()&&actual[first]==expected[first])++first;throw FormatError("live original shadow differs at word "+std::to_string(first));}
            bool rejected=false;try{(void)evaluate(row,true);}catch(const FormatError&e){rejected=std::string_view(e.what())=="live observed model fade differs";}require(rejected,"live reconstructed fade metric negative control did not respond");
            rejected=false;try{(void)evaluate(row,false,true);}catch(const FormatError&e){rejected=std::string_view(e.what())=="live observed local-player priority differs";}require(rejected,"live reconstructed priority local-player negative control did not respond");}
        ++cases;words+=unsigned(actual.size());
    }require(cases!=0,"no live shadow cases");if(!probe)std::cout<<cases<<" actual OpenGL source phase, "<<words<<" reconstructed fade/priority/candidate/history/root/hierarchy/frame/uniform words exact; observed-metric and local-player negative controls respond\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
