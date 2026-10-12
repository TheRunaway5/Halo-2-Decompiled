#include "halo2/source_object_light_attribution.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>

using namespace halo2;
namespace {
using Words=std::vector<std::uint32_t>;
struct Reader {
    Words words;std::size_t at{};
    explicit Reader(std::string_view line) {
        std::istringstream in{std::string(line)};std::uint32_t value;
        while(in>>std::hex>>value)words.push_back(value);
        if(!in.eof())throw FormatError("invalid attribution fixture word");
    }
    std::uint32_t take() {
        if(at==words.size())throw FormatError("short attribution fixture");
        return words[at++];
    }
    float scalar(){return std::bit_cast<float>(take());}
    Vec3 point(){const auto x=scalar(),y=scalar(),z=scalar();return {x,y,z};}
    Transform transform(){const auto s=scalar();const auto a=point(),b=point(),c=point(),p=point();return {s,{a,b,c},p};}
    std::vector<std::byte> bytes() {
        const auto count=take();std::vector<std::byte> out;out.reserve(count);
        for(std::uint32_t i=0;i<(count+3)/4;++i) {
            const auto value=take();
            for(unsigned b=0;b<4 && out.size()<count;++b)out.push_back(std::byte(value>>(b*8)));
        }
        return out;
    }
    void done() const {if(at!=words.size())throw FormatError("extra attribution fixture words");}
};
struct Model {SourceObjectLightAttributionModel render;ModelCollision collision;};
struct Owner {std::uint32_t render{};std::vector<std::uint8_t> active;std::vector<Transform> world;};
Words result_words(const SourceObjectLightAttribution& out) {
    return {0xffffffffu,0xffffffffu,out.owner,out.render_model,0xffffffffu,out.section,
        0xffffffffu,std::uint32_t(out.region),out.part,out.strip,out.lightmap_triangle,out.node_hint,
        std::bit_cast<std::uint32_t>(out.weight_b),std::bit_cast<std::uint32_t>(out.weight_c)};
}
Model load_model(Reader& r) {
    Model model;auto& m=model.render;m.render_model=r.take();
    for(auto& v:m.position_bounds)v=r.scalar();
    for(auto n=r.take();n;--n)m.inverse_binds.push_back(r.transform());
    for(auto n=r.take();n;--n) {
        std::vector<std::int16_t> region;
        for(auto p=r.take();p;--p)region.push_back(std::int16_t(r.take()));
        m.highest_sections.push_back(std::move(region));
    }
    for(auto n=r.take();n;--n) {
        SourceObjectLightAttributionModel::Section section;
        section.resident=bool(r.take());section.positions.format=std::uint8_t(r.take());
        section.positions.vertex_count=r.take();section.positions.bytes=r.bytes();
        for(auto k=r.take();k;--k)section.indices.push_back(std::uint16_t(r.take()));
        for(auto v:r.bytes())section.leaves.nodes.push_back(std::to_integer<std::uint8_t>(v));
        for(auto k=r.take();k;--k) {
            ObjectLightRenderLeaves::NodeMap map;
            for(auto l=r.take();l;--l) {
                const auto cluster=std::int16_t(r.take()),count=std::int16_t(r.take());
                const auto first=std::int32_t(r.take());map.leaves.push_back({cluster,count,first});
            }
            for(auto f=r.take();f;--f) {
                const auto strip=std::uint16_t(r.take()),triangle=std::uint16_t(r.take());
                const auto node=std::int32_t(r.take());map.references.push_back({strip,triangle,node});
            }
            section.leaves.maps.push_back(std::move(map));
        }
        m.sections.push_back(std::move(section));
    }
    for(auto n=r.take();n;--n) {
        std::vector<ModelCollision::Permutation> region;
        for(auto p=r.take();p;--p) {
            ModelCollision::Permutation permutation;
            for(auto b=r.take();b;--b) {
                RawCollisionBsp bsp;bsp.leaves.resize(r.take());
                for(auto v=r.take();v;--v)
                    bsp.nodes.push_back({std::bit_cast<std::int16_t>(std::uint16_t(r.take())),{}});
                permutation.push_back(std::move(bsp));
            }
            region.push_back(std::move(permutation));
        }
        model.collision.regions.push_back(std::move(region));
    }
    r.done();return model;
}
PosedCollisionHit hit(const Words& packet) {
    PosedCollisionHit out;out.fraction=std::bit_cast<float>(packet[1]);
    out.plane={{std::bit_cast<float>(packet[5]),std::bit_cast<float>(packet[6]),
        std::bit_cast<float>(packet[7])},std::bit_cast<float>(packet[8])};
    out.region=std::bit_cast<std::int16_t>(std::uint16_t(packet[17]));
    out.node=std::bit_cast<std::int16_t>(std::uint16_t(packet[17]>>16));
    out.packed_bsp=packet[18];out.leaf=std::int32_t(packet[19]);out.surface=std::int32_t(packet[20]);
    out.signed_plane=std::bit_cast<std::int16_t>(std::uint16_t(packet[21]));return out;
}
void controls(const Model& original,const Owner& owner,const SourceObjectLightAttributionInput& actual) {
    const auto reject=[](const auto& operation) {
        try{operation();}catch(const FormatError&){return;}
        throw FormatError("incomplete object attribution bank silently admitted");
    };
    auto model=original.render;auto input=actual;
    auto active=owner.active;active[input.contact.region]=255;input.active_permutations=active;
    if(source_object_light_attribution(model,input))throw FormatError("source absent active region admitted");
    input=actual;model.highest_sections[input.contact.region][owner.active[input.contact.region]]=255;
    if(source_object_light_attribution(model,input))throw FormatError("source section255 admitted");
    model=original.render;
    const auto section=model.highest_sections[input.contact.region][owner.active[input.contact.region]];
    model.sections[section].resident=false;
    if(source_object_light_attribution(model,input))throw FormatError("source nonresident geometry admitted");
    model=original.render;input.collision=nullptr;
    if(source_object_light_attribution(model,input))throw FormatError("source missing collision model admitted");
    input=actual;input.active_permutations={};reject([&]{source_object_light_attribution(model,input);});
    input=actual;input.contact.node=std::int16_t(owner.world.size());reject([&]{source_object_light_attribution(model,input);});
    input=actual;model.sections[section].positions.bytes.clear();
    reject([&]{source_object_light_attribution(model,input);});
    model=original.render;model.sections[section].indices.clear();
    reject([&]{source_object_light_attribution(model,input);});
    const auto& stream=original.render.sections[section].positions;
    reject([&]{source_object_light_render_position(stream,model.position_bounds,stream.vertex_count);});
    auto unsupported=stream;unsupported.format=0;
    reject([&]{source_object_light_render_position(unsupported,model.position_bounds,0);});
}
} // namespace
int main(int argc,char** argv) {try {
    if(argc!=2)throw FormatError("attribution test requires untouched source fixture");
    std::ifstream file(argv[1]);if(!file)throw FormatError("attribution fixture missing");
    std::unordered_map<std::uint32_t,Model> models;std::unordered_map<std::uint32_t,Owner> owners;
    std::size_t cases=0,successes=0,position_cases=0,inverse_cases=0,exact_words=0;
    bool guarded=false;std::string line;
    while(std::getline(file,line)) {
        if(line.empty() || line[0]=='#')continue;
        Reader r{std::string_view(line).substr(2)};
        if(line[0]=='M') {
            auto model=load_model(r);const auto datum=model.render.render_model;
            if(!models.emplace(datum,std::move(model)).second)throw FormatError("duplicate attribution model");
        } else if(line[0]=='O') {
            const auto handle=r.take();Owner owner;owner.render=r.take();
            for(auto b:r.bytes())owner.active.push_back(std::to_integer<std::uint8_t>(b));
            for(auto n=r.take();n;--n)owner.world.push_back(r.transform());r.done();
            if(!owners.emplace(handle,std::move(owner)).second)throw FormatError("duplicate attribution owner");
        } else if(line[0]=='C') {
            const auto datum=r.take(),handle=r.take(),expected_hit=r.take();Words raw,expected;
            for(unsigned i=0;i<24;++i)raw.push_back(r.take());
            for(unsigned i=0;i<14;++i)expected.push_back(r.take());r.done();
            if(raw[0]!=4 || raw[16]!=handle)throw FormatError("attribution fixture contact identity differs");
            const auto& owner=owners.at(handle);const auto& model=models.at(datum);
            if(owner.render!=datum)throw FormatError("attribution owner/model bank mismatch");
            const SourceObjectLightAttributionInput input{handle,owner.active,owner.world,&model.collision,
                hit(raw),{std::bit_cast<float>(raw[2]),std::bit_cast<float>(raw[3]),std::bit_cast<float>(raw[4])}};
            const auto result=source_object_light_attribution(model.render,input);
            if(bool(result)!=bool(expected_hit))throw FormatError("original14A8E0 success/miss differs at case"+std::to_string(cases));
            ++exact_words;
            if(result) {
                if(result_words(*result)!=expected)throw FormatError("original14A8E0 attribution words differ at case"+std::to_string(cases));
                ++successes;exact_words+=14;
                if(!guarded){controls(model,owner,input);guarded=true;}
            }
            ++cases;
        } else if(line[0]=='P') {
            SourceObjectLightPositionStream stream;stream.format=std::uint8_t(r.take());std::array<float,6> bounds;
            for(auto& b:bounds)b=r.scalar();stream.vertex_count=r.take();stream.bytes=r.bytes();
            const Words expected{r.take(),r.take(),r.take(),r.take()};r.done();
            const auto out=source_object_light_render_position(stream,bounds,0);
            const Words actual{std::bit_cast<std::uint32_t>(out.point.x),std::bit_cast<std::uint32_t>(out.point.y),
                std::bit_cast<std::uint32_t>(out.point.z),out.node_hint};
            if(actual!=expected)throw FormatError("original23AC30 CPU position differs at case"+std::to_string(position_cases));
            ++position_cases;exact_words+=4;
        } else if(line[0]=='I') {
            const auto transform=r.transform();const auto p=r.point();const Words expected{r.take(),r.take(),r.take()};r.done();
            const auto out=source_object_light_inverse_point(transform,p);
            const Words actual{std::bit_cast<std::uint32_t>(out.x),std::bit_cast<std::uint32_t>(out.y),std::bit_cast<std::uint32_t>(out.z)};
            if(actual!=expected)throw FormatError("original142700 inverse point differs at case"+std::to_string(inverse_cases));
            ++inverse_cases;exact_words+=3;
        } else throw FormatError("unknown attribution fixture record");
    }
    if(models.size()!=3 || owners.size()!=5 || cases!=65 || successes!=8 ||
       position_cases!=512 || inverse_cases!=142 || !guarded)throw FormatError("attribution fixture coverage differs");
    std::cout<<"original14A8E0: "<<models.size()<<" literal models, "<<owners.size()<<" current owners, "<<cases
        <<" complete calls, "<<successes<<" resolved/"<<(cases-successes)<<" source misses; "
        <<position_cases<<" CPU positions, "<<inverse_cases<<" inverse points, "<<exact_words
        <<" exact source words; missing-input guards pass\n";
    return 0;
} catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
