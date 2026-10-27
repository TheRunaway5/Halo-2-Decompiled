#include "halo2/path_tracing/geometry.hpp"
#include <cfenv>
#include <cstring>
#include <iostream>
#include <stdexcept>

using namespace halo2;
namespace {
void need(bool value,const char* message) {if(!value)throw std::runtime_error(message);}
template<class F> void rejects(F&& function,const char* message) {
    try {function();}catch(const FormatError&){return;}
    throw std::runtime_error(message);
}
}
int main() {
    try {
        std::vector<Vertex> input(4);
        for(auto& vertex:input) {
            vertex.position={1,2,3};vertex.normal={0,0,1};
            vertex.tangent={1,0,0};vertex.binormal={0,1,0};
            vertex.material=27;vertex.lightmap=4;vertex.uv={.25f,.75f};
        }
        auto geometry=std::make_shared<ModelSkinGeometry>();
        geometry->vertices={input[1],input[2]};
        geometry->bindings.resize(2);geometry->source_vertices={0,1};
        geometry->palette.push_back({1,0,0,7,0,1,0,8,0,0,1,9});
        geometry->post_identity_placement=true;
        std::vector<ModelSkinSubmission> submissions{{1,geometry}};
        const auto original=input;
        const auto old_round=std::fegetround();std::fesetround(FE_DOWNWARD);
        const auto output=path_tracing::posed_vertices(input,submissions);
        need(std::fegetround()==FE_DOWNWARD,"Path Tracing geometry changed caller rounding mode");
        std::fesetround(old_round);
        need(output[1].position.x==8 && output[1].position.y==10 && output[1].position.z==12,
            "Path Tracing BVH received local rather than posed positions");
        need(output[2].normal.z==1 && output[1].tangent.x==1 && output[1].binormal.y==1,
            "Path Tracing skin changed the source basis");
        need(output[1].material==27 && output[1].lightmap==4 && output[1].uv.y==.75f,
            "Path Tracing skin lost material metadata");
        need(!std::memcmp(output.data(),input.data(),sizeof(Vertex)) &&
            !std::memcmp(&output[3],&input[3],sizeof(Vertex)),"Path Tracing skin touched undeclared ranges");
        need(!std::memcmp(original.data(),input.data(),input.size()*sizeof(Vertex)),
            "Path Tracing skin mutated its caller's bank");
        const auto copied=path_tracing::posed_vertices(input,{});
        need(!std::memcmp(copied.data(),input.data(),input.size()*sizeof(Vertex)),
            "Already posed geometry changed without a submission");
        submissions.push_back({2,geometry});
        rejects([&]{path_tracing::posed_vertices(input,submissions);},"Overlapping deferred owners were accepted");
        submissions={{3,geometry}};
        rejects([&]{path_tracing::posed_vertices(input,submissions);},"Out-of-range deferred owner was accepted");
        submissions={{0,{}}};
        rejects([&]{path_tracing::posed_vertices(input,submissions);},"Missing deferred owner was accepted");
        submissions={{1,geometry}};geometry->bindings.front().bones[0]=1;
        rejects([&]{path_tracing::posed_vertices(input,submissions);},"Out-of-range palette index was accepted");
        std::cout<<"Path Tracing current-pose geometry checks passed\n";
        return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
