#include "halo2/bsp_surface_lighting.hpp"
#include "halo2/math.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <sstream>
using namespace halo2;
namespace {
void check(bool ok,const std::string& why){if(!ok)throw FormatError(why);}
constexpr std::uint32_t none=0xffffffff;
using RayKey=std::array<std::uint32_t,7>;
std::map<RayKey,std::vector<std::uint32_t>> source_contacts(const std::filesystem::path& path){
    std::ifstream file(path);check(bool(file),"missing complete original accelerator fixture");std::map<RayKey,std::vector<std::uint32_t>> result;std::string line;
    while(std::getline(file,line)){
        if(line.empty()||line[0]=='#')continue;const auto tab=line.find('\t'),bar=line.find('|');
        check(tab!=std::string::npos&&bar!=std::string::npos,"invalid complete original accelerator fixture");
        std::istringstream input(line.substr(tab+1,bar-tab-1)),output(line.substr(bar+1));std::vector<std::uint32_t> in,out;std::uint32_t word;
        while(input>>std::hex>>word)in.push_back(word);while(output>>std::hex>>word)out.push_back(word);
        check(in.size()==12,"invalid complete original contact inputs");if(in[2]!=1)continue;
        check(out.size()==14&&out[0]==1,"invalid actual original tie contact");
        RayKey key{in[1],in[6],in[7],in[8],in[9],in[10],in[11]};check(result.emplace(key,std::move(out)).second,"duplicate original tied ray");
    }check(result.size()==17,"complete original tied ray count changed");return result;
}
}
int main(){try{
    const std::filesystem::path root=HALO2_SOURCE_DIR;
    const std::filesystem::path maps[]{root/"assets/disc/maps/headlong.map",root/"assets/map-pack/maps/triplicate.map"};
    for(const auto& p:maps)if(!std::filesystem::exists(p)){std::cout<<"source instance tie assets absent\n";return 77;}
    std::array<std::unique_ptr<BspSurfaceLighting>,2> banks;
    const auto actual_contacts=source_contacts(root/"tests/fixtures/retail_instance_accelerator_contacts.tsv");
    std::ifstream fixture(root/"tests/fixtures/retail_instance_contact_ties.tsv");check(bool(fixture),"missing source instance tie fixture");
    std::string line;std::size_t cases=0,queries=0,contacts=0,bounds=0,map_queries=0,controlled_callbacks=0;
    while(std::getline(fixture,line)){
        if(line.empty()||line[0]=='#')continue;
        const auto tab=line.find('\t');check(tab!=std::string::npos,"invalid source tie row");const auto label=line.substr(0,tab);
        std::istringstream input(line.substr(tab+1));std::vector<std::uint32_t> words;std::uint32_t word;while(input>>std::hex>>word)words.push_back(word);
        std::size_t at=0;const auto take=[&](){check(at<words.size(),label+": truncated row");return words[at++];};
        const auto scalar=[&](){return std::bit_cast<float>(take());};
        check(take()==1,label+": unsupported fixture version");const auto map=take(),main_definition=take(),copy=take(),over=take();check(map<2,"invalid map");
        if(!banks[map])banks[map]=std::make_unique<BspSurfaceLighting>(Cache(maps[map]).load_bsp_surface_lighting());
        BspSurfaceLighting edited;const auto* bank=banks[map].get();
        if(over!=none){edited=*bank;bank=&edited;
            if(copy!=none){check(over==edited.instances.size(),label+": copy index escaped original bank");edited.instances.push_back(edited.instances.at(copy));
                for(auto& cluster:edited.cluster_instances){if(std::find(cluster.begin(),cluster.end(),std::uint16_t(copy))!=cluster.end())cluster.push_back(std::uint16_t(over));}
            }
            auto& instance=edited.instances.at(over);instance.scale=scalar();for(auto& axis:instance.axes)axis={scalar(),scalar(),scalar()};instance.translation={scalar(),scalar(),scalar()};instance.collision_center={scalar(),scalar(),scalar()};instance.collision_radius=scalar();
        }
        if(main_definition!=none){check(over!=none,label+": static control lacks explicit instance");edited.collision[0]=edited.definition_collision.at(main_definition);
            // Explicit controlled static bank and one-cluster membership. No
            // authored-map/registration claim is made for these test inputs.
            for(auto& leaf:edited.collision[0].leaves)leaf.cluster=0;
            edited.clusters.resize(1);edited.cluster_instances={{{std::uint16_t(over)}}};
        }
        const Vec3 origin{scalar(),scalar(),scalar()},ray{scalar(),scalar(),scalar()};
        const auto kind=take(),owner=take(),fraction=take(),leaf=take(),surface=take(),unresolved=take(),count=take();check(kind<2&&unresolved<2&&count<=3,label+": invalid expected record");
        std::vector<std::uint16_t> callback_order;
        for(unsigned i=0;i<count;++i){const auto id=take(),definition=take(),q_fraction=take(),q_leaf=take(),q_surface=take(),exact=take(),next=take();
            callback_order.push_back(std::uint16_t(id));
            const auto contact=original_instance_vector_contact(*bank,std::uint16_t(id),origin,ray);
            check(bool(contact),label+": original full-length instance contact lost");const auto& c=*contact;
            check(c.source.instance==int(id)&&c.source.record==int(definition)&&c.source.surface==int(q_surface)&&c.contact.surface==q_surface&&c.contact.leaf==q_leaf&&std::bit_cast<std::uint32_t>(c.contact.fraction)==q_fraction,label+": source contact/leaf words changed");++queries;contacts+=6;
            if(exact!=none){check(bool(original_instance_vector_contact(*bank,std::uint16_t(id),origin,ray,std::bit_cast<float>(fraction)))==bool(exact),label+": exact bound result changed");++bounds;}
            if(next!=none){check(bool(original_instance_vector_contact(*bank,std::uint16_t(id),origin,ray,std::bit_cast<float>(fraction+1)))==bool(next),label+": one-ULP bound result changed");++bounds;}
        }
        check(at==words.size(),label+": trailing input");
        if(over==none){
            const RayKey key{map==0?7u:18u,std::bit_cast<std::uint32_t>(origin.x),std::bit_cast<std::uint32_t>(origin.y),std::bit_cast<std::uint32_t>(origin.z),std::bit_cast<std::uint32_t>(ray.x),std::bit_cast<std::uint32_t>(ray.y),std::bit_cast<std::uint32_t>(ray.z)};
            const auto& expected=actual_contacts.at(key);
            for(auto flags:{0x4480000du,0x04800c0du,0x04800005u}){
                bool ambiguous=false;const auto contact=original_map_vector_contact(*bank,origin,ray,&ambiguous,flags);check(bool(contact),label+": map contact lost");const auto& c=*contact;
                check(c.source.instance==int(expected[3])&&c.contact.surface==expected[6]&&c.contact.leaf==expected[5]&&std::bit_cast<std::uint32_t>(c.contact.fraction)==expected[2],label+": actual original accelerator owner/contact changed");
                check(!ambiguous,label+": proved actual accelerator ownership remains unresolved");contacts+=6;++map_queries;
            }
        }else{
            // These three source controls supply explicit callback order. The
            // appended/moved instance is not part of the authored MOPP bank.
            // Replay the original collision+strict collector kernel without
            // making an accelerator visitation claim for controlled data.
            std::optional<MapVectorContact> best;
            if(main_definition!=none){const auto main=original_bsp_vector_query(bank->collision[0],origin,ray,std::numeric_limits<float>::max(),9);
                check(bool(main.contact),label+": explicit static contact lost");const auto& h=*main.contact;
                best=MapVectorContact{{bank->bsp,0,int(h.surface),-1,h.oriented_plane},origin+ray*h.fraction,h};
            }
            for(const auto id:callback_order){const float maximum=best?best->contact.fraction:1.f;
                const auto contact=original_instance_vector_contact(*bank,id,origin,ray,maximum,9);
                if(contact&&contact->contact.fraction<maximum)best=contact;++controlled_callbacks;
            }
            check(bool(best),label+": explicit original callback control lost final contact");const auto& c=*best;
            check((c.source.instance>=0)==bool(kind)&&c.source.instance==(owner==none?-1:int(owner))&&c.contact.surface==surface&&c.contact.leaf==leaf&&std::bit_cast<std::uint32_t>(c.contact.fraction)==fraction,label+": explicit original strict callback control changed");contacts+=6;
        }
        ++cases;
    }
    check(cases==20&&map_queries==51&&controlled_callbacks==7,"source tie fixture case count changed");
    std::cout<<cases<<" original contact/callback fixtures; "<<queries<<" full-length queries; "<<bounds<<" endpoint controls; "<<map_queries<<" actual accelerator map queries; "<<controlled_callbacks<<" explicitly ordered controlled callbacks; "<<contacts<<" contact and ownership words exact\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
