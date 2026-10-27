#include "halo2/cache.hpp"
#include "halo2/mcc_native_no_alpha.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <set>

namespace {
using namespace halo2;using Json=nlohmann::json;
constexpr std::array<std::pair<unsigned,unsigned>,14> fields{{
    {4,12},{12,4},{20,16},{28,6},{36,2},{44,2},{52,10},{60,20},
    {68,4},{76,2},{84,4},{92,4},{100,12},{108,4}}};
void need(bool value,const char* message){if(!value)throw FormatError(message);}
std::vector<std::byte> unhex(const Json& value){const auto text=value.get<std::string>();std::vector<std::byte> out;
    need(text.size()%2==0,"Invalid NoAlpha fixture hex");for(unsigned i=0;i<text.size();i+=2)out.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));return out;}
Tag tag(const Json& value){return {value.at("group"),value.at("name"),value.at("datum"),value.at("address"),value.at("size")};}
void word(std::vector<std::byte>& raw,unsigned at,std::uint32_t value){Bytes(raw).require(at,4);for(unsigned i=0;i<4;++i)raw[at+i]=std::byte(value>>(i*8));}
struct Fixture {
    Tag stem,pass,normal_stem,normal_pass;
    std::vector<std::byte> stem_payload,pass_payload;
    std::map<std::uint32_t,Tag> names;
    std::map<std::uint32_t,unsigned> frames;
    std::map<std::uint32_t,std::string> strings;
    std::vector<std::pair<std::uint32_t,std::vector<std::byte>>> regions;
    explicit Fixture(const Json& input):stem(tag(input.at("setup").at("source_stem"))),pass(tag(input.at("setup").at("source_pass"))),
        normal_stem(tag(input.at("native_stem"))),normal_pass(tag(input.at("native_pass"))),
        stem_payload(unhex(input.at("native_stem_payload_hex"))),pass_payload(unhex(input.at("native_pass_payload_hex"))) {
        for(const auto& value:input.at("namespace")){auto t=tag(value);names.emplace(t.datum,std::move(t));}
        for(const auto& [handle,count]:input.at("bitmap_frame_counts").items())frames.emplace(std::stoul(handle),count.get<unsigned>());
        for(const auto& row:input.at("cases"))for(const auto& [id,text]:row.at("source_string_ids").items())strings.emplace(std::stoul(id),text.get<std::string>());
        for(const auto& region:input.at("regions"))regions.emplace_back(region.at(0),unhex(region.at(1)));
    }
    const Tag* find(std::uint32_t handle)const{const auto at=names.find(handle);return at==names.end()?nullptr:&at->second;}
    const Tag* named(std::string_view group,std::string_view name)const{for(const auto& [_,tag]:names)if(tag.group==group&&tag.name==name)return &tag;return nullptr;}
    std::span<const std::byte> read(std::uint32_t address,std::size_t size)const{
        if(!size)return {};for(const auto& [first,raw]:regions)
            if(address>=first&&std::uint64_t(address)-first<=raw.size()&&size<=raw.size()-(address-first))return std::span(raw).subspan(address-first,size);
        throw FormatError("NoAlpha fixture read escaped its actual source allocation");}
    void mutate(std::uint32_t address){bool found=false;for(auto& [first,raw]:regions)if(address>=first&&std::uint64_t(address)-first<raw.size()){
        raw[address-first]^=std::byte{1};found=true;}need(found,"NoAlpha mutation escaped source ownership");}
    auto owners()const{return project_mcc_no_alpha_native_owners(stem,pass,normal_stem,normal_pass,stem_payload,pass_payload,
        [&](auto address,auto size){return read(address,size);},[&](auto handle){return find(handle);},
        [&](auto group,auto name){return named(group,name);});}
    auto material(const Tag& shader)const{return project_mcc_no_alpha_material(shader,stem,pass,normal_stem,stem_payload,
        [&](auto address,auto size){return read(address,size);},[&](auto handle){return find(handle);},
        [&](auto handle,auto frame){const auto at=frames.find(handle);return at!=frames.end()&&frame<at->second;},
        [&](auto id){const auto at=strings.find(id);if(at==strings.end())throw FormatError("NoAlpha source StringID is absent");return at->second;});}
};
}
int main(int argc,char**argv)try {
    need(argc==2,"Expected complete literal NoAlpha fixture");std::ifstream input(argv[1]);const auto data=Json::parse(input);
    need(data.at("schema")=="H2_MCC_NATIVE_NO_ALPHA_FIXTURE_V1"&&data.at("cases").size()==2,"NoAlpha actual owner census changed");
    Fixture source(data);const auto owners=source.owners();need(bool(owners),"Complete authored NoAlpha source was not qualified");
    unsigned rejects=0,banks=0,words=0;const Bytes pass(owners->pass.bytes);const auto pp=pass.u32(32),impl=pass.u32(pp+4),states=pass.u32(pp+20);
    for(unsigned i=0;i<4;++i){const auto range=pass.u16(impl+i*306+2);unsigned masks=0;
        for(unsigned n=(range&511u);n<(range&511u)+(range>>9);++n)if(std::to_integer<unsigned>(pass.slice(states+n*5,1)[0])==67){
            need(pass.u32(states+n*5+1)==(i?0x01010101u:0x00010101u),"NoAlpha authored implementation write mask changed");++masks;}
        need(masks==1,"NoAlpha original color-state range changed");}
    // All original shader/program bytes survive. Only actual namespace words
    // and the one authenticated alpha write-enable byte may differ.
    auto expected=source.pass_payload;const auto& record=data.at("setup").at("native_pass_record");
    for(const auto& reference:record.at("references")){
        const auto* actual=source.named(reference.at(1).at(0).get<std::string>(),reference.at(1).at(1).get<std::string>());need(actual,"Missing original native dependency");word(expected,reference.at(0),actual->datum);}
    const Bytes original(expected);const auto original_pp=original.u32(32),original_impl=original.u32(original_pp+4),original_states=original.u32(original_pp+20);
    const auto range=original.u16(original_impl+2);unsigned mask_at=0;
    for(unsigned i=(range&511u);i<(range&511u)+(range>>9);++i)if(std::to_integer<unsigned>(original.slice(original_states+i*5,1)[0])==67)mask_at=original_states+i*5+1;
    need(mask_at!=0,"Missing source TU color state");word(expected,mask_at,0x00010101u);need(expected==owners->pass.bytes,"NoAlpha changed another original shader/program/state byte");
    for(const auto& [field,target]:owners->pass.pointers)need(pass.u32(field)==target&&target<owners->pass.bytes.size(),"NoAlpha native pass pointer provenance changed");
    for(const auto& row:data.at("cases")) {
        const auto shader=tag(row.at("shader"));const auto projected=source.material(shader);need(bool(projected),"Actual NoAlpha material/NIEx setup was not admitted");
        const Bytes out(projected->bytes);need(out.u32(4)==source.stem.datum,"NoAlpha published template salt changed");const auto pp=out.u32(36);
        need(out.u32(pp)==source.stem.datum&&out.u32(8)==0x110000a3u,"NoAlpha published owner/SID changed");
        std::set<unsigned> pointers;for(const auto& [field,target]:projected->pointers)need(pointers.insert(field).second&&out.u32(field)==target&&target<projected->bytes.size(),"NoAlpha material pointer provenance changed");
        for(const auto& [field,stride]:fields){const auto actual=out.slice(out.u32(pp+field+4),std::size_t(out.u32(pp+field))*stride);const auto expected=unhex(row.at("expected_banks").at(std::to_string(field)));
            need(std::ranges::equal(actual,expected),"NoAlpha complete authored bank/NIE conversion changed");++banks;words+=unsigned(expected.size()/4);}
        need(std::ranges::equal(out.slice(out.u32(16),80),unhex(row.at("runtime_hex"))),"NoAlpha authored runtime bank changed");
        need(std::ranges::equal(out.slice(out.u32(48),out.u32(44)*8),unhex(row.at("predictions_hex"))),"NoAlpha authored prediction bank changed");
        // Every constructor-consumed authored material byte is protected;
        // physical pointers are tested too because this finite source setup
        // keeps its complete exact declared allocation graph.
        std::set<std::uint32_t> addresses;const auto root=Bytes(source.read(shader.address,84));const auto definition=Bytes(source.read(root.u32(36),124));
        const auto add=[&](std::uint32_t first,std::size_t count){for(unsigned i=0;i<count;++i)addresses.insert(first+i);};
        add(shader.address,84);add(root.u32(16),80);add(root.u32(36),372);add(root.u32(48),root.u32(44)*8);
        for(const auto& [field,stride]:fields)if(definition.u32(field))add(definition.u32(field+4),std::size_t(definition.u32(field))*stride);
        for(const auto address:addresses){auto bad=source;bad.mutate(address);need(!bad.material(shader),"Changed complete NoAlpha material/NIEx byte admitted");++rejects;}
        {auto alias=source;auto renamed=shader;renamed.name="another-authored-instance";alias.names[shader.datum]=renamed;need(bool(alias.material(renamed)),"NoAlpha admission depends on material name");}
        {auto bad=source;bad.strings[0x110000a3]="wrong-surface-role";need(!bad.material(shader),"Changed actual NoAlpha StringID namespace admitted");++rejects;}
    }
    std::set<std::uint32_t> setup_bytes;
    const auto add_setup=[&](std::uint32_t first,unsigned size){for(unsigned i=0;i<size;++i)setup_bytes.insert(first+i);};
    add_setup(source.stem.address,source.stem.size);add_setup(source.pass.address,source.pass.size);
    for(const auto& row:data.at("setup").at("vertex_owners")){
        const auto* actual=source.find(row.at("datum"));need(actual,"Missing actual seven-class source vertex owner");add_setup(actual->address,actual->size);}
    for(const auto& row:data.at("setup").at("external_pass_allocations"))add_setup(row.at("address"),row.at("bytes"));
    for(const auto address:setup_bytes){auto bad=source;bad.mutate(address);need(!bad.owners(),"Changed NoAlpha pass/VS/PS/setup byte admitted");++rejects;}
    for(unsigned i=0;i<source.stem_payload.size();++i){auto bad=source;bad.stem_payload[i]^=std::byte{1};need(!bad.owners(),"Changed original TU template admitted");++rejects;}
    for(unsigned i=0;i<source.pass_payload.size();++i){auto bad=source;bad.pass_payload[i]^=std::byte{1};need(!bad.owners(),"Changed original TU shader/pass admitted");++rejects;}
    for(const auto& [handle,_]:source.names){auto bad=source;bad.names[handle].group="invalid";
        if(bad.owners()){unsigned affected=0;for(const auto& row:data.at("cases"))affected+=!bad.material(tag(row.at("shader")));need(affected>0,"Unowned NoAlpha typed dependency admitted");}++rejects;}
    for(const auto& [handle,_]:source.frames){auto bad=source;bad.frames[handle]=0;unsigned affected=0;
        for(const auto& row:data.at("cases"))affected+=!bad.material(tag(row.at("shader")));need(affected>0,"Missing actual NoAlpha frame admitted");++rejects;}
    std::cout<<Json{{"passed",true},{"profiles",2},{"complete_banks",banks},{"exact_authored_words",words},{"original_implementations",4},
        {"source_vertex_class_instances",28},{"source_pixel_payloads",39},{"strict_rejections",rejects}}.dump()<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
