#include "halo2/source_netgame_weapon.hpp"
#include "halo2/source_shadow.hpp"
#include "halo2/model.hpp"
#include "halo2/model_selection.hpp"
#include "halo2/object_lighting.hpp"
#include "halo2/render_program.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>

using namespace halo2;
namespace {
void require(bool value,const char *message){if(!value)throw FormatError(message);}
float value(std::uint32_t word){return std::bit_cast<float>(word);}
std::uint32_t word(float value){return std::bit_cast<std::uint32_t>(value);}
std::int16_t short_value(std::uint32_t word){return std::bit_cast<std::int16_t>(std::uint16_t(word));}
std::uint32_t short_word(std::int16_t value){return std::uint16_t(value);}
Vec3 vector(std::span<const std::uint32_t> values){return {value(values[0]),value(values[1]),value(values[2])};}
std::filesystem::path map_path(const std::string &name){
    auto path=std::filesystem::path("assets/disc/maps")/(name+".map");
    if(!std::filesystem::exists(path))path=std::filesystem::path("assets/map-pack/maps")/(name+".map");
    return path;
}
template<class F>void rejected(F action,const char *message){
    bool failed=false;try{action();}catch(const FormatError&){failed=true;}require(failed,message);
}
std::vector<std::uint32_t> update_words(const SourceLevitatedWeaponUpdate &state){return {
    state.object_flags,state.parent,state.item_flags,state.ignored_object,state.item_owner,
    state.control_c1,short_word(state.remaining_ticks),short_word(state.collision_model),state.object_owner_c8};}
std::vector<std::uint32_t> cold_words(const SourceNetgameWeaponInitialState &state){return {
    state.item.object_flags,state.item.parent,state.item.control_c1,state.control10a,
    short_word(state.item.collision_model),state.shader.item_flags,short_word(state.item.remaining_ticks),
    state.item.ignored_object,state.item.item_owner,state.shader.weapon_flags,word(state.shader.power_fraction),
    state.item.object_owner_c8};}
void boundaries(const NetgameEquipment &row,const SourceWorldWeaponDefinition &definition){
    const auto birth=source_netgame_weapon_birth(row,definition,2);
    auto changed=row;changed.classification=1;
    rejected([&]{source_netgame_weapon_birth(changed,definition,2);},"vehicle descriptor must require its source producer");
    rejected([&]{source_netgame_weapon_birth(row,definition,2,1);},"nonstock weapon option must require its source selector");
    changed=row;changed.items.push_back(changed.items.front());
    rejected([&]{source_netgame_weapon_birth(changed,definition,2);},"weighted collection must require its original RNG");
    changed=row;changed.collection=0xffffffffu;
    rejected([&]{source_netgame_weapon_birth(changed,definition,2);},"NONE collection must not invent a weapon");
    changed=row;changed.items[0].definition^=1;
    rejected([&]{source_netgame_weapon_birth(changed,definition,2);},"definition mismatch must be rejected");
    changed=row;changed.position.x=INFINITY;
    rejected([&]{source_netgame_weapon_birth(changed,definition,2);},"nonfinite row pose must be rejected");
    auto ground=birth;ground.levitated=false;
    rejected([&]{source_netgame_weapon_initial_state(definition,ground);},"ground settling must require its original service");
    auto graph=definition;graph.animation_graph=1;
    rejected([&]{source_netgame_weapon_initial_state(graph,birth);},"animation graph constructor must remain unresolved");
    auto spin=definition;spin.item_flags|=1;
    rejected([&]{source_netgame_weapon_initial_state(spin,birth);},"spin constructor must require its original alignment owner");
    auto widget=definition;widget.widget_groups={0x636f6e74u};
    rejected([&]{source_netgame_weapon_initial_state(widget,birth);},"unproven contrail widget pool must remain unresolved");
    const auto initial=source_netgame_weapon_initial_state(definition,birth);
    auto update=initial.item;update.control_c1=0;
    rejected([&]{source_levitated_weapon_update(definition,update);},"moving item pose must require its source producer");
    update=initial.item;update.parent=3;
    rejected([&]{source_levitated_weapon_update(definition,update);},"attached item must require its source owner");
    update=initial.item;update.remaining_ticks=1;
    rejected([&]{source_levitated_weapon_update(definition,update);},"expiry item must require its source effect service");
    update=initial.item;update.item_flags=0x10;
    rejected([&]{source_levitated_weapon_update(definition,update);},"spinning item must require its source pose producer");
    update=initial.item;update.ignored_object=3;
    rejected([&]{source_levitated_weapon_update(definition,update);},"contact owner must remain explicit");
    update=initial.item;update.item_owner=3;
    rejected([&]{source_levitated_weapon_update(definition,update);},"inventory owner must remain explicit");
    auto functions=definition;functions.owner_function_count=1;update=initial.item;update.object_owner_c8=3;
    rejected([&]{source_levitated_weapon_update(functions,update);},"object C8 function service must remain explicit");
    NetgameWeaponState timer;SourceNetgameWeaponStepInput clock;
    rejected([&]{source_netgame_weapon_step(timer,row,100,clock,{});},"original row capacity must be enforced");
    clock.tick_rate=0;
    rejected([&]{source_netgame_weapon_step(timer,row,0,clock,{});},"invalid source tick rate must be rejected");
    // Allocation is supplied by the caller; an old binding is retired before
    // a failed allocation, and successful creation resets both signed shorts.
    std::vector<int> calls;NetgameWeaponState state{0x12340000u,17,11};
    source_netgame_weapon_allocate(state,0,birth,[&](auto handle){require(handle==0x12340000u,"wrong retired handle");calls.push_back(1);},
        [&](const auto &actual,unsigned ordinal){require(actual.descriptor==birth.descriptor&&ordinal==0,"allocation lost source descriptor");
            require(state.object==0xffffffffu&&calls==std::vector<int>{1},"allocation preceded old binding retirement");calls.push_back(2);return 0xffffffffu;});
    require(calls==std::vector<int>({1,2})&&state.object==0xffffffffu&&state.timer==0&&state.empty_counter==0,"failed source allocation row differs");
    std::array<NetgameWeaponState,2> states{{{0x12340000u,7,9},{0xffffffffu,-1,3}}};
    source_netgame_weapon_reset(states,true,8,0,[&](auto){throw FormatError("playback reset destroyed an item");});
    require(states[0].timer==7&&states[1].empty_counter==3,"playback reset mutated row state");
    source_netgame_weapon_reset(states,false,8,1,[&](auto handle){require(handle==0x12340000u,"reset retired wrong source handle");});
    for(const auto &state:states)require(state.object==0xffffffffu&&state.timer==1&&state.empty_counter==0,"source reset row differs");
}
void camouflage_controls(){
    std::ifstream fixture("tests/fixtures/retail_source_unattached_weapon_camouflage.tsv");
    require(bool(fixture),"missing original unattached weapon camouflage fixture");
    std::string line;unsigned owners=0,controls=0,banks=0,checked=0;std::unique_ptr<Cache> cache;std::string previous;
    while(std::getline(fixture,line)){
        if(line.empty()||line[0]=='#')continue;
        const auto divider=line.find('|');require(divider!=std::string::npos,"camouflage fixture separator missing");
        std::istringstream in(line.substr(0,divider));char kind;std::string map;unsigned ordinal;in>>kind>>map>>ordinal>>std::hex;
        std::vector<std::uint32_t> values,expected;std::uint32_t next;while(in>>next)values.push_back(next);require(in.eof(),"camouflage fixture input malformed");
        std::istringstream out(line.substr(divider+1));out>>std::hex;while(out>>next)expected.push_back(next);require(out.eof(),"camouflage fixture output malformed");
        if(kind=='O'){
            require(values.size()==4&&expected.size()==1&&values[1]==2,"original constructor camouflage owner type differs");
            SourceLevitatedWeaponUpdate state;state.parent=values[2];
            require(word(source_unattached_weapon_camouflage(state))==expected[0],"original cold weapon camouflage differs");++owners;
        }else if(kind=='A'){
            require(values.size()==63&&expected.size()==60,"original carbine alpha bank length differs");
            if(map!=previous){cache=std::make_unique<Cache>(map_path(map));previous=map;}
            auto material=cache->load_material(values[0]);require(material.name.ends_with("\\carbine_display"),"camouflage material identity differs");
            require(material.passes.size()==1&&material.passes[0].pixel_externs.size()==1,"carbine authored extern range differs");
            const auto &pass=material.passes[0];const auto mapping=std::bit_cast<ShaderRemap>(values[1]);
            require(std::bit_cast<std::uint32_t>(pass.pixel_externs[0])==values[1]&&mapping.source==37&&mapping.component==3,"carbine authored camouflage alpha mapping differs");
            PixelProgram program;std::copy(values.begin()+3,values.end(),program.words.begin());
            std::array<float,256> scalars{};scalars[37]=value(values[2]);
            remap_pixel_externs(program,scalars,{},std::span(&mapping,1),pass.fragments,material.shader_switches);
            require(std::equal(program.words.begin(),program.words.end(),expected.begin()),"original19CA0 authored carbine scalar-alpha bank differs");
            auto unresolved=material;require(!prepare_dynamic_material(*cache,unresolved,{},true),"carbine owner camouflage received an unproved default");
            require(prepare_dynamic_material(*cache,material,{},true,value(values[2])),"actual carbine pass rejected explicit original camouflage");
            const unsigned slot=(mapping.destination>>6)&15,bank=(mapping.destination>>10)&1;
            const unsigned index=slot==8?43+bank:10+bank*8+slot;
            require((material.forward_program.words.at(index)&0xff000000u)==(expected[index]&0xff000000u),"actual carbine pass lost original camouflage alpha");
            animate_forward_material(material,0,[](auto){return 0.f;});
            require((material.forward_program.words.at(index)&0xff000000u)==(expected[index]&0xff000000u),"carbine animation reset original owner camouflage");++banks;
        }else{
            require(values.size()==4&&expected.size()==1,"original camouflage parent control length differs");
            SourceLevitatedWeaponUpdate state;state.parent=values[1];
            if(kind=='N')require(values[0]==2&&word(source_unattached_weapon_camouflage(state))==expected[0],"unattached weapon read its unrelated unit-layout field");
            else if(kind=='P'||kind=='Q'||kind=='Z'){
                rejected([&]{source_unattached_weapon_camouflage(state);},"attached weapon must require its actual source parent chain");
                require(expected[0]==(kind=='P'?values[3]:0),"original unit/nonunit/null parent dependency differs");
            }else if(kind=='V')require(values[0]==0&&expected[0]==values[3],"unit camouflage producer was confused with an unattached weapon");
            else throw FormatError("unknown original camouflage fixture branch");
            ++controls;
        }
        checked+=unsigned(expected.size());
    }
    require(fixture.eof()&&owners==31&&controls==13&&banks==6&&checked==404,"original weapon camouflage coverage incomplete");
    std::cout<<"31 original cold weapon camouflage owners,13 parent/type controls,6 actual carbine alpha banks;404 words exact\n";
}
void magazine_controls(){
    std::ifstream fixture("tests/fixtures/retail_source_cold_weapon_magazines.tsv");
    require(bool(fixture),"missing original cold magazine fixture");
    std::string line,previous;std::unique_ptr<Cache> cache;unsigned cold=0,controls=0,checked=0;
    while(std::getline(fixture,line)){
        if(line.empty()||line[0]=='#')continue;
        const auto divider=line.find('|');require(divider!=std::string::npos,"magazine fixture separator missing");
        std::istringstream in(line.substr(0,divider));char kind;std::string map;unsigned ordinal;in>>kind>>map>>ordinal>>std::hex;
        std::vector<std::uint32_t> values,expected;std::uint32_t next;while(in>>next)values.push_back(next);
        std::istringstream out(line.substr(divider+1));out>>std::hex;while(out>>next)expected.push_back(next);
        require(in.eof()&&out.eof()&&values.size()==5&&expected.size()==5,"original magazine fixture malformed");
        std::vector<SourceWeaponMagazineDefinition> definitions;
        std::vector<SourceWeaponMagazineState> states;
        if(kind=='K'){
            if(map!=previous){cache=std::make_unique<Cache>(map_path(map));previous=map;}
            const auto definition=cache->load_source_world_weapon_definition(values[0]);definitions=definition.magazines;
            require(definitions.size()==values[1]&&definitions.size()<=1,"actual cold magazine count differs");
            if(!definitions.empty())require(short_word(definitions[0].initial_rounds)==values[2]&&short_word(definitions[0].maximum_loaded)==values[3]&&short_word(definitions[0].maximum_reserve)==values[4],"actual authored magazine initialization differs");
            const auto content=cache->load_content(0,true);require(ordinal<content.netgame_equipment.size(),"actual magazine source row missing");
            const auto birth=source_netgame_weapon_birth(content.netgame_equipment[ordinal],definition,2);
            states=source_netgame_weapon_initial_state(definition,birth).magazines;++cold;
        }else if(kind=='D'){
            require(values[1]==1,"declared FDAD0 control magazine count differs");
            definitions.push_back({short_value(values[2]),short_value(values[3]),short_value(values[4])});
            states.push_back(source_weapon_magazine_initial_state(definitions[0]));++controls;
        }else throw FormatError("unknown original magazine fixture branch");
        require(states.size()==definitions.size(),"cold magazine state count differs");
        const SourceWeaponMagazineState state=states.empty()?SourceWeaponMagazineState{}:states[0];
        const auto ammunition=source_weapon_primary_ammunition(definitions,states);
        require(short_word(state.loaded)==expected[0]&&short_word(state.reserve)==expected[1],"original FDAD0 loaded/reserve signed shorts differ");
        require(expected[2]==1&&word(ammunition.value)==expected[3]&&ammunition.enabled==bool(expected[4]),"original FF5F0 magazine fraction differs");
        checked+=unsigned(expected.size());
    }
    require(fixture.eof()&&cold==31&&controls==9&&checked==200,"original constructor magazine coverage incomplete");
    const std::array<SourceWeaponMagazineDefinition,1> definition{{{36,18,72}}};
    rejected([&]{source_weapon_primary_ammunition(definition,{});},"magazine state must match its actual authored bank");
    std::cout<<"31 actual cold magazine banks,9 original FDAD0 signed initialization controls;200 words exact\n";
}
void carbine_ammunition_controls(){
    std::ifstream fixture("tests/fixtures/retail_source_cold_carbine_ammunition.tsv");
    require(bool(fixture),"missing original carbine ammunition fixture");
    std::string line,previous;std::unique_ptr<Cache> cache;unsigned cold=0,controls=0,checked=0;bool boundaries=false;
    while(std::getline(fixture,line)){
        if(line.empty()||line[0]=='#')continue;
        const auto divider=line.find('|');require(divider!=std::string::npos,"carbine fixture separator missing");
        std::istringstream in(line.substr(0,divider));char kind;std::string map;unsigned ordinal;in>>kind>>map>>ordinal>>std::hex;
        std::vector<std::uint32_t> values,expected;std::uint32_t next;while(in>>next)values.push_back(next);
        std::istringstream out(line.substr(divider+1));out>>std::hex;while(out>>next)expected.push_back(next);
        require(in.eof()&&out.eof()&&values.size()==16&&expected.size()==10,"original carbine function fixture malformed");
        if(map!=previous){cache=std::make_unique<Cache>(map_path(map));previous=map;}
        const auto definition=cache->load_source_world_weapon_definition(values[0]);
        const auto &functions=definition.functions;require(functions.size()==1,"actual carbine objectfunction count differs");
        const auto &function=functions[0];require(function.flags==values[5]&&function.input==values[6]&&function.output==values[7]&&function.enable==values[8]&&word(function.threshold)==values[9]&&function.scale==values[10],"actual carbine objectfunction metadata differs");
        require(function.curve.size()==20,"actual carbine function curve length differs");
        for(unsigned i=0;i<5;++i){std::uint32_t curve_word;std::memcpy(&curve_word,function.curve.data()+i*4,4);require(curve_word==values[11+i],"actual carbine identity curve differs");}
        auto material=cache->load_material(values[1]);require(material.name.ends_with("\\carbine_display")&&material.animation.tables&&material.animation.overlays.size()==2,"actual carbine overlays differ");
        std::vector<SourceWeaponMagazineDefinition> definitions=definition.magazines;
        std::vector<SourceWeaponMagazineState> states;
        if(kind=='C'){
            const auto content=cache->load_content(0,true);require(ordinal<content.netgame_equipment.size(),"cold carbine row missing");
            const auto birth=source_netgame_weapon_birth(content.netgame_equipment[ordinal],definition,2);
            states=source_netgame_weapon_initial_state(definition,birth).magazines;
            require(definitions.size()==values[2]&&definitions.size()==1&&short_word(definitions[0].maximum_loaded)==values[3]&&short_word(states.at(0).loaded)==values[4],"cold carbine actual magazine state differs");++cold;
        }else if(kind=='M'){
            definitions.clear();if(values[2])definitions.push_back({0,short_value(values[3]),0});
            if(values[2])states.push_back({short_value(values[4]),0});++controls;
        }else throw FormatError("unknown original carbine function fixture branch");
        require(expected[0]==0&&expected[1]==0xdeadbeefu&&expected[2]==0xdeadbeefu,"unknown direct FF5F0 export changed poisoned outputs");
        require(!source_weapon_shader_input(function.output,{}),"held weapon input scope changed for a world authored export");
        const auto builtin=source_weapon_primary_ammunition(definitions,states);
        require(expected[3]==1&&word(builtin.value)==expected[4]&&builtin.enabled==bool(expected[5]),"original primary ammunition built-in result differs");
        const auto resolved=source_weapon_magazine_shader_input(function.output,definitions,states,functions,*material.animation.tables);
        require(resolved&&resolved->enabled==bool(expected[6])&&word(resolved->value)==expected[7],"complete original BAB40/108D90/BA8C0 carbine export differs");
        require(!source_weapon_magazine_shader_input(function.output,definitions,states,{},*material.animation.tables),"authored ammo export resolved without its actual objectfunction bank");
        for(unsigned i=0;i<2;++i){
            const auto &overlay=material.animation.overlays[i];require(overlay.input==function.output,"actual carbine overlay reads a different authored export");
            require(overlay.range==(i?0x0100030au:0u),"actual carbine overlay range source differs");
            // Original BE8B0 -> BAB40 constant0 input gives1; the unrelated
            // nonzero range name is absent from built-ins/authored functions.
            const float range=i?0.f:1.f;
            require(word(shader_function_fraction(overlay.curve,resolved->value,range,*material.animation.tables))==expected[8+i],"original BE8B0/1B230 authored carbine overlay differs");
        }
        if(!boundaries){
            auto invalid=functions;
            for(unsigned field=0;field<7;++field){
                invalid=functions;auto &f=invalid[0];
                if(field==0)f.flags=1;else if(field==1)f.input^=1;else if(field==2)f.enable=1;
                else if(field==3)f.threshold=.5f;else if(field==4)f.scale=1;
                else if(field==5)f.curve.pop_back();else f.curve[8]=std::byte{1};
                rejected([&]{source_weapon_magazine_shader_input(f.output,definitions,states,invalid,*material.animation.tables);},"unproved authored world function branch must remain explicit");
            }
            boundaries=true;
        }
        checked+=unsigned(expected.size());
    }
    require(fixture.eof()&&cold==3&&controls==33&&checked==360&&boundaries,"original carbine function coverage incomplete");
    std::cout<<"3 actual cold carbine functions,33 declared magazine controls,72 original callback/overlay evaluations;360 words exact\n";
}
}
int main(int argc,char **argv){try{
    const bool probe=argc==2&&std::string_view(argv[1])=="--probe";
    std::ifstream fixture; if(!probe){fixture.open("tests/fixtures/retail_source_netgame_weapon.tsv");require(bool(fixture),"missing original netgame weapon fixture");}
    auto &input=probe?std::cin:fixture;
    std::unique_ptr<Cache> cache;LevelContent content;std::string previous,line;
    std::map<char,unsigned> counts;std::set<std::string> maps;unsigned constructor_count=0,checked_words=0;
    bool boundaries_checked=false,on=false,off=false,request_seen=false,bound_seen=false;
    while(std::getline(input,line)){
        if(line.empty()||line[0]=='#')continue;
        const auto divider=line.find('|');require(divider!=std::string::npos,"source fixture separator missing");
        std::istringstream in(line.substr(0,divider));char kind;std::string name;unsigned ordinal;in>>kind>>name>>ordinal;
        require(bool(in),"source fixture identity truncated");in>>std::hex;
        std::vector<std::uint32_t> values,expected,actual;std::uint32_t next;
        while(in>>next)values.push_back(next);require(in.eof(),"source fixture input is malformed");
        std::istringstream out(line.substr(divider+1));out>>std::hex;while(out>>next)expected.push_back(next);
        require(out.eof(),"source fixture output is malformed");
        if(name!=previous){cache=std::make_unique<Cache>(map_path(name));content=cache->load_content(0,true);previous=name;maps.insert(name);}
        require(ordinal<content.netgame_equipment.size(),"source row ordinal missing from cache");
        if(kind=='B'){
            require(values.size()>=10&&values.size()==10+values[9],"source birth fixture length differs");
            const auto &row=content.netgame_equipment[ordinal];const auto definition=cache->load_source_world_weapon_definition(values[0]);
            require(row.flags==values[1]&&row.items.size()==1,"source birth literal row fields differ");
            require(word(row.items[0].weight)==values[2]&&row.items[0].group==values[3]&&row.items[0].definition==values[4]&&row.items[0].variant==values[5],"source singleton collection metadata differs");
            require(word(definition.multiplayer_scale)==values[6]&&word(definition.campaign_scale)==values[7]&&definition.animation_graph==values[8],"source weapon metadata differs");
            require(definition.widget_groups==std::vector<std::uint32_t>(values.begin()+10,values.end()),"source weapon widget metadata differs");
            const auto birth=source_netgame_weapon_birth(row,definition,2);actual.assign(birth.descriptor.begin(),birth.descriptor.end());actual.push_back(word(birth.object_scale));
            require(expected.size()==50||expected.size()==75,"original birth expected bank length differs");
            if(expected.size()==75){
                auto root=source_object_transform(birth.position,birth.forward,birth.up,birth.object_scale,false);
                if(definition.render_model!=0xffffffffu){
                    const auto selection=cache->load_initial_model_selection(definition.definition,birth.variant);
                    require(selection.resolved(),"source weapon default model selection unresolved");
                    const auto model=cache->load_model(definition.definition,false,birth.variant,selection);
                    root=compose_transform(root,pose_model_node(model,0));
                }
                const auto root_words=std::bit_cast<std::array<std::uint32_t,13>>(root);actual.insert(actual.end(),root_words.begin(),root_words.end());
                const auto initial=source_netgame_weapon_initial_state(definition,birth);const auto cold=cold_words(initial);actual.insert(actual.end(),cold.begin(),cold.end());
                const auto turned_on=source_weapon_turned_on(initial.shader);on|=turned_on.enabled;off|=!turned_on.enabled;
                if(!boundaries_checked){boundaries(row,definition);boundaries_checked=true;}++constructor_count;
            }
        }else if(kind=='S'){
            require(values.size()==4&&expected.size()==1,"source scale fixture length differs");
            SourceWorldWeaponDefinition definition;definition.multiplayer_scale=value(values[2]);definition.campaign_scale=value(values[3]);
            actual.push_back(word(source_world_weapon_scale(definition,int(values[0]),values[1])));
        }else if(kind=='U'){
            require(values.size()==10&&expected.size()==10,"source item update fixture length differs");
            const auto definition=cache->load_source_world_weapon_definition(values[0]);
            SourceLevitatedWeaponUpdate state{values[1],values[2],values[3],values[4],values[5],std::uint8_t(values[6]),short_value(values[7]),short_value(values[8]),values[9]};
            actual.push_back(source_levitated_weapon_update(definition,state));const auto result=update_words(state);actual.insert(actual.end(),result.begin(),result.end());
        }else if(kind=='T'){
            require(values.size()==21&&expected.size()==4&&values[0]==ordinal,"source timer fixture length differs");
            auto row=content.netgame_equipment[ordinal];
            require(word(row.position.x)==values[16]&&word(row.position.y)==values[17]&&word(row.position.z)==values[18],"source timer authored sphere anchor differs");
            row.timer_start=short_value(values[10]);row.respawn_seconds=short_value(values[19]);row.collection_delay=short_value(values[20]);
            NetgameWeaponState state{values[7],short_value(values[8]),short_value(values[9])};
            SourceNetgameWeaponStepInput clock{std::bit_cast<std::int32_t>(values[1]),short_value(values[2]),bool(values[3]),bool(values[4]),int(values[5]),int(values[6])};
            std::optional<SourceNetgameWeaponBound> bound;
            if(state.object!=0xffffffffu){bound=SourceNetgameWeaponBound{state.object,vector(std::span(values).subspan(13,3)),values[11],std::uint8_t(values[12])};bound_seen=true;}
            const bool request=source_netgame_weapon_step(state,row,ordinal,clock,bound);request_seen|=request;
            actual={std::uint32_t(request),state.object,short_word(state.timer),short_word(state.empty_counter)};
        }else throw FormatError("unknown source netgame fixture branch");
        if(probe){for(auto word:actual)std::cout<<std::hex<<word<<' ';std::cout<<'\n';}
        else if(actual!=expected){
            const auto mismatch=std::mismatch(actual.begin(),actual.end(),expected.begin(),expected.end());
            const auto index=std::size_t(mismatch.first-actual.begin());std::ostringstream message;
            message<<kind<<' '<<name<<' '<<ordinal<<" original word "<<index<<" differs";
            if(index<actual.size()&&index<expected.size())message<<": native "<<std::hex<<actual[index]<<", original "<<expected[index];
            throw FormatError(message.str());
        }
        ++counts[kind];checked_words+=unsigned(actual.size());
    }
    require(input.eof(),"source netgame fixture read failed");
    if(!probe){camouflage_controls();magazine_controls();carbine_ammunition_controls();require(maps.size()==21&&counts['B']==465&&counts['S']==1386&&counts['U']==155&&counts['T']==1008&&constructor_count==31,"source netgame original coverage incomplete");
        require(boundaries_checked&&on&&!off&&request_seen&&bound_seen,"source constructor/timer control coverage incomplete");
        std::cout<<"465 original singleton births across21 caches,1386 scale controls,31 complete constructor roots/cold states,155 item updates,1008 timer controls; "<<checked_words<<" words exact\n";}
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
