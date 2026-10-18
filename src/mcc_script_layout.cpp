#include "halo2/mcc_script_layout.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <array>
#include <set>

namespace halo2 {
namespace {
struct Builtin {
    std::uint16_t mcc,xbox,return_type,arity,xbox_flags;
    const char* name;
    std::array<std::uint16_t,7> arguments;
};
struct ExternalGlobal {std::uint16_t mcc,xbox,type;const char* name;};
struct SemanticAlias {std::uint16_t mcc,xbox;const char* name;const char* canonical;};
#include "mcc_script_layout.inc"
const Builtin* builtin(std::uint16_t opcode) {
    const auto at=std::lower_bound(builtins.begin(),builtins.end(),opcode,
        [](const auto& row,auto value){return row.mcc<value;});
    return at!=builtins.end() && at->mcc==opcode?&*at:nullptr;
}
std::string_view string_at(std::span<const std::byte> strings,std::uint32_t offset) {
    if(offset>=strings.size())throw FormatError("MCC script name escapes string bank");
    const auto first=strings.subspan(offset);
    const auto end=std::find(first.begin(),first.end(),std::byte{});
    if(end==first.end())throw FormatError("MCC script name is unterminated");
    return {reinterpret_cast<const char*>(first.data()),std::size_t(end-first.begin())};
}
void word(std::span<std::byte> out,std::size_t offset,std::uint16_t value) {
    out[offset]=std::byte(value);out[offset+1]=std::byte(value>>8);
}
struct CairoRow {std::uint32_t datum;std::array<std::byte,20> raw;};
struct CairoString {std::uint32_t offset;const char* text;};
enum class CairoBindingKind {Tag,StringId,Object,Flag,Global,External};
struct CairoBinding {
    std::uint32_t datum,value;CairoBindingKind kind;const char* name;const char* group;
    std::uint16_t type,extra;
};
struct CairoStatic {std::uint16_t index;const char* name;};
struct CairoPatch {std::uint32_t predecessor,removed_call,next;};
struct CairoSubtree {
    const char* name;std::uint16_t type,return_type;std::uint32_t root;
    std::span<const CairoRow> rows;std::span<const CairoString> strings;
    std::span<const CairoBinding> bindings;std::span<const CairoStatic> statics;
    std::span<const CairoPatch> patches;std::span<const std::uint32_t> retired;
};
#include "mcc_cairo_vanilla_scripts.inc"
bool cairo_matches(const Cache& cache,const Bytes& scenario,const CairoSubtree& profile,
    std::span<const std::byte> nodes,std::span<const std::byte> strings,
    std::span<const std::byte> scripts) {
    try {
        bool definition=false;
        for(std::size_t at=0;at<scripts.size();at+=0x28) {
            const Bytes script(scripts.subspan(at,0x28));
            if(script.string(0,32)!=profile.name)continue;
            if(definition || script.u16(32)!=profile.type || script.u16(34)!=profile.return_type ||
               script.u32(36)!=profile.root)return false;
            definition=true;
        }
        if(!definition)return false;
        for(const auto& row:profile.rows) {
            const auto offset=std::size_t(row.datum&0xffffu)*20;
            if(offset>nodes.size() || nodes.size()-offset<20 ||
               !std::ranges::equal(row.raw,nodes.subspan(offset,20)))return false;
        }
        for(const auto& text:profile.strings)
            if(string_at(strings,text.offset)!=text.text)return false;
        for(const auto& call:profile.statics) {
            const auto offset=std::size_t(call.index)*0x28;
            if(offset>scripts.size() || scripts.size()-offset<0x28 ||
               Bytes(scripts.subspan(offset,0x28)).string(0,32)!=call.name)return false;
        }
        const auto block=[&](unsigned offset,std::uint32_t index,unsigned stride) {
            if(index>=scenario.u32(offset))throw FormatError("Cairo binding escapes authored namespace");
            const auto first=scenario.u32(offset+4);
            if(std::uint64_t(first)+std::uint64_t(index)*stride>0xffffffffu)
                throw FormatError("Cairo authored namespace address overflows");
            return cache.read_tag_bytes(first+index*stride,stride);
        };
        for(const auto& binding:profile.bindings) {
            switch(binding.kind) {
            case CairoBindingKind::Tag: {
                const auto* tag=cache.find_tag(TagHandle{binding.value});
                if(!tag || tag->group!=binding.group || tag->name!=binding.name)return false;
                break;
            }
            case CairoBindingKind::StringId:
                if(cache.string_id(binding.value)!=binding.name)return false;
                break;
            case CairoBindingKind::Object: {
                const Bytes named(block(0x48,binding.value&0xffffu,0x24));
                if(named.string(0,32)!=binding.name || named.u16(32)!=binding.type ||
                   named.u16(34)!=binding.extra)return false;
                break;
            }
            case CairoBindingKind::Flag:
                if(Bytes(block(0x1e0,binding.value&0xffffu,0x38)).string(4,32)!=binding.name)return false;
                break;
            case CairoBindingKind::Global: {
                const Bytes global(block(0x1c0,binding.value&0xffffu,0x28));
                if(global.string(0,32)!=binding.name || global.u16(32)!=binding.type)return false;
                break;
            }
            case CairoBindingKind::External: {
                const auto entry=std::find_if(external_globals.begin(),external_globals.end(),
                    [&](const auto& g){return g.mcc==binding.type && g.name==std::string_view(binding.name);});
                if(entry==external_globals.end() || (binding.value&0xffffu)!=(0x8000u|binding.type))return false;
                break;
            }
            }
        }
        const auto retired=[&](std::uint32_t datum) {
            return std::ranges::find(profile.retired,datum)!=profile.retired.end();
        };
        // Inspect the complete input bank, not only the guarded subtree. A
        // custom script/global which shares these rows prevents retirement.
        for(std::size_t index=0;index<nodes.size()/20;++index) {
            const Bytes row(nodes.subspan(index*20,20));
            if(!row.u16(0) || row.u16(6)==0xbaba)continue;
            const auto datum=(std::uint32_t(row.u16(0))<<16)|std::uint32_t(index);
            if(retired(datum))continue;
            if(retired(row.u32(8))) {
                const auto edge=std::find_if(profile.patches.begin(),profile.patches.end(),
                    [&](const auto& p){return p.predecessor==datum && p.removed_call==row.u32(8);});
                if(edge==profile.patches.end())return false;
            }
            if(!(row.u16(6)&1) && retired(row.u32(16)))return false;
        }
        for(std::size_t at=0;at<scripts.size();at+=0x28)
            if(retired(Bytes(scripts.subspan(at,0x28)).u32(36)))return false;
        for(std::uint32_t index=0;index<scenario.u32(0x1c0);++index)
            if(retired(Bytes(block(0x1c0,index,0x28)).u32(36)))return false;
        return true;
    } catch(const FormatError&) {return false;}
}
}
std::optional<MccScriptBuiltinSignature> mcc_script_builtin_signature(std::uint16_t opcode) {
    const auto* row=builtin(opcode);if(!row)return {};
    return MccScriptBuiltinSignature{row->mcc,row->return_type,row->name,
        std::span(row->arguments).first(row->arity),
        row->xbox==0xffffu?std::optional<std::uint16_t>{}:row->xbox,row->xbox_flags};
}
std::optional<std::string_view> mcc_script_projected_builtin_name(
    std::uint16_t opcode,std::string_view name) {
    for(const auto &alias:semantic_aliases)
        if(alias.xbox==opcode && name==alias.name)return alias.canonical;
    return {};
}
void MccScriptLayout::require_native_ready() const {
    if(!requirements.empty())throw FormatError("MCC script requires an explicit native owner: "+
        requirements.front().name+" at syntax datum "+std::to_string(requirements.front().node_datum));
}
MccScriptLayout project_mcc_script_layout(std::span<const std::byte> nodes,
    std::span<const std::byte> strings,std::span<const std::byte> scripts,MccScriptLayoutOptions options) {
    if(nodes.size()%20 || nodes.size()/20>0x10000 || scripts.size()%0x28)
        throw FormatError("MCC script record bank extent differs");
    MccScriptLayout result;result.nodes.assign(nodes.begin(),nodes.end());
    const auto count=nodes.size()/20;
    const auto resolve=[&](std::uint32_t datum) {
        const auto index=datum&0xffffu;
        if(datum==0xffffffffu || index>=count)throw FormatError("MCC script link escapes syntax bank");
        const auto row=nodes.subspan(index*20,20);const Bytes bytes(row);
        if(!bytes.u16(0) || bytes.u16(0)!=(datum>>16) || bytes.u16(6)==0xbaba)
            throw FormatError("MCC script link has a stale datum salt");
        return row;
    };
    for(std::size_t index=0;index<count;++index) {
        const auto row=nodes.subspan(index*20,20);const Bytes node(row);
        const auto salt=node.u16(0),flags=node.u16(6);if(!salt || flags==0xbaba)continue;
        const auto datum=(std::uint32_t(salt)<<16)|std::uint32_t(index);
        const auto requirement=[&](MccScriptLayoutNeed need,std::uint16_t donor,std::string_view name) {
            result.requirements.push_back({need,datum,donor,std::string(name)});
        };
        if(flags&4) {
            // Source211CF5 and2125BB consume exactly the low16-bit global
            // token. The high16 bits are retained compiler data.
            const auto value=node.u32(16);if(!(value&0x8000u))continue;
            if(!(flags&1))throw FormatError("MCC external global is not a primitive expression");
            ++result.external_globals;const auto donor=std::uint16_t(value&0x7fffu);
            const auto name=string_at(strings,node.u32(12));
            if(!options.convert_external_globals) {
                requirement(MccScriptLayoutNeed::ExternalGlobalsDisabled,donor,name);continue;
            }
            const auto entry=std::find_if(external_globals.begin(),external_globals.end(),
                [&](const auto& g){return g.mcc==donor;});
            if(entry==external_globals.end()) {
                requirement(MccScriptLayoutNeed::UnknownExternalGlobal,donor,name);continue;
            }
            // The declaration's exact type is pinned in the profile. Both
            // node.type and node.opcode can contain a requested coercion
            // (actual ai_current_actor references also use object_list31),
            // or opcodeFFFF. Source211D09 reads the declaration type instead.
            if(name!=entry->name) {
                requirement(MccScriptLayoutNeed::ExternalGlobalIdentity,donor,name);continue;
            }
            word(result.nodes,index*20+16,std::uint16_t(0x8000u|entry->xbox));
            ++result.converted_external_globals;continue;
        }
        if(flags&1)continue;
        const Bytes function(resolve(node.u32(16)));
        if(!(function.u16(6)&1) || function.u16(4)!=2)
            throw FormatError("MCC script call lacks a primitive function-name node");
        const auto name=string_at(strings,function.u32(12));
        unsigned arguments=0;std::set<std::uint32_t> visited;
        for(auto next=function.u32(8);next!=0xffffffffu;) {
            if(!visited.insert(next).second)throw FormatError("MCC script argument sibling cycle");
            const Bytes argument(resolve(next));++arguments;next=argument.u32(8);
        }
        if(flags&2) {
            ++result.static_calls;const auto script=node.u16(2);
            if(script>=scripts.size()/0x28 || Bytes(scripts.subspan(script*0x28,32)).string(0,32)!=name)
                throw FormatError("MCC static script call name/index differs");
            continue;
        }
        ++result.builtin_calls;const auto donor=node.u16(2);const auto* entry=builtin(donor);
        if(!entry) {requirement(MccScriptLayoutNeed::UnknownBuiltin,donor,name);continue;}
        if(name!=entry->name) {requirement(MccScriptLayoutNeed::BuiltinIdentity,donor,name);continue;}
        // Original flag2 identifies custom parsers (begin, if, arithmetic,
        // sleep, unit, etc.), which own their variable/optional arguments.
        if(!(entry->xbox_flags&2) && arguments!=entry->arity) {
            requirement(MccScriptLayoutNeed::ArgumentCount,donor,name);continue;
        }
        if(entry->xbox==0xffffu) {
            requirement(donor==0x38fu || donor==0x390u?MccScriptLayoutNeed::NativeClockGetter:
                MccScriptLayoutNeed::NativeBuiltin,donor,name);continue;
        }
        word(result.nodes,index*20+2,entry->xbox);++result.converted_calls;
        if(donor!=entry->xbox)++result.rewritten_calls;
    }
    return result;
}
MccScriptLayout project_mcc_script_layout(const Cache& cache,std::span<const std::byte> nodes,
    std::span<const std::byte> strings,std::span<const std::byte> scripts,MccScriptLayoutOptions options) {
    if(!cache.is_mcc())throw FormatError("MCC script projection requires an MCC donor cache");
    if(nodes.size()%20 || scripts.size()%0x28)
        throw FormatError("MCC script record bank extent differs");
    const auto* tag=cache.find_tag(cache.scenario_tag());
    if(!tag || tag->group!="scnr" || tag->name!=
        "scenarios\\solo\\01b_spacestation\\01b_spacestation")
        return project_mcc_script_layout(nodes,strings,scripts,options);
    const Bytes scenario(cache.read_tag_bytes(tag->address,0x240));
    std::vector<std::byte> vanilla(nodes.begin(),nodes.end());
    for(const auto& profile:cairo_subtrees) {
        if(!cairo_matches(cache,scenario,profile,nodes,strings,scripts))continue;
        for(const auto& patch:profile.patches) {
            const auto offset=std::size_t(patch.predecessor&0xffffu)*20+8;
            word(vanilla,offset,std::uint16_t(patch.next));
            word(vanilla,offset+2,std::uint16_t(patch.next>>16));
        }
        for(const auto datum:profile.retired)
            std::copy(cairo_retired_row.begin(),cairo_retired_row.end(),
                vanilla.begin()+std::size_t(datum&0xffffu)*20);
    }
    return project_mcc_script_layout(vanilla,strings,scripts,options);
}
} // namespace halo2
