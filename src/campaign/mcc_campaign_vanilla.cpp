#include "halo2/mcc_campaign_vanilla.hpp"
#include <algorithm>
#include <array>
#include <map>
#include <openssl/evp.h>
#include <set>
#include <stdexcept>
#include <utility>
namespace halo2 {
namespace {
using Span=std::span<const std::byte>;
[[noreturn]] void mismatch(const char* text){throw std::runtime_error(text);}
struct Data {
    Span bytes;
    unsigned word(std::size_t at,unsigned size=4) const {
        if(at>bytes.size() || size>bytes.size()-at)mismatch("campaign metadata extent differs");
        unsigned value=0;for(unsigned i=0;i<size;++i)value|=std::to_integer<unsigned>(bytes[at+i])<<(i*8);return value;
    }
    std::string_view text(std::size_t at,std::size_t size) const {
        if(at>bytes.size() || size>bytes.size()-at)mismatch("campaign name extent differs");
        const auto first=bytes.begin()+at,last=first+size,end=std::find(first,last,std::byte{});
        if(end==last)mismatch("campaign name is unterminated");
        return {reinterpret_cast<const char*>(bytes.data()+at),std::size_t(end-first)};
    }
};
void put(std::span<std::byte> bytes,std::size_t at,std::uint32_t value,unsigned size=4) {
    if(at>bytes.size() || size>bytes.size()-at)mismatch("campaign output extent differs");
    for(unsigned i=0;i<size;++i)bytes[at+i]=std::byte(value>>(i*8));
}
std::array<std::byte,32> digest(Span bytes) {
    std::array<std::byte,32> result{};unsigned size=0;
    if(EVP_Digest(bytes.data(),bytes.size(),reinterpret_cast<unsigned char*>(result.data()),&size,EVP_sha256(),nullptr)!=1 || size!=32)
        mismatch("campaign fingerprint could not be computed");
    return result;
}
struct GuardBank {const char* name;std::uint32_t token,size;std::array<std::byte,32> before,after;};
struct RemovedScript {std::uint16_t index,type,return_type;std::uint32_t root;const char* name;};
struct ScriptReference {std::uint32_t datum;std::uint16_t index,kind;std::uint32_t function;const char* name;bool after_only;};
struct WakePatch {std::uint32_t predecessor,removed,next;};
struct AssignmentPatch {
    std::uint32_t predecessor,removed,next,function,global,literal,global_index,literal_value;
};
struct MoviePatch {
    std::uint32_t predecessor,removed,next,function,argument,string_offset,string_value;
    const char* movie;
};
struct CallPatch {
    std::uint32_t predecessor,removed,next,function;
    std::uint16_t opcode;
    const char* name;
    std::span<const std::uint32_t> rows;
};
struct NodeWordPatch {
    std::uint32_t datum;
    std::uint16_t offset,width;
    std::uint32_t before,after;
};
struct StringDebugUse {std::uint32_t datum,offset;std::uint16_t opcode,type,flags;};
struct StringSlotPatch {
    std::uint32_t function,after_function,offset,size;
    std::uint16_t declaration;
    std::uint32_t declaration_root;
    const char *before,*after;
    std::span<const StringDebugUse> debug;
};
struct SyntaxRow {
    std::uint16_t salt,opcode,type,flags;
    std::uint32_t next,label,value;
};
struct SyntaxRowRewrite {std::uint16_t index;SyntaxRow before,after;bool allocated;};
struct SyntaxAllocation {std::uint16_t index,salt;};
struct WrapperPromotion {
    std::uint32_t root,function,child,predecessor,next,child_next;
    std::uint16_t child_opcode;
};
struct GlobalRename {
    std::uint16_t index,type;
    std::uint32_t root,function,string_offset;
    std::uint16_t before_references,after_references;
    const char *before,*after;
};
struct NavpointNamespace {
    std::uint32_t hud,hud_header,bitmap,bitmap_header,shader,palette,sequences;
};
struct SoundDurationPromotion {
    std::uint32_t root,function,scalar,scalar_function,child,child_function,predecessor,next;
};
struct ScenarioOrdinal {std::uint32_t token;std::uint16_t index;const char* name;};
struct ScenarioName {std::uint32_t token;const char* name;};
enum class BindingKind {Tag,StringId,Object,Flag,Global,External,Script,Trigger,AiGroup,AiSquad,AiLocation,NullObject,Enum,Camera,Device,Bsp,AiOrder,Point,Navpoint};
struct LiteralBinding {
    std::uint32_t datum,value;
    std::uint16_t opcode,type,flags;
    BindingKind kind;
    std::uint16_t extra,extra2;
    const char *text,*name,*group;
    bool after_only;
};
struct Profile {
    const char *name,*path;
    std::uint32_t scenario;
    unsigned before_count,after_count,ordinal_slots,name_slots,script_words,scenario_words;
    std::span<const GuardBank> banks;
    std::span<const RemovedScript> removed;
    std::span<const ScriptReference> references;
    std::span<const WakePatch> patches;
    std::span<const AssignmentPatch> assignments;
    std::span<const MoviePatch> movies;
    std::span<const CallPatch> calls;
    std::span<const NodeWordPatch> words;
    std::span<const StringSlotPatch> slots;
    std::span<const WrapperPromotion> wrappers;
    std::span<const GlobalRename> renames;
    std::span<const NavpointNamespace> navpoints;
    std::span<const SoundDurationPromotion> durations;
    std::span<const SyntaxRowRewrite> rows;
    std::span<const SyntaxAllocation> allocations;
    std::span<const std::uint32_t> retired;
    std::span<const ScenarioOrdinal> ordinals;
    std::span<const ScenarioName> names;
    std::span<const LiteralBinding> bindings;
};
// The reviewed manifest pins fourteen authenticated MCC/original map pairs and
// the executable signature authorities (de65b4f4... / 21b207e7...). Removing
// only these dormant helper families, wakes, three added breadcrumb-index
// assignments,38 exact movie calls (opcode0x37F,895 decimal), and39 added
// navigation/oversteer/sound/animation/print calls makes each
// complete affected holder AST equal to its original
// counterpart. Full incoming edges and typed namespaces are retained in the
// manifest; unresolved surviving calls stay intact.
#include "campaign/mcc_campaign_vanilla_profiles.inc"
struct OwnedBank {const GuardBank* guard;Span bytes;};
struct Audit {
    const MccCampaignVanillaSource& source;
    const Profile& profile;
    std::vector<OwnedBank> banks;
    bool after{};
    Span nodes,strings,scripts,globals;
    Data root;
    std::set<std::uint32_t> retired;
    unsigned count{};
    Span owned(std::uint32_t token,std::size_t size) const {
        if(std::uint64_t(token)+size>0x100000000ull)mismatch("campaign source token overflows");
        for(const auto& bank:banks)if(token>=bank.guard->token &&
            std::uint64_t(token-bank.guard->token)+size<=bank.bytes.size())
            return bank.bytes.subspan(token-bank.guard->token,size);
        mismatch("campaign namespace is outside its verified owner");
    }
    Span named(std::string_view name) const {
        const auto found=std::find_if(banks.begin(),banks.end(),[&](const auto& b){return name==b.guard->name;});
        if(found==banks.end())mismatch("campaign source bank is missing");
        return found->bytes;
    }
    std::string_view string(unsigned offset) const {
        if(offset>=strings.size())mismatch("campaign syntax string escapes its owner");
        return Data{strings}.text(offset,strings.size()-offset);
    }
    Data row(std::uint32_t datum) const {
        const auto offset=std::size_t(datum&65535u)*20;
        if(datum==0xffffffffu || offset>nodes.size() || nodes.size()-offset<20)mismatch("campaign syntax handle escapes its owner");
        const Data value{nodes.subspan(offset,20)};
        if(!value.word(0,2) || value.word(0,2)!=(datum>>16) || value.word(6,2)==0xbaba)
            mismatch("campaign syntax handle has a stale salt");
        return value;
    }
    bool removed(unsigned index) const {
        return std::ranges::any_of(profile.removed,[&](const auto& r){return r.index==index;});
    }
    unsigned mapped(unsigned index) const {
        if(index>=profile.before_count || removed(index))mismatch("campaign reference targets a removed declaration");
        return index-std::ranges::count_if(profile.removed,[&](const auto& r){return r.index<index;});
    }
    unsigned current(unsigned old) const {return after?mapped(old):old;}
    Data declaration(unsigned index) const {
        if(index>=count)mismatch("campaign script ordinal escapes declarations");
        return {scripts.subspan(std::size_t(index)*40,40)};
    }
    std::pair<std::uint32_t,Span> block(Data parent,unsigned offset,unsigned stride) const {
        const auto size=parent.word(offset),address=parent.word(offset+4);
        if(size>65536 || std::uint64_t(size)*stride>0xffffffffu)mismatch("campaign block extent differs");
        if(!size)return {address,{}};
        return {address,owned(address,std::size_t(size)*stride)};
    }
    Data element(unsigned offset,unsigned index,unsigned stride) const {
        if(index>=root.word(offset))mismatch("campaign literal escapes its authored namespace");
        const auto token=std::uint64_t(root.word(offset+4))+std::uint64_t(index)*stride;
        if(token>0xffffffffu)mismatch("campaign namespace token overflows");
        return {owned(token,stride)};
    }
    void namespaces() const {
        if(root.word(0x1b8)!=count || scripts.size()!=profile.before_count*40 ||
            root.word(0x238)*20ull!=nodes.size() || root.word(0x1b0)!=strings.size() ||
            root.word(0x1c0)*40ull!=globals.size())mismatch("campaign declaration extents differ");
        if(!after)for(const auto& r:profile.removed) {
            const auto d=declaration(r.index);
            if(d.text(0,32)!=r.name || d.word(32,2)!=r.type || d.word(34,2)!=r.return_type ||
                d.word(36)!=r.root || r.type!=1)mismatch("campaign dormant declaration differs");
            row(r.root);
        }
        for(unsigned i=0;i<count;++i)if(after || !removed(i)) {
            const auto h=declaration(i).word(36);row(h);
            if(retired.contains(h))mismatch("campaign retired body has another script root");
        }
        for(unsigned at=0;at<globals.size();at+=40) {
            const auto h=Data{globals}.word(at+36);row(h);
            if(retired.contains(h))mismatch("campaign retired body has a global root");
        }
        if(!after)for(const auto h:retired)row(h);
        else for(const auto h:retired) {
            const auto offset=std::size_t(h&65535u)*20;
            if(Data{nodes}.word(offset,2) || !std::ranges::all_of(nodes.subspan(offset+2,18),[](auto b){return b==std::byte{0xba};}))
                mismatch("campaign free-row representation differs");
        }
        // Include every live row, not only roots reachable from declarations.
        for(unsigned at=0;at<nodes.size();at+=20) {
            const Data n{nodes.subspan(at,20)};const auto salt=n.word(0,2),flags=n.word(6,2);
            if(!salt || flags==0xbaba)continue;
            const auto h=(salt<<16)|at/20;if(retired.contains(h))continue;
            if(retired.contains(n.word(8))) {
                const auto edge=[&](const auto& p){return p.predecessor==h && p.removed==n.word(8);};
                const bool allowed=!after && (std::ranges::any_of(profile.patches,edge) || std::ranges::any_of(profile.assignments,edge) || std::ranges::any_of(profile.movies,edge) || std::ranges::any_of(profile.calls,edge) || std::ranges::any_of(profile.words,[&](const auto& w){return w.datum==h && w.offset==8 && w.before==n.word(8); }));
                if(!allowed)mismatch("campaign retired syntax has an additional incoming link");
            }
            if(!(flags&1) && retired.contains(n.word(16)) &&
                (after || !std::ranges::any_of(profile.words,[&](const auto& w){return w.datum==h && w.offset==16 && w.before==n.word(16); })))
                mismatch("campaign retired syntax has another parent");
        }
        for(const auto& word:profile.words) {
            if((word.width!=2 && word.width!=4) || (word.offset!=2 && word.offset!=4 && word.offset!=6 && word.offset!=8 && word.offset!=12 && word.offset!=16) ||
                row(word.datum).word(word.offset,word.width)!=(after?word.after:word.before))
                mismatch("campaign original typed fade word differs");
        }
        std::set<unsigned> rewritten_indices;
        for(const auto& rewrite:profile.rows) {
            if(!rewritten_indices.insert(rewrite.index).second || rewrite.index>=nodes.size()/20)mismatch("campaign reconstructed syntax row ownership differs");
            const Data actual{nodes.subspan(std::size_t(rewrite.index)*20,20)};const auto& expected=after?rewrite.after:rewrite.before;
            if(actual.word(0,2)!=expected.salt || actual.word(2,2)!=expected.opcode || actual.word(4,2)!=expected.type || actual.word(6,2)!=expected.flags ||
                actual.word(8)!=expected.next || actual.word(12)!=expected.label || actual.word(16)!=expected.value)
                mismatch("campaign reconstructed typed syntax row differs");
            if(rewrite.allocated && (rewrite.before.salt!=0 || rewrite.after.salt==0))mismatch("campaign reconstructed syntax slot was not free");
            if(!rewrite.allocated && rewrite.before.salt!=rewrite.after.salt)mismatch("campaign reused syntax row changed its identity");
        }
        if(!profile.allocations.empty()) {
            // Original2129F7 names the owner "script node".16E510 copies its
            // first two bytes and ORs8000;16E6A0 chooses the first free slots,
            // zeroes each new row and advances this actual name-derived salt.
            constexpr std::string_view pool_name="script node";
            unsigned salt=(unsigned(pool_name[0])|(unsigned(pool_name[1])<<8)|0x8000u),next=0;
            for(const auto& allocation:profile.allocations) {
                if(!after) {
                    while(next<nodes.size()/20 && Data{nodes}.word(next*20,2))++next;
                    if(allocation.index!=next++)mismatch("campaign syntax allocation differs from original free-slot policy");
                }
                if(allocation.salt!=salt)mismatch("campaign syntax allocation differs from original name-derived salt");
                if(++salt==65535)salt=0x8000;
                const auto rewritten=std::find_if(profile.rows.begin(),profile.rows.end(),[&](const auto& r){return r.index==allocation.index && r.allocated && r.after.salt==allocation.salt;});
                if(rewritten==profile.rows.end())mismatch("campaign allocated syntax row lacks its exact typed owner");
            }
            if(std::ranges::count_if(profile.rows,[](const auto& r){return r.allocated;})!=profile.allocations.size())mismatch("campaign syntax allocation census differs");
        }
        for(const auto& rename:profile.renames) {
            if(rename.index>=globals.size()/40)mismatch("campaign original clock global index differs");
            const Data declaration{globals.subspan(rename.index*40,40)};
            if(declaration.text(0,32)!=(after?rename.after:rename.before) || declaration.word(32,2)!=rename.type || declaration.word(36)!=rename.root || string(rename.string_offset)!=rename.after)
                mismatch("campaign original clock global declaration differs");
            unsigned references=0;
            for(unsigned at=0;at<nodes.size();at+=20) {
                const Data n{nodes.subspan(at,20)};const auto salt=n.word(0,2),flags=n.word(6,2);
                if(!salt || flags==0xbaba || !(flags&4) || n.word(16)!=rename.index)continue;
                if(string(n.word(12))!=(after?rename.after:rename.before))mismatch("campaign original clock reference label differs");
                ++references;
            }
            if(references!=(after?rename.after_references:rename.before_references))mismatch("campaign complete original clock reference census differs");
            if(!after) {
                const auto root=row(rename.root),fn=row(rename.function);
                if(root.word(2,2)!=911 || root.word(4,2)!=7 || root.word(6,2)!=8 || root.word(16)!=rename.function ||
                    fn.word(2,2)!=911 || fn.word(4,2)!=2 || fn.word(6,2)!=9 || fn.word(8)!=0xffffffffu || string(fn.word(12))!="game_tick_rate" || !retired.contains(rename.function))
                    mismatch("campaign original clock initializer owner differs");
            } else {
                const auto root=row(rename.root);
                if(root.word(2,2)!=7 || root.word(4,2)!=7 || root.word(6,2)!=9 || root.word(16)!=0xffff001eu)
                    mismatch("campaign original clock initializer literal differs");
            }
        }
        for(const auto& ns:profile.navpoints) {
            if(!source.resolve_tag)mismatch("campaign navpoint tag namespace is unavailable");
            const auto identity=[&](unsigned datum,std::string_view group,std::string_view path) {
                const auto tag=source.resolve_tag(datum);
                if(!tag || tag->group!=group || tag->path!=path)mismatch("campaign navpoint full-salt tag identity differs");
            };
            identity(ns.hud,"hudg","ui\\hud\\default");
            identity(ns.bitmap,"bitm","ui\\hud\\bitmaps\\new_hud\\hud_waypoints");
            identity(ns.shader,"shad","ui\\hud\\shaders\\waypoint_arrow");
            const Data hud{owned(ns.hud_header,8)},bitmap{owned(ns.bitmap_header,8)},palette{owned(ns.palette,8*24)},sequences{owned(ns.sequences,9*60)};
            if(hud.word(0)!=8 || hud.word(4)!=ns.palette || bitmap.word(0)!=9 || bitmap.word(4)!=ns.sequences ||
                palette.word(0)!=0x6269746du || palette.word(4)!=ns.bitmap || palette.word(8)!=0x73686164u || palette.word(12)!=ns.shader ||
                palette.word(16,2)!=1 || palette.word(18,2)!=1 || palette.word(20,2)!=65535 ||
                sequences.word(60+32,2)!=1 || sequences.word(60+34,2)!=1 || sequences.word(60+52)!=0)
                mismatch("campaign navpoint palette/bitmap sequence owner differs");
        }
        if(!after)for(const auto& wrapper:profile.wrappers) {
            const auto root=row(wrapper.root),fn=row(wrapper.function),child=row(wrapper.child),predecessor=row(wrapper.predecessor);
            if(root.word(2,2)!=0 || root.word(4,2)!=4 || root.word(6,2)!=8 || root.word(8)!=wrapper.next || root.word(16)!=wrapper.function ||
                fn.word(2,2)!=0 || fn.word(4,2)!=2 || fn.word(6,2)!=9 || fn.word(8)!=wrapper.child || string(fn.word(12))!="begin" ||
                child.word(2,2)!=wrapper.child_opcode || child.word(4,2)!=4 || child.word(6,2)!=8 || child.word(8)!=wrapper.child_next ||
                predecessor.word(8)!=wrapper.root || !retired.contains(wrapper.root) || !retired.contains(wrapper.function) || retired.contains(wrapper.child))
                mismatch("campaign original navigation wrapper promotion differs");
            unsigned incoming=0,child_incoming=0;
            for(unsigned at=0;at<nodes.size();at+=20) {
                const Data n{nodes.subspan(at,20)};const auto salt=n.word(0,2),flags=n.word(6,2);if(!salt || flags==0xbaba)continue;
                const auto h=(salt<<16)|at/20;
                if(h!=wrapper.root && h!=wrapper.function) {
                    if(n.word(8)==wrapper.root || n.word(8)==wrapper.function)++incoming;
                    if(!(flags&1) && (n.word(16)==wrapper.root || n.word(16)==wrapper.function))++incoming;
                }
                if(n.word(8)==wrapper.child)++child_incoming;
                if(!(flags&1) && n.word(16)==wrapper.child)++child_incoming;
            }
            if(incoming!=1 || child_incoming!=1)mismatch("campaign navigation wrapper or promoted child has shared ownership");
            auto next=wrapper.child_next;std::set<unsigned> seen;
            while(next!=wrapper.next) {
                if(!seen.insert(next).second)mismatch("campaign navigation wrapper children are cyclic");
                const auto added=std::find_if(profile.calls.begin(),profile.calls.end(),[&](const auto& p){return p.removed==next;});
                if(added==profile.calls.end())mismatch("campaign navigation wrapper has another surviving child");
                next=added->next;
            }
        }
        if(!after)for(const auto& promotion:profile.durations) {
            const auto root=row(promotion.root),fn=row(promotion.function),scalar=row(promotion.scalar),scalar_fn=row(promotion.scalar_function);
            const auto child=row(promotion.child),child_fn=row(promotion.child_function),predecessor=row(promotion.predecessor);
            if(root.word(2,2)!=9 || root.word(4,2)!=7 || root.word(6,2)!=8 || root.word(8)!=promotion.next || root.word(16)!=promotion.function ||
                fn.word(2,2)!=9 || fn.word(4,2)!=2 || fn.word(6,2)!=9 || fn.word(8)!=promotion.scalar || string(fn.word(12))!="*" ||
                scalar.word(2,2)!=912 || scalar.word(4,2)!=6 || scalar.word(6,2)!=8 || scalar.word(8)!=promotion.child || scalar.word(16)!=promotion.scalar_function ||
                scalar_fn.word(2,2)!=912 || scalar_fn.word(4,2)!=2 || scalar_fn.word(6,2)!=9 || scalar_fn.word(8)!=0xffffffffu || string(scalar_fn.word(12))!="game_tick_rate_scalar" ||
                child.word(2,2)!=604 || child.word(4,2)!=6 || child.word(6,2)!=8 || child.word(8)!=promotion.next || child.word(16)!=promotion.child_function ||
                child_fn.word(2,2)!=604 || child_fn.word(4,2)!=2 || child_fn.word(6,2)!=9 || string(child_fn.word(12))!="sound_impulse_language_time" ||
                predecessor.word(8)!=promotion.root)
                mismatch("campaign original sound-duration expression differs");
            const auto sound=row(child_fn.word(8));
            if(sound.word(2,2)!=32 || sound.word(4,2)!=32 || sound.word(6,2)!=9 || sound.word(8)!=0xffffffffu)
                mismatch("campaign original sound-duration argument differs");
            const std::set<unsigned> removed{promotion.root,promotion.function,promotion.scalar,promotion.scalar_function};
            if(removed.size()!=4 || std::ranges::any_of(removed,[&](auto h){return !retired.contains(h);}) || retired.contains(promotion.child) || retired.contains(promotion.child_function))
                mismatch("campaign sound-duration retirement ownership differs");
            unsigned incoming=0,child_incoming=0;
            for(unsigned at=0;at<nodes.size();at+=20) {
                const Data n{nodes.subspan(at,20)};const auto salt=n.word(0,2),flags=n.word(6,2);if(!salt || flags==0xbaba)continue;
                const auto h=(salt<<16)|at/20;
                if(!removed.contains(h)) {
                    if(removed.contains(n.word(8)))++incoming;
                    if(!(flags&1) && removed.contains(n.word(16)))++incoming;
                }
                if(n.word(8)==promotion.child)++child_incoming;
                if(!(flags&1) && n.word(16)==promotion.child)++child_incoming;
            }
            if(incoming!=1 || child_incoming!=1)mismatch("campaign original sound-duration promotion has shared ownership");
        }
        for(const auto& slot:profile.slots) {
            const auto expected=std::string_view(after?slot.after:slot.before);
            if(expected.size()+1>slot.size || slot.offset>strings.size() || slot.size>strings.size()-slot.offset)
                mismatch("campaign original fade string slot extent differs");
            const auto bytes=strings.subspan(slot.offset,slot.size);
            for(unsigned i=0;i<slot.size;++i)if(bytes[i]!=(i<expected.size()?std::byte(expected[i]):std::byte{}))
                mismatch("campaign original fade string slot bytes differ");
            const auto fn=row(after?slot.after_function:slot.function);
            if(fn.word(4,2)!=2 || fn.word(6,2)!=9 || fn.word(12)!=slot.offset || fn.word(8)!=0xffffffffu)
                mismatch("campaign original fade function-name owner differs");
            std::set<std::uint32_t> meaningful,debug;
            for(unsigned at=0;at<nodes.size();at+=20) {
                const Data n{nodes.subspan(at,20)};const auto salt=n.word(0,2),flags=n.word(6,2),offset=n.word(12),op=n.word(2,2);
                if(!salt || flags==0xbaba || offset<slot.offset || offset>=std::uint64_t(slot.offset)+slot.size)continue;
                const bool numeric=(flags&1) && !(flags&4) && op>=4 && op<=8;
                ((flags&1)&&!numeric?meaningful:debug).insert((salt<<16)|at/20);
            }
            std::set<unsigned> expected_debug;for(const auto& use:slot.debug)expected_debug.insert(use.datum);
            if(meaningful!=std::set<std::uint32_t>{after?slot.after_function:slot.function} || debug!=expected_debug)
                mismatch("campaign original fade string has another meaningful owner");
            // Original211CB0..211D93 queues expression handles without syntax+C.
            // Ordinary numeric primitives at211D48 consume+10. Every overlapping
            // numeric/source/debug field remains byte-identical and checked.
            for(const auto& use:slot.debug) {
                const auto n=row(use.datum);const bool numeric=(n.word(6,2)&1) && !(n.word(6,2)&4) && n.word(2,2)>=4 && n.word(2,2)<=8;
                if(n.word(12)!=use.offset || n.word(2,2)!=use.opcode || n.word(4,2)!=use.type || n.word(6,2)!=use.flags || ((n.word(6,2)&1)&&!numeric))
                    mismatch("campaign overlapping original source/debug tuple changed");
            }
            const auto d=declaration(slot.declaration);
            if(d.text(0,32)!=slot.after || d.word(32,2)!=3 || d.word(34,2)!=4 || d.word(36)!=slot.declaration_root)
                mismatch("campaign restored original static declaration differs");
        }
        std::set<std::uint32_t> verified;
        for(const auto& ref:profile.references) {
            if((ref.after_only && !after) || (after && retired.contains(ref.datum)))continue;
            const auto n=row(ref.datum);const auto flags=n.word(6,2),index=current(ref.index);
            if(declaration(index).text(0,32)!=ref.name)mismatch("campaign script ordinal namespace differs");
            if(!ref.kind) {
                const auto fn=row(ref.function);
                if((flags&3)!=2 || n.word(2,2)!=index || n.word(16)!=ref.function ||
                    !(fn.word(6,2)&1) || fn.word(4,2)!=2 || fn.word(2,2)!=index ||
                    string(fn.word(12))!=ref.name)mismatch("campaign static script reference differs");
            } else if(!(flags&1) || flags&4 || n.word(4,2)==2 || n.word(2,2)!=ref.kind ||
                (n.word(16)&65535u)!=index || string(n.word(12))!=ref.name)
                mismatch("campaign typed script literal differs");
            if(!verified.insert(ref.datum).second)mismatch("campaign duplicate typed script reference");
        }
        for(unsigned at=0;at<nodes.size();at+=20) {
            const Data n{nodes.subspan(at,20)};const auto salt=n.word(0,2),flags=n.word(6,2),op=n.word(2,2),type=n.word(4,2);
            if(!salt || flags==0xbaba)continue;
            const bool reference=(!(flags&1) && (flags&2)) ||
                ((flags&1) && !(flags&4) && type!=2 && (op==10 || op==21 || type==10 || type==21));
            if(reference && !verified.contains((salt<<16)|at/20))mismatch("campaign additional typed script ordinal");
        }
        if(!after)for(const auto& patch:profile.patches) {
            const auto predecessor=row(patch.predecessor),call=row(patch.removed);
            if(predecessor.word(8)!=patch.removed || call.word(8)!=patch.next || call.word(6,2)&3)
                mismatch("campaign wake predecessor/successor differs");
            const auto fn=row(call.word(16));const auto arg=row(fn.word(8));
            if(!(fn.word(6,2)&1) || fn.word(4,2)!=2 || string(fn.word(12))!="wake" ||
                !(arg.word(6,2)&1) || arg.word(6,2)&4 || arg.word(2,2)!=10 ||
                arg.word(8)!=0xffffffffu || !removed(arg.word(16)&65535u) ||
                !retired.contains(call.word(16)) || !retired.contains(fn.word(8)))
                mismatch("campaign added wake is not the verified sole typed reference");
        }
        if(!after)for(const auto& patch:profile.assignments) {
            const auto predecessor=row(patch.predecessor),call=row(patch.removed);
            const auto fn=row(patch.function),target=row(patch.global),literal=row(patch.literal);
            if(predecessor.word(8)!=patch.removed || call.word(8)!=patch.next ||
                call.word(2,2)!=4 || call.word(4,2)!=4 || call.word(6,2)!=8 || call.word(16)!=patch.function ||
                fn.word(2,2)!=4 || fn.word(4,2)!=2 || fn.word(6,2)!=9 || fn.word(8)!=patch.global || string(fn.word(12))!="set" ||
                target.word(2,2)!=65535 || target.word(4,2)!=7 || target.word(6,2)!=13 || target.word(8)!=patch.literal ||
                target.word(16)!=patch.global_index || string(target.word(12))!="g_breadcrumb_nav_index" ||
                literal.word(2,2)!=7 || literal.word(4,2)!=7 || literal.word(6,2)!=9 || literal.word(8)!=0xffffffffu || literal.word(16)!=patch.literal_value)
                mismatch("campaign added breadcrumb assignment differs");
            if(patch.global_index>=globals.size()/40 || Data{globals}.text(patch.global_index*40,32)!="g_breadcrumb_nav_index" ||
                Data{globals}.word(patch.global_index*40+32,2)!=7)
                mismatch("campaign added assignment global namespace differs");
            for(const auto h:{patch.removed,patch.function,patch.global,patch.literal})if(!retired.contains(h))
                mismatch("campaign added assignment is outside retired ownership");
        }
        if(!after)for(const auto& patch:profile.movies) {
            const auto predecessor=row(patch.predecessor),call=row(patch.removed);
            const auto fn=row(patch.function),argument=row(patch.argument);
            // Actual node bytes7F03 encode opcode0x37F (895 decimal). Every
            // admitted call belongs to a complete restored original holder;
            // source-proven companion timing/authoring changes are explicit.
            if(predecessor.word(8)!=patch.removed || call.word(8)!=patch.next ||
                call.word(2,2)!=0x37f || call.word(4,2)!=4 || call.word(6,2)!=8 || call.word(16)!=patch.function ||
                fn.word(2,2)!=0x37f || fn.word(4,2)!=2 || fn.word(6,2)!=9 || fn.word(8)!=patch.argument || string(fn.word(12))!="cinematic_start_movie" ||
                argument.word(2,2)!=9 || argument.word(4,2)!=9 || argument.word(6,2)!=9 || argument.word(8)!=0xffffffffu ||
                argument.word(12)!=patch.string_offset || argument.word(16)!=patch.string_value || string(patch.string_offset)!=patch.movie)
                mismatch("campaign added movie closure differs");
            for(const auto h:{patch.removed,patch.function,patch.argument})if(!retired.contains(h))
                mismatch("campaign added movie is outside retired ownership");
        }
        if(!after)for(const auto& patch:profile.calls) {
            const auto predecessor=row(patch.predecessor),call=row(patch.removed),fn=row(patch.function);
            if(predecessor.word(8)!=patch.removed || call.word(8)!=patch.next ||
                call.word(2,2)!=patch.opcode || call.word(4,2)!=4 || call.word(6,2)!=8 || call.word(16)!=patch.function ||
                fn.word(2,2)!=patch.opcode || fn.word(4,2)!=2 || fn.word(6,2)!=9 || string(fn.word(12))!=patch.name)
                mismatch("campaign added authoring call differs");
            std::set<std::uint32_t> expected(patch.rows.begin(),patch.rows.end()),seen;
            if(expected.size()!=patch.rows.size() || !expected.contains(patch.removed) || !expected.contains(patch.function))
                mismatch("campaign added call ownership is duplicated or incomplete");
            const auto visit=[&](const auto& self,std::uint32_t datum)->void {
                if(!expected.contains(datum) || !retired.contains(datum) || !seen.insert(datum).second)
                    mismatch("campaign added call closure is shared, cyclic or outside its owner");
                const auto node=row(datum);
                if(!(node.word(6,2)&1))self(self,node.word(16));
                if(datum!=patch.removed && node.word(8)!=0xffffffffu)self(self,node.word(8));
            };
            visit(visit,patch.removed);
            if(seen!=expected)mismatch("campaign added call closure census differs");
        }
        for(const auto& binding:profile.bindings) {
            if((binding.after_only && !after) || (after && retired.contains(binding.datum)))continue;
            const auto n=row(binding.datum);auto expected=binding.value;
            if(after && binding.kind==BindingKind::Script)expected=(expected&0xffff0000u)|mapped(expected&65535u);
            std::string_view expected_text=binding.text,expected_name=binding.name;
            if(after && binding.kind==BindingKind::Global)for(const auto& rename:profile.renames)if((binding.value&65535u)==rename.index)expected_text=expected_name=rename.after;
            if(n.word(2,2)!=binding.opcode || n.word(4,2)!=binding.type || n.word(6,2)!=binding.flags ||
                n.word(16)!=expected || string(n.word(12))!=expected_text)mismatch("campaign literal source tuple differs");
            const auto sid=[&](unsigned value) {
                if(!source.resolve_string_id)mismatch("campaign StringID namespace is unavailable");
                return source.resolve_string_id(value);
            };
            const auto name=expected_name;const auto value=n.word(16);
            switch(binding.kind) {
            case BindingKind::Tag: {
                if(!source.resolve_tag)mismatch("campaign tag namespace is unavailable");
                const auto tag=source.resolve_tag(value);
                if(!tag || tag->group!=binding.group || tag->path!=name)mismatch("campaign full-salt tag namespace differs");
                break;
            }
            case BindingKind::StringId:if(sid(value)!=name)mismatch("campaign StringID identity differs");break;
            case BindingKind::Object: {
                const auto d=element(0x48,value&65535u,0x24);
                if(d.text(0,32)!=name || d.word(32,2)!=binding.extra || d.word(34,2)!=binding.extra2)mismatch("campaign object-name namespace differs");
                break;
            }
            case BindingKind::Flag:if(element(0x1e0,value&65535u,0x38).text(4,32)!=name)mismatch("campaign cutscene-flag namespace differs");break;
            case BindingKind::Camera:if(element(0x1e8,value&65535u,0x40).text(4,32)!=name)mismatch("campaign camera-point namespace differs");break;
            case BindingKind::Device:if(element(0xa0,value&65535u,0x28).text(0,32)!=name)mismatch("campaign device-group namespace differs");break;
            case BindingKind::Bsp: {
                const auto reference=element(0x210,value&65535u,0x44);
                if(reference.word(0x10)!=0x73627370u || !source.resolve_tag)mismatch("campaign BSP tag namespace is unavailable");
                const auto tag=source.resolve_tag(reference.word(0x14));
                if(!tag || tag->group!=binding.group || tag->path!=name)mismatch("campaign BSP full-salt tag namespace differs");
                break;
            }
            case BindingKind::Global: {
                const auto index=value&65535u;if(index>=globals.size()/40)mismatch("campaign global ordinal differs");
                const Data d{globals.subspan(index*40,40)};
                if(d.text(0,32)!=name || d.word(32,2)!=binding.extra)mismatch("campaign global namespace differs");
                break;
            }
            case BindingKind::External:if(!(value&0x8000u) || (value&0x7fffu)!=binding.extra || string(n.word(12))!=name)mismatch("campaign external-global namespace differs");break;
            case BindingKind::Script: {
                const auto d=declaration(value&65535u);
                if(d.text(0,32)!=name || d.word(32,2)!=binding.extra || d.word(34,2)!=binding.extra2)mismatch("campaign script literal namespace differs");
                break;
            }
            case BindingKind::Trigger:if(sid(element(0x108,value&65535u,0x44).word(0))!=name)mismatch("campaign trigger-volume namespace differs");break;
            case BindingKind::Navpoint:if(profile.navpoints.size()!=1 || value!=0 || name!="default" || binding.opcode!=27 || binding.type!=27 || binding.flags!=9)mismatch("campaign retained navpoint literal differs");break;
            case BindingKind::AiOrder:if(element(0x240,value&65535u,0x7c).text(0,32)!=name)mismatch("campaign AI order namespace differs");break;
            case BindingKind::Point: {
                const auto [data_token,data]=block(root,0x1d8,0x80);(void)data_token;
                if(data.size()!=0x80)mismatch("campaign point script-data owner differs");
                const auto [sets_token,sets]=block(Data{data},0,0x30);(void)sets_token;
                const auto set_index=value>>16,index=value&65535u;
                if(set_index>=sets.size()/0x30)mismatch("campaign point-set ordinal differs");
                const Data set{sets.subspan(set_index*0x30,0x30)};
                const auto [points_token,points]=block(set,0x20,0x3c);(void)points_token;
                if(index>=points.size()/0x3c || std::string(set.text(0,32))+"/"+std::string(Data{points.subspan(index*0x3c,0x3c)}.text(0,32))!=name)
                    mismatch("campaign point-reference identity differs");
                break;
            }
            case BindingKind::AiGroup:case BindingKind::AiSquad: {
                const bool group=binding.kind==BindingKind::AiGroup;
                if(value>>30!=unsigned(group) || element(group?0x158:0x160,value&65535u,group?0x24:0x74).text(0,32)!=name)mismatch("campaign AI squad/group namespace differs");
                break;
            }
            case BindingKind::AiLocation: {
                if(value>>30!=3)mismatch("campaign AI starting-location type differs");
                const auto squad=element(0x160,(value>>16)&0x3fffu,0x74);const auto index=value&65535u;
                const auto [token,locations]=block(squad,0x48,0x64);(void)token;
                if(index>=locations.size()/0x64)mismatch("campaign AI starting-location ordinal differs");
                if(std::string(squad.text(0,32))+"/"+sid(Data{locations.subspan(index*0x64,0x64)}.word(0))!=name)mismatch("campaign AI starting-location identity differs");
                break;
            }
            case BindingKind::NullObject:if(value!=0xffffffffu || name!="none" || binding.opcode!=binding.extra)mismatch("campaign null object literal differs");break;
            case BindingKind::Enum:if((value&65535u)!=binding.extra || string(n.word(12))!=name)mismatch("campaign enum literal differs");break;
            }
        }
    }
    void scenario_fields() const {
        std::map<unsigned,const ScenarioOrdinal*> expected;
        for(const auto& field:profile.ordinals) {
            const auto [found,added]=expected.emplace(field.token,&field);
            if(!added && (found->second->index!=field.index || std::string_view(found->second->name)!=field.name))
                mismatch("campaign aliased scenario ordinals disagree");
        }
        std::map<unsigned,std::string_view> names;
        for(const auto& field:profile.names) {
            const auto [found,added]=names.emplace(field.token,field.name);
            if(!added && found->second!=field.name)mismatch("campaign aliased authored names disagree");
        }
        std::set<unsigned> seen,seen_names;unsigned ordinal_slots=0,name_slots=0;
        const auto ordinal=[&](unsigned token) {
            ++ordinal_slots;const auto value=Data{owned(token,2)}.word(0,2);if(value==65535)return;
            const auto found=expected.find(token);
            if(found==expected.end() || value!=current(found->second->index) || declaration(value).text(0,32)!=found->second->name)
                mismatch("campaign scenario script ordinal namespace differs");
            seen.insert(token);
        };
        const auto name=[&](unsigned token) {
            ++name_slots;const auto text=Data{owned(token,32)}.text(0,32);const auto found=names.find(token);
            if(text.empty()){if(found!=names.end())mismatch("campaign authored name disappeared");return;}
            if(found==names.end() || found->second!=text)mismatch("campaign authored script name differs");
            bool defined=false;for(unsigned i=0;i<count;++i)defined|=declaration(i).text(0,32)==text;
            if(!defined)mismatch("campaign authored script name lacks a declaration");
            seen_names.insert(token);
        };
        const auto each=[&](Data parent,unsigned offset,unsigned stride,const auto& function) {
            const auto [token,data]=block(parent,offset,stride);
            for(unsigned i=0;i<data.size()/stride;++i)function(token+i*stride,Data{data.subspan(i*stride,stride)});
        };
        each(root,0x160,0x74,[&](unsigned token,Data squad) {
            name(token+0x50);ordinal(token+0x70);
            each(squad,0x48,0x64,[&](unsigned child,Data){name(child+0x40);ordinal(child+0x60);});
        });
        each(root,0x190,0x28,[&](unsigned token,Data){name(token);});
        each(root,0x240,0x7c,[&](unsigned token,Data){name(token+0x2c);ordinal(token+0x4c);});
        each(root,0x248,0x30,[&](unsigned,Data trigger){each(trigger,0x28,0x38,[&](unsigned token,Data){name(token+0x10);ordinal(token+0x30);});});
        if(ordinal_slots!=profile.ordinal_slots || name_slots!=profile.name_slots || seen.size()!=expected.size() || seen_names.size()!=names.size())
            mismatch("campaign complete scenario reference census differs");
    }
};
}
MccCampaignVanillaPlan plan_mcc_campaign_vanilla(const MccCampaignVanillaSource& source) {
    const auto reject=[](std::string why){MccCampaignVanillaPlan result;result.reason=std::move(why);return result;};
    if(!source.is_mcc)return reject("not an MCC campaign donor");
    const auto found=std::find_if(campaign_profiles.begin(),campaign_profiles.end(),[&](const auto& p){return source.scenario_path==p.path && source.scenario_address==p.scenario;});
    if(found==campaign_profiles.end())return reject("no authenticated campaign authoring profile for this source identity");
    if(!source.read)return reject("campaign source reader is unavailable");
    try {
        const auto& profile=*found;Audit audit{source,profile,{},false,{},{},{},{},{},{},0};bool before=true,after=true;
        for(const auto& guard:profile.banks) {
            if(std::uint64_t(guard.token)+guard.size>0x100000000ull)mismatch("campaign bank token overflows");
            const auto bytes=source.read(guard.token,guard.size);
            if(bytes.size()!=guard.size)mismatch("campaign source reader returned a truncated owner");
            const auto hash=digest(bytes);before&=hash==guard.before;after&=hash==guard.after;audit.banks.push_back({&guard,bytes});
        }
        if(!before && !after)return reject("campaign donor bank differs from authenticated retail authoring");
        audit.after=after;audit.nodes=audit.named("block_568");audit.strings=audit.named("block_432");audit.scripts=audit.named("block_440");audit.globals=audit.named("block_448");audit.root={audit.named("scenario")};audit.count=after?profile.after_count:profile.before_count;audit.retired.insert(profile.retired.begin(),profile.retired.end());
        audit.namespaces();audit.scenario_fields();
        MccCampaignVanillaPlan result;result.matched=true;result.profile=profile.name;
        if(after){result.already_equivalent=true;return result;}
        const auto add=[&](unsigned address,Span old,std::vector<std::byte> next) {
            if(old.size()!=next.size())mismatch("campaign planned write changes owner extent");
            if(!std::ranges::equal(old,next))result.writes.push_back({address,{old.begin(),old.end()},std::move(next)});
        };
        std::vector<std::byte> nodes(audit.nodes.begin(),audit.nodes.end());
        std::map<std::uint32_t,std::uint32_t> removed_successors;
        const auto remember=[&](const auto& patches) {
            for(const auto& patch:patches)if(!removed_successors.emplace(patch.removed,patch.next).second)
                mismatch("campaign removed call has duplicate splice ownership");
        };
        remember(profile.patches);remember(profile.assignments);remember(profile.movies);remember(profile.calls);
        const auto splice=[&](const auto& patches) {
            for(const auto& patch:patches) {
                auto next=patch.next;std::set<std::uint32_t> chain;
                while(removed_successors.contains(next)) {
                    if(!chain.insert(next).second)mismatch("campaign removed sibling chain is cyclic");
                    next=removed_successors.at(next);
                }
                if(audit.retired.contains(next))mismatch("campaign splice targets a retired non-call row");
                put(nodes,std::size_t(patch.predecessor&65535u)*20+8,next);
            }
        };
        // Consecutive added calls each retain their checked source edge. Their
        // surviving predecessor skips the entire chain before rows are freed.
        splice(profile.patches);splice(profile.assignments);splice(profile.movies);splice(profile.calls);
        for(const auto& word:profile.words)put(nodes,std::size_t(word.datum&65535u)*20+word.offset,word.after,word.width);
        for(const auto& reference:profile.references) {
            if(audit.retired.contains(reference.datum))continue;
            const auto mapped=audit.mapped(reference.index);if(mapped==reference.index)continue;
            if(!reference.kind) {
                put(nodes,std::size_t(reference.datum&65535u)*20+2,mapped,2);
                put(nodes,std::size_t(reference.function&65535u)*20+2,mapped,2);result.script_ordinal_words+=2;
            } else {put(nodes,std::size_t(reference.datum&65535u)*20+16,mapped,2);++result.script_ordinal_words;}
        }
        for(const auto& rewrite:profile.rows) {
            const auto at=std::size_t(rewrite.index)*20;const auto& row=rewrite.after;
            put(nodes,at,row.salt,2);put(nodes,at+2,row.opcode,2);put(nodes,at+4,row.type,2);put(nodes,at+6,row.flags,2);
            put(nodes,at+8,row.next);put(nodes,at+12,row.label);put(nodes,at+16,row.value);
        }
        // Original16E510/32A4E0 releases a row as zero salt plus18 BA bytes.
        // No remaining literal/string/builtin fields are canonicalized here.
        for(const auto datum:profile.retired) {
            const auto offset=std::size_t(datum&65535u)*20;std::fill_n(nodes.begin()+offset,20,std::byte{0xba});put(nodes,offset,0,2);
        }
        add(audit.root.word(0x23c),audit.nodes,std::move(nodes));
        std::vector<std::byte> strings(audit.strings.begin(),audit.strings.end());
        for(const auto& slot:profile.slots) {
            std::fill_n(strings.begin()+slot.offset,slot.size,std::byte{});
            const auto text=std::string_view(slot.after);
            std::transform(text.begin(),text.end(),strings.begin()+slot.offset,[](char c){return std::byte(c);});
        }
        add(audit.root.word(0x1b4),audit.strings,std::move(strings));

        std::vector<std::byte> scripts(audit.scripts.begin(),audit.scripts.end());unsigned target=0;
        for(unsigned i=0;i<profile.before_count;++i)if(!audit.removed(i)) {
            std::copy_n(audit.scripts.begin()+i*40,40,scripts.begin()+target*40);++target;
        }
        if(target!=profile.after_count)mismatch("campaign declaration compaction census differs");
        add(audit.root.word(0x1bc),audit.scripts,std::move(scripts));
        std::vector<std::byte> globals(audit.globals.begin(),audit.globals.end());
        for(const auto& rename:profile.renames) {
            const auto at=std::size_t(rename.index)*40;std::fill_n(globals.begin()+at,32,std::byte{});
            const auto name=std::string_view(rename.after);
            std::transform(name.begin(),name.end(),globals.begin()+at,[](char c){return std::byte(c);});
        }
        add(audit.root.word(0x1c4),audit.globals,std::move(globals));
        auto count=audit.owned(profile.scenario+0x1b8,4);std::vector<std::byte> next_count(count.begin(),count.end());put(next_count,0,profile.after_count);add(profile.scenario+0x1b8,count,std::move(next_count));
        for(const auto& field:profile.ordinals) {
            const auto mapped=audit.mapped(field.index);if(mapped==field.index)continue;
            const auto old=audit.owned(field.token,2);std::vector<std::byte> next(old.begin(),old.end());put(next,0,mapped,2);add(field.token,old,std::move(next));++result.scenario_ordinal_words;
        }
        if(result.script_ordinal_words!=profile.script_words || result.scenario_ordinal_words!=profile.scenario_words)mismatch("campaign rewrite census differs");
        std::sort(result.writes.begin(),result.writes.end(),[](const auto& a,const auto& b){return a.address<b.address;});
        for(unsigned i=1;i<result.writes.size();++i)if(std::uint64_t(result.writes[i-1].address)+result.writes[i-1].after.size()>result.writes[i].address)mismatch("campaign planned writes overlap");
        // Verify the complete output tuple, including legitimate shared source
        // blocks. A planner bug cannot publish a partially matching profile.
        for(const auto& bank:audit.banks) {
            std::vector<std::byte> projected(bank.bytes.begin(),bank.bytes.end());
            for(const auto& write:result.writes) {
                const auto first=std::max(std::uint64_t(bank.guard->token),std::uint64_t(write.address));
                const auto last=std::min(std::uint64_t(bank.guard->token)+bank.bytes.size(),std::uint64_t(write.address)+write.after.size());
                if(first<last)std::copy(write.after.begin()+first-write.address,write.after.begin()+last-write.address,projected.begin()+first-bank.guard->token);
            }
            if(digest(projected)!=bank.guard->after)mismatch("campaign planned output differs from verified original-authoring projection");
        }
        result.removed_scripts=profile.removed.size();result.retired_nodes=profile.retired.size();result.wake_patches=profile.patches.size();return result;
    } catch(const std::exception& error) {return reject(error.what());}
}
} // namespace halo2
