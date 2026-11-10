#include "halo2/source_object_admission.hpp"
#include <iostream>
#include <bit>
#include <string>
#include <utility>
using namespace halo2;
namespace association_reference {
SourceBspAdmission bsp(const SourceBspAdmissionBsp&,const SourcePortalVisibilitySet&,const SourceBspAdmissionInput&);
SourceObjectAdmission objects(const SourcePortalVisibilitySet&,const ObjectLightSpatialLists&,const ObjectLightSpatialLists&,
    std::span<const SourceObjectAdmissionObject>,const SourceObjectAdmissionInput&,std::span<const std::array<std::uint32_t,4>>);
}
namespace {
using Words=std::vector<std::uint32_t>;
void record(Words&words,const SourceBspAdmissionRecord&r){words.insert(words.end(),{r.id,r.flags,r.association,r.other});}
void masks(Words&words,const auto&out){words.push_back(out.association_capacity_reached);words.push_back(out.associations.size());for(const auto&row:out.associations)words.insert(words.end(),row.begin(),row.end());}
Words encode(const SourceBspAdmission&out){Words words;for(auto v:out.cluster_entries)words.push_back(v);words.insert(words.end(),out.marked_clusters.begin(),out.marked_clusters.end());for(const auto*queue:{&out.clusters,&out.instances}){words.push_back(queue->size());for(const auto&r:*queue)record(words,r);}masks(words,out);return words;}
Words encode(const SourceObjectAdmission&out){Words words{std::uint32_t(out.objects.size())};for(const auto&r:out.objects)record(words,r);masks(words,out);return words;}
template<class Function>auto outcome(Function&&function){try{return std::pair{encode(function()),std::string{}};}catch(const std::exception&error){return std::pair{Words{},std::string(error.what())};}}
unsigned comparisons=0,errors=0,saturations=0;
template<class Native,class Reference>void compare(Native&&native,Reference&&reference){const auto a=outcome(native),b=outcome(reference);if(a!=b)throw FormatError("whole public geometry predecessor mismatch "+std::to_string(comparisons));++comparisons;errors+=!a.second.empty();}
SourcePortalVisibilitySet visible(unsigned count){SourcePortalVisibilitySet set;set.groups.resize(1);set.frustums.resize(1);auto&f=set.frustums[0];f.bounds={-10,10,-10,10,-10,10};for(unsigned p=0;p<6;++p){f.planes[p]={0,0,0,9};f.planes[p][p/2]=p%2?-1.f:1.f;}set.clusters.resize(count);for(auto&c:set.clusters){c.source_cluster=0;c.counts[0]=1;}return set;}
std::pair<Vec3,float> sphere(unsigned variant){const std::array<std::uint32_t,12>values{0,0x80000000u,0x3f800000u,0x40200000u,0x40200001u,0x40400000u,1u,0x80000001u,0x7fc01234u,0x7fc05678u,0x7f800000u,0xff800000u};return{{std::bit_cast<float>(values[variant%12]),0,0},std::bit_cast<float>(values[(variant/12)%12])};}

void bsp_case(unsigned count,unsigned ordinals,unsigned capacity,unsigned options,unsigned variant=0){
    auto bank=visible(ordinals);SourceBspAdmissionBsp bsp;bsp.clusters.resize(1);bsp.definitions.push_back({1});bsp.instances.resize(count);
    for(unsigned i=0;i<count;++i){const auto [center,radius]=variant?sphere(variant):std::pair{Vec3{0,0,0},2.f};bsp.instances[i]={0,center,radius};bsp.clusters[0].instances.push_back(i);}bsp.clusters[0].geometry_count=1;
    const std::array<std::uint8_t,1> fog{std::uint8_t(options&1u)};SourceBspAdmissionInput input;input.containing_fog=fog;input.mode=variant?variant%2:1;input.sphere_radius=1;input.instance_capacity=capacity;
    if(options&2u){input.flags=0x200;input.clipping_plane={1,0,0,1};}if(options&4u)input.flags|=0x20;
    if(options&8u)input.flags|=0x80;if(options&16u)input.mode=2;
    compare([&]{return source_bsp_admission(bsp,bank,input);},[&]{return association_reference::bsp(bsp,bank,input);});
    if(count>512&&capacity>513&&ordinals&&!(options&20u)){const auto result=source_bsp_admission(bsp,bank,input);if(!result.association_capacity_reached||result.associations.size()!=512)throw FormatError("BSP capacity branch was not exercised");++saturations;}
}
void object_case(unsigned count,unsigned ordinals,unsigned prior_count,unsigned capacity,unsigned options,unsigned variant=0){
    auto bank=visible(ordinals);ObjectLightSpatialLists first(1),second(1);std::vector<SourceObjectAdmissionObject> objects(count);const std::array<std::int16_t,1> memberships{0};
    for(unsigned i=0;i<count;++i){auto&o=objects[i];o.full_handle=0x10000+i;o.flags=0x80000000u;o.type=i%8;const auto [center,radius]=variant?sphere(variant):std::pair{Vec3{0,0,0},2.f};o.rendering_sphere={center,radius};
        if(options&32u)o.lighting_sphere=ObjectLightSpatialSphere{{1,0,0},2};
        (i&1?first:second).insert(o.full_handle,{o.type,0,{0,0,0},2},memberships);}
    std::vector<std::array<std::uint32_t,4>> prior(prior_count);for(unsigned i=0;i<prior_count;++i)prior[i]={i,~i,0x12345678u,0x87654321u};
    const std::array<std::uint8_t,1> fog{std::uint8_t(options&1u)};SourceObjectAdmissionInput input;input.containing_fog=fog;input.mode=variant?variant%2:1;input.sphere_radius=1;input.capacity=capacity;
    if(options&2u){input.flags=0x200;input.clipping_plane={1,0,0,1};}if(options&4u)input.flags|=0x40;if(options&8u)input.flags|=0x400;
    if(options&16u)input.sky_cluster=0;if(options&32u)input.context_2a78=1;if(options&64u)input.flags|=2;
    if(options&128u)input.context_2a78=1; // actual missing retained lighting-sphere error
    compare([&]{return source_object_admission(bank,first,second,objects,input,prior);},[&]{return association_reference::objects(bank,first,second,objects,input,prior);});
    if(count>512&&capacity>513&&ordinals&&prior_count<=512&&options==0){const auto result=source_object_admission(bank,first,second,objects,input,prior);if(!result.association_capacity_reached||result.associations.size()!=512)throw FormatError("object capacity branch was not exercised");++saturations;}
}
}
int main(){try{
    for(unsigned count:{0u,1u,2u,7u,31u,127u,511u,512u,513u,600u})for(unsigned ordinals:{0u,1u,2u,31u,32u,33u,64u,95u,127u,128u})for(unsigned capacity:{1u,2u,16u,1024u})bsp_case(count,ordinals,capacity,0);
    for(unsigned count:{0u,1u,7u,513u})for(unsigned ordinals:{0u,1u,2u,32u,33u,64u,128u})for(unsigned prior:{0u,1u,511u,512u,513u})for(unsigned capacity:{1u,2u,1024u})object_case(count,ordinals,prior,capacity,0);
    for(unsigned options=0;options<256;++options){bsp_case(7,3,16,options&31u);object_case(7,3,options%3==0?512:0,16,options);}
    for(unsigned variant=1;variant<288;++variant){bsp_case(3,2,16,variant%8,variant);object_case(3,2,0,16,variant%16,variant);}
    if(errors<100||saturations<10)throw FormatError("public association coverage incomplete");
    std::cout<<"{\"comparisons\":"<<comparisons<<",\"exact_errors\":"<<errors<<",\"saturation_witnesses\":"<<saturations<<"}\n";return 0;
}catch(const std::exception&error){std::cerr<<error.what()<<'\n';return 1;}}
