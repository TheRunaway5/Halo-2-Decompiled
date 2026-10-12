#include "halo2/source_object_spatial_query.hpp"
#include "halo2/source_control_target.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
using namespace halo2;
namespace {
using Words=std::vector<std::uint32_t>;
void require(bool v,const char *s){if(!v)throw FormatError(s);}
std::uint32_t hash(std::span<const std::uint32_t> words) {
    std::uint32_t h=2166136261u;
    for(const auto w:words)for(unsigned b=0;b<4;++b){h^=std::uint8_t(w>>(b*8));h*=16777619u;}
    return h;
}
Words run(const Words &input) {
    std::size_t at=0;
    const auto word=[&](){require(at<input.size(),"truncated spatial query input");return input[at++];};
    const auto scalar=[&](){return std::bit_cast<float>(word());};
    const auto point=[&](){return Vec3{scalar(),scalar(),scalar()};};
    SourceObjectSpatialQueryInput q;q.lists=word();q.types=word();q.location.cluster=std::int16_t(word());
    q.center=point();q.radius=scalar();q.capacity=std::uint16_t(word());
    SourceObjectSpatialQueryState state;state.object_epoch=word();state.cluster_epoch=word();
    state.objects.fill(word());state.clusters.fill(word());
    ObjectLightSpatialBsp bsp;const auto count=word(),planes=word(),portals=word();
    require(count<=512 && planes<=512 && portals<=2048,"oversized spatial query fixture");
    bsp.clusters.resize(count);
    for(unsigned n=0;n<planes;++n){std::array<float,4> p;for(auto &v:p)v=scalar();bsp.planes.push_back(p);}
    for(unsigned n=0;n<portals;++n){ObjectLightSpatialBsp::Portal p;const auto sides=word();
        p.back=std::int16_t(sides);p.front=std::int16_t(sides>>16);p.plane=word();p.center=point();p.radius=scalar();
        const auto vertices=word();require(vertices<=128,"oversized spatial portal fixture");
        for(unsigned i=0;i<vertices;++i)p.vertices.push_back(point());bsp.portals.push_back(p);}
    for(auto &c:bsp.clusters){const auto n=word();require(n<=portals,"invalid fixture cluster links");
        for(unsigned i=0;i<n;++i)c.push_back(std::uint16_t(word()));}
    const auto read_pool=[&](bool collidable){
        auto p=std::make_unique<SourceObjectSpatialPool>(count,collidable);unsigned slot=0;
        for(auto &head:p->heads){const auto n=word();require(n<=2048-slot,"oversized fixture spatial pool");
            if(n)head=0x80000000u|slot;
            for(unsigned i=0;i<n;++i,++slot){auto &r=p->entries.records[slot];r[0]=0x8000u;r[1]=word();
                r[2]=i+1<n?(0x80000000u|(slot+1)):0xffffffffu;
                for(unsigned k=3;k<8;++k)r[k]=word();}}
        return p;
    };
    const auto collide=read_pool(true),noncollide=read_pool(false);
    auto n=word();require(n<=2048,"oversized object stamp fixture");
    for(unsigned i=0;i<n;++i){const auto slot=word();require(slot<2048,"invalid object stamp fixture");state.objects[slot]=word();}
    n=word();require(n<=512,"oversized cluster stamp fixture");
    for(unsigned i=0;i<n;++i){const auto slot=word();require(slot<512,"invalid cluster stamp fixture");state.clusters[slot]=word();}
    require(at==input.size(),"extra spatial query input");
    const auto result=source_object_spatial_query(bsp,*collide,*noncollide,q,state);
    Words output{std::uint32_t(result.size())};output.insert(output.end(),result.begin(),result.end());
    output.insert(output.end(),{state.object_epoch,state.cluster_epoch,hash(state.objects),hash(state.clusters)});
    return output;
}
Words parse(const std::string &line){std::istringstream s(line);Words w;std::uint32_t v;
    while(s>>std::hex>>v)w.push_back(v);require(s.eof(),"invalid spatial query fixture word");return w;}
}
int main(int argc,char **argv){try{
    if(argc==2 && std::string_view(argv[1])=="--probe") {
        std::string line;while(std::getline(std::cin,line)){
            const auto output=run(parse(line));for(const auto w:output)std::cout<<std::hex<<w<<' ';std::cout<<'\n';}
        return 0;
    }
    if(argc==3 && std::string_view(argv[1])=="--fixture") {
        std::ifstream file(argv[2]);require(bool(file),"spatial query fixture missing");std::string line;unsigned cases=0;
        while(std::getline(file,line)){
            if(line.empty() || line[0]=='#')continue;
            const auto separator=line.find('|');require(separator!=line.npos,"spatial query fixture lacks output");
            require(run(parse(line.substr(0,separator)))==parse(line.substr(separator+1)),"original spatial query mismatch");++cases;}
        require(cases>0,"empty spatial query fixture");std::cout<<cases<<" original spatial query cases matched\n";return 0;
    }
    ObjectLightSpatialBsp bsp;bsp.clusters.resize(1);
    auto a=std::make_unique<SourceObjectSpatialPool>(1),b=std::make_unique<SourceObjectSpatialPool>(1,false);
    std::uint32_t head=~0u;const std::array<std::int16_t,1> members{0};
    a->insert(0x10001,head,{8,0,{.5f,0,0},.2f},members);
    SourceObjectSpatialQueryInput in;in.location.cluster=0;in.radius=1;in.types=0xffffebcf;
    SourceObjectSpatialQueryState state;
    require(source_object_spatial_query(bsp,*a,*b,in,state)==Words{0x10001},"control absent from source spatial query");
    in.capacity=0;require(source_object_spatial_query(bsp,*a,*b,in,state).empty() && state.objects[1]==state.object_epoch,
        "full query did not stamp eligible candidate before capacity return");
    in.location.cluster=-1;const auto epoch=state.cluster_epoch;
    require(source_object_spatial_query(bsp,*a,*b,in,state).empty() && state.cluster_epoch==epoch,
        "NONE location invented spatial membership");
    in.location.cluster=1;bool rejected=false;
    try{source_object_spatial_query(bsp,*a,*b,in,state);}catch(const FormatError&){rejected=true;}
    require(rejected,"out-of-bank query location accepted");
    SourceControlTargetQuery target;target.unit_bound_radius=.5f;
    const SourceObjectLocation location{~0u,0,0};
    require(source_control_spatial_candidates(target,location,bsp,*a,*b,state)==Words{0x10001},
        "source interaction did not consume real spatial pool control");
    target.unit_parent=0x10004;const auto before=state.object_epoch;
    require(source_control_spatial_candidates(target,location,bsp,*a,*b,state).empty() && state.object_epoch==before,
        "parented interaction advanced the spatial query owner");
    std::cout<<"spatial query order, capacity stamping and location guards passed\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
