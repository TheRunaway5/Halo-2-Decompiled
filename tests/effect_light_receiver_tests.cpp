#include "halo2/effect_light_receiver.hpp"
#include "halo2/bsp_atmosphere.hpp"
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>

using namespace halo2;
namespace {
void require(bool value,const char* message){if(!value)throw FormatError(message);}
class Fixture {
    std::vector<std::byte> bytes;
    std::size_t cursor{};
public:
    explicit Fixture(const char* path) {
        std::ifstream in(path,std::ios::binary|std::ios::ate);
        require(bool(in),"receiver fixture missing");bytes.resize(std::size_t(in.tellg()));
        in.seekg(0);in.read(reinterpret_cast<char*>(bytes.data()),std::streamsize(bytes.size()));
        require(bool(in),"receiver fixture truncated");
    }
    template<class T>T read() {
        require(cursor+sizeof(T)<=bytes.size(),"receiver fixture read exceeds file");
        T out;std::memcpy(&out,bytes.data()+cursor,sizeof(T));cursor+=sizeof(T);return out;
    }
    std::uint32_t word(){return read<std::uint32_t>();}
    void align(){cursor=(cursor+3)&~std::size_t(3);}
    bool done() const{return cursor==bytes.size();}
};
}
int main(int argc,char** argv) {try {
    require(argc==3,"receiver cache and original fixture required");
    Cache cache(argv[1]);const auto spatial=cache.load_object_light_spatial_bsp(0);
    const auto geometry=cache.load_source_bsp_admission(0);
    const auto locations=cache.load_bsp_atmosphere(0);
    Fixture file(argv[2]);require(file.word()==0x3152434c,"receiver fixture version");
    const auto cases=file.word(),clusters=file.word(),camera_words=file.word();
    const auto bsp=std::int16_t(file.word());const auto fog_datum=file.word(),fog_flags=file.word();
    require(clusters==geometry.clusters.size()&&camera_words==6*0x1bc/4,"receiver original input bank shape");
    const auto cameras=file.read<std::array<SourceFrustumCamera,6>>();
    std::vector<std::uint8_t> fog;for(unsigned i=0;i<clusters;++i)fog.push_back(file.read<std::uint8_t>());file.align();
    require(fog==source_bsp_fog_admission(locations,{true,true,fog_datum,fog_flags}),"receiver main-view fog differs from unchanged16E210");
    SourceBspAdmission main;
    for(auto* queue:{&main.clusters,&main.instances}) {
        const auto count=file.word();
        for(unsigned i=0;i<count;++i){SourceBspAdmissionRecord r;r.id=file.word();r.flags=std::uint16_t(file.word());
            r.other=file.word();r.association=std::uint16_t(file.word());queue->push_back(r);}
    }
    const SourceEffectLightReceiverWorld world{spatial,geometry,locations,bsp};
    EffectLightSphere shape;const auto position=cameras[0].camera_to_world.position;
    shape.shape[11]=std::bit_cast<std::uint32_t>(position.x);shape.shape[12]=std::bit_cast<std::uint32_t>(position.y);
    shape.shape[13]=std::bit_cast<std::uint32_t>(position.z);shape.shape[17]=std::bit_cast<std::uint32_t>(cameras[0].far_distance);
    std::size_t words=0,positive=0,association_count=0;
    for(unsigned index=0;index<cases;++index) {
        const auto cluster=std::int16_t(file.word());const auto center=file.read<Vec3>();const auto radius=file.read<float>();
        const auto count=file.word();std::vector<std::uint32_t> expected;
        for(unsigned i=0;i<count;++i)expected.push_back(file.word());
        const auto out=source_effect_light_receiver_view(world,cluster,center,radius,shape,fog,&main);
        require(out.visibility.groups.size()==6,"receiver lost original six cameras");
        require(std::memcmp(out.visibility.groups.data(),cameras.data(),sizeof(cameras))==0,"receiver C1D90 camera words differ");
        std::vector<std::uint32_t> actual{out.flags,out.pass_mode,out.query_mode,out.location.leaf,
            std::uint32_t(std::int32_t(out.location.cluster)),std::uint32_t(std::int32_t(out.location.bsp)),6,
            unsigned(out.visibility.clusters.size())};
        for(const auto& c:out.visibility.clusters)actual.insert(actual.end(),{unsigned(c.source_cluster)&65535,unsigned(c.counts[0])&65535});
        actual.insert(actual.end(),out.visibility.sorted_cluster_indices.begin(),out.visibility.sorted_cluster_indices.end());
        for(unsigned i=0;i<512;i+=4)actual.push_back(unsigned(out.bsp.cluster_entries[i])|(unsigned(out.bsp.cluster_entries[i+1])<<8)|
            (unsigned(out.bsp.cluster_entries[i+2])<<16)|(unsigned(out.bsp.cluster_entries[i+3])<<24));
        actual.insert(actual.end(),out.bsp.marked_clusters.begin(),out.bsp.marked_clusters.end());
        for(const auto* queue:{&out.bsp.clusters,&out.bsp.instances}) {
            actual.push_back(unsigned(queue->size()));
            for(const auto& r:*queue)actual.insert(actual.end(),{r.id,r.flags,r.other,r.association});
        }
        actual.push_back(unsigned(out.bsp.associations.size()));
        for(const auto& a:out.bsp.associations)actual.insert(actual.end(),a.begin(),a.end());
        if(actual!=expected) {
            for(std::size_t i=0;i<std::min(actual.size(),expected.size());++i)
                if(actual[i]!=expected[i]){std::cerr<<"case "<<index<<" word "<<i<<" source="<<std::hex<<expected[i]<<" native="<<actual[i]<<'\n';break;}
            throw FormatError("whole original receiver preparation differs at case "+std::to_string(index));
        }
        words+=actual.size()+camera_words;positive+=!out.bsp.clusters.empty()||!out.bsp.instances.empty();
        association_count+=out.bsp.associations.size();
    }
    require(file.done()&&cases>=64&&positive>=20&&association_count>0,"receiver fixture lacks complete positive coverage");
    std::cout<<"PASS "<<cases<<" original receiver queries, "<<words<<" exact owned/camera words, "<<positive<<" nonempty receivers, "<<association_count<<" complete association masks\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
