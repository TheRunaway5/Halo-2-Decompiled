#include "halo2/prepared_original_sound_catalog_tu15.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <fstream>
#include <map>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <set>

namespace halo2 {
namespace {
using J=nlohmann::json;namespace fs=std::filesystem;
void need(bool value,const char* why){if(!value)throw FormatError(why);}
bool digest_text(std::string_view text){return text.size()==64&&std::ranges::all_of(text,[](char c){return(c>='0'&&c<='9')||(c>='a'&&c<='f');});}
std::string hash(std::span<const std::byte> bytes){std::array<unsigned char,32> digest;unsigned size{};
    need(EVP_Digest(bytes.data(),bytes.size(),digest.data(),&size,EVP_sha256(),nullptr)==1&&size==32,"Cannot authenticate original sound catalog");
    constexpr char hex[]="0123456789abcdef";std::string out;for(auto b:digest){out+=hex[b>>4];out+=hex[b&15];}return out;}
std::string component(const J& value){const auto text=value.get<std::string>();
    need(!text.empty()&&text.size()<=255&&text!="."&&text!=".."&&std::ranges::all_of(text,[](unsigned char c){
        return(c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='.';}),"Original catalog component is not a literal private name");return text;}
J read_catalog(const fs::path& root,std::string_view expected){
    need(digest_text(expected),"Original sound catalog requires its external admission digest");
    need(fs::is_directory(fs::symlink_status(root)),"Original sound catalog directory is not an owned directory");
    const auto path=root/"catalog.json";need(fs::is_regular_file(fs::symlink_status(path)),"Original sound catalog is not a regular owned file");
    const auto bytes=fs::file_size(path);need(bytes&&bytes<=4*1024*1024,"Original sound catalog exceeds its format bound");
    std::ifstream stream(path,std::ios::binary);std::string raw(std::size_t(bytes),'\0');need(bool(stream.read(raw.data(),raw.size())),"Original sound catalog is truncated");
    need(hash(std::as_bytes(std::span(raw.data(),raw.size())))==expected,"Original sound catalog differs from its external admission digest");
    std::vector<std::set<std::string>> keys;
    return J::parse(raw,[&](int,J::parse_event_t event,J& value){
        if(event==J::parse_event_t::object_start)keys.emplace_back();
        else if(event==J::parse_event_t::key)need(!keys.empty()&&keys.back().insert(value.get<std::string>()).second,"Original sound catalog has duplicate fields");
        else if(event==J::parse_event_t::object_end)keys.pop_back();
        return true;});
}
struct Bank {std::string directory,digest;};
struct Context {std::string runtime_map,original_map,source_digest;std::vector<Bank> banks;};
struct Names {
    std::map<std::uint32_t,std::pair<std::uint32_t,std::string>> original;
    std::map<std::uint32_t,std::string> native;
    void bind(std::uint32_t old,std::uint32_t id,const std::string& name){
        const auto [i,added]=original.emplace(old,std::pair{id,name});need(added||i->second==std::pair{id,name},"Original catalog import identity aliases different names");
        const auto [n,unique]=native.emplace(id,name);need(unique||n->second==name,"Original catalog aliases actual native StringIDs");
    }
};
void imports(const Cache& cache,const Tag& tag,const PreparedOriginalSoundBank::Metadata& metadata,Names& names){
    const Bytes snd(cache.read_tag_bytes(tag.address,28));need(!(snd.u16(0)&0x800u),"Original catalog target is not a Classic sound");
    const auto& owner=cache.mcc_sound_gestalt_owner(tag.handle());const Bytes gestalt(cache.read_tag_bytes(owner.address,120));
    const auto name=[&](unsigned index){need(index<gestalt.u32(24),"Original catalog import escapes its actual gestalt");
        const auto id=Bytes(cache.read_tag_bytes(gestalt.u32(28)+index*4u,4)).u32(0);return std::pair{id,cache.string_id(id)};};
    const auto count=std::to_integer<unsigned>(snd.slice(10,1)[0]);need(std::uint64_t(snd.u16(8))+count<=gestalt.u32(40),"Original catalog pitch ranges escape their actual owner");
    std::map<std::string,std::uint32_t> ranges;
    for(unsigned n=0;n<count;++n){const auto at=gestalt.u32(44)+(snd.u16(8)+n)*12u;const auto named=name(Bytes(cache.read_tag_bytes(at,12)).u16(0));
        need(ranges.emplace(named.second,at).second,"Original catalog native pitch name is ambiguous");}
    for(const auto& pitch:metadata.pitch_ranges){need(pitch.import_name_id&&ranges.contains(pitch.name),"Original catalog lacks its exact native pitch import");
        const Bytes range(cache.read_tag_bytes(ranges.at(pitch.name),12));const auto rn=name(range.u16(0));names.bind(*pitch.import_name_id,rn.first,rn.second);
        need(std::uint64_t(range.u16(8))+range.u16(10)<=gestalt.u32(48),"Original catalog permutations escape their actual owner");
        std::map<std::string,std::uint32_t> permutations;
        for(unsigned n=0;n<range.u16(10);++n){const Bytes row(cache.read_tag_bytes(gestalt.u32(52)+(range.u16(8)+n)*16u,16));const auto named=name(row.u16(0));
            need(permutations.emplace(named.second,named.first).second,"Original catalog native permutation name is ambiguous");}
        for(const auto& permutation:pitch.permutations){need(permutation.import_name_id&&permutations.contains(permutation.name),"Original catalog lacks its exact native permutation import");
            names.bind(*permutation.import_name_id,permutations.at(permutation.name),permutation.name);}
    }
}
}
struct PreparedOriginalSoundCatalogTu15::Impl {fs::path directory;std::vector<Context> contexts;};
struct PreparedOriginalSoundCatalogBindingTu15::Impl {
    const Cache* cache{};std::string context;std::vector<OriginalSoundRuntimeBindingTu15> bindings;
    std::shared_ptr<const Names> names;
};
PreparedOriginalSoundCatalogTu15::PreparedOriginalSoundCatalogTu15(std::shared_ptr<const Impl> impl):impl_(std::move(impl)){}
PreparedOriginalSoundCatalogBindingTu15::PreparedOriginalSoundCatalogBindingTu15(std::shared_ptr<const Impl> impl):impl_(std::move(impl)){}
std::shared_ptr<const PreparedOriginalSoundCatalogTu15> PreparedOriginalSoundCatalogTu15::load(const fs::path& directory,std::string_view expected){
    const auto raw=read_catalog(directory,expected);need(raw.at("schema")=="H2_PREPARED_ORIGINAL_SOUND_CATALOG_TU15_V1","Unsupported original sound catalog schema");
    const auto& contexts=raw.at("contexts");need(contexts.is_array()&&!contexts.empty()&&contexts.size()<=256,"Original sound catalog context extent is invalid");
    auto result=std::make_shared<Impl>();result->directory=fs::absolute(directory).lexically_normal();std::set<std::string> digests,runtime_maps,original_maps;
    for(const auto& entry:contexts){Context context;context.runtime_map=component(entry.at("runtime_map"));context.original_map=component(entry.at("original_map"));
        context.source_digest=entry.at("source_runtime_sha256").get<std::string>();
        need(digest_text(context.source_digest)&&digests.insert(context.source_digest).second&&runtime_maps.insert(context.runtime_map).second&&original_maps.insert(context.original_map).second,
            "Original sound catalog snapshot or context identity is ambiguous");
        const auto& banks=entry.at("banks");need(banks.is_array()&&!banks.empty()&&banks.size()<=256,"Original sound catalog bank extent is invalid");std::set<std::string> directories;
        for(const auto& value:banks){Bank bank{component(value.at("directory")),value.at("sha256").get<std::string>()};
            need(digest_text(bank.digest)&&directories.insert(bank.directory).second,"Original sound catalog bank identity is invalid or duplicated");context.banks.push_back(std::move(bank));}
        result->contexts.push_back(std::move(context));
    }
    return std::shared_ptr<const PreparedOriginalSoundCatalogTu15>(new PreparedOriginalSoundCatalogTu15(std::move(result)));
}
std::shared_ptr<const PreparedOriginalSoundCatalogBindingTu15> PreparedOriginalSoundCatalogTu15::bind(const Cache& cache)const{
    need(cache.is_mcc(),"Original sound catalog requires converted MCC ownership");const auto digest=hash(cache.source_runtime_cache_bytes());
    const auto found=std::ranges::find(impl_->contexts,digest,&Context::source_digest);
    need(found!=impl_->contexts.end(),"Original sound catalog has no exact converted Cache snapshot");
    need(cache.path().filename()==found->runtime_map,"Original sound catalog runtime filename differs from its admitted snapshot");
    auto result=std::make_shared<PreparedOriginalSoundCatalogBindingTu15::Impl>();result->cache=&cache;result->context=found->original_map;auto names=std::make_shared<Names>();
    std::set<std::uint32_t> identities;std::set<std::string> paths;
    for(const auto& entry:found->banks){const auto directory=impl_->directory/entry.directory;
        need(fs::is_directory(fs::symlink_status(directory)),"Original sound bank directory is not an owned directory");
        const auto bank=PreparedOriginalSoundBank::load(directory,entry.digest);const auto& metadata=bank->metadata();
        need(metadata.origin&&metadata.sound_classes&&metadata.origin->map==found->original_map,"Original sound catalog bank has a different original context");
        const Tag* tag{};for(const auto& candidate:cache.tags())if(candidate.address&&candidate.group=="snd!"&&candidate.name==metadata.name){
            need(!tag,"Original sound catalog target path is ambiguous");tag=&candidate;}
        need(tag&&identities.insert(tag->handle().value).second&&paths.insert(metadata.name).second,"Original sound catalog target is missing or duplicated");
        imports(cache,*tag,metadata,*names);result->bindings.push_back({tag->handle(),bank});
    }
    result->names=std::move(names);
    return std::shared_ptr<const PreparedOriginalSoundCatalogBindingTu15>(new PreparedOriginalSoundCatalogBindingTu15(std::move(result)));
}
MccOriginalSoundRuntimeInputTu15 PreparedOriginalSoundCatalogBindingTu15::input(const Cache& expected)const{
    need(&expected==impl_->cache,"Original sound catalog binding belongs to another Cache");const auto names=impl_->names;
    return {impl_->bindings,{impl_,[names](auto old,auto label){const auto i=names->original.find(old);need(i!=names->original.end()&&i->second.second==label,"Original sound catalog import label changed");return i->second.first;},
        [names](auto id){const auto i=names->native.find(id);need(i!=names->native.end(),"Original sound catalog native StringID is absent");return i->second;}}};
}
std::string_view PreparedOriginalSoundCatalogBindingTu15::original_context()const{return impl_->context;}
std::size_t PreparedOriginalSoundCatalogBindingTu15::size()const{return impl_->bindings.size();}
} // namespace halo2
