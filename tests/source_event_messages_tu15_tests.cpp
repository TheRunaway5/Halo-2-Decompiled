#include "halo2/source_event_messages_tu15.hpp"
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>

using namespace halo2;
using J=nlohmann::json;
namespace {
void check(bool value,const std::string& message) {
    if(!value)throw std::runtime_error(message);
}
J read(const char* path) {
    std::ifstream file(path);check(bool(file),std::string("Open original fixture ")+path);
    auto out=J::parse(file);
    if(out.contains("source_xbe_sha256"))
        check(out.at("source_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd",
            "Original event-message fixture executable identity");
    else check(out.at("asset_sha256")=="14c54046f259842bdac765526e7fe2d991369502041647f47db409ce845987a9",
        "Original event-message catalog cache identity");
    return out;
}
void word(VistaAnnouncerResponse& row,unsigned at,std::uint32_t value,unsigned size=4) {
    for(unsigned n=0;n<size;++n)row[at+n]=std::byte(value>>(8*n));
}
VistaAnnouncerResponse response(unsigned type,int subtype,unsigned audience,std::uint32_t key=1) {
    VistaAnnouncerResponse out{};word(out,0,type<<16);word(out,4,std::uint32_t(subtype),2);
    word(out,6,audience,2);word(out,0xc,key);return out;
}
SourceMultiplayerEventTu15 event(unsigned type,int subtype) {
    SourceMultiplayerEventTu15 out;out.words={type,std::uint32_t(subtype),0xffffffffu,
        0x12340000,0,0x12340001,1,3};return out;
}
void formatter() {
    const auto resolver=[](std::u16string_view token)->std::optional<std::u16string> {
        if(token==u"cause_player")return u"Zo\u00eb\ue123";
        if(token==u"effect_player")return u"#cause_player";
        if(token==u"empty")return u"";
        return {};
    };
    check(source_event_message_format_tu15(u"#cause_player killed #effect_player.",resolver)==
        u"Zo\u00eb\ue123 killed #cause_player.","UTF16/glyph substitution is not recursively expanded");
    check(source_event_message_format_tu15(u"You killed #effect_player",
        [](std::u16string_view)->std::optional<std::u16string>{return u"\u96ea\U0001f98a#tag";})==
        u"You killed \u96ea\U0001f98a#tag","Player name retains Unicode and literal hash text");
    check(source_event_message_format_tu15(u"#unknown #UPPER #cause_player2 #",resolver)==
        u"#unknown #UPPER Zo\u00eb\ue1232 #","Unknown tokens and lowercase/underscore token grammar");
    check(source_event_message_format_tu15(u"A#empty!B",resolver)==u"AB",
        "Original zero-length resolver skips following input unit");
    check(source_event_message_format_tu15(u"#cause_player",resolver,3)==u"Zo\u00eb" &&
        source_event_message_format_tu15(u"abcdef",resolver,2)==u"ab" &&
        source_event_message_format_tu15(u"#cause_player",resolver,0).empty(),"Bounded UTF16 output");
    const std::u16string embedded{u'a',0,u'b'};
    check(source_event_message_format_tu15(embedded,resolver)==u"a","Original NUL termination");
    const std::string unicode="Zo\xc3\xab\xee\x84\xa3\xf0\x9f\x8e\xae";
    check(source_event_message_utf8_tu15(source_event_message_utf16_tu15(unicode))==unicode,
        "Authored UTF8/UTF16 scalar round trip");
    for(const auto& bad:{std::string("\xc0\xaf"),std::string("\xed\xa0\x80"),std::string("\xf4\x90\x80\x80"),std::string("\xe2\x82")}) {
        bool rejected=false;try{(void)source_event_message_utf16_tu15(bad);}catch(const FormatError&){rejected=true;}
        check(rejected,"Malformed locale UTF8 admitted");
    }
}
void original_policy(const J& fixture) {
    unsigned cases{},guards{};
    for(const auto& row:fixture.at("cases")) {
        const auto allowed=source_event_message_text_allowed_tu15(row[0],row[1],row[4],row[3],false);
        check(allowed==(row[5]=="text"),"Original1A83B0 census/admission case "+std::to_string(cases++));
    }
    for(const auto& row:fixture.at("guards")) {
        auto input=event(row[3],row[4]);input.mode=row[0];input.game_over=row[2].get<unsigned>()!=0;
        SourceEventMessageAssetsTu15 assets;assets.strings[1]=u"declared fixture";
        assets.responses.banks[input.words[0]].definitions.push_back(response(input.words[0],row[4],4));
        const VistaAnnouncerLocalPlayer player{0x12340000,1,0,
            std::bit_cast<std::int16_t>(row[1].get<std::uint16_t>())};
        const auto outputs=source_event_messages_tu15(assets,input,player,1,{});
        check(!outputs.empty()==(row[5]=="text"),"Original1A83B0 locality/postgame case "+std::to_string(guards++));
    }
    check(cases==4960 && guards==64,"Complete original guard/admission corpus");
}
void catalog(const J& fixture) {
    unsigned count{},literal{},primary{};
    for(const auto& bank:fixture.at("banks"))for(const auto& row:bank.at("rows")) {
        ++count;const auto type=bank.at("type").get<unsigned>();
        const auto audience=row.at("audience").get<unsigned>();
        auto raw=response(type,row.at("subtype"),audience,
            std::stoul(row.at("primary_key_raw").get<std::string>(),nullptr,0));
        word(raw,0,row.at("flags"));word(raw,0x10,row.at("required"),2);word(raw,0x12,row.at("excluded"),2);
        const auto secondary=std::uint32_t(std::stoul(row.at("secondary_key_raw").get<std::string>(),nullptr,0));
        word(raw,0x14,secondary);word(raw,0x18,row.at("primary_duration_seconds"));
        word(raw,0x1c,std::stoul(row.at("special_key_raw").get<std::string>(),nullptr,0));
        SourceEventMessageAssetsTu15 assets;assets.responses.banks[type].definitions.push_back(raw);
        const Bytes bytes(raw);
        assets.strings[bytes.u32(0xc)]=source_event_message_utf16_tu15(row.at("primary_template").get<std::string>());
        assets.strings[secondary]=source_event_message_utf16_tu15(row.at("secondary_template").get<std::string>());
        auto input=event(type,row.at("subtype"));
        VistaAnnouncerLocalPlayer player{0x12340002,1,2,0};
        if(audience==0)player.datum=input.words[3];
        if(audience==2)player.datum=input.words[5];
        if(audience==1)player.membership=0;
        if(audience==3)player.membership=1;
        const auto outputs=source_event_messages_tu15(assets,input,player,1,
            [](std::u16string_view token)->std::optional<std::u16string>{return std::u16string(token);});
        const auto expected=unsigned(bytes.u32(0xc)!=0 && bytes.u32(0xc)!=0xffffffffu)+unsigned(secondary!=0 && secondary!=0xffffffffu);
        check(outputs.size()==expected,"Authored catalog channel omission/recipient "+std::to_string(count));
        for(const auto& output:outputs) {
            check(output.audience==audience && output.text.find(u'#')==std::u16string::npos,"Catalog role/token expansion");
            if(output.channel==SourceEventMessageChannelTu15::literal)++literal;
            else if(output.channel==SourceEventMessageChannelTu15::primary){++primary;check(output.duration_seconds==0,"Original sudden death zero duration");}
            else check(false,"Original catalog unexpectedly has a quantity row");
        }
    }
    check(count==167 && literal==154 && primary==1,"Complete original authored channel census");
}
void channels_and_targets() {
    SourceEventMessageAssetsTu15 assets;auto raw=response(0,0,0);
    word(raw,0,1);word(raw,0x1c,2);word(raw,0x14,3);word(raw,0x18,7);
    assets.responses.banks[0].definitions.push_back(raw);
    assets.strings={{1,u"singular"},{2,u"plural"},{3,u"primary"}};
    auto input=event(0,0);input.words[7]=0xfffe;
    VistaAnnouncerLocalPlayer player{input.words[3],1,0,0};
    auto outputs=source_event_messages_tu15(assets,input,player,4,{});
    check(outputs.size()==2 && outputs[0].channel==SourceEventMessageChannelTu15::quantity &&
        outputs[0].quantity==-2 && outputs[0].plural_text==u"plural" && outputs[1].duration_seconds==7 &&
        outputs[1].channel==SourceEventMessageChannelTu15::primary,"Distinct quantity/primary channels and signed16 quantity");
    player.flags=3;
    check(source_event_messages_tu15(assets,input,player,1,{}).empty(),"Broadcast iterator flag2");
    input.words[2]=player.datum;
    check(source_event_messages_tu15(assets,input,player,1,{}).size()==2,"Explicit target bypasses iterator flag2");
    input.words[2]=player.datum+1;
    check(source_event_messages_tu15(assets,input,player,1,{}).empty(),"Different explicit target");
}
void actual_map(const char* path) {
    Cache cache(path);const auto assets=load_source_event_message_assets_tu15(cache);
    unsigned count{},literals{},primaries{};std::set<std::string> tokens;
    for(const auto& bank:assets.responses.banks)for(const auto& raw:bank.definitions) {
        ++count;const Bytes row(raw);
        for(const auto at:{0xcu,0x14u,0x1cu}) {
            const auto key=row.u32(at);if(!key || key==0xffffffffu)continue;
            const auto found=assets.strings.find(key);check(found!=assets.strings.end() && !found->second.empty(),"Actual MCC response locale owner");
            if(at==0xc)++literals;else if(at==0x14)++primaries;
            (void)source_event_message_format_tu15(found->second,[&](std::u16string_view token)->std::optional<std::u16string>{
                tokens.insert(source_event_message_utf8_tu15(token));return u"fixture";
            });
        }
    }
    check(count==168 && literals==154 && primaries==1,"Actual MCC complete authored channel census");
    for(const auto& name:assets.team_names)check(!name.empty(),"Actual MCC separate team_names unic");
    if(cache.name()=="coagulation") {
        // Independently read from this actual map's header-owned locale bank;
        // the shared matg bank's same four ordinals are weapon prompts.
        const std::array<std::u16string,8> expected{u"the hill",u"the red base",u"the blue base",u"the rockslide"};
        check(assets.territory_names==expected,"Actual MCC Coagulation owns four exact localized territory names");
    }
    check(tokens==std::set<std::string>{"cause_player","effect_player","cause_team","effect_team","territory_name"},
        "Actual MCC event template token census");
    std::cout<<"Actual MCC168 responses,154 literals,1 primary; tokens:";
    for(const auto& token:tokens)std::cout<<' '<<token;
    std::cout<<"; territory names:";
    for(const auto& name:assets.territory_names)std::cout<<" ["<<source_event_message_utf8_tu15(name)<<']';
    std::cout<<"; ";
}
}
int main(int argc,char** argv) {try {
    check(argc==3 || argc==4,"Supply original admission and response fixtures, optional imported MCC map");
    formatter();original_policy(read(argv[1]));catalog(read(argv[2]));channels_and_targets();
    if(argc==4)actual_map(argv[3]);
    std::cout<<"4960 original admission cases,64 guards,167 catalog rows and UTF16/channel/target controls passed\n";
    return 0;
} catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
