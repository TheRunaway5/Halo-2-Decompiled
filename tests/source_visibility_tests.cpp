#include "halo2/source_visibility.hpp"
#include <bit>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {
void require(bool value,const char* message){if(!value)throw FormatError(message);}
float read_float(std::istream& input){std::uint32_t bits;input>>bits;return std::bit_cast<float>(bits);}
SourceSphereFrustum read_frustum(std::istream& input){
    SourceSphereFrustum f;
    for(auto& value:f.bounds)value=read_float(input);
    for(auto& plane:f.planes)for(auto& value:plane)value=read_float(input);
    return f;
}
void sphere_probe(std::istream& input,std::ostream& output){
    while(input.peek()!=EOF){auto f=read_frustum(input);if(!input)break;
        Vec3 center{read_float(input),read_float(input),read_float(input)};const auto radius=read_float(input);
        output<<unsigned(source_sphere_frustum_visibility(f,center,radius))<<'\n';input>>std::ws;
    }
}
void cluster_probe(std::istream& input,std::ostream& output){
    int group_count;unsigned clusters,frustums,section;
    while(input>>group_count>>clusters>>frustums>>section){
        SourceClusterFrustumSet set;set.group_count=std::int16_t(group_count);set.clusters.resize(clusters);
        for(auto& entry:set.clusters){input>>entry.source_cluster;
            for(auto& value:entry.counts)input>>value;
            for(auto& value:entry.first_indices)input>>value;}
        for(unsigned i=0;i<frustums;++i)set.frustums.push_back(read_frustum(input));
        Vec3 center{read_float(input),read_float(input),read_float(input)};const auto radius=read_float(input);
        const auto result=source_cluster_frustum_visibility(set,section,center,radius);
        output<<unsigned(result.visible)<<' '<<unsigned(result.contained)<<'\n';
    }
}
}
int main(int argc,char**argv){try{
    if(argc==2 && std::string_view(argv[1])=="--sphere-probe"){sphere_probe(std::cin,std::cout);return 0;}
    if(argc==2 && std::string_view(argv[1])=="--cluster-probe"){cluster_probe(std::cin,std::cout);return 0;}
    std::ifstream input("tests/fixtures/retail_sphere_frustum.tsv");require(bool(input),"missing original181800 fixtures");
    unsigned fixtures=0,expected;
    while(input.peek()!=EOF){auto f=read_frustum(input);if(!input)break;
        Vec3 center{read_float(input),read_float(input),read_float(input)};const auto radius=read_float(input);input>>expected;
        require(bool(input)&&unsigned(source_sphere_frustum_visibility(f,center,radius))==expected,"original sphere/frustum classification differs");
        ++fixtures;input>>std::ws;
    }
    require(fixtures==512,"invalid original181800 fixture count");
    std::ifstream clusters("tests/fixtures/retail_cluster_frustums.tsv");require(bool(clusters),"missing original165010 fixtures");
    fixtures=0;int group_count;unsigned count,frustums,section;
    while(clusters>>group_count>>count>>frustums>>section){
        SourceClusterFrustumSet set;set.group_count=std::int16_t(group_count);set.clusters.resize(count);
        for(auto& entry:set.clusters){clusters>>entry.source_cluster;
            for(auto& value:entry.counts)clusters>>value;
            for(auto& value:entry.first_indices)clusters>>value;}
        for(unsigned i=0;i<frustums;++i)set.frustums.push_back(read_frustum(clusters));
        Vec3 center{read_float(clusters),read_float(clusters),read_float(clusters)};const auto radius=read_float(clusters);
        unsigned visible,contained;clusters>>visible>>contained;
        const auto result=source_cluster_frustum_visibility(set,section,center,radius);
        require(bool(clusters)&&result.visible==bool(visible)&&result.contained==bool(contained),"original cluster frustum aggregation differs");
        ++fixtures;
    }
    require(clusters.eof()&&fixtures==256,"invalid original165010 fixture count");
    SourceClusterFrustumSet unresolved;unresolved.group_count=1;unresolved.clusters.resize(1);unresolved.clusters[0].counts[0]=1;
    bool rejected=false;try{source_cluster_frustum_visibility(unresolved,0,{},1);}catch(const FormatError&){rejected=true;}
    require(rejected,"missing authored frustum must remain unresolved");
    std::cout<<"512 original sphere classifications and 256 cluster frustum aggregations match\n";return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}
