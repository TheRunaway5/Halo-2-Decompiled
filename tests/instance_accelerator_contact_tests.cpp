#include "halo2/bsp_surface_lighting.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
using namespace halo2;
namespace {
void require(bool condition,const std::string& why){if(!condition)throw FormatError(why);}
std::vector<std::uint32_t> parse(const std::string& text){std::istringstream stream(text);std::vector<std::uint32_t> out;std::uint32_t v;while(stream>>std::hex>>v)out.push_back(v);return out;}
struct Row {std::string label;std::vector<std::uint32_t> input,expected;};
bool next(std::ifstream& file,Row& row){std::string line;while(std::getline(file,line)){
    if(line.empty()||line[0]=='#')continue;
    const auto tab=line.find('\t'),bar=line.find('|');require(tab!=std::string::npos&&bar!=std::string::npos&&tab<bar,"invalid original contact row");
    row={line.substr(0,tab),parse(line.substr(tab+1,bar-tab-1)),parse(line.substr(bar+1))};return true;
}return false;}
}
int main(){try{
    const std::filesystem::path root=HALO2_SOURCE_DIR;
    constexpr const char* names[]{"ascension","beavercreek","burial_mounds","coagulation","colossus","cyclotron","foundation","headlong","lockout","midship","waterworks","zanzibar","backwash","containment","deltatap","dune","elongation","gemini","triplicate","turf","warlock"};
    std::ifstream fixture(root/"tests/fixtures/retail_instance_accelerator_contacts.tsv");require(bool(fixture),"missing original static contact fixture");
    std::ifstream callbacks(root/"tests/fixtures/retail_instance_accelerator_callbacks.tsv");require(bool(callbacks),"missing original static callback fixture");
    std::unique_ptr<BspSurfaceLighting> bank;std::uint32_t loaded=0xffffffff;Row row,trace;
    std::size_t cases=0,words=0,callback_words=0,callback_count=0,misses=0,static_hits=0,instance_hits=0,ties=0,duplicate_callbacks=0,prior_static=0,prior_instance=0;bool triplicate_miss=false;
    while(next(fixture,row)){
        const auto& label=row.label;const auto& input=row.input;const auto& expected=row.expected;
        require(next(callbacks,trace)&&trace.label==label&&trace.input==input,label+": original contact and callback inputs differ");
        require(!trace.expected.empty()&&trace.expected.size()==1+5*trace.expected[0],label+": invalid callback fixture length");
        require(input.size()==12&&input[0]==1&&input[1]<21&&input[2]<=1&&input[5]==0x04800005,label+": invalid explicit static query inputs");
        const auto map=input[1];if(loaded!=map){const auto path=root/(map<12?"assets/disc/maps":"assets/map-pack/maps")/(std::string(names[map])+".map");
            if(!std::filesystem::exists(path)){std::cout<<"original map assets absent\n";return 77;}
            bank=std::make_unique<BspSurfaceLighting>(Cache(path).load_bsp_surface_lighting());loaded=map;
        }
        const Vec3 origin{std::bit_cast<float>(input[6]),std::bit_cast<float>(input[7]),std::bit_cast<float>(input[8])},ray{std::bit_cast<float>(input[9]),std::bit_cast<float>(input[10]),std::bit_cast<float>(input[11])};
        std::vector<std::uint32_t> observed{0};std::vector<std::uint32_t> keys;
        bool unresolved=false;const auto hit=original_map_vector_contact(*bank,origin,ray,&unresolved,input[5],[&](const MapInstanceCallback& c){
            observed.insert(observed.end(),{c.cluster,c.key,std::bit_cast<std::uint32_t>(c.fraction),c.previous_type,c.previous_owner});
            ++observed[0];++callback_count;prior_static+=c.previous_type==1;prior_instance+=c.previous_type==3;
            duplicate_callbacks+=std::find(keys.begin(),keys.end(),c.key)!=keys.end();keys.push_back(c.key);
        });
        require(!unresolved,label+": actual source accelerator owner remains unresolved");
        require(observed==trace.expected,label+": original callback order/key/fraction/prior owner words differ");callback_words+=observed.size();
        std::vector<std::uint32_t> actual{std::uint32_t(bool(hit))};
        if(hit){const auto& h=*hit;const auto type=h.source.instance<0?1u:3u;
            actual.insert(actual.end(),{type,std::bit_cast<std::uint32_t>(h.contact.fraction),std::uint32_t(h.source.instance),std::uint32_t(h.source.record),h.contact.leaf,h.contact.surface,h.contact.oriented_plane,h.contact.flags,h.contact.breakable,std::uint32_t(h.contact.material),std::bit_cast<std::uint32_t>(h.point.x),std::bit_cast<std::uint32_t>(h.point.y),std::bit_cast<std::uint32_t>(h.point.z)});
            if(type==1)++static_hits;else ++instance_hits;
            if(map==18&&input[2]==0&&input[3]==116&&input[4]==0){
                require(!bank->attribute(h.source,h.point,h.contact.leaf),"retained Triplicate116 authored attribution miss changed");triplicate_miss=true;
            }
        }else ++misses;
        require(actual==expected,label+": original owner/contact/leaf/surface/material/point words differ");words+=actual.size();++cases;ties+=input[2]==1;
    }
    require(!next(callbacks,trace),"original callback fixture has trailing queries");
    require(cases==13229&&ties==17&&static_hits==11420&&instance_hits==562&&misses==1247&&triplicate_miss,"original all-map contact coverage changed");
    require(callback_count==2641&&duplicate_callbacks==135&&prior_static==1826&&prior_instance==121,"original callback admission/dedup coverage changed");
    std::cout<<cases<<" unchanged original static contacts across 21 maps; "<<words<<" contact/owner/point words and "<<callback_words<<" callback words exact; "<<callback_count<<" ordered callbacks; "<<ties<<" real tied-ray owners; Triplicate116 attribution miss preserved\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
