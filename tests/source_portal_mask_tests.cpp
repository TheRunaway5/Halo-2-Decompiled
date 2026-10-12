#include "halo2/source_portal_mask.hpp"
#include "halo2/content.hpp"
#include "halo2/source_device.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
using namespace halo2;
using Words=std::vector<std::uint32_t>;
void require(bool yes,const char *text){if(!yes)throw FormatError(text);}
Words evaluate(const Words &row) {
    std::size_t at=0;const auto next=[&]{return row.at(at++);};
    const auto real=[&]{return std::bit_cast<float>(next());};SourcePortalMaskDefinition d;
    const unsigned portals=next();for(unsigned i=0;i<portals;++i)d.portal_flags.push_back(next());
    const unsigned records=next();for(unsigned i=0;i<records;++i){SourcePortalAssociation r;
        r.owner={next(),std::int16_t(next()),std::int8_t(next()),std::int8_t(next())};
        r.first=std::int16_t(next());r.count=std::int16_t(next());d.associations.push_back(r);}
    const unsigned indices=next();for(unsigned i=0;i<indices;++i)d.portal_indices.push_back(std::int16_t(next()));
    std::vector<SourcePortalOwnerState> owners(next());
    for(auto &o:owners){o.handle=next();o.header_type=std::uint8_t(next());
        o.identity={next(),std::int16_t(next()),std::int8_t(next()),std::int8_t(next())};
        o.first=std::int16_t(next());o.count=std::int16_t(next());o.definition.type=std::uint16_t(next());
        o.definition.closed_acoustic=real();o.definition.open_acoustic=real();o.current_position=real();
        o.position_group=next();o.group_position=real();o.device_flags=next();}
    SourcePortalMaskState state;for(auto &w:state.words)w=next();for(auto &w:state.acoustic)w=std::uint16_t(next());
    const unsigned steps=next();for(unsigned i=0;i<steps;++i){const auto op=next(),owner=next();
        switch(op){case 0:state.initialize(d,owners);break;case 1:state.bind(d,owners);break;
        case 2:state.clear_flag8(d);break;case 3:state.step(d,owners.at(owner));break;
        case 4:state.destroy(d,owners.at(owner));break;case 5:state.reset_owners(owners);break;
        case 6:state.reset_acoustic();break;default:throw FormatError("unknown portal mask operation");}}
    require(at==row.size(),"portal mask input trailing fields");Words out(state.words.begin(),state.words.end());
    for(auto w:state.acoustic)out.push_back(w);
    for(const auto &o:owners){out.push_back(std::uint16_t(o.first));out.push_back(std::uint16_t(o.count));}
    return out;
}
void boundaries() {
    SourcePortalMaskDefinition d{{8,4},{{{42,0,7,0},0,1},{{42,0,7,0},1,1},{{43,99,7,1},0,1}},{0,1}};
    std::vector<SourcePortalOwnerState> owners(3);
    owners[0].handle=0x20007;owners[0].identity={42,0,7,0};
    owners[1].handle=0x10002;owners[1].identity={42,0,7,0};
    owners[2].handle=0x30003;owners[2].identity={43,-2,7,1};
    SourcePortalMaskState state;state.acoustic.fill(77);state.initialize(d,owners);
    require(owners[0].first==-1 && owners[0].count==0 && owners[1].first==1 && owners[1].count==1,
        "portal binding must choose first pool slot and retain last matching authored range");
    require(owners[2].first==0 && owners[2].count==1,"source1 portal identity incorrectly required origin BSP");
    state.step(d,owners[1],{true,0,0,0});require(!(state.words[0]&2),"typed native group-presence step failed");
    state.step(d,owners[2],{true,1,1,0});require(state.words[0]&1,"later owner must overwrite flag8 mask state");
    state.destroy(d,owners[2]);require((state.words[0]&1) && state.acoustic[0]==0,"destroy must restore open without flag8 reapplication");
    auto full=state,typed=state;owners[1].position_group=0;owners[1].current_position=0;
    owners[1].group_position=1;owners[1].device_flags=0x100;owners[1].definition.open_acoustic=1;
    full.step(d,owners[1]);typed.step(d,owners[1],{true,0,1,0x100});
    require(full.words==typed.words && full.acoustic==typed.acoustic,"native presence/value overload differs from original datum input");
    state.destroy(d,owners[1]);require(state.words[0]&2,"destroyed shared owner did not restore open");
    owners.erase(owners.begin()+1);state.bind(d,owners);
    require(owners[0].first==1 && owners[0].count==1,"explicit rebind did not choose remaining matching lifetime");
    const auto retained=state.words;state.reset_owners(owners);require(state.words==retained,"range reset invented mask writes");
    auto invalid=d;invalid.portal_indices[0]=512;bool rejected=false;
    try{state.initialize(invalid,owners);}catch(const FormatError &){rejected=true;}
    require(rejected,"invalid source portal index silently admitted");
    rejected=false;owners[1].handle=owners[0].handle;
    try{state.bind(d,owners);}catch(const FormatError &){rejected=true;}
    require(rejected,"two live generations shared original pool slot");
}
void assets() {
    const std::vector<std::string> disc{"ascension","beavercreek","burial_mounds","coagulation","colossus","cyclotron","foundation","headlong","lockout","midship","waterworks","zanzibar"};
    const std::vector<std::string> pack{"backwash","containment","deltatap","dune","elongation","gemini","triplicate","turf","warlock"};
    unsigned maps=0,associations=0,indices=0;
    for(const auto &group:{std::pair{std::string("assets/disc/maps/"),disc},std::pair{std::string("assets/map-pack/maps/"),pack}})
        for(const auto &name:group.second){Cache cache(group.first+name+".map");const auto d=cache.load_source_portal_mask_definition(0);
            ++maps;associations+=d.associations.size();indices+=d.portal_indices.size();
            for(auto flag:d.portal_flags)require(!(flag&8),"unexpected authored flag8 portal");
            std::vector<SourcePortalOwnerState> owners;const auto content=cache.load_content(0,true);
            for(const auto &r:d.associations){const auto object=std::find_if(content.objects.begin(),content.objects.end(),[&](const auto &o){
                    return o.unique_id==r.owner.unique_id && o.type==r.owner.type && o.source==r.owner.source &&
                        (o.source!=0 || o.origin_bsp==r.owner.origin_bsp);});
                require(object!=content.objects.end(),"portal association full identity lost authored machine");
                const auto placement=cache.load_source_machine_placement(object->placement);
                SourcePortalOwnerState owner;owner.handle=0x10000u+unsigned(owners.size());owner.identity=r.owner;
                owner.definition=cache.load_source_machine_portal_definition(object->definition);owner.current_position=placement.initial_position;
                // Explicit constructor group state: both named/private groups are allocated
                // by source1076E0. Source handles remain caller inputs in the live module.
                owner.position_group=unsigned(owners.size());owner.group_position=placement.initial_position;
                owner.device_flags=0x80u|((placement.device_flags&8)>>3)|((placement.device_flags&16)>>3);owners.push_back(owner);}
            SourcePortalMaskState cold;cold.acoustic.fill(0x1357);cold.initialize(d,owners);
            for(auto w:cold.words)require(w==0xffffffffu,"source cold portal mask must remain open before door steps");
            for(auto w:cold.acoustic)require(w==0x1357,"mask constructor invented acoustic initialization");
            if(name=="gemini"){
                require(d.associations.size()==6 && d.portal_indices==std::vector<std::int16_t>{33,32,25,36,30,34,35,26},"Gemini literal portal association bank changed");
                const std::array<unsigned,6> identities{0x30f0,0x318c,0x318e,0x3190,0x4b51,0x4b52};
                for(unsigned i=0;i<owners.size();++i){require(owners[i].identity.unique_id==identities[i],"Gemini full portal owner identity changed");
                    require(owners[i].definition.type==0,"Gemini portal owner must be a source door");
                    std::cout<<"Gemini owner "<<std::hex<<identities[i]<<std::dec<<" current="<<owners[i].current_position<<" flags="<<owners[i].device_flags<<'\n';}
            }else require(d.associations.empty() && d.portal_indices.empty(),"unexpected supplied-map portal owner association");
        }
    require(maps==21 && associations==6 && indices==8,"all21 portal association inventory changed");
    std::cout<<"21 literal cache banks: 6 Gemini owners, 8 portal indices, cold mask allones\n";
}
}
int main(int argc,char **argv){try{
    if(argc!=2)throw std::runtime_error("portal mask fixture required");
    const bool probe=std::string(argv[1])=="--probe",content=std::string(argv[1])=="--assets";
    if(content){assets();return 0;}
    std::ifstream file;if(!probe){file.open(argv[1]);if(!file)throw std::runtime_error("portal mask fixture unavailable");}
    auto &stream=probe?std::cin:file;std::string line;unsigned cases=0;
    while(std::getline(stream,line)){if(line.empty() || line[0]=='#')continue;
        std::istringstream input(line);input>>std::hex;unsigned count;if(!(input>>count))throw std::runtime_error("portal mask count unavailable");
        Words row(count);for(auto &w:row)if(!(input>>w))throw std::runtime_error("portal mask input unavailable");const auto result=evaluate(row);
        if(probe){for(auto w:result)std::cout<<std::hex<<w<<' ';std::cout<<'\n';}
        else {for(auto w:result){unsigned expected;if(!(input>>expected) || expected!=w)throw std::runtime_error("portal mask differs case "+std::to_string(cases));}
            std::string extra;if(input>>extra)throw std::runtime_error("portal mask fixture trailing fields");}++cases;}
    require(cases!=0,"empty portal mask fixtures");if(!probe){boundaries();std::cout<<cases<<" original portal mask/state cases matched\n";}
    return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
