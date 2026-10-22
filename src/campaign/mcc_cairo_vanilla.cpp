#include "halo2/mcc_cairo_vanilla.hpp"
#include "halo2/mcc_script_layout.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <array>
#include <map>
#include <set>
#include <openssl/evp.h>
namespace halo2 {
namespace {
struct GuardBank {const char* name;std::uint32_t token,size;std::array<std::byte,32> digest;};
struct ScriptReference {std::uint32_t datum;std::uint16_t index,kind;std::uint32_t function;const char* name;};
struct AuthoringPatch {std::uint32_t predecessor,removed,last,next;};
struct ScenarioOrdinal {std::uint32_t token;std::uint16_t index;const char* name;};
struct RemovedScript {std::uint16_t index,type,return_type;std::uint32_t root;const char* name;};
struct LiteralRestore {std::uint32_t datum,parent,argument,before,after;};
struct ClockInitializer {std::uint32_t index,root,function,label;};
struct ClockLiteralRestore {std::uint32_t datum,parent,other,context,other_value;};
struct BrandRestore {std::uint32_t parent,function,effect,object,marker;const char* effect_path;const char* object_path;};
struct RemovedGlobal {std::uint32_t index,type,kind,root;const char* name;};
struct RemovedGlobalReference {std::uint32_t datum,opcode;};
struct SoundRemoval {
    std::uint32_t root,function,sound,emitter,volume,ai_function,ai_literal,predecessor,
        opcode,xbox_opcode,sound_type,handle;
    const char* path;const char* name;
};
struct SoundRow {std::uint32_t datum,salt,opcode,type,flags,next,label,value;};
struct TypedRestore {bool allocated;SoundRow before,after;};
struct IncomingOwner {std::uint32_t datum,from;bool child;};
struct FinalLink {std::uint32_t datum,before,after;};
struct FlushCall {std::uint32_t root,function,opcode,xbox_opcode;const char* name;};
struct FunctionSlot {
    std::uint32_t offset,size,before_owner;std::array<std::uint32_t,2> after_owners;
    const char* before_name;const char* after_name;
};
#include "campaign/mcc_cairo_vanilla_authoring.inc"
bool matches_digest(std::span<const std::byte> bytes,const GuardBank& bank) {
    std::array<std::byte,32> digest{};unsigned size=0;
    return bytes.size()==bank.size && EVP_Digest(bytes.data(),bytes.size(),reinterpret_cast<unsigned char*>(digest.data()),&size,
        EVP_sha256(),nullptr)==1 && size==digest.size() && digest==bank.digest;
}
void put(std::span<std::byte> bytes,std::size_t at,std::uint32_t value,unsigned width=4) {
    for(unsigned n=0;n<width;++n)bytes[at+n]=std::byte(value>>(n*8));
}
std::string_view text(std::span<const std::byte> bytes,unsigned at) {
    if(at>=bytes.size())throw FormatError("Cairo authoring string escapes bank");
    const auto end=std::find(bytes.begin()+at,bytes.end(),std::byte{});
    if(end==bytes.end())throw FormatError("Cairo authoring string is unterminated");
    return {reinterpret_cast<const char*>(bytes.data()+at),std::size_t(end-(bytes.begin()+at))};
}
}
MccCairoVanillaPlan plan_mcc_cairo_vanilla(const Cache& cache) {
    return plan_mcc_cairo_vanilla(cache,[&](auto address,auto size){return cache.read_tag_bytes(address,size);});
}
MccCairoVanillaPlan plan_mcc_cairo_vanilla(const Cache& cache,const MccCairoSourceReader& read) {
    MccCairoVanillaPlan result;
    const auto reject=[&](const char* why){MccCairoVanillaPlan rejected;rejected.reason=why;return rejected;};
    if(!cache.is_mcc())return reject("not an MCC donor");
    const auto* scenario=cache.find_tag(cache.scenario_tag());
    if(!scenario || scenario->group!="scnr" || scenario->name!=
        "scenarios\\solo\\01b_spacestation\\01b_spacestation" || scenario->address!=cairo_scenario_address)
        return reject("not the authenticated Cairo scenario");
    try {
        std::map<std::string_view,std::span<const std::byte>> banks;
        for(const auto& bank:guard_banks) {
            const auto bytes=read(bank.token,bank.size);
            if(!matches_digest(bytes,bank))return reject("Cairo donor bank differs from authenticated retail authoring");
            banks.emplace(bank.name,bytes);
        }
        const Bytes root(banks.at("scenario"));
        const auto nodes=banks.at("block_568"),strings=banks.at("block_432"),scripts=banks.at("block_440");
        const Bytes ns(nodes),defs(scripts),globals(banks.at("block_448"));
        const auto row=[&](unsigned datum) {
            const auto offset=std::size_t(datum&0xffffu)*20;
            ns.require(offset,20);
            if(ns.u16(offset)!=(datum>>16) || !ns.u16(offset) || ns.u16(offset+6)==0xbaba)
                throw FormatError("Cairo authoring datum salt differs");
            return offset;
        };
        // The unchanged v3 projector verifies actual tag/stringID/object/flag/
        // global/static namespaces for the three cinematic donor subtrees.
        const auto v3=project_mcc_script_layout(cache,nodes,strings,scripts,{true});
        for(unsigned datum:{0xea2b06b8u,0xea2c06b9u,0xea2d06bau,0x81c71e53u,0x81c81e54u,
            0x81c91e55u,0xf7ca1457u,0xf7cb1458u,0xf84f14dcu,0xf85014ddu})
            if(Bytes(v3.nodes).u16(row(datum)))return reject("Cairo cinematic namespace guard differs");
        const auto clock_root=row(clock_initializer.root),clock_function=row(clock_initializer.function);
        const auto getter=mcc_script_builtin_signature(0x38f);
        if(root.u32(0x1c0)!=52 || globals.string(clock_initializer.index*40,32)!="tick_seconds" ||
            globals.u16(clock_initializer.index*40+32)!=7 || globals.u32(clock_initializer.index*40+36)!=clock_initializer.root ||
            !getter || getter->name!="game_tick_rate" || getter->declared_return_type!=8 ||
            !getter->argument_types.empty() || getter->xbox_opcode ||
            ns.u16(clock_root+2)!=0x38f || ns.u16(clock_root+4)!=7 || ns.u16(clock_root+6)!=8 ||
            ns.u32(clock_root+8)!=0xffffffffu || ns.u32(clock_root+16)!=clock_initializer.function ||
            ns.u16(clock_function+2)!=0x38f || ns.u16(clock_function+4)!=2 || ns.u16(clock_function+6)!=9 ||
            ns.u32(clock_function+8)!=0xffffffffu || text(strings,ns.u32(clock_function+12))!="game_tick_rate" ||
            text(strings,18)!="tick_seconds" || text(strings,clock_initializer.label)!="seconds")
            return reject("Cairo original seconds initializer namespace differs");
        const std::set<unsigned> clock_reference_set(clock_references.begin(),clock_references.end());
        for(const auto datum:clock_references) {
            const auto at=row(datum);
            if(ns.u16(at+2)!=6 || ns.u16(at+4)!=6 || ns.u16(at+6)!=13 ||
                ns.u32(at+16)!=18 || ns.u32(at+12)!=18)
                return reject("Cairo typed seconds reference differs");
        }
        for(unsigned i=0;i<nodes.size()/20;++i) {
            const auto at=i*20;if(!ns.u16(at) || ns.u16(at+6)==0xbaba)continue;
            if(ns.u32(at+8)==clock_initializer.root ||
                (!(ns.u16(at+6)&1) && ns.u32(at+16)==clock_initializer.root))
                return reject("Cairo seconds initializer has an incoming syntax owner");
            if((ns.u16(at+6)&4) && (ns.u32(at+16)&0xffffu)==18 &&
                !clock_reference_set.contains((unsigned(ns.u16(at))<<16)|i))
                return reject("Cairo has another seconds global reference");
        }
        const auto unique_incoming=[&](unsigned target,unsigned predecessor,bool child) {
            unsigned count=0;
            for(unsigned i=0;i<nodes.size()/20;++i) {
                const auto at=i*20;if(!ns.u16(at) || ns.u16(at+6)==0xbaba)continue;
                const auto datum=(unsigned(ns.u16(at))<<16)|i;
                if(ns.u32(at+8)==target) {if(child || datum!=predecessor)return false;++count;}
                if(!(ns.u16(at+6)&1) && ns.u32(at+16)==target) {if(!child || datum!=predecessor)return false;++count;}
            }
            return count==1;
        };
        const auto matches_row=[&](const SoundRow& expected) {
            const auto at=std::size_t(expected.datum&0xffffu)*20;ns.require(at,20);
            return ns.u16(at)==expected.salt && ns.u16(at+2)==expected.opcode &&
                ns.u16(at+4)==expected.type && ns.u16(at+6)==expected.flags &&
                ns.u32(at+8)==expected.next && ns.u32(at+12)==expected.label &&
                ns.u32(at+16)==expected.value;
        };
        std::set<unsigned> allocated_rows,timing_rows;
        unsigned free_cursor=0,allocation_salt=0x8000u|unsigned('s')|(unsigned('c')<<8);
        if(root.u32(0x238)!=27722)return reject("Cairo original syntax allocation extent differs");
        for(const auto& restore:final_row_restores) {
            const auto& before=restore.before;const auto& after=restore.after;
            if(before.datum!=after.datum || after.salt!=after.datum>>16 || !matches_row(before))
                return reject("Cairo original holder typed restoration differs");
            if(restore.allocated) {
                while(free_cursor<nodes.size()/20 && ns.u16(free_cursor*20))++free_cursor;
                // Original 16E4B0/16E510 use the actual "script node" name;
                // 16E6A0 allocates the first free index and advances its salt.
                if(before.salt || free_cursor!=unsigned(after.datum&0xffffu) ||
                    after.salt!=allocation_salt++ || !allocated_rows.insert(after.datum).second)
                    return reject("Cairo original allocator slot or salted handle differs");
                ++free_cursor;
            } else row(before.datum);
        }
        if(allocated_rows.size()!=8)return reject("Cairo original flush allocation census differs");
        for(const auto& owner:timing_incoming) {
            row(owner.datum);timing_rows.insert(owner.datum);
            if(!unique_incoming(owner.datum,owner.from,owner.child))
                return reject("Cairo original timing closure has another incoming owner");
        }
        for(const auto& holder:final_holders) {
            const auto at=holder.index*40;
            if(defs.string(at,32)!=holder.name || defs.u16(at+32)!=holder.type ||
                defs.u16(at+34)!=holder.return_type || defs.u32(at+36)!=holder.root)
                return reject("Cairo original holder declaration differs");
            row(holder.root);
        }
        for(unsigned at=0;at<scripts.size();at+=40)
            if(allocated_rows.contains(defs.u32(at+36)) || timing_rows.contains(defs.u32(at+36)))
                return reject("Cairo restored row has an additional script declaration owner");
        for(unsigned at=0;at<globals.size();at+=40)
            if(allocated_rows.contains(globals.u32(at+36)) || timing_rows.contains(globals.u32(at+36)))
                return reject("Cairo restored row has an additional global declaration owner");
        for(const auto& debug:preserved_string_debug_rows)if(!matches_row(debug))
            return reject("Cairo preserved expression or numeric debug row differs");
        for(const auto& slot:final_function_slots) {
            const auto fn=row(slot.before_owner);
            if(ns.u16(fn+4)!=2 || ns.u16(fn+6)!=9 || ns.u32(fn+12)!=slot.offset ||
                text(strings,slot.offset)!=slot.before_name ||
                std::string_view(slot.before_name).size()+1!=slot.size ||
                std::string_view(slot.after_name).size()+1>slot.size)
                return reject("Cairo original flush function name slot differs");
            for(unsigned i=0;i<nodes.size()/20;++i) {
                const auto at=i*20;const auto flags=ns.u16(at+6),op=ns.u16(at+2);
                if(!ns.u16(at) || flags==0xbaba)continue;
                const auto datum=(unsigned(ns.u16(at))<<16)|i;
                const auto inside=[&](unsigned value){return value>=slot.offset && value-slot.offset<slot.size;};
                if(inside(ns.u32(at+12)) && datum!=slot.before_owner) {
                    const bool ignored=(!(flags&1) || ((flags&1) && !(flags&4) && op>=4 && op<=8));
                    const bool proved=std::ranges::any_of(preserved_string_debug_rows,
                        [&](const auto& debug){return debug.datum==datum;});
                    // Original 211D48 consumes numeric +10; expression dispatch
                    // 211D60 never consumes +C. Preserve these offsets exactly.
                    if(!ignored || !proved)return reject("Cairo function slot has another meaningful string owner");
                }
                if((flags&1) && !(flags&4) && op==9 && inside(ns.u32(at+16)))
                    return reject("Cairo function slot has a primitive STRING owner");
            }
        }
        for(unsigned i=0;i<nodes.size()/20;++i) {
            const auto at=i*20;const auto flags=ns.u16(at+6);
            if(!ns.u16(at) || flags==0xbaba)continue;
            if(allocated_rows.contains(ns.u32(at+8)) ||
                (!(flags&1) && allocated_rows.contains(ns.u32(at+16))))
                return reject("Cairo new salted syntax handle already has an incoming owner");
        }
        for(const auto& link:flush_links) {
            const auto at=row(link.datum);
            const auto holder=std::ranges::find_if(final_holders,[&](const auto& h){
                return ns.u32(row(h.root)+16)==link.datum;});
            if(holder==final_holders.end() || ns.u16(at+2)!=0 || ns.u16(at+4)!=2 ||
                ns.u16(at+6)!=9 || text(strings,ns.u32(at+12))!="begin" ||
                ns.u32(at+8)!=link.before || !unique_incoming(link.datum,holder->root,true))
                return reject("Cairo original leading flush holder boundary differs");
        }
        for(const auto& flush:flush_calls) {
            const auto signature=mcc_script_builtin_signature(flush.opcode);
            if(!signature || signature->name!=flush.name || signature->declared_return_type!=4 ||
                signature->xbox_opcode!=flush.xbox_opcode || signature->xbox_flags ||
                !signature->argument_types.empty())
                return reject("Cairo original null flush builtin signature differs");
        }
        const auto special_signature=[&](unsigned op,std::string_view name,unsigned result,unsigned xbox,
            unsigned flags,std::span<const std::uint16_t> arguments) {
            const auto signature=mcc_script_builtin_signature(op);
            return signature && signature->name==name && signature->declared_return_type==result &&
                signature->xbox_opcode==xbox && signature->xbox_flags==flags &&
                std::ranges::equal(signature->argument_types,arguments);
        };
        constexpr std::array<std::uint16_t,1> sound_argument{32},device_argument{54},string_argument{9};
        const auto* dialog=cache.find_tag(TagHandle{2230985407u});const Bytes named_timing(banks.at("block_72"));
        if(!special_signature(19,"sleep",4,19,2,{}) || !special_signature(21,"sleep_until",4,21,2,{}) ||
            !special_signature(16,"<",5,16,2,{}) ||
            !special_signature(604,"sound_impulse_language_time",8,602,0,sound_argument) ||
            !special_signature(258,"device_get_position",6,257,0,device_argument) ||
            !special_signature(27,"print",4,27,0,string_argument) ||
            !dialog || dialog->group!="snd!" || dialog->name!="sound\\dialog\\levels\\01_spacestation\\mission\\l01_0130_mm1" ||
            text(strings,33264)!=dialog->name || text(strings,110)!="sleep" ||
            text(strings,31294)!="sound_impulse_language_time" || text(strings,31229)!="klang!" ||
            named_timing.string(472*36,32)!="1st_craft" || named_timing.u16(472*36+32)!=7 ||
            named_timing.u16(472*36+34)!=139 || !unique_incoming(2625517834u,2625452297u,false) ||
            !unique_incoming(2625452297u,2625386760u,true))
            return reject("Cairo original dynamic duration or retained object/string namespace differs");
        std::set<unsigned> sound_retired;
        for(const auto& expected:sound_rows) {
            const auto at=row(expected.datum);
            if(ns.u16(at)!=expected.salt || ns.u16(at+2)!=expected.opcode ||
                ns.u16(at+4)!=expected.type || ns.u16(at+6)!=expected.flags ||
                ns.u32(at+8)!=expected.next || ns.u32(at+12)!=expected.label ||
                ns.u32(at+16)!=expected.value || !sound_retired.insert(expected.datum).second)
                return reject("Cairo removed sound typed row differs");
        }
        const Bytes sound_objects(banks.at("block_72")),sound_squads(banks.at("block_352"));
        if(root.u32(0x48)!=486 || root.u32(0x4c)!=228112 ||
            root.u32(0x160)!=117 || root.u32(0x164)!=361360 ||
            sound_objects.string(428*36,32)!="bay1_door_exit" ||
            sound_objects.u16(428*36+32)!=7 || sound_objects.u16(428*36+34)!=11 ||
            sound_squads.string(0,32)!="1st_hum_johnson")
            return reject("Cairo removed sound object or AI owner namespace differs");
        for(const auto& sound:sound_removals) {
            const auto call=row(sound.root),fn=row(sound.function),tag_row=row(sound.sound),
                emitter=row(sound.emitter),volume=row(sound.volume);
            const auto signature=mcc_script_builtin_signature(sound.opcode);
            const std::array<std::uint16_t,3> arguments{std::uint16_t(sound.sound_type),50,6};
            const auto* tag=cache.find_tag(TagHandle{sound.handle});
            if(ns.u16(call+2)!=sound.opcode || ns.u16(call+4)!=4 || ns.u16(call+6)!=8 ||
                ns.u32(call+16)!=sound.function || ns.u16(fn+2)!=sound.opcode ||
                ns.u16(fn+4)!=2 || ns.u16(fn+6)!=9 || ns.u32(fn+8)!=sound.sound ||
                text(strings,ns.u32(fn+12))!=sound.name || !signature ||
                signature->name!=sound.name || signature->declared_return_type!=4 ||
                signature->xbox_opcode!=sound.xbox_opcode ||
                !std::ranges::equal(signature->argument_types,arguments) ||
                ns.u16(tag_row+2)!=sound.sound_type || ns.u16(tag_row+4)!=sound.sound_type ||
                ns.u16(tag_row+6)!=9 || ns.u32(tag_row+16)!=sound.handle ||
                !tag || tag->name!=sound.path || tag->group!=(sound.sound_type==32?"snd!":"lsnd") ||
                text(strings,ns.u32(tag_row+12))!=sound.path ||
                ns.u32(tag_row+8)!=sound.emitter || ns.u32(emitter+8)!=sound.volume ||
                ns.u16(volume+2)!=6 || ns.u16(volume+4)!=6 || ns.u16(volume+6)!=9 ||
                ns.u32(volume+16)!=0x3f800000 || ns.u32(volume+8)!=0xffffffffu ||
                !unique_incoming(sound.root,sound.predecessor,false) ||
                !unique_incoming(sound.function,sound.root,true) ||
                !unique_incoming(sound.sound,sound.function,false) ||
                !unique_incoming(sound.emitter,sound.sound,false) ||
                !unique_incoming(sound.volume,sound.emitter,false))
                return reject("Cairo removed sound signature, salted tag or closure owner differs");
            if(sound.ai_function) {
                const auto ai_fn=row(sound.ai_function),ai=row(sound.ai_literal);
                const auto ai_signature=mcc_script_builtin_signature(285);
                constexpr std::array<std::uint16_t,1> ai_arguments{19};
                if(ns.u16(emitter+2)!=285 || ns.u16(emitter+4)!=50 || ns.u16(emitter+6)!=8 ||
                    ns.u32(emitter+16)!=sound.ai_function || ns.u16(ai_fn+2)!=285 ||
                    ns.u16(ai_fn+4)!=2 || ns.u16(ai_fn+6)!=9 ||
                    text(strings,ns.u32(ai_fn+12))!="ai_get_object" || ns.u32(ai_fn+8)!=sound.ai_literal ||
                    !ai_signature || ai_signature->name!="ai_get_object" ||
                    ai_signature->declared_return_type!=50 || ai_signature->xbox_opcode!=284 ||
                    !std::ranges::equal(ai_signature->argument_types,ai_arguments) ||
                    ns.u16(ai+2)!=19 || ns.u16(ai+4)!=19 || ns.u16(ai+6)!=9 ||
                    ns.u32(ai+16)!=0 || text(strings,ns.u32(ai+12))!="1st_hum_johnson" ||
                    ns.u32(ai+8)!=0xffffffffu ||
                    !unique_incoming(sound.ai_function,sound.emitter,true) ||
                    !unique_incoming(sound.ai_literal,sound.ai_function,false))
                    return reject("Cairo removed sound AI emitter differs");
            } else if(ns.u16(emitter+2)==56) {
                if(ns.u16(emitter+4)!=50 || ns.u16(emitter+6)!=9 ||
                    ns.u32(emitter+16)!=0xffff01acu ||
                    text(strings,ns.u32(emitter+12))!="bay1_door_exit")
                    return reject("Cairo removed sound named emitter differs");
            } else if(ns.u16(emitter+2)!=50 || ns.u16(emitter+4)!=50 || ns.u16(emitter+6)!=9 ||
                ns.u32(emitter+16)!=0xffffffffu || text(strings,ns.u32(emitter+12))!="none")
                return reject("Cairo removed sound null emitter differs");
        }
        for(unsigned at=0;at<scripts.size();at+=40)if(sound_retired.contains(defs.u32(at+36)))
            return reject("Cairo removed sound has a script declaration owner");
        for(unsigned at=0;at<globals.size();at+=40)if(sound_retired.contains(globals.u32(at+36)))
            return reject("Cairo removed sound has a global declaration owner");
        if(!unique_incoming(clock_initializer.function,clock_initializer.root,true))
            return reject("Cairo clock initializer has another incoming owner");
        for(const auto& restore:clock_literal_restores) {
            const auto at=row(restore.datum),parent=row(restore.parent),other=row(restore.other);
            const auto fn=row(ns.u32(parent+16));const auto signature=mcc_script_builtin_signature(ns.u16(parent+2));
            if(ns.u16(at+2)!=6 || ns.u16(at+4)!=6 || ns.u16(at+6)!=9 ||
                ns.u32(at+16)!=0x41f00000u || ns.u32(at+8)!=0xffffffffu ||
                ns.u16(parent+2)!=9 || ns.u16(parent+4)!=restore.context || ns.u16(parent+6)!=8 ||
                !signature || signature->name!="*" || signature->declared_return_type!=6 || signature->xbox_opcode!=9 ||
                ns.u16(fn+2)!=9 || ns.u16(fn+4)!=2 || ns.u16(fn+6)!=9 ||
                text(strings,ns.u32(fn+12))!="*" || ns.u32(fn+8)!=restore.other ||
                ns.u16(other+2)!=6 || ns.u16(other+4)!=6 || ns.u16(other+6)!=9 ||
                ns.u32(other+16)!=restore.other_value || ns.u32(other+8)!=restore.datum ||
                !unique_incoming(restore.datum,restore.other,false) ||
                !unique_incoming(restore.other,ns.u32(parent+16),false) ||
                !unique_incoming(ns.u32(parent+16),restore.parent,true))
                return reject("Cairo original mutable seconds parent differs");
        }
        const auto brand_parent=row(brand_restore.parent),brand_function=row(brand_restore.function),
            brand_effect=row(brand_restore.effect),brand_object=row(brand_restore.object),brand_marker=row(brand_restore.marker);
        constexpr std::array<std::uint16_t,3> effect_arguments{33,50,11};
        const auto effect_signature=mcc_script_builtin_signature(ns.u16(brand_parent+2));
        const auto* effect=cache.find_tag(TagHandle{ns.u32(brand_effect+16)});
        const Bytes named(banks.at("block_72"));
        if(ns.u16(brand_parent+2)!=41 || ns.u16(brand_parent+4)!=4 || ns.u16(brand_parent+6)!=8 ||
            ns.u32(brand_parent+16)!=brand_restore.function || !effect_signature ||
            effect_signature->name!="effect_new_on_object_marker" || effect_signature->declared_return_type!=4 ||
            effect_signature->xbox_opcode!=41 || !std::ranges::equal(effect_signature->argument_types,effect_arguments) ||
            ns.u16(brand_function+2)!=41 || ns.u16(brand_function+4)!=2 || ns.u16(brand_function+6)!=9 ||
            text(strings,ns.u32(brand_function+12))!="effect_new_on_object_marker" || ns.u32(brand_function+8)!=brand_restore.effect ||
            ns.u16(brand_effect+2)!=33 || ns.u16(brand_effect+4)!=33 || ns.u16(brand_effect+6)!=9 ||
            ns.u32(brand_effect+8)!=brand_restore.object || !effect || effect->group!="effe" || effect->name!=brand_restore.effect_path ||
            ns.u16(brand_object+2)!=56 || ns.u16(brand_object+4)!=50 || ns.u16(brand_object+6)!=9 ||
            ns.u32(brand_object+16)!=0xffff0019u || ns.u32(brand_object+12)!=13938 || ns.u32(brand_object+8)!=brand_restore.marker ||
            ns.u16(brand_marker+2)!=11 || ns.u16(brand_marker+4)!=11 || ns.u16(brand_marker+6)!=9 ||
            ns.u32(brand_marker+16)!=83888633 || ns.u32(brand_marker+12)!=13955 || ns.u32(brand_marker+8)!=0xffffffffu ||
            cache.string_id(ns.u32(brand_marker+16))!="smoke" || !cache.string_id(0).empty() ||
            named.string(24*36,32)!="branding_iron_01" || named.u16(24*36+32)!=6 || named.u16(24*36+34)!=19 ||
            named.string(25*36,32)!="branding_iron_02" || named.u16(25*36+32)!=6 || named.u16(25*36+34)!=20 ||
            text(strings,13938)!="branding_iron_02" || text(strings,13979)!="branding_iron_01" ||
            text(strings,13955)!="smoke" || !text(strings,0).empty() ||
            !unique_incoming(brand_restore.function,brand_restore.parent,true) ||
            !unique_incoming(brand_restore.effect,brand_restore.function,false) ||
            !unique_incoming(brand_restore.object,brand_restore.effect,false) ||
            !unique_incoming(brand_restore.marker,brand_restore.object,false))
            return reject("Cairo branding effect literal namespace differs");
        const Bytes placed(read(root.u32(0x54)+19*0x5c,0x5c));
        const Bytes palette(read(root.u32(0x5c)+placed.u16(0)*0x28,0x28));
        const auto* object=cache.find_tag(TagHandle{palette.u32(4)});
        const Bytes object_header(banks.at("branding_object_header")),model_header(banks.at("branding_model_header")),
            render_header(banks.at("branding_render_header")),model_nodes(banks.at("branding_nodes"));
        const auto* model=cache.find_tag(TagHandle{object_header.u32(0x38)});
        const auto* render=cache.find_tag(TagHandle{model_header.u32(4)});
        const auto token_for=[&](std::string_view name){return std::ranges::find_if(guard_banks,
            [&](const auto& bank){return bank.name==name;})->token;};
        if(placed.u16(2)!=24 || !object || object->group!="scen" || object->name!=brand_restore.object_path ||
            object->address!=token_for("branding_object_header") ||
            !model || model->group!="hlmt" || model->name!=brand_restore.object_path ||
            model->address!=token_for("branding_model_header") ||
            !render || render->group!="mode" || render->name!=brand_restore.object_path ||
            render->address!=token_for("branding_render_header") ||
            render_header.u32(0x48)!=1 || render_header.u32(0x58)!=2 ||
            cache.string_id(model_nodes.u32(0))!="branding_iron" || model_nodes.u16(4)!=0xffff)
            return reject("Cairo branding object/model namespace differs");
        std::set<unsigned> retired(authoring_retired.begin(),authoring_retired.end());
        for(const auto datum:retired)row(datum);
        const auto removed_global_root=row(removed_final_global.root);
        const auto removed_global_at=removed_final_global.index*40;
        if(root.u32(0x1c0)!=removed_final_global.index+1 ||
            globals.string(removed_global_at,32)!=removed_final_global.name ||
            globals.u16(removed_global_at+32)!=removed_final_global.type ||
            globals.u16(removed_global_at+34)!=removed_final_global.kind ||
            globals.u32(removed_global_at+36)!=removed_final_global.root ||
            ns.u16(removed_global_root+2)!=7 || ns.u16(removed_global_root+4)!=7 ||
            ns.u16(removed_global_root+6)!=9 || ns.u32(removed_global_root+8)!=0xffffffffu ||
            ns.u32(removed_global_root+16)!=0xffff0000u || retired.contains(removed_final_global.root))
            return reject("Cairo final unused global declaration differs");
        std::set<unsigned> removed_global_reference_set;
        for(const auto& reference:removed_global_references) {
            const auto at=row(reference.datum);
            if(!retired.contains(reference.datum) || ns.u16(at+2)!=reference.opcode ||
                ns.u16(at+4)!=7 || ns.u16(at+6)!=13 || ns.u32(at+16)!=removed_final_global.index ||
                text(strings,ns.u32(at+12))!=removed_final_global.name)
                return reject("Cairo final unused global reference differs");
            removed_global_reference_set.insert(reference.datum);
        }
        // Full raw-bank fingerprints are additionally checked above; explicit
        // incoming-edge checks document and enforce the retirement boundary.
        for(unsigned i=0;i<nodes.size()/20;++i) {
            const auto at=i*20;const auto salt=ns.u16(at),flags=ns.u16(at+6);
            if(!salt || flags==0xbaba)continue;
            const unsigned datum=(unsigned(salt)<<16)|i;
            if((flags&4) && (ns.u32(at+16)&0xffffu)==removed_final_global.index &&
                !removed_global_reference_set.contains(datum))
                return reject("Cairo final unused global has another typed reference");
            if(retired.contains(datum))continue;
            if(ns.u32(at+8)==removed_final_global.root ||
                (!(flags&1) && ns.u32(at+16)==removed_final_global.root))
                return reject("Cairo orphan global initializer has a retained incoming expression");
            if(retired.contains(ns.u32(at+8))) {
                const auto edge=std::find_if(authoring_patches.begin(),authoring_patches.end(),
                    [&](const auto&p){return p.predecessor==datum && p.removed==ns.u32(at+8);});
                const bool restored=std::ranges::any_of(final_row_restores,[&](const auto& r){
                    return !r.allocated && r.before.datum==datum && r.before.next==ns.u32(at+8) &&
                        r.after.next!=r.before.next;});
                if(edge==authoring_patches.end() && !restored)return reject("Cairo retired expression has a custom incoming link");
            }
            if(!(flags&1) && retired.contains(ns.u32(at+16)) &&
                !(datum==clock_initializer.root && ns.u32(at+16)==clock_initializer.function) &&
                !std::ranges::any_of(final_row_restores,[&](const auto& r){
                    return !r.allocated && r.before.datum==datum && r.before.value==ns.u32(at+16) &&
                        ((r.after.flags&1) || r.after.value!=r.before.value);}))
                return reject("Cairo retired expression has a custom parent");
        }
        if(root.u32(0x1b8)!=420)return reject("Cairo script declaration count differs");
        const auto removed_script=[&](unsigned index) {
            return std::ranges::any_of(removed_scripts,[&](const auto& d){return d.index==index;});
        };
        const auto new_script_index=[&](unsigned index) {
            return index-std::ranges::count_if(removed_scripts,[&](const auto& d){return d.index<index;});
        };
        for(const auto& declaration:removed_scripts) {
            const auto at=declaration.index*40;
            if(defs.string(at,32)!=declaration.name || defs.u16(at+32)!=declaration.type ||
                defs.u16(at+34)!=declaration.return_type || defs.u32(at+36)!=declaration.root)
                return reject("Cairo added declaration differs");
        }
        for(unsigned i=0;i<420;++i)if(!removed_script(i) && retired.contains(defs.u32(i*40+36)))
            return reject("Cairo retired expression has another script root");
        for(unsigned i=0;i<420;++i)if(defs.u32(i*40+36)==clock_initializer.root)
            return reject("Cairo seconds initializer has a script root owner");
        for(unsigned i=0;i<420;++i)if(!removed_script(i) && defs.u32(i*40+36)==removed_final_global.root)
            return reject("Cairo orphan global initializer has a retained script root owner");
        for(unsigned at=0;at<globals.size();at+=40)if(retired.contains(globals.u32(at+36)))
            return reject("Cairo retired expression has a global root");
        for(unsigned at=0;at<globals.size();at+=40)if(globals.u32(at+36)==clock_initializer.root && at!=clock_initializer.index*40)
            return reject("Cairo seconds initializer has another global root owner");
        for(unsigned at=0;at<globals.size();at+=40)if(globals.u32(at+36)==removed_final_global.root && at!=removed_global_at)
            return reject("Cairo orphan global initializer has another global root owner");
        std::set<unsigned> verified_references;
        for(const auto& reference:script_references) {
            const auto at=row(reference.datum);const auto flags=ns.u16(at+6);
            if(defs.string(reference.index*40,32)!=reference.name)
                return reject("Cairo script ordinal namespace differs");
            if(removed_script(reference.index) && !retired.contains(reference.datum))
                return reject("Cairo removed declaration has a retained incoming reference");
            if(!reference.kind) {
                const auto fn=row(reference.function);
                if((flags&3)!=2 || ns.u16(at+2)!=reference.index || ns.u32(at+16)!=reference.function ||
                    ns.u16(fn+4)!=2 || ns.u16(fn+2)!=reference.index ||
                    text(strings,ns.u32(fn+12))!=reference.name)
                    return reject("Cairo static script reference differs");
            } else if(!(flags&1) || (flags&4) || ns.u16(at+4)==2 ||
                ns.u16(at+2)!=reference.kind || (ns.u32(at+16)&0xffffu)!=reference.index ||
                text(strings,ns.u32(at+12))!=reference.name)
                return reject("Cairo typed script literal differs");
            verified_references.insert(reference.datum);
        }
        for(unsigned i=0;i<nodes.size()/20;++i) {
            const auto at=i*20;const auto flags=ns.u16(at+6),op=ns.u16(at+2),type=ns.u16(at+4);
            if(!ns.u16(at) || flags==0xbaba)continue;
            const bool reference=(!(flags&1) && (flags&2)) ||
                ((flags&1) && !(flags&4) && type!=2 && (op==10 || op==21 || type==10 || type==21));
            if(reference && !verified_references.contains((unsigned(ns.u16(at))<<16)|i))
                return reject("Cairo has an additional script ordinal reference");
        }
        const auto add=[&](unsigned address,std::span<const std::byte> before,std::vector<std::byte> after) {
            if(!std::ranges::equal(before,after))result.writes.push_back({address,
                {before.begin(),before.end()},std::move(after)});
        };
        std::vector<std::byte> vanilla(nodes.begin(),nodes.end());
        for(const auto& patch:authoring_patches) {
            const auto at=row(patch.predecessor);
            if(ns.u32(at+8)!=patch.removed || ns.u32(row(patch.last)+8)!=patch.next)
                return reject("Cairo authored predecessor/successor differs");
            put(vanilla,at+8,patch.next);
        }
        for(const auto datum:retired) {
            const auto at=row(datum);std::fill_n(vanilla.begin()+at,20,std::byte{0xba});
            put(vanilla,at,0,2);
        }
        put(vanilla,clock_root+2,7,2);put(vanilla,clock_root+6,9,2);put(vanilla,clock_root+16,0xffff001e);
        for(const auto datum:clock_references)put(vanilla,row(datum)+12,clock_initializer.label);
        for(const auto& restore:clock_literal_restores) {
            const auto at=row(restore.datum);put(vanilla,at+6,13,2);
            put(vanilla,at+12,clock_initializer.label);put(vanilla,at+16,clock_initializer.index);
        }
        put(vanilla,brand_object+12,13979);put(vanilla,brand_object+16,0xffff0018u);
        put(vanilla,brand_marker+12,0);put(vanilla,brand_marker+16,0);
        for(const auto& reference:script_references) {
            if(retired.contains(reference.datum))continue;
            const auto new_index=new_script_index(reference.index);
            if(new_index==reference.index)continue;
            const auto at=row(reference.datum);
            if(!reference.kind) {
                put(vanilla,at+2,new_index,2);
                put(vanilla,row(reference.function)+2,new_index,2);
                result.script_ordinal_words+=2;
            } else {
                put(vanilla,at+16,new_index,2);++result.script_ordinal_words;
            }
        }
        for(const auto& restore:literal_restores) {
            const auto at=row(restore.datum),parent=row(restore.parent);
            const auto signature=mcc_script_builtin_signature(ns.u16(parent+2));
            constexpr std::array<std::uint16_t,6> arguments{6,6,6,6,8,6};
            if(ns.u16(at+2)!=8 || ns.u16(at+4)!=8 || ns.u16(at+6)!=9 ||
                ns.u32(at+16)!=restore.before || !signature ||
                signature->name!="set_global_sound_environment" ||
                signature->declared_return_type!=4 || signature->xbox_opcode!=878 ||
                !std::ranges::equal(signature->argument_types,arguments))
                return reject("Cairo environment literal signature differs");
            const auto fn=row(ns.u32(parent+16));
            if(text(strings,ns.u32(fn+12))!="set_global_sound_environment")
                return reject("Cairo environment literal parent name differs");
            auto argument=ns.u32(fn+8);
            for(unsigned i=0;i<restore.argument;++i)argument=ns.u32(row(argument)+8);
            if(argument!=restore.datum)return reject("Cairo environment literal argument differs");
            put(vanilla,at+16,restore.after);
        }
        for(const auto& restore:final_row_restores) {
            const auto& r=restore.after;const auto at=std::size_t(r.datum&0xffffu)*20;
            put(vanilla,at,r.salt,2);put(vanilla,at+2,r.opcode,2);put(vanilla,at+4,r.type,2);
            put(vanilla,at+6,r.flags,2);put(vanilla,at+8,r.next);put(vanilla,at+12,r.label);put(vanilla,at+16,r.value);
        }
        for(const auto& link:flush_links)put(vanilla,row(link.datum)+8,link.after);
        if(!matches_digest(vanilla,final_nodes_postimage))
            return reject("Cairo complete original holder syntax postimage differs");
        add(root.u32(0x23c),nodes,std::move(vanilla));
        std::vector<std::byte> final_strings(strings.begin(),strings.end());
        for(const auto& slot:final_function_slots) {
            std::fill_n(final_strings.begin()+slot.offset,slot.size,std::byte{});
            const std::string_view name(slot.after_name);
            std::copy_n(reinterpret_cast<const std::byte*>(name.data()),name.size(),final_strings.begin()+slot.offset);
        }
        if(!matches_digest(final_strings,final_strings_postimage))
            return reject("Cairo complete original holder string postimage differs");
        add(root.u32(0x1b4),strings,std::move(final_strings));
        std::vector<std::byte> compacted(scripts.begin(),scripts.end());
        unsigned output=0;
        for(unsigned index=0;index<420;++index)if(!removed_script(index)) {
            std::copy_n(scripts.begin()+index*40,40,compacted.begin()+output*40);++output;
        }
        // Records beyond the new count retain their raw bytes.
        add(root.u32(0x1bc),scripts,std::move(compacted));
        const auto global_bytes=banks.at("block_448");std::vector<std::byte> vanilla_globals(global_bytes.begin(),global_bytes.end());
        const auto global_name=clock_initializer.index*40;std::fill_n(vanilla_globals.begin()+global_name,32,std::byte{});
        constexpr std::string_view seconds="seconds";std::copy_n(reinterpret_cast<const std::byte*>(seconds.data()),seconds.size(),vanilla_globals.begin()+global_name);
        add(root.u32(0x1c4),banks.at("block_448"),std::move(vanilla_globals));
        auto count=read(cairo_scenario_address+0x1b8,4);std::vector<std::byte> new_count(count.begin(),count.end());
        put(new_count,0,418);add(cairo_scenario_address+0x1b8,count,std::move(new_count));
        const auto global_count=read(cairo_scenario_address+0x1c0,4);std::vector<std::byte> new_global_count(global_count.begin(),global_count.end());
        put(new_global_count,0,removed_final_global.index);
        add(cairo_scenario_address+0x1c0,global_count,std::move(new_global_count));
        for(const auto& field:scenario_ordinals) {
            const auto old=read(field.token,2);
            if(Bytes(old).u16(0)!=field.index || defs.string(field.index*40,32)!=field.name)
                return reject("Cairo scenario script ordinal namespace differs");
            std::vector<std::byte> changed(old.begin(),old.end());put(changed,0,new_script_index(field.index),2);
            add(field.token,old,std::move(changed));++result.scenario_ordinal_words;
        }
        if(result.script_ordinal_words!=374 || result.scenario_ordinal_words!=17)
            return reject("Cairo script ordinal rewrite census differs");
        result.retired_nodes=retired.size();result.matched=true;return result;
    } catch(const FormatError& error) {return reject(error.what());}
}
} // namespace halo2
