#include "halo2/source_draw_unit.hpp"
#include "halo2/source_sphere_rectangle.hpp"
#include "halo2/source_draw_resources.hpp"
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
    std::uint32_t word(){require(cursor<words.size(),"draw allocator fixture truncated");return words[cursor++];}
    float real(){return std::bit_cast<float>(word());}
    void done(){require(cursor==words.size(),"draw allocator fixture trailing words");}
};
void state_read(Reader& r,SourceDrawUnitState& s) {
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
    Reader r{input};const bool begin=r.word()!=0;SourceDrawUnitContext context;
    context.mode=r.word();context.mask_arena_address=r.word();context.force_outer=r.word()!=0;context.force_inner=r.word()!=0;
    SourceDrawUnitInput draw;draw.geometry=r.word();draw.instance=r.word();draw.distance_byte=r.word();for(auto& w:draw.stack)w=r.word();
    const Vec3 center{r.real(),r.real(),r.real()};
    std::array<std::uint32_t,48> camera_words;for(auto& w:camera_words)w=r.word();SourceViewProjectionBank camera;std::memcpy(&camera,camera_words.data(),sizeof(camera));
    const Vec3 eye{r.real(),r.real(),r.real()};std::array<std::int16_t,4> viewport;for(auto& v:viewport)v=std::bit_cast<std::int16_t>(std::uint16_t(r.word()));
    SourceDrawUnitState state;state_read(r,state);
    std::map<std::uint32_t,std::array<std::byte,0x78>> contexts;const auto context_count=r.word();
    for(unsigned i=0;i<context_count;++i){const auto pointer=r.word();auto& bytes=contexts[pointer];for(unsigned n=0;n<30;++n){const auto w=r.word();std::memcpy(bytes.data()+n*4,&w,4);}}
    std::vector<std::vector<std::uint32_t>> calls(r.word());for(auto& call:calls){call.resize(r.word());for(auto& w:call)w=r.word();}
    SourceDrawResourceWorld world;
    std::map<std::uint32_t,std::vector<std::byte>> memory;
    std::vector<std::array<std::uint32_t,3>> requests;unsigned consumed_requests=0;
    if(version>=3) {
        world.tag_table=r.word();world.bsp=r.word();world.lightmaps=r.word();world.objects=r.word();
        const auto segments=r.word();
        for(unsigned i=0;i<segments;++i) {
            const auto address=r.word(),count=r.word();auto& bytes=memory[address];bytes.resize(std::size_t(count)*4);
            for(unsigned n=0;n<count;++n){const auto word=r.word();std::memcpy(bytes.data()+n*4,&word,4);}
        }
        requests.resize(r.word());for(auto& row:requests)for(auto& word:row)word=r.word();
        world.read=[&](std::uint32_t address,std::size_t extent)->std::span<const std::byte> {
            auto it=memory.upper_bound(address);require(it!=memory.begin(),"draw allocator native resource read absent");--it;
            const auto offset=std::size_t(address-it->first);
            if(offset>it->second.size() || extent>it->second.size()-offset)
                throw FormatError("draw allocator native resource read missing at"+std::to_string(address));
            return std::span(it->second).subspan(offset,extent);
        };
        world.request=[&](std::uint32_t mode,std::uint32_t address) {
            require(consumed_requests<requests.size(),"draw allocator extra12DE70 resource request");
            const auto row=requests[consumed_requests++];require(row[0]==mode && row[1]==address,"draw allocator12DE70 input/order differs");return row[2]!=0;
        };
    }
    r.done();const auto resources=source_draw_resource_services(world);
    unsigned consumed=0;
    const auto consume=[&](std::initializer_list<std::uint32_t> arguments,const SourceCullDrawUnit* unit=nullptr)->std::span<const std::uint32_t>{
        require(consumed<calls.size(),"draw allocator consumed extra source service");const auto& call=calls[consumed++];
        std::vector<std::uint32_t> actual(arguments);if(unit)actual.insert(actual.end(),unit->words.begin(),unit->words.end());
        require(call.size()>=actual.size() && std::equal(actual.begin(),actual.end(),call.begin()),"draw allocator source service inputs/order differ");
        return std::span(call).subspan(actual.size());
    };
    SourceDrawUnitServices services;
    services.geometry=[&](std::uint16_t unit,std::uint32_t selector,const SourceCullDrawUnit& record){const auto result=consume({0x44940,unit,selector},&record);require(result.size()==(version>=3?3:2),"draw allocator metadata fixture result");const auto actual=version>=3?resources.geometry(unit,selector,record):SourceDrawUnitGeometry{std::uint16_t(result[0]),std::uint16_t(result[1])};require(actual.classification==result[0] && actual.subpart_count==result[1],"draw allocator native44940 metadata differs");if(version>=3)require(source_draw_geometry_address(record,selector,world)==result[2],"draw allocator native44940 address differs");return actual;};
    services.lighting=[&](std::uint32_t owner,std::uint32_t geometry,std::uint32_t instance){const auto result=consume({0x2d000,owner,geometry,instance});require(result.size()==1,"draw allocator lighting fixture result");const auto actual=version>=3?resources.lighting(owner,geometry,instance):result[0];require(actual==result[0],"draw allocator native2D000 classification differs");return actual;};
    services.resident=[&](std::uint16_t unit,std::uint32_t selector,bool bsp,const SourceCullDrawUnit& record){const auto result=consume({0x449e0,unit,selector,std::uint32_t(bsp)},&record);require(result.size()==1,"draw allocator resource fixture result");const auto actual=version>=3?source_draw_resident_geometry(record,selector,bsp,world):result[0];require(actual==result[0],"draw allocator native449E0 payload pointer differs");return actual!=0;};
    services.context=[&](std::uint32_t pointer)->std::span<std::byte>{const auto found=contexts.find(pointer);require(found!=contexts.end(),"draw allocator context token differs");return found->second;};
    services.center=[&](std::uint32_t pointer){require(pointer==draw.stack[19],"draw allocator section center token differs");return center;};
    services.rectangle=[&](std::uint32_t pointer,float radius){const auto result=consume({0x30290,pointer,std::bit_cast<std::uint32_t>(radius)});require(result.size()==4 && pointer==draw.stack[19],"draw allocator rectangle fixture result");const auto out=source_sphere_rectangle(camera,eye,viewport,center,radius);for(unsigned i=0;i<4;++i)require(std::bit_cast<std::uint32_t>(out[i])==result[i],"draw allocator native30290 result differs");return out;};
    std::int16_t result=-1;
    if(begin)source_draw_units_begin(state,context.mode);else result=source_draw_unit_append(state,draw,context,services);
    require(consumed==calls.size(),"draw allocator omitted source services");checked_services+=consumed;
    require(consumed_requests==requests.size(),"draw allocator omitted12DE70 requests");checked_requests+=consumed_requests;
    std::vector<std::uint32_t> out;emit(out,state);out.push_back(context_count);
    for(const auto& [pointer,bytes]:contexts){out.push_back(pointer);std::array<std::uint32_t,30> raw;std::memcpy(raw.data(),bytes.data(),bytes.size());out.insert(out.end(),raw.begin(),raw.end());}
    out.push_back(std::uint16_t(result));return out;
}
}
int main(int argc,char** argv)try {
    const char* path=argc>1?argv[1]:HALO2_SOURCE_DIR "/tests/fixtures/source_draw_unit.original.bin";
    std::ifstream f(path,std::ios::binary|std::ios::ate);require(bool(f),"draw allocator original fixture missing");const auto size=f.tellg();require(size>=12 && size%4==0,"draw allocator fixture size");std::vector<std::uint32_t> data(std::size_t(size)/4);f.seekg(0);require(bool(f.read(reinterpret_cast<char*>(data.data()),size)),"draw allocator fixture read");
    Reader r{data};require(r.word()==0x55443248,"draw allocator fixture magic");const auto version=r.word();require(version==2 || version==3,"draw allocator fixture schema");const auto count=r.word();unsigned calls=0,requests=0;std::uint64_t checked=0;
    for(unsigned i=0;i<count;++i){const auto ni=r.word(),no=r.word();require(std::uint64_t(r.cursor)+ni+no<=data.size(),"draw allocator fixture case extent");std::vector<std::uint32_t> result;try{result=solve(r.words.subspan(r.cursor,ni),calls,version,requests);}catch(const std::exception& e){throw FormatError("draw allocator case"+std::to_string(i)+": "+e.what());}r.cursor+=ni;require(result.size()==no,"draw allocator output size differs");for(unsigned j=0;j<no;++j)if(result[j]!=r.words[r.cursor+j])throw FormatError("draw allocator case"+std::to_string(i)+" word"+std::to_string(j)+" got"+std::to_string(result[j])+" expected"+std::to_string(r.words[r.cursor+j]));r.cursor+=no;checked+=no;}
    r.done();std::cout<<"PASS "<<count<<" whole original2CB30/2CBF0 bank cases / "<<checked<<" exact words / "<<calls<<" ordered services / "<<requests<<" original12DE70 requests; fixture3 uses native44940/2D000/449E0/30290; platform publication and live collector production separate\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
