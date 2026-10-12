#include "halo2/source_object_posed_target.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_map>
using namespace halo2;
namespace {
struct Reader {
    std::vector<std::uint32_t> values;std::size_t at{};
    explicit Reader(std::string_view line){std::istringstream in{std::string(line)};std::uint32_t v;while(in>>std::hex>>v)values.push_back(v);if(!in.eof())throw FormatError("invalid posed fixture word");}
    std::uint32_t word(){if(at==values.size())throw FormatError("short posed fixture");return values[at++];}
    float real(){return std::bit_cast<float>(word());}
    Vec3 point(){const auto x=real(),y=real(),z=real();return{x,y,z};}
    Transform transform(){const auto s=real();const auto a=point(),b=point(),c=point(),p=point();return{s,{a,b,c},p};}
    std::vector<std::byte> raw(std::size_t n){std::vector<std::byte> out;out.reserve(n);for(std::size_t i=0;i<n/4;++i){const auto w=word();for(unsigned b=0;b<4;++b)out.push_back(std::byte(w>>(8*b)));}return out;}
    void done(){if(at!=values.size())throw FormatError("extra posed fixture words");}
};
std::uint8_t byte(const Bytes& b,std::size_t p){return std::to_integer<std::uint8_t>(b.slice(p,1)[0]);}
std::int16_t sh(const Bytes& b,std::size_t p){return std::bit_cast<std::int16_t>(b.u16(p));}
Vec3 point(const Bytes& b,std::size_t p){return{b.f32(p),b.f32(p+4),b.f32(p+8)};}
std::int32_t child(const Bytes& b,std::size_t p){const auto v=std::uint32_t(byte(b,p))|(std::uint32_t(byte(b,p+1))<<8)|(std::uint32_t(byte(b,p+2))<<16);return std::bit_cast<std::int32_t>(v|(v&0x800000u?0xff000000u:0));}
RawCollisionBsp load_bsp(Reader& r){
    RawCollisionBsp out;out.node=std::int16_t(r.word());
    constexpr std::size_t strides[]{8,16,4,4,16,8,12,16};
    for(unsigned k=0;k<8;++k){const auto n=r.word();const auto raw=r.raw(n*strides[k]);const Bytes b(raw);
        for(std::size_t i=0;i<n;++i){const auto p=i*strides[k];switch(k){
            case 0:out.nodes.push_back({sh(b,p),{child(b,p+2),child(b,p+5)}});break;
            case 1:out.planes.push_back({point(b,p),b.f32(p+12)});break;
            case 2:out.leaves.push_back({byte(b,p),byte(b,p+1),b.u16(p+2)});break;
            case 3:out.references.push_back({sh(b,p),sh(b,p+2)});break;
            case 4:out.nodes2.push_back({{b.f32(p),b.f32(p+4)},b.f32(p+8),{sh(b,p+12),sh(b,p+14)}});break;
            case 5:out.surfaces.push_back({sh(b,p),b.u16(p+2),byte(b,p+4),byte(b,p+5),sh(b,p+6)});break;
            case 6:out.edges.push_back({{b.u16(p),b.u16(p+2)},{b.u16(p+4),b.u16(p+6)},{b.u16(p+8),b.u16(p+10)}});break;
            case 7:out.vertices.push_back(point(b,p));break;
        }}
    }return out;
}
ModelCollision load_model(Reader& r){
    ModelCollision m;m.model=r.word();m.collision=r.word();
    for(auto n=r.word();n;--n){const auto d=std::int16_t(r.word()),g=std::int16_t(r.word());m.materials.push_back({d,g});}
    for(auto n=r.word();n;--n){ModelCollision::Region region{std::uint8_t(r.word()),{}};for(auto p=r.word();p;--p)region.permutations.push_back(std::uint8_t(r.word()));m.model_regions.push_back(std::move(region));}
    for(auto n=r.word();n;--n){std::vector<ModelCollision::Permutation> region;for(auto p=r.word();p;--p){ModelCollision::Permutation permutation;for(auto b=r.word();b;--b)permutation.push_back(load_bsp(r));region.push_back(std::move(permutation));}m.regions.push_back(std::move(region));}
    return m;
}
struct Owner {std::uint32_t model{};std::vector<Transform> nodes;std::vector<std::uint8_t> active;};
std::vector<std::uint32_t> packet(const PosedCollisionHit& h){return{
    std::bit_cast<std::uint32_t>(h.fraction),std::bit_cast<std::uint32_t>(h.plane.normal.x),std::bit_cast<std::uint32_t>(h.plane.normal.y),std::bit_cast<std::uint32_t>(h.plane.normal.z),std::bit_cast<std::uint32_t>(h.plane.distance),h.packed_bsp,
    std::uint32_t(h.leaf),std::uint32_t(h.surface),std::uint32_t(h.signed_plane),std::uint32_t(h.region),std::uint32_t(h.node),std::uint32_t(h.model_material),std::uint32_t(h.global_material),h.flags,h.breakable};}
void require(bool ok,const std::string& why){if(!ok)throw FormatError(why);}
}
int main(){try{
    std::unordered_map<std::uint32_t,ModelCollision> models;std::unordered_map<std::uint32_t,Owner> owners;std::size_t queries=0,hits=0,exact=0,lookup_misses=0;
    std::ifstream file(std::string(HALO2_SOURCE_DIR)+"/tests/fixtures/retail_source_object_posed_target.tsv");require(bool(file),"missing original posed target fixture");
    for(std::string line;std::getline(file,line);){if(line.empty()||line[0]=='#')continue;Reader r(std::string_view(line).substr(2));
        if(line[0]=='M'){auto m=load_model(r);const auto id=m.model;models.emplace(id,std::move(m));}
        else if(line[0]=='O'){const auto h=r.word();Owner o;o.model=r.word();for(auto n=r.word();n;--n)o.nodes.push_back(r.transform());for(auto n=r.word();n;--n)o.active.push_back(std::uint8_t(r.word()));owners.emplace(h,std::move(o));}
        else if(line[0]=='N'){r.word();++lookup_misses;}
        else if(line[0]=='Q'||line[0]=='X'){const auto h=r.word(),flags=r.word();const auto origin=r.point(),delta=r.point();const auto& o=owners.at(h);
            const ModelCollision* model=&models.at(o.model);ModelCollision controlled;
            if(line[0]=='X'){const auto region=r.word(),permutation=r.word(),bsp=r.word(),surface=r.word(),plane=r.word();controlled=*model;
                controlled.regions.at(region).at(permutation).at(bsp).surfaces.at(surface).plane=std::int16_t(plane);model=&controlled;}
            const auto expected=r.word();
            const auto result=source_object_posed_target(*model,o.nodes,std::span<const std::uint8_t>(o.active),origin,delta,flags);
            const auto label=std::to_string(queries)+" owner"+std::to_string(h);require(bool(result)==bool(expected),label+" hit");++exact;
            if(result){const auto words=packet(*result);for(std::size_t i=0;i<words.size();++i){const auto v=r.word();require(v==words[i],label+" field"+std::to_string(i)+" expected"+std::to_string(v)+" actual"+std::to_string(words[i]));++exact;}++hits;}++queries;
        }else throw FormatError("unknown posed target fixture row");r.done();
    }
    require(queries>0 && hits>0,"empty original posed target cases");std::cout<<queries<<" complete original posed targets / "<<hits<<" hits / "<<exact<<" exact words / "<<models.size()<<" literal models / "<<owners.size()<<" current owners / "<<lookup_misses<<" lookup misses recorded\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
