#include "halo2/cache.hpp"
#include <algorithm>
#include <fstream>
#include <map>
#include <nlohmann/json.hpp>
#include <set>
#include <tuple>

namespace halo2 {
namespace {
using Json=nlohmann::json;
constexpr std::size_t maximum=128u*1024u*1024u;
std::vector<std::byte> read(const std::filesystem::path& path,std::size_t limit) {
    std::ifstream stream(path,std::ios::binary|std::ios::ate);
    if(!stream || stream.tellg()<0 || std::uint64_t(stream.tellg())>limit)throw FormatError("Invalid preserved native shader file");
    std::vector<std::byte> result(static_cast<std::size_t>(stream.tellg()));stream.seekg(0);
    if(!result.empty() && !stream.read(reinterpret_cast<char*>(result.data()),std::streamsize(result.size())))
        throw FormatError("Truncated preserved native shader file");
    return result;
}
void word(std::vector<std::byte>& bytes,std::size_t at,std::uint32_t value) {
    Bytes(bytes).require(at,4);for(unsigned i=0;i<4;++i)bytes[at+i]=std::byte(value>>(8*i));
}
std::uint64_t fnv(std::span<const std::byte> bytes) {
    std::uint64_t result=14695981039346656037ull;
    for(auto b:bytes)result=(result^std::to_integer<unsigned>(b))*1099511628211ull;
    return result;
}
struct CloneNode {unsigned first,count;};
struct CloneField {unsigned offset,stride,child;bool data;};
#include "mcc_globals_clone_layout.inc"
template<class Read>
std::vector<std::byte> clone_globals(std::span<const std::byte> original,std::uint32_t source,
                                    std::uint32_t target,const Read& read) {
    if(original.size()<0x284 || original.size()>maximum || std::uint64_t(source)+original.size()>0x100000000ull)
        throw FormatError("Invalid MCC shader globals allocation");
    std::vector<std::byte> out(original.begin(),original.end());
    std::set<std::tuple<unsigned,std::uint32_t,std::size_t>> visited;
    std::map<std::tuple<std::uint32_t,unsigned,unsigned>,std::size_t> copied;
    auto walk=[&](auto&& self,unsigned node,std::uint32_t address,std::size_t output,unsigned depth)->void {
        if(depth>32 || visited.size()>1000000)throw FormatError("Excessive shader globals pointer graph");
        if(!visited.emplace(node,address,output).second)return;
        const auto layout=clone_nodes.at(node);
        for(unsigned i=0;i<layout.count;++i) {
            const auto field=clone_fields.at(layout.first+i);
            if(std::uint64_t(address)+field.offset+8>0x100000000ull)throw FormatError("Shader globals descriptor overflow");
            const Bytes descriptor(read(address+field.offset,8));
            const auto count=descriptor.u32(0),pointer=descriptor.u32(4);
            if(!count){word(out,output+field.offset+4,0);continue;}
            const auto bytes=std::size_t(count)*field.stride;
            if(bytes>maximum || (!field.data && count>65536) || std::uint64_t(pointer)+bytes>0x100000000ull)
                throw FormatError("Excessive shader globals child bank");
            std::size_t offset;
            if(pointer>=source && std::uint64_t(pointer)-source<=original.size() && bytes<=original.size()-(pointer-source))
                offset=pointer-source;
            else {
                const auto key=std::tuple(pointer,count,field.stride);
                auto it=copied.find(key);
                if(it==copied.end()) {
                    offset=(out.size()+15)&~std::size_t(15);
                    if(offset>maximum || bytes>maximum-offset)throw FormatError("Excessive shader globals allocation");
                    const auto bank=read(pointer,bytes);out.resize(offset);out.insert(out.end(),bank.begin(),bank.end());
                    it=copied.emplace(key,offset).first;
                }
                offset=it->second;
            }
            if(std::uint64_t(target)+offset>0xffffffffu)throw FormatError("Shader globals relocation overflow");
            word(out,output+field.offset+4,target+std::uint32_t(offset));
            if(!field.data)for(unsigned n=0;n<count;++n)self(self,field.child,pointer+n*field.stride,offset+std::size_t(n)*field.stride,depth+1);
        }
    };
    walk(walk,0,source,0,0);return out;
}
}

void Cache::initialize_mcc_native_shaders() {
    const auto directory=shared_directory_.parent_path().parent_path()/"native-shaders"/name_;
    const auto manifest=directory/"library.json";
    if(!std::filesystem::exists(manifest))return;
    const auto raw=read(manifest,16u*1024u*1024u);
    const auto bank=Json::parse(raw.begin(),raw.end());
    if(bank.at("schema")!="H2_PRESERVED_NATIVE_SHADERS_V1" || bank.at("map")!=name_)
        throw FormatError("Preserved shader library identity mismatch");
    const std::set<std::string> shader_groups{"shad","stem","spas","vrtx"};
    const auto& records=bank.at("shader_tags");
    if(!records.is_array() || records.size()>65536)throw FormatError("Excessive preserved shader directory");
    std::map<std::pair<std::string,std::string>,std::uint32_t> identities;
    for(const auto& t:tags_)if(t.datum!=0xffffffffu)
        if(!identities.emplace(std::pair(t.group,t.name),t.datum).second)throw FormatError("Duplicate MCC tag identity for native shader binding");
    auto bind=[&](const std::string& group,const std::string& name) {
        if(auto found=identities.find({group,name});found!=identities.end())return found->second;
        if(!shader_groups.contains(group) && group!="bitm")throw FormatError("Preserved shader references a non-shader asset group");
        if(tags_.size()>=65536 || !tag_group(TagGroup::from_string(group)))throw FormatError("Native shader tag directory capacity/group failure");
        const auto handle=0xd0000000u|std::uint32_t(tags_.size());
        tags_.push_back({group,name,handle,0,0});identities.emplace(std::pair(group,name),handle);return handle;
    };
    std::set<std::uint32_t> restored;
    std::set<std::uint32_t> referenced_shaders;
    for(const auto& record:records) {
        const auto group=record.at("group").get<std::string>(),name=record.at("name").get<std::string>();
        if(!shader_groups.contains(group) || !restored.insert(bind(group,name)).second)
            throw FormatError("Invalid/duplicate preserved shader definition");
        for(const auto& ref:record.at("references")) {
            const auto group=ref.at(1).at(0).get<std::string>();
            const auto handle=bind(group,ref.at(1).at(1));
            if(shader_groups.contains(group))referenced_shaders.insert(handle);
        }
    }
    for(const auto& ref:bank.at("rasterizer").at("references")) {
        const auto group=ref.at(1).at(0).get<std::string>();
        const auto handle=bind(group,ref.at(1).at(1));
        if(shader_groups.contains(group))referenced_shaders.insert(handle);
    }
    // A nonresident original shader reference stays unresolved. Never fill
    // that hole with a same-named MCC D3D program and parse it as NV2A.
    for(auto handle:referenced_shaders)if(!restored.contains(handle)) {
        auto& t=tags_[handle&0xffffu];t.address=0;t.size=0;
    }
    // Append missing shader parameter names to the owned native StringID bank;
    // existing gameplay IDs keep their exact indices and lengths.
    std::vector<std::uint32_t> offsets;std::vector<std::byte> strings;
    std::map<std::string,std::uint32_t> string_ids;
    {
        const Bytes source(storage_);const auto count=source.u32(48),bytes=source.u32(56),table=source.u32(60),text=source.u32(52);
        if(count>65536)throw FormatError("Excessive MCC string bank");
        const auto original=source.slice(text,bytes);strings.assign(original.begin(),original.end());
        for(unsigned i=0;i<count;++i) {
            offsets.push_back(source.u32(std::size_t(table)+i*4));
            if(offsets.back()==0xffffffffu)continue;
            const auto value=source.string(std::size_t(text)+offsets.back(),bytes-offsets.back());
            string_ids.emplace(value,(std::uint32_t(value.size())<<24)|i);
        }
    }
    auto bind_string=[&](const std::string& value) {
        if(auto found=string_ids.find(value);found!=string_ids.end())return found->second;
        if(value.size()>255 || offsets.size()>=65536)throw FormatError("Preserved shader StringID capacity failure");
        const auto id=(std::uint32_t(value.size())<<24)|std::uint32_t(offsets.size());
        offsets.push_back(std::uint32_t(strings.size()));
        for(char c:value)strings.push_back(std::byte(static_cast<unsigned char>(c)));
        strings.push_back(std::byte{});
        string_ids.emplace(value,id);return id;
    };
    auto prepare=[&](const Json& record,std::uint64_t va) {
        const auto filename=record.at("payload").get<std::string>();
        if(filename.size()!=68 || filename.substr(64)!=".bin" || filename.find_first_not_of("0123456789abcdef")<64)
            throw FormatError("Invalid preserved shader payload filename");
        auto payload=read(directory/"payloads"/filename,16u*1024u*1024u);
        if(payload.size()!=record.at("bytes").get<std::size_t>() ||
           fnv(payload)!=std::stoull(record.at("fnv64").get<std::string>(),nullptr,16))
            throw FormatError("Preserved shader payload fingerprint mismatch");
        const auto at=std::size_t(std::int64_t(va)-metadata_magic_);
        if(va+payload.size()>0x100000000ull || at+payload.size()>1024ull*1024*1024 ||
           (!vista_metadata_regions_.empty() && va+payload.size()-vista_metadata_regions_.front().first>maximum))
            throw FormatError("Excessive preserved shader allocation");
        std::set<std::size_t> fixups;
        for(const auto& fixup:record.at("pointers")) {
            const auto location=fixup.at(0).get<std::size_t>(),offset=fixup.at(1).get<std::size_t>();
            if(offset>=payload.size() || !fixups.insert(location).second)throw FormatError("Invalid native shader pointer relocation");
            word(payload,location,std::uint32_t(va+offset));
        }
        for(const auto& fixup:record.at("references")) {
            const auto location=fixup.at(0).get<std::size_t>();
            if(!fixups.insert(location).second)throw FormatError("Overlapping native shader fixup");
            word(payload,location,bind(fixup.at(1).at(0),fixup.at(1).at(1)));
        }
        for(const auto& fixup:record.at("strings")) {
            const auto location=fixup.at(0).get<std::size_t>();
            if(!fixups.insert(location).second)throw FormatError("Overlapping native shader string fixup");
            word(payload,location,bind_string(fixup.at(1)));
        }
        return payload;
    };
    auto next_address=[&] {return (std::uint64_t(std::int64_t(storage_.size())+metadata_magic_)+15)&~std::uint64_t(15);};
    auto append=[&](const Json& record) {
        const auto va=next_address();const auto payload=prepare(record,va);
        storage_.resize(std::size_t(std::int64_t(va)-metadata_magic_));storage_.insert(storage_.end(),payload.begin(),payload.end());
        return std::pair(std::uint32_t(va),std::uint32_t(payload.size()));
    };
    for(const auto& record:records) {
        const auto handle=bind(record.at("group"),record.at("name"));
        const auto [address,size]=append(record);auto& t=tags_.at(handle&0xffffu);t.address=address;t.size=size;
    }
    const auto* globals=query_tag({.handle=globals_tag_.is_null()?std::nullopt:std::optional<TagHandle>{globals_tag_},.group=tag_groups::globals,.require_address=true});
    if(!globals)throw FormatError("Native shader rasterizer globals owner unavailable");
    const auto old_globals=std::pair(globals->address,globals->size);
    const auto handle=globals->datum;
    const auto va=next_address();
    auto global_bytes=clone_globals(Bytes(storage_).slice(metadata_offset(globals->address,globals->size),globals->size),
        globals->address,std::uint32_t(va),[&](std::uint32_t pointer,std::size_t size){return Bytes(storage_).slice(metadata_offset(pointer,size),size);});
    const auto raster_offset=(global_bytes.size()+15)&~std::size_t(15);
    const auto raster=prepare(bank.at("rasterizer"),va+raster_offset);
    if(raster_offset>maximum || raster.size()>maximum-raster_offset || va+raster_offset+raster.size()>0x100000000ull)
        throw FormatError("Excessive preserved rasterizer globals allocation");
    global_bytes.resize(raster_offset);global_bytes.insert(global_bytes.end(),raster.begin(),raster.end());
    word(global_bytes,0x108,1);word(global_bytes,0x10c,std::uint32_t(va+raster_offset));
    storage_.resize(std::size_t(std::int64_t(va)-metadata_magic_));storage_.insert(storage_.end(),global_bytes.begin(),global_bytes.end());
    tags_[handle&0xffffu].address=std::uint32_t(va);tags_[handle&0xffffu].size=std::uint32_t(global_bytes.size());
    const auto text=storage_.size();storage_.insert(storage_.end(),strings.begin(),strings.end());
    const auto table=storage_.size();storage_.resize(table+offsets.size()*4);
    for(std::size_t i=0;i<offsets.size();++i)word(storage_,table+i*4,offsets[i]);
    word(storage_,48,std::uint32_t(offsets.size()));word(storage_,52,std::uint32_t(text));
    word(storage_,56,std::uint32_t(strings.size()));word(storage_,60,std::uint32_t(table));
    vista_metadata_regions_.clear();
    for(const auto& t:tags_)if(t.address && t.size)vista_metadata_regions_.emplace_back(t.address,t.size);
    vista_metadata_regions_.push_back(old_globals);std::ranges::sort(vista_metadata_regions_);
    mcc_native_shaders_.resize(tags_.size());
    for(auto handle:restored)mcc_native_shaders_[handle&0xffffu]=true;
    const auto tables=read(directory/"function-tables.bin",28*1024);
    if(tables.size()!=28*1024 || fnv(tables)!=0x22708b4cc478ddb7ull)throw FormatError("Unsupported preserved shader function tables");
    auto functions=std::make_shared<ShaderFunctionTables>();
    for(unsigned i=0;i<28;++i)for(unsigned j=0;j<1024;++j)
        (i<12?functions->periodic[i][j]:functions->transition[i-12][j])=std::to_integer<std::uint8_t>(tables[i*1024+j]);
    function_tables_=functions;
}
}
