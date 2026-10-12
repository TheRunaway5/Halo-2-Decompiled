#include "halo2/source_object_render_entry.hpp"
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
using namespace halo2;
namespace {
void require(bool value,const char* why){if(!value)throw FormatError(why);}
struct Reader {
    std::span<const std::uint32_t> data;std::size_t at{};
    std::uint32_t word(){require(at<data.size(),"object render entry fixture truncated");return data[at++];}
    float scalar(){return std::bit_cast<float>(word());}
    Vec3 vector(){return {scalar(),scalar(),scalar()};}
    void done(){require(at==data.size(),"object render entry trailing fixture words");}
};
void append(std::vector<std::uint32_t>& out,const SourceObjectDrawCollector& c) {
    const auto begin=out.size();out.resize(begin+c.bytes.size()/4);
    std::memcpy(out.data()+begin,c.bytes.data(),c.bytes.size());
}
void load(Reader& r,SourceObjectDrawCollector& c) {
    for(unsigned i=0;i<c.bytes.size()/4;++i){const auto v=r.word();std::memcpy(c.bytes.data()+i*4,&v,4);}
}
std::vector<std::uint32_t> solve(std::span<const std::uint32_t> input,unsigned& boundaries) {
    Reader r{input};SourceBspAdmissionRecord row;row.id=r.word();row.flags=r.word();row.association=r.word();
    SourceObjectRenderEntryRequest q;q.alternate=r.word();q.section_counter=r.word();q.section_group=r.word();q.children=r.word();
    SourceObjectRenderEntryContext c;c.mode=r.word();c.eye=r.vector();c.distance_scale=r.scalar();c.coverage_multiplier=r.scalar();
    for(auto& f:c.coverage.inverse_view_z)f=r.scalar();c.coverage.viewport_height_metric=r.scalar();
    c.flags.query_flags=r.word();c.flags.fog_classification=std::bit_cast<std::int32_t>(r.word());c.flags.planar_datum=r.word();c.flags.eye_fraction=r.scalar();c.flags.planar_only=r.word()!=0;c.flags.planar_flags=std::uint8_t(r.word());
    const SourceObjectRenderSphere sphere{r.vector(),r.scalar()};const auto modifier=std::uint8_t(r.word());
    std::vector<SourceModelFadeOwner> owners(r.word());
    for(auto& o:owners){o.identity=r.word();o.flags=r.word();o.parent=r.word();o.definition=r.word();o.radius=r.scalar();}
    std::vector<SourceModelFadeDefinition> definitions(r.word());
    for(auto& d:definitions){d.identity=r.word();d.model=r.word();}
    std::vector<SourceModelFadeModel> models(r.word());
    for(auto& m:models){m.identity=r.word();m.fade_end=r.scalar();m.fade_start=r.scalar();for(auto& f:m.render_cutoffs)f=r.scalar();m.render_lod_count=std::uint16_t(r.word());}
    SourceObjectDrawCollector collector;load(r,collector);
    std::vector<std::vector<std::uint32_t>> events(r.word());
    for(auto& e:events){e.resize(r.word());for(auto& w:e)w=r.word();}r.done();unsigned used=0;
    const auto event=[&](const std::vector<std::uint32_t>& actual)->Reader {
        require(used<events.size(),"object render entry extra owner boundary");const auto& expected=events[used++];
        for(unsigned i=0;i<actual.size();++i)if(i>=expected.size() || actual[i]!=expected[i])
            throw FormatError("object render entry event"+std::to_string(used-1)+" word"+std::to_string(i)+" got"+std::to_string(actual[i])+" expected"+(i<expected.size()?std::to_string(expected[i]):"absent"));
        return {std::span(expected).subspan(actual.size())};
    };
    SourceObjectRenderEntryServices s;
    s.sphere=[&](std::uint32_t owner){require(owner==row.id,"object render entry sphere owner differs");return sphere;};
    s.fade=[&](std::uint32_t owner,float metric) {
        auto e=event({2,owner,std::bit_cast<std::uint32_t>(metric)});
        const auto fade=source_model_fade(owners,definitions,models,{owner,metric,modifier});
        require(e.word()==fade.first && e.word()==fade.second,"object render entry joined native4BAF0 differs");e.done();return fade;
    };
    s.build=[&](SourceObjectDrawCollector& out,const SourceObjectBuildRequest& a) {
        std::vector<std::uint32_t> values{0,a.owner,a.scalar_1b8,a.scalar_1b4,a.scalar_1a8,a.mode,a.flags,
            std::bit_cast<std::uint32_t>(a.center.x),std::bit_cast<std::uint32_t>(a.center.y),std::bit_cast<std::uint32_t>(a.center.z)};
        append(values,out);auto e=event(values);load(e,out);e.done();
    };
    s.present=[&](SourceObjectDrawCollector& out,std::uint32_t children,std::uint32_t skip,std::uint32_t flags) {
        std::vector<std::uint32_t> values{1,children,skip,flags};append(values,out);
        auto e=event(values);load(e,out);e.done();
    };
    const auto result=source_object_render_entry(row,collector,q,c,s);
    require(used==events.size(),"object render entry omitted owner boundary");boundaries+=used;
    std::vector<std::uint32_t> out{result,row.flags};append(out,collector);return out;
}
}
int main(int argc,char** argv)try {
    const char* path=argc>1?argv[1]:HALO2_SOURCE_DIR "/tests/fixtures/source_object_render_entry.original.bin";
    std::ifstream file(path,std::ios::binary|std::ios::ate);require(bool(file),"object render entry original fixture missing");const auto bytes=file.tellg();require(bytes>=12 && bytes%4==0,"object render entry fixture extent");
    std::vector<std::uint32_t> data(std::size_t(bytes)/4);file.seekg(0);require(bool(file.read(reinterpret_cast<char*>(data.data()),bytes)),"object render entry fixture read");
    Reader r{data};require(r.word()==0x454e5253 && r.word()==1,"object render entry fixture schema");const auto count=r.word();std::uint64_t checked=0;unsigned boundaries=0;
    for(unsigned i=0;i<count;++i) {
        const auto ni=r.word(),no=r.word();require(r.at+ni+no<=data.size(),"object render entry case extent");
        std::vector<std::uint32_t> actual;
        try{actual=solve(std::span(data).subspan(r.at,ni),boundaries);}catch(const std::exception& e){throw FormatError("object render entry case"+std::to_string(i)+": "+e.what());}
        r.at+=ni;require(actual.size()==no,"object render entry output extent differs");
        for(unsigned n=0;n<no;++n)if(actual[n]!=r.data[r.at+n])throw FormatError("object render entry case"+std::to_string(i)+" output word"+std::to_string(n)+" differs");
        r.at+=no;checked+=no;
    }
    r.done();std::cout<<"PASS "<<count<<" whole original133790 calls / "<<checked<<" retained words / "<<boundaries<<" ordered boundaries; native coverage/distance/1332F0/4BAF0;4B5D0/133AB0 remain explicit record services\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
