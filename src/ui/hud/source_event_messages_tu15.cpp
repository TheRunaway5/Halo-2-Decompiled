#include "halo2/source_event_messages_tu15.hpp"
#include "halo2/cache.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <bit>

namespace halo2 {
namespace {
constexpr std::uint32_t none=0xffffffffu;
bool display_key(std::uint32_t id) { return id!=0 && id!=none; }
std::u16string_view visible(std::u16string_view text) {
    return text.substr(0,text.find(u'\0'));
}
std::u16string_view localized(const SourceEventMessageAssetsTu15& assets,std::uint32_t id) {
    const auto found=assets.strings.find(id);
    return found==assets.strings.end()?std::u16string_view{}:visible(found->second);
}
}

std::u16string source_event_message_utf16_tu15(std::string_view text) {
    std::u16string out;
    for(std::size_t at=0;at<text.size();) {
        const auto lead=static_cast<unsigned char>(text[at++]);
        std::uint32_t code{};unsigned extra{};
        if(lead<0x80)code=lead;
        else if(lead>=0xc2 && lead<=0xdf){code=lead&0x1f;extra=1;}
        else if(lead>=0xe0 && lead<=0xef){code=lead&0xf;extra=2;}
        else if(lead>=0xf0 && lead<=0xf4){code=lead&7;extra=3;}
        else throw FormatError("Invalid event-message locale UTF8 lead");
        if(extra>text.size()-at)throw FormatError("Truncated event-message locale UTF8");
        for(unsigned n=0;n<extra;++n) {
            const auto byte=static_cast<unsigned char>(text[at++]);
            if((byte&0xc0)!=0x80)throw FormatError("Invalid event-message locale UTF8 continuation");
            code=(code<<6)|(byte&0x3f);
        }
        if((extra==1 && code<0x80)||(extra==2 && code<0x800)||
           (extra==3 && code<0x10000)||code>0x10ffff||(code>=0xd800 && code<=0xdfff))
            throw FormatError("Invalid event-message locale UTF8 scalar");
        if(code<0x10000)out+=char16_t(code);
        else {code-=0x10000;out+=char16_t(0xd800+(code>>10));out+=char16_t(0xdc00+(code&0x3ff));}
    }
    return out;
}

std::string source_event_message_utf8_tu15(std::u16string_view text) {
    std::string out;
    for(std::size_t at=0;at<text.size();++at) {
        std::uint32_t code=text[at];
        if(code>=0xd800 && code<=0xdbff) {
            if(at+1<text.size() && text[at+1]>=0xdc00 && text[at+1]<=0xdfff)
                code=0x10000+((code-0xd800)<<10)+(text[++at]-0xdc00);
            else code=0xfffd;
        } else if(code>=0xdc00 && code<=0xdfff)code=0xfffd;
        if(code<0x80)out+=char(code);
        else if(code<0x800){out+=char(0xc0|(code>>6));out+=char(0x80|(code&0x3f));}
        else if(code<0x10000){out+=char(0xe0|(code>>12));out+=char(0x80|((code>>6)&0x3f));out+=char(0x80|(code&0x3f));}
        else {out+=char(0xf0|(code>>18));out+=char(0x80|((code>>12)&0x3f));out+=char(0x80|((code>>6)&0x3f));out+=char(0x80|(code&0x3f));}
    }
    return out;
}

SourceEventMessageAssetsTu15 load_source_event_message_assets_tu15(const Cache& cache,unsigned language) {
    if(!cache.is_mcc())throw FormatError("Event messages require imported MCC resource owners");
    if(language>=9)throw FormatError("Event-message language has no authored locale slot");
    const Tag* globals=nullptr;
    for(const auto& tag:cache.select_tags({.group=tag_groups::globals,.require_address=true})) {
        if(globals)throw FormatError("Event messages have ambiguous selected globals");
        globals=&tag;
    }
    if(!globals)throw FormatError("Event messages have no selected globals");
    const auto matg=cache.load_source_tag_metadata(globals->datum);
    const auto matg_header=Bytes(matg.bytes_at(matg.source_address(),0x170));
    const auto mulg=cache.load_source_tag_metadata(matg_header.u32(0x16c));
    if(mulg.group()!="mulg")throw FormatError("Event-message runtime globals have wrong group");
    const auto mulg_header=Bytes(mulg.bytes_at(mulg.source_address(),16));
    if(mulg_header.u32(8)!=1)throw FormatError("Event messages have no unique runtime block");
    const auto runtime=Bytes(mulg.bytes_at(mulg_header.u32(12),0xf0));
    struct LocaleBank {
        std::uint32_t count{},size{};
        std::vector<std::byte> references,text;
    };
    const auto load_bank=[&](const Bytes& locale) {
        LocaleBank out;out.count=locale.u32(0);out.size=locale.u32(4);
        const auto indices=locale.u32(8),strings=locale.u32(12),location=indices>>30;
        if(out.count>65536 || out.size>16*1024*1024)throw FormatError("Excessive event-message locale table");
        if(location!=(strings>>30)||(location!=0 && location!=2))
            throw FormatError("MCC event-message locale has an unowned local/shared route");
        if(out.count)out.references=cache.load_source_runtime_resource(indices,out.count*8);
        if(out.size)out.text=cache.load_source_runtime_resource(strings,out.size);
        return out;
    };
    // Shared unic rows index the selected matg bank. A local MCC unic instead
    // indexes the header-owned bank, even when its matg reference is shared.
    // Retained Assembly fd14ef3c Halo2MCC LayoutsU1 defines header+2E4/+2E8
    // and locale-globals+8's seven-DWORD language descriptors.
    const auto globals_bank=load_bank(Bytes(matg.bytes_at(matg.source_address()+0x190+language*0x1c,16)));
    const Bytes raw(cache.source_runtime_cache_bytes());
    const auto original_size=raw.u32(8),index=raw.u32(16);
    const auto require_original=[&](std::size_t at,std::size_t size) {
        if(at>original_size || size>original_size-at)throw FormatError("MCC locale owner escapes original cache");
        raw.require(at,size);
    };
    require_original(index,32);
    const auto tag_count=raw.u32(index+24);
    if(!tag_count || tag_count>65536)throw FormatError("Invalid MCC locale owner tag count");
    const auto header_magic=std::int64_t(raw.u32(index))-index-32;
    const auto tag_table=std::int64_t(raw.u32(index+8))-header_magic;
    if(tag_table<0)throw FormatError("MCC locale owner tag table precedes cache");
    require_original(std::size_t(tag_table),std::size_t(tag_count)*16);
    std::optional<LocaleBank> local_bank;
    SourceEventMessageAssetsTu15 out;
    out.language=language;out.responses=load_source_announcer_responses_tu15(cache);
    const auto read_locale=[&](std::uint32_t datum,const auto& store) {
        const auto unic=cache.load_source_tag_metadata(datum);
        if(unic.group()!="unic")throw FormatError("Event-message text owner has wrong group");
        const auto unic_header=Bytes(unic.bytes_at(unic.source_address(),0x34));
        const auto first=unic_header.u16(0x10+language*4),length=unic_header.u16(0x12+language*4);
        const auto slot=datum&0xffffu;
        if(slot>=tag_count)throw FormatError("Event-message unic has no original directory slot");
        const auto entry=std::size_t(tag_table)+slot*16;
        if(raw.u32(entry+4)!=datum || raw.u32(entry)!=TagGroup::from_string("unic").value)
            throw FormatError("Event-message unic differs from original directory identity");
        const LocaleBank* bank=&globals_bank;
        if(raw.u32(entry+8)) {
            if(!local_bank) {
                require_original(0x2e4,8);
                const auto offset=raw.u32(0x2e4),size=raw.u32(0x2e8);
                const auto descriptor=8+language*0x1c;
                if(!offset || size>4096 || descriptor+16>size)
                    throw FormatError("Local MCC event-message locale globals are incomplete");
                require_original(offset,size);
                local_bank=load_bank(Bytes(raw.slice(std::size_t(offset)+descriptor,16)));
            }
            bank=&*local_bank;
        }
        if(first>bank->count || length>bank->count-first)throw FormatError("Event-message unic range outside locale table");
        const Bytes references(bank->references),table(bank->text);
        for(unsigned n=first;n<unsigned(first)+length;++n) {
            const auto id=references.u32(n*8),offset=references.u32(n*8+4);
            if(offset>=bank->size)throw FormatError("Event-message text outside locale table");
            store(id,source_event_message_utf16_tu15(table.string(offset,bank->size-offset)));
        }
    };
    read_locale(runtime.u32(0x8c),[&](std::uint32_t id,std::u16string text) {
        // First authored match wins, as1A9D90's owned unic search does.
        out.strings.try_emplace(id,std::move(text));
    });
    const auto universals=mulg_header.u32(0);
    if(universals>16)throw FormatError("Excessive event-message universal globals count");
    std::array<bool,9> have_team{};
    if(universals) {
        const auto universal=Bytes(mulg.bytes_at(mulg_header.u32(4),0x20));
        if(const auto team_strings=universal.u32(0xc);team_strings!=none)
            read_locale(team_strings,[&](std::uint32_t id,std::u16string text) {
                //1A86F0 selects universal+0C and1A9910's nine ordinal keys.
                // MCC keeps the tokens while relocating their numeric SIDs.
                const auto token=cache.string_id(id);
                if(token.size()==1 && token[0]>='0' && token[0]<='8') {
                    const auto index=unsigned(token[0]-'0');
                    if(!have_team[index])out.team_names[index]=std::move(text);
                    have_team[index]=true;
                }
            });
    }
    std::array<bool,8> have_territory{};
    if(const auto* scenario=cache.find_tag(cache.scenario_tag(),tag_groups::scenario);
        scenario && scenario->address && scenario->size>=0x3a8) {
        const auto scnr=cache.load_source_tag_metadata(scenario->datum);
        const auto header=Bytes(scnr.bytes_at(scnr.source_address(),0x3a8));
        if(const auto locations=header.u32(0x3a4);locations!=none)
            read_locale(locations,[&](std::uint32_t id,std::u16string text) {
                //2CA170 maps the event's signed16+20 to these eight fixed
                // location keys, then resolves scenario+3A4's owned unic.
                const auto token=cache.string_id(id);
                constexpr std::string_view prefix="ge_territories_territory_";
                if(token.size()==prefix.size()+1 && token.starts_with(prefix) &&
                    token.back()>='0' && token.back()<='7') {
                    const auto index=unsigned(token.back()-'0');
                    if(!have_territory[index])out.territory_names[index]=std::move(text);
                    have_territory[index]=true;
                }
            });
    }
    return out;
}

std::u16string source_event_message_format_tu15(std::u16string_view source,
    const SourceEventMessageTokenResolverTu15& resolver,std::size_t maximum) {
    source=visible(source);
    std::u16string out;
    auto append=[&](std::u16string_view text) {
        text=visible(text);out.append(text.substr(0,std::min(text.size(),maximum-out.size())));
    };
    for(std::size_t at=0;at<source.size() && out.size()<maximum;) {
        const auto token=source.find(u'#',at);
        if(token==std::u16string_view::npos){append(source.substr(at));break;}
        append(source.substr(at,token-at));
        if(out.size()==maximum)break;
        auto end=token+1;
        while(end<source.size() && ((source[end]>=u'a' && source[end]<=u'z')||source[end]==u'_'))++end;
        const auto replacement=resolver?resolver(source.substr(token+1,end-token-1)):std::nullopt;
        if(replacement) {
            const auto text=visible(*replacement);append(text);
            //1A7C99..A2 advances one further UTF16 unit when the resolver
            // writes zero units, including a recognized but absent value.
            at=end+(text.empty() && end<source.size()?1:0);
        } else {append(source.substr(token,end-token));at=end;}
    }
    return out;
}

bool source_event_message_text_allowed_tu15(std::uint32_t type,std::int32_t subtype,
    unsigned audience,unsigned local_count,bool game_over) {
    if(game_over && (type!=0 || subtype!=19))return false;
    if(local_count<2)return true;
    if(type==0) {
        switch(subtype) {
        case 0:case 35:case 36:case 41:case 42:case 43:
            return audience!=1 && audience!=3;
        case 7:case 8:case 9:case 10:case 14:case 15:case 16:case 17:
        case 25:case 26:case 27:case 28:case 29:case 30:case 32:case 33:
        case 44:case 47:case 48:return false;
        default:return true;
        }
    }
    if(type==4 && subtype==2)return false;
    //1A84AA decrements the subtype before indexing1A8664. The zero entries
    // suppress1,2,5,6; a game-start0 falls outside that unsigned table range.
    if(type==6 && (subtype==1 || subtype==2 || subtype==5 || subtype==6))return false;
    if(type==8 && subtype==1)return false;
    return true;
}

std::vector<SourceEventMessageOutputTu15> source_event_messages_tu15(
    const SourceEventMessageAssetsTu15& assets,const SourceMultiplayerEventTu15& event,
    const VistaAnnouncerLocalPlayer& player,unsigned local_count,
    const SourceEventMessageTokenResolverTu15& resolver) {
    if(player.datum==none || player.output_user==-1)return {};
    if(event.words[2]!=none) {if(event.words[2]!=player.datum)return {};}
    else if(player.flags&2u)return {};
    if(event.words[0]>=assets.responses.banks.size())return {};
    const VistaAnnouncerEvent prefix{event.words,event.data};
    const auto& definitions=assets.responses.banks[event.words[0]].definitions;
    const auto recipient=source_announcer_recipient_tu15(definitions,prefix,player);
    if(!recipient || !source_event_message_text_allowed_tu15(event.words[0],
        std::bit_cast<std::int32_t>(event.words[1]),recipient->audience,local_count,event.game_over))return {};
    const Bytes row(definitions[recipient->ordinal]);
    std::vector<SourceEventMessageOutputTu15> out;
    if(const auto id=row.u32(0xc);display_key(id)) {
        SourceEventMessageOutputTu15 value;
        value.channel=(row.u16(0)&1u)?SourceEventMessageChannelTu15::quantity:SourceEventMessageChannelTu15::literal;
        value.string_id=id;value.text=source_event_message_format_tu15(localized(assets,id),resolver);
        value.response_ordinal=recipient->ordinal;value.audience=recipient->audience;
        if(value.channel==SourceEventMessageChannelTu15::quantity) {
            value.quantity=std::bit_cast<std::int16_t>(std::uint16_t(event.words[7]));
            value.plural_string_id=row.u32(0x1c);
            value.plural_text=source_event_message_format_tu15(localized(assets,value.plural_string_id),resolver);
        }
        out.push_back(std::move(value));
    }
    if(const auto id=row.u32(0x14);display_key(id)) {
        SourceEventMessageOutputTu15 value;value.channel=SourceEventMessageChannelTu15::primary;
        value.string_id=id;value.text=source_event_message_format_tu15(localized(assets,id),resolver);
        value.duration_seconds=std::bit_cast<std::int32_t>(row.u32(0x18));
        value.response_ordinal=recipient->ordinal;value.audience=recipient->audience;
        out.push_back(std::move(value));
    }
    return out;
}
} // namespace halo2
