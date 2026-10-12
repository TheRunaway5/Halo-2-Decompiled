#include "halo2/source_object_draw_collector.hpp"
#include "halo2/source_draw_resources.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
using namespace halo2;
namespace {
void require(bool value,const char* why){if(!value)throw FormatError(why);}
struct Reader {
    std::span<const std::uint32_t> words;unsigned cursor{};
    std::uint32_t word(){require(cursor<words.size(),"object collector fixture truncated");return words[cursor++];}
    float real(){return std::bit_cast<float>(word());}
    void done(){require(cursor==words.size(),"object collector fixture trailing words");}
};
void read_state(Reader& r,SourceDrawUnitState& s) {
    s.main_prefix=r.word();s.draw_count=r.word();s.override_count=r.word();s.section_highwater=r.word();
    for(auto& unit:s.units)for(auto& w:unit.words)w=r.word();
    for(auto& v:s.coverage)v=std::uint8_t(r.word());for(auto& v:s.override_slots)v=std::uint8_t(r.word());
    for(auto& v:s.overrides)v=r.word();
    for(auto& section:s.sections){std::array<std::uint32_t,9> raw;for(auto& w:raw)w=r.word();std::memcpy(&section,raw.data(),36);}
    s.arena.used=std::uint16_t(r.word());for(auto& v:s.arena.words)v=r.word();
}
void emit(std::vector<std::uint32_t>& out,const SourceDrawUnitState& s) {
    out.insert(out.end(),{s.main_prefix,s.draw_count,s.override_count,s.section_highwater});
    for(const auto& unit:s.units)out.insert(out.end(),unit.words.begin(),unit.words.end());
    out.insert(out.end(),s.coverage.begin(),s.coverage.end());out.insert(out.end(),s.override_slots.begin(),s.override_slots.end());out.insert(out.end(),s.overrides.begin(),s.overrides.end());
    for(const auto& section:s.sections){std::array<std::uint32_t,9> raw;std::memcpy(raw.data(),&section,36);out.insert(out.end(),raw.begin(),raw.end());}
    out.push_back(s.arena.used);out.insert(out.end(),s.arena.words.begin(),s.arena.words.end());
}
std::vector<std::uint32_t> solve(std::span<const std::uint32_t> input,unsigned& checked_services,
    unsigned version,unsigned& checked_requests) {
    Reader r{input};SourceObjectDrawCollector collector;SourceObjectDrawContext context;
    collector.address=r.word();context.temporary_address=r.word();const auto filter=r.word();
    context.draw.mode=r.word();context.draw.mask_arena_address=r.word();context.draw.force_outer=r.word()!=0;context.draw.force_inner=r.word()!=0;
    for(unsigned i=0;i<collector.bytes.size()/4;++i){const auto w=r.word();std::memcpy(collector.bytes.data()+i*4,&w,4);}
    std::array<std::uint32_t,48> camera_words;for(auto& w:camera_words)w=r.word();std::memcpy(&context.rendering,camera_words.data(),sizeof(context.rendering));
    context.eye={r.real(),r.real(),r.real()};for(auto& v:context.viewport)v=std::bit_cast<std::int16_t>(std::uint16_t(r.word()));
    std::map<std::uint32_t,SourceObjectDrawOwner> owners;const auto owner_count=r.word();
    for(unsigned i=0;i<owner_count;++i){const auto token=r.word();owners.emplace(token,SourceObjectDrawOwner{std::uint8_t(r.word()),r.word()});}
    std::map<std::uint32_t,bool> policies;const auto policy_count=r.word();
    for(unsigned i=0;i<policy_count;++i){const auto token=r.word();policies.emplace(token,r.word()!=0);}
    SourceDrawUnitState state;read_state(r,state);
    std::map<std::uint32_t,std::array<std::byte,0x78>> contexts;const auto context_count=r.word();
    for(unsigned i=0;i<context_count;++i){const auto pointer=r.word();auto& bytes=contexts[pointer];for(unsigned n=0;n<30;++n){const auto w=r.word();std::memcpy(bytes.data()+n*4,&w,4);}}
    std::vector<std::vector<std::uint32_t>> calls(r.word());for(auto& call:calls){call.resize(r.word());for(auto& w:call)w=r.word();}
    SourceDrawResourceWorld world;std::map<std::uint32_t,std::vector<std::byte>> memory;
    std::vector<std::array<std::uint32_t,3>> requests;unsigned consumed_requests=0;
    if(version>=2) {
        world.tag_table=r.word();world.bsp=r.word();world.lightmaps=r.word();world.objects=r.word();
        const auto segments=r.word();
        for(unsigned i=0;i<segments;++i) {
            const auto address=r.word(),count=r.word();auto& bytes=memory[address];bytes.resize(std::size_t(count)*4);
            for(unsigned n=0;n<count;++n){const auto word=r.word();std::memcpy(bytes.data()+n*4,&word,4);}
        }
        requests.resize(r.word());for(auto& row:requests)for(auto& word:row)word=r.word();
        world.read=[&](std::uint32_t address,std::size_t extent)->std::span<const std::byte> {
            auto it=memory.upper_bound(address);require(it!=memory.begin(),"object collector native resource read absent");--it;
            const auto offset=std::size_t(address-it->first);
            if(offset>it->second.size() || extent>it->second.size()-offset)
                throw FormatError("object collector native resource read missing at"+std::to_string(address));
            return std::span(it->second).subspan(offset,extent);
        };
        world.request=[&](std::uint32_t mode,std::uint32_t address) {
            require(consumed_requests<requests.size(),"object collector extra12DE70 resource request");
            const auto row=requests[consumed_requests++];require(row[0]==mode && row[1]==address,"object collector12DE70 input/order differs");return row[2]!=0;
        };
    }
    r.done();const auto resources=source_object_draw_resource_services(world);
    unsigned consumed=0;
    const auto consume=[&](std::initializer_list<std::uint32_t> arguments,const SourceCullDrawUnit* unit=nullptr)->std::span<const std::uint32_t>{
        require(consumed<calls.size(),"object collector consumed extra source service");const auto& call=calls[consumed++];
        std::vector<std::uint32_t> actual(arguments);if(unit)actual.insert(actual.end(),unit->words.begin(),unit->words.end());
        if(call.size()<actual.size() || !std::equal(actual.begin(),actual.end(),call.begin())) {
            unsigned at=0;while(at<std::min(call.size(),actual.size()) && call[at]==actual[at])++at;
            throw FormatError("object collector source service"+std::to_string(consumed-1)+" input"+std::to_string(at)+" differs");
        }
        return std::span(call).subspan(actual.size());
    };
    SourceObjectDrawServices services;
    services.draw.geometry=[&](std::uint16_t unit,std::uint32_t selector,const SourceCullDrawUnit& record){const auto result=consume({0x44940,unit,selector},&record);require(result.size()==(version>=2?3:2),"object collector metadata fixture result");const auto actual=version>=2?resources.draw.geometry(unit,selector,record):SourceDrawUnitGeometry{std::uint16_t(result[0]),std::uint16_t(result[1])};require(actual.classification==result[0] && actual.subpart_count==result[1],"object collector native44940 metadata differs");if(version>=2)require(source_draw_geometry_address(record,selector,world)==result[2],"object collector native44940 address differs");return actual;};
    services.draw.lighting=[&](std::uint32_t owner,std::uint32_t geometry,std::uint32_t instance){const auto result=consume({0x2d000,owner,geometry,instance});require(result.size()==1,"object collector lighting fixture result");const auto actual=version>=2?resources.draw.lighting(owner,geometry,instance):result[0];require(actual==result[0],"object collector native2D000 classification differs");return actual;};
    services.draw.resident=[&](std::uint16_t unit,std::uint32_t selector,bool bsp,const SourceCullDrawUnit& record){const auto result=consume({0x449e0,unit,selector,std::uint32_t(bsp)},&record);require(result.size()==1,"object collector resource fixture result");const auto actual=version>=2?source_draw_resident_geometry(record,selector,bsp,world):result[0];require(actual==result[0],"object collector native449E0 payload differs");return actual!=0;};
    services.draw.context=[&](std::uint32_t pointer)->std::span<std::byte>{const auto found=contexts.find(pointer);require(found!=contexts.end(),"object collector context token differs");return found->second;};
    services.owner=[&](std::uint32_t owner){const auto found=owners.find(owner);require(found!=owners.end(),"object collector body token differs");const auto actual=version>=2?resources.owner(owner):found->second;require(actual.kind==found->second.kind && actual.definition==found->second.definition,"object collector native body lookup differs");return actual;};
    services.weapon_section_policy=[&](std::uint32_t definition){const auto found=policies.find(definition);require(found!=policies.end(),"object collector weapon policy token differs");const auto actual=version>=2?resources.weapon_section_policy(definition):found->second;require(actual==found->second,"object collector native weapon policy differs");return actual;};
    const auto before=collector.bytes;
    source_object_collect_draw_units(state,collector,filter,context,services);
    require(collector.bytes==before,"object collector changed caller record");
    require(consumed==calls.size(),"object collector omitted source services");checked_services+=consumed;
    require(consumed_requests==requests.size(),"object collector omitted12DE70 requests");checked_requests+=consumed_requests;
    std::vector<std::uint32_t> out;emit(out,state);out.push_back(context_count);
    for(const auto& [pointer,bytes]:contexts){out.push_back(pointer);std::array<std::uint32_t,30> raw;std::memcpy(raw.data(),bytes.data(),bytes.size());out.insert(out.end(),raw.begin(),raw.end());}
    return out;
}
}
int main(int argc,char** argv)try {
    const char* path=argc>1?argv[1]:HALO2_SOURCE_DIR "/tests/fixtures/source_object_draw_collector.original.bin";
    std::ifstream f(path,std::ios::binary|std::ios::ate);require(bool(f),"object collector original fixture missing");const auto size=f.tellg();require(size>=12 && size%4==0,"object collector fixture size");std::vector<std::uint32_t> data(std::size_t(size)/4);f.seekg(0);require(bool(f.read(reinterpret_cast<char*>(data.data()),size)),"object collector fixture read");
    Reader r{data};require(r.word()==0x434f444f,"object collector fixture magic");const auto version=r.word();require(version==1 || version==2,"object collector fixture schema");const auto count=r.word();unsigned calls=0,requests=0;std::uint64_t checked=0;
    for(unsigned i=0;i<count;++i){const auto ni=r.word(),no=r.word();require(std::uint64_t(r.cursor)+ni+no<=data.size(),"object collector fixture case extent");std::vector<std::uint32_t> result;try{result=solve(r.words.subspan(r.cursor,ni),calls,version,requests);}catch(const std::exception& e){throw FormatError("object collector case"+std::to_string(i)+": "+e.what());}r.cursor+=ni;require(result.size()==no,"object collector output size differs");for(unsigned j=0;j<no;++j)if(result[j]!=r.words[r.cursor+j])throw FormatError("object collector case"+std::to_string(i)+" word"+std::to_string(j)+" got"+std::to_string(result[j])+" expected"+std::to_string(r.words[r.cursor+j]));r.cursor+=no;checked+=no;}
    r.done();std::cout<<"PASS "<<count<<" whole original133F70/2CBF0 collector cases / "<<checked<<" exact words / "<<calls<<" ordered services / "<<requests<<" original12DE70 requests; fixture2 uses native body/weapon/44940/2D000/449E0/30290; upstream4B5D0, publication and live frame delivery separate\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
