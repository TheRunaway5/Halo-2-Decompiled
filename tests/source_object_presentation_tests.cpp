#include "halo2/source_object_presentation.hpp"
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
    std::uint32_t word(){require(cursor<words.size(),"object presentation fixture truncated");return words[cursor++];}
    float real(){return std::bit_cast<float>(word());}
    void done(){require(cursor==words.size(),"object presentation fixture trailing words");}
};
void read_collector(Reader& r,SourceObjectDrawCollector& c) {
    for(unsigned i=0;i<c.bytes.size()/4;++i){const auto w=r.word();std::memcpy(c.bytes.data()+i*4,&w,4);}
}
void append(std::vector<std::uint32_t>& out,const SourceObjectDrawCollector& c) {
    for(unsigned i=0;i<c.bytes.size()/4;++i){std::uint32_t w;std::memcpy(&w,c.bytes.data()+i*4,4);out.push_back(w);}
}
void read_state(Reader& r,SourceDrawUnitState& s) {
    s.main_prefix=r.word();s.draw_count=r.word();s.override_count=r.word();s.section_highwater=r.word();
    for(auto& unit:s.units)for(auto& w:unit.words)w=r.word();
    for(auto& v:s.coverage)v=std::uint8_t(r.word());for(auto& v:s.override_slots)v=std::uint8_t(r.word());
    for(auto& v:s.overrides)v=r.word();
    for(auto& section:s.sections){std::array<std::uint32_t,9> raw;for(auto& w:raw)w=r.word();std::memcpy(&section,raw.data(),36);}
    s.arena.used=std::uint16_t(r.word());for(auto& v:s.arena.words)v=r.word();
}
void append(std::vector<std::uint32_t>& out,const SourceDrawUnitState& s) {
    out.insert(out.end(),{s.main_prefix,s.draw_count,s.override_count,s.section_highwater});
    for(const auto& unit:s.units)out.insert(out.end(),unit.words.begin(),unit.words.end());
    out.insert(out.end(),s.coverage.begin(),s.coverage.end());out.insert(out.end(),s.override_slots.begin(),s.override_slots.end());out.insert(out.end(),s.overrides.begin(),s.overrides.end());
    for(const auto& section:s.sections){std::array<std::uint32_t,9> raw;std::memcpy(raw.data(),&section,36);out.insert(out.end(),raw.begin(),raw.end());}
    out.push_back(s.arena.used);out.insert(out.end(),s.arena.words.begin(),s.arena.words.end());
}
std::vector<std::uint32_t> solve(std::span<const std::uint32_t> input,unsigned& checked_calls,unsigned version) {
    Reader r{input};const auto children=r.word(),skip=r.word(),flags=r.word();
    SourceObjectPresentationContext ctx;
    ctx.eye={r.real(),r.real(),r.real()};ctx.forward={r.real(),r.real(),r.real()};ctx.mode=r.word();
    SourceObjectDrawCollector c;c.address=0x71000000;read_collector(r,c);
    SourceObjectDrawContext draw;SourceDrawUnitState state;
    std::map<std::uint32_t,std::array<std::byte,0x78>> contexts;
    if(version>=2) {
        draw.draw.mode=r.word();draw.draw.mask_arena_address=r.word();draw.draw.force_outer=r.word()!=0;draw.draw.force_inner=r.word()!=0;
        std::array<std::uint32_t,48> camera;for(auto& w:camera)w=r.word();std::memcpy(&draw.rendering,camera.data(),sizeof(draw.rendering));
        draw.eye={r.real(),r.real(),r.real()};for(auto& v:draw.viewport)v=std::bit_cast<std::int16_t>(std::uint16_t(r.word()));
        draw.temporary_address=0x73000000;read_state(r,state);
        const auto count=r.word();
        for(unsigned i=0;i<count;++i){const auto token=r.word();auto& bytes=contexts[token];for(unsigned n=0;n<30;++n){const auto w=r.word();std::memcpy(bytes.data()+n*4,&w,4);}}
    }
    std::map<std::uint32_t,std::uint32_t> counters;const auto count=r.word();
    for(unsigned i=0;i<count;++i){const auto token=r.word();counters[token]=r.word();}
    SourceDrawResourceWorld world;world.objects=r.word();world.tag_table=r.word();
    if(version>=2){world.bsp=r.word();world.lightmaps=r.word();}
    std::map<std::uint32_t,std::vector<std::byte>> memory;const auto segments=r.word();
    for(unsigned i=0;i<segments;++i) {
        const auto address=r.word(),extent=r.word();auto& raw=memory[address];raw.resize(std::size_t(extent)*4);
        for(unsigned n=0;n<extent;++n){const auto w=r.word();std::memcpy(raw.data()+n*4,&w,4);}
    }
    world.read=[&](std::uint32_t address,std::size_t extent)->std::span<const std::byte> {
        auto it=memory.upper_bound(address);require(it!=memory.begin(),"object presentation resource read absent");--it;
        const auto offset=std::size_t(address-it->first);
        if(offset>it->second.size() || extent>it->second.size()-offset)
            throw FormatError("object presentation resource read missing at"+std::to_string(address));
        return std::span(it->second).subspan(offset,extent);
    };
    std::vector<std::vector<std::uint32_t>> calls(r.word());
    for(auto& call:calls){call.resize(r.word());for(auto& w:call)w=r.word();}
    std::vector<std::array<std::uint32_t,3>> requests;unsigned consumed_requests=0;
    if(version>=2){requests.resize(r.word());for(auto& row:requests)for(auto& w:row)w=r.word();}r.done();
    world.request=[&](std::uint32_t mode,std::uint32_t address) {
        require(consumed_requests<requests.size(),"object presentation extra12DE70 request");
        const auto row=requests[consumed_requests++];require(row[0]==mode && row[1]==address,"object presentation12DE70 inputs/order differ");return row[2]!=0;
    };
    auto resources=source_object_draw_resource_services(world);
    resources.draw.context=[&](std::uint32_t pointer)->std::span<std::byte> {
        const auto it=contexts.find(pointer);require(it!=contexts.end(),"object presentation joined context absent");return it->second;
    };
    auto s=source_object_presentation_resource_services(world);unsigned consumed=0;
    const auto compare=[&](const std::vector<std::uint32_t>& actual)->std::span<const std::uint32_t> {
        require(consumed<calls.size(),"object presentation extra source call");const auto& expected=calls[consumed++];
        if(expected.size()<actual.size()||!std::equal(actual.begin(),actual.end(),expected.begin())) {
            unsigned at=0;while(at<std::min(expected.size(),actual.size()) && actual[at]==expected[at])++at;
            throw FormatError("object presentation call"+std::to_string(consumed-1)+" word"+std::to_string(at)+
                " got"+(at<actual.size()?std::to_string(actual[at]):"missing")+
                " expected"+(at<expected.size()?std::to_string(expected[at]):"absent"));
        }
        return std::span(expected).subspan(actual.size());
    };
    s.section_counter=[&](std::uint32_t address)->std::uint32_t& {
        const auto it=counters.find(address);require(it!=counters.end(),"object presentation counter identity differs");return it->second;
    };
    std::uint32_t identity=0x72000000;s.copy_identity=[&](){const auto token=identity;identity+=0x200;return token;};
    s.collect=[&](const SourceObjectDrawCollector& collector,std::uint32_t filter) {
        std::vector<std::uint32_t> actual{1,filter};append(actual,collector);
        require(compare(actual).empty(),"object presentation collect boundary extent");
        if(version>=2)source_object_collect_draw_units(state,collector,filter,draw,resources);
    };
    s.build=[&](SourceObjectDrawCollector& collector,const SourceObjectBuildRequest& q) {
        std::vector<std::uint32_t> actual{2,q.owner,q.scalar_1b8,q.scalar_1b4,q.scalar_1a8,q.mode,q.flags,
            std::bit_cast<std::uint32_t>(q.center.x),std::bit_cast<std::uint32_t>(q.center.y),std::bit_cast<std::uint32_t>(q.center.z)};
        append(actual,collector);const auto result=compare(actual);require(result.size()>=121,"object presentation build boundary extent");
        std::memcpy(collector.bytes.data(),result.data(),collector.bytes.size());
        Reader patch{result.subspan(121)};
        if(version>=2) {
            const auto count=patch.word();
            for(unsigned i=0;i<count;++i){const auto token=patch.word();const auto it=contexts.find(token);require(it!=contexts.end(),"object presentation produced context absent");for(unsigned n=0;n<30;++n){const auto w=patch.word();std::memcpy(it->second.data()+n*4,&w,4);}}
        }
        patch.done();
    };
    source_object_present(c,children,skip,flags,ctx,s);
    require(consumed==calls.size(),"object presentation omitted original call");checked_calls+=consumed;
    require(consumed_requests==requests.size(),"object presentation omitted12DE70 request");
    std::vector<std::uint32_t> out;append(out,c);out.push_back(counters.size());
    for(const auto& [token,value]:counters)out.insert(out.end(),{token,value});
    if(version>=2) {
        append(out,state);out.push_back(contexts.size());
        for(const auto& [token,bytes]:contexts){out.push_back(token);std::array<std::uint32_t,30> raw;std::memcpy(raw.data(),bytes.data(),bytes.size());out.insert(out.end(),raw.begin(),raw.end());}
    }
    return out;
}
}
int main(int argc,char** argv)try {
    const char* path=argc>1?argv[1]:HALO2_SOURCE_DIR "/tests/fixtures/source_object_presentation.original.bin";
    std::ifstream f(path,std::ios::binary|std::ios::ate);require(bool(f),"object presentation original fixture missing");
    const auto size=f.tellg();require(size>=12 && size%4==0,"object presentation fixture size");
    std::vector<std::uint32_t> data(std::size_t(size)/4);f.seekg(0);require(bool(f.read(reinterpret_cast<char*>(data.data()),size)),"object presentation fixture read");
    Reader r{data};require(r.word()==0x50524f53,"object presentation fixture magic");const auto version=r.word();require(version==1 || version==2,"object presentation fixture schema");
    const auto count=r.word();unsigned calls=0;std::uint64_t checked=0;
    for(unsigned i=0;i<count;++i) {
        const auto ni=r.word(),no=r.word();require(std::uint64_t(r.cursor)+ni+no<=data.size(),"object presentation case extent");
        std::vector<std::uint32_t> result;
        try{result=solve(r.words.subspan(r.cursor,ni),calls,version);}catch(const std::exception& e){throw FormatError("object presentation case"+std::to_string(i)+": "+e.what());}
        r.cursor+=ni;require(result.size()==no,"object presentation output size differs");
        for(unsigned j=0;j<no;++j)if(result[j]!=r.words[r.cursor+j])throw FormatError("object presentation case"+std::to_string(i)+" word"+std::to_string(j)+" differs");
        r.cursor+=no;checked+=no;
    }
    r.done();std::cout<<"PASS "<<count<<" whole original133AB0 cases / "<<checked<<" exact retained words / "<<calls<<" ordered callee boundaries; fixture2 joins native133F70/2CBF0/44940/2D000/449E0/30290;4B5D0 production/publication and gameplay remain explicit\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
