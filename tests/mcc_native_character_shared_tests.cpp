#include "halo2/cache.hpp"
#include "halo2/mcc_native_character_shared.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <set>

namespace {
using namespace halo2;using Json=nlohmann::json;
constexpr std::array<std::pair<unsigned,unsigned>,14> fields{{
    {4,12},{12,4},{20,16},{28,6},{36,2},{44,2},{52,10},
    {60,20},{68,4},{76,2},{84,4},{92,4},{100,12},{108,4}}};
void need(bool value,const std::string& message){if(!value)throw FormatError(message);}
std::vector<std::byte> unhex(const Json& value){const auto text=value.get<std::string>();std::vector<std::byte> bytes;
    need(text.size()%2==0,"Invalid character fixture hex");for(unsigned i=0;i<text.size();i+=2)bytes.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));return bytes;}
Tag tag(const Json& value){return {value.at("group"),value.at("name"),value.at("datum"),value.at("address"),value.at("size")};}
void word(std::vector<std::byte>& out,unsigned at,std::uint32_t value){Bytes(out).require(at,4);for(unsigned i=0;i<4;++i)out[at+i]=std::byte(value>>(8*i));}
struct Fixture {
    Tag shader,source_stem,native_stem;std::string payload_name;std::vector<std::byte> payload;
    std::map<std::uint32_t,Tag> names;std::map<std::uint32_t,unsigned> frames;
    std::map<std::uint32_t,std::string> strings;
    std::vector<std::pair<std::uint32_t,std::vector<std::byte>>> regions;
    explicit Fixture(const Json& input):shader(tag(input.at("shader"))),source_stem(tag(input.at("source_mcc_stem"))),
        native_stem(tag(input.at("native_stem"))),payload_name(input.at("native_payload_name")),payload(unhex(input.at("native_payload_hex"))) {
        for(const auto& value:input.at("namespace")){auto t=tag(value);names.emplace(t.datum,std::move(t));}
        for(const auto& [id,count]:input.at("bitmap_frame_counts").items())frames.emplace(std::stoul(id),count.get<unsigned>());
        for(const auto& [id,text]:input.at("string_ids").items())strings.emplace(std::stoul(id),text.get<std::string>());
        for(const auto& region:input.at("regions"))regions.emplace_back(region.at(0),unhex(region.at(1)));
    }
    std::span<const std::byte> read(std::uint32_t address,std::size_t size) const {
        if(!size)return {};for(const auto& [first,raw]:regions)
            if(address>=first&&std::uint64_t(address)-first<=raw.size()&&size<=raw.size()-(address-first))return std::span(raw).subspan(address-first,size);
        throw FormatError("Character fixture allocation escaped its actual owner");
    }
    void mutate(std::uint32_t address){bool found=false;for(auto& [first,raw]:regions)
        if(address>=first&&std::uint64_t(address)-first<raw.size()){raw[address-first]^=std::byte{1};found=true;}
        need(found,"Character mutation escaped its source region");}
    void write_word(std::uint32_t address,std::uint32_t value){for(unsigned i=0;i<4;++i)for(auto& [first,raw]:regions)
        if(address+i>=first&&std::uint64_t(address+i)-first<raw.size())raw[address+i-first]=std::byte(value>>(8*i));}
    std::optional<MccOwnedShaderProjection> run()const{return project_mcc_character_shared(shader,source_stem,native_stem,payload_name,payload,
        [&](auto at,auto size){return read(at,size);},[&](auto handle)->const Tag*{const auto it=names.find(handle);return it==names.end()?nullptr:&it->second;},
        [&](auto handle,auto frame){const auto it=frames.find(handle);return it!=frames.end()&&frame<it->second;},
        [&](auto id){const auto it=strings.find(id);if(it==strings.end())throw FormatError("Missing actual source input StringID");return it->second;});}
};
}
int main(int argc,char**argv)try {
    need(argc==2,"Expected literal shared character fixture");std::ifstream input(argv[1]);const auto fixture=Json::parse(input);
    need(fixture.at("schema")=="H2_MCC_CHARACTER_SHARED_FIXTURE_V1"&&fixture.at("cases").size()==2,"Character fixture census changed");
    unsigned rejects=0,banks_checked=0,curves_checked=0,words_checked=0;
    for(const auto& row:fixture.at("cases")) {
        Fixture value(row);const auto result=value.run();need(bool(result),"Actual shared character source was not admitted");
        const Bytes out(result->bytes);const auto pp=out.u32(36);std::set<unsigned> pointer_fields;
        for(const auto& [field,target]:result->pointers)need(pointer_fields.insert(field).second&&target<result->bytes.size()&&out.u32(field)==target,"Character pointer ownership changed");
        need(out.u32(32)==1,"Character native platform count changed");
        for(const auto& [field,stride]:fields) {
            const auto expected=unhex(row.at("expected_banks").at(std::to_string(field)));
            auto actual=std::vector<std::byte>(out.slice(out.u32(pp+field+4),std::size_t(out.u32(pp+field))*stride).begin(),
                out.slice(out.u32(pp+field+4),std::size_t(out.u32(pp+field))*stride).end());
            if(field==60)for(unsigned i=0;i<row.at("curves").size();++i) {
                const auto curve=unhex(row.at("curves").at(i));const auto pointer=Bytes(actual).u32(i*20+16);
                need(Bytes(actual).u32(i*20+12)==curve.size()&&std::ranges::equal(out.slice(pointer,curve.size()),curve)&&pointer_fields.contains(out.u32(pp+64)+i*20+16),"Complete authored character function relocation changed");
                word(actual,i*20+16,0);++curves_checked;
            }
            need(actual==expected,"Complete paired original character bank changed: "+std::to_string(field));
            words_checked+=unsigned(expected.size()/4);++banks_checked;
        }
        need(std::ranges::equal(out.slice(out.u32(16),80),unhex(row.at("runtime_hex"))),"Character authored runtime bank changed");
        const auto predictions=unhex(row.at("predictions_hex"));need(std::ranges::equal(out.slice(out.u32(48),out.u32(44)*8),predictions),"Character authored prediction bank changed");
        const auto fail=[&](const Fixture& bad,const char* message){need(!bad.run(),message);++rejects;};
        // Physical allocation words may legitimately point to another complete
        // identical owned bank. Reject unmapped pointers separately; every
        // scalar/SID/range/period/function/role byte remains an exact control.
        std::set<std::uint32_t> addresses;
        for(const auto& [first,raw]:value.regions)for(unsigned i=0;i<raw.size();++i)addresses.insert(first+i);
        std::set<std::uint32_t> source_pointers{value.source_stem.address+92,value.shader.address+16,value.shader.address+36,value.shader.address+48};
        const Bytes source_template(value.read(value.source_stem.address,96));
        for(unsigned platform=0;platform<3;++platform)for(unsigned field:{0u,8u,16u,24u,32u})
            source_pointers.insert(source_template.u32(92)+platform*40+field+4);
        const Bytes source_root(value.read(value.shader.address,84));
        const auto source_pp=source_root.u32(36);const Bytes source_definition(value.read(source_pp,124));
        for(const auto& [field,stride]:fields){(void)stride;source_pointers.insert(source_pp+field+4);}
        for(unsigned i=0;i<source_definition.u32(60);++i)source_pointers.insert(source_definition.u32(64)+i*20+16);
        for(const auto field:source_pointers){for(unsigned i=0;i<4;++i)addresses.erase(field+i);auto bad=value;bad.write_word(field,0xffffffffu);fail(bad,"Unowned character source pointer admitted");}
        for(const auto address:addresses){auto bad=value;bad.mutate(address);need(!bad.run(),"Changed complete character source byte admitted at "+std::to_string(address)+" for "+value.shader.name);++rejects;}
        for(unsigned i=0;i<value.payload.size();++i){auto bad=value;bad.payload[i]^=std::byte{1};fail(bad,"Changed opaque original native payload admitted");}
        for(const auto& [handle,_]:value.names){auto bad=value;bad.names[handle].group="invalid";fail(bad,"Changed complete typed namespace admitted");}
        for(const auto& [handle,_]:value.frames)if(value.names.contains(handle)){auto bad=value;bad.frames[handle]=0;fail(bad,"Absent actual source bitmap frame admitted");}
        for(const auto& [id,_]:value.strings){auto bad=value;bad.strings[id]="unowned-input";fail(bad,"Changed actual source StringID namespace admitted");}
        {auto bad=value;bad.source_stem.datum^=0x10000u;fail(bad,"Wrong source template salt admitted");}
        {auto bad=value;bad.native_stem.size+=1;bad.names[bad.native_stem.datum]=bad.native_stem;fail(bad,"Wrong native allocation extent admitted");}
        {auto same=value;same.shader.name="unrelated-authored-instance-name";same.names[same.shader.datum]=same.shader;need(bool(same.run()),"Character setup was replaced by material-name aliasing");}
    }
    const Json report={{"passed",true},{"profiles",2},{"complete_banks",banks_checked},{"authored_curves",curves_checked},{"exact_words",words_checked},{"strict_rejections",rejects}};
    std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
