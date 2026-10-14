#include "halo2/source_object_admission.hpp"
#include "halo2/source_object_admission_memory.hpp"
#include "halo2/source_object_spatial_pool.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace halo2;
namespace {
using Json=nlohmann::json;
void require(bool ok,const char* why){if(!ok)throw FormatError(why);}
float scalar(const Json& j){return std::bit_cast<float>(j.get<std::uint32_t>());}
ObjectLightSpatialSphere sphere(const Json& j){return {{scalar(j.at(0)),scalar(j.at(1)),scalar(j.at(2))},scalar(j.at(3))};}
SourceObjectAdmission queue(const Json& j){
    SourceObjectAdmission out;
    for(const auto& r:j)out.objects.push_back({r.at(0),r.at(1),r.at(3),r.at(2)});
    return out;
}
}
int main(int argc,char** argv)try {
    require(argc==2,"Supply original TU1.5 receiver admission fixture");
    std::ifstream file(argv[1]);if(!file)return 77;
    const auto j=Json::parse(file);
    require(j.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd",
        "Receiver fixture is not owned TU1.5");
    require(j.at("replaced_instructions")==0 && j.at("replaced_callees")==0,
        "Receiver fixture substitutes original execution");
    SourceObjectSpatialPool collidable(512),noncollidable(512,false);
    unsigned n=0;
    for(auto* p:{&collidable,&noncollidable}) {
        const auto& raw=j.at("pools").at(n++);
        p->heads=raw.at("heads").get<decltype(p->heads)>();
        p->entries.records=raw.at("records").get<decltype(p->entries.records)>();
    }
    std::vector<SourceObjectAdmissionObject> objects;
    for(const auto& o:j.at("objects"))objects.push_back({o.at("id"),o.at("flags"),
        std::int8_t(o.at("type").get<unsigned>()),sphere(o.at("rendering")),sphere(o.at("lighting"))});
    SourcePortalVisibilitySet visible;visible.groups.resize(j.at("groups"));
    for(const auto& e:j.at("entries")) {
        SourceClusterFrustumRanges r;r.source_cluster=e.at(0);
        for(unsigned k=0;k<6;++k){r.counts[k]=e.at(1+k);r.first_indices[k]=e.at(7+k);}
        visible.clusters.push_back(r);
    }
    const auto auxiliary=queue(j.at("auxiliary"));
    const auto fog=j.at("fog").get<std::vector<std::uint8_t>>();
    const auto prior=j.at("prior").get<std::vector<std::array<std::uint32_t,4>>>();
    std::vector<std::pair<std::uint32_t,std::vector<std::byte>>> banks;
    for(const auto& b:j.at("memory_banks")) {
        const auto hex=b.at("hex").get<std::string>();std::vector<std::byte> bytes;
        for(unsigned k=0;k<hex.size();k+=2)bytes.push_back(std::byte(std::stoul(hex.substr(k,2),nullptr,16)));
        banks.emplace_back(b.at("address"),std::move(bytes));
    }
    const auto roots=j.at("memory_roots").get<std::array<std::uint32_t,5>>();
    SourceObjectAdmissionMemory memory{roots[0],roots[1],roots[2],roots[3],roots[4],
        [&](std::uint32_t address,std::size_t size)->std::span<const std::byte>{
            for(const auto& [p,b]:banks)if(address>=p && std::uint64_t(address)+size<=std::uint64_t(p)+b.size())
                return std::span(b).subspan(address-p,size);
            throw FormatError("Receiver memory adapter consumed an uncaptured original bank");
        }};
    unsigned cases=0,words=0,records=0;
    for(const auto& c:j.at("cases")) {
        SourceObjectAdmissionInput in;in.capacity=j.at("capacity");in.flags=c.at("flags");
        in.context_2a78=j.at("context_2a78");in.mode=j.at("mode");
        const auto query=sphere(c.at("sphere"));in.sphere_center=query.center;in.sphere_radius=query.radius;
        for(unsigned k=0;k<4;++k)in.clipping_plane[k]=scalar(j.at("plane").at(k));
        in.containing_fog=fog;in.auxiliary=c.at("auxiliary").get<bool>()?&auxiliary:nullptr;
        const auto actual=source_object_admission(visible,collidable,noncollidable,objects,in,prior);
        const auto from_memory=source_object_admission_memory(visible,memory,in,prior);
        require(from_memory.objects.size()==actual.objects.size() && from_memory.associations==actual.associations,
            "Current memory admission differs from original spatial-bank projection");
        const auto expected=queue(c.at("queue"));
        if(actual.objects.size()!=expected.objects.size())throw FormatError("TU1.5 receiver count differs at case "+std::to_string(cases));
        for(unsigned k=0;k<actual.objects.size();++k) {
            const auto& a=actual.objects[k];const auto& e=expected.objects[k];
            const auto& m=from_memory.objects[k];
            require(a.id==m.id && a.flags==m.flags && a.other==m.other && a.association==m.association,
                "Current memory admission changed a receiver row");
            if(a.id!=e.id || a.flags!=e.flags || a.other!=e.other || a.association!=e.association) {
                std::cerr<<"case "<<cases<<" record "<<k<<" native "<<std::hex<<a.id<<' '<<a.flags<<' '<<a.association
                    <<" original "<<e.id<<' '<<e.flags<<' '<<e.association<<'\n';
                throw FormatError("TU1.5 receiver flags/order/association differ");
            }
        }
        require(actual.associations==c.at("associations").get<decltype(actual.associations)>(),
            "TU1.5 receiver shared association masks differ");
        words+=unsigned(2+actual.objects.size()*4+actual.associations.size()*4);
        records+=unsigned(actual.objects.size());++cases;
    }
    require(cases && records,"Receiver comparison did not consume actual objects");
    std::cout<<"PASS TU1.5 object light admission: "<<cases<<" queries, "<<records
        <<" records, "<<words<<" exact words over "<<objects.size()<<" actual owners\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
