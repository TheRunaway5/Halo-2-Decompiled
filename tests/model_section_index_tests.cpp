// The section index selects exactly what the linear scan selects, in order.
#include "halo2/model_section_index.hpp"
#include <cstdlib>
#include <iostream>
#include <random>
using namespace halo2;
static int failures=0;
static void need(bool ok,const char *what){if(!ok){++failures;std::cerr<<"FAIL: "<<what<<'\n';}}
int main() {
    std::mt19937 rng(7);
    RenderModel model;
    for(unsigned n=0;n<5000;++n) {
        Vertex v;v.source_section=rng()%7==0?0xffffffffu:rng()%23;v.source_part=n%3;
        v.position={float(n),float(n*2),float(n*3)};model.mesh.vertices.push_back(v);
    }
    model.checksum=0x1234;
    std::vector<Vertex> posed=model.mesh.vertices;
    for(auto &v:posed)v.position.x+=1;
    for(std::uint32_t section:{0u,1u,5u,22u,23u,0xffffffffu,99u}) {
        std::vector<Vertex> a,b;std::vector<std::uint32_t> oa,ob;
        for(std::uint32_t n=0;n<posed.size();++n)if(posed[n].source_section==section){a.push_back(posed[n]);oa.push_back(n);}
        const auto count=model_section_select(model,posed,section,b,&ob);
        need(count==a.size() && b.size()==a.size() && ob==oa,"index selection matches the scan");
        for(std::size_t i=0;i<a.size() && i<b.size();++i)need(a[i].position.x==b[i].position.x && a[i].source_part==b[i].source_part,"records come from posed in scan order");
    }
    // Appends after existing content, and reuses the published index.
    std::vector<Vertex> out(3);std::vector<std::uint32_t> ords{9};
    model_section_select(model,posed,3,out,&ords);
    need(out.size()>3 && ords.size()>1 && ords[0]==9,"selection appends");
    need(model.section_index.load()==model_section_index(model),"index published once per bank");
    RenderModel copy=model;
    need(!copy.section_index.load(),"a copy starts without an index");
    std::vector<Vertex> c;model_section_select(copy,posed,3,c);
    need(c.size()==out.size()-3,"copy rebuilds its own index");
    std::vector<Vertex> wrong;bool threw=false;
    try{model_section_select(model,std::span<const Vertex>(posed).first(10),3,wrong);}catch(const std::exception&){threw=true;}
    need(threw,"incomplete pose is rejected");
    if(failures)return 1;
    std::cout<<"model_section_index: all checks passed\n";return 0;
}
