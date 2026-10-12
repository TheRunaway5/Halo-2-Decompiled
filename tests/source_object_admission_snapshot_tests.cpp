#include "halo2/source_object_admission.hpp"
#include "halo2/source_object_spatial_pool.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace halo2;
namespace {
void require(bool value,const char *message){if(!value)throw FormatError(message);}
struct Bytes {
    std::vector<std::uint8_t> data;
    std::size_t at{};
    std::uint32_t take(unsigned count=4){require(at+count<=data.size(),"truncated restored admission snapshot");std::uint32_t v=0;for(unsigned i=0;i<count;++i)v|=std::uint32_t(data[at++])<<(i*8);return v;}
    Bytes block(){const auto count=take();require(at+count<=data.size(),"truncated restored raw bank");Bytes b; b.data.assign(data.begin()+at,data.begin()+at+count);at+=count;return b;}
    std::uint32_t word(std::size_t p,unsigned count=4)const{require(p+count<=data.size(),"raw source field exceeds record");std::uint32_t v=0;for(unsigned i=0;i<count;++i)v|=std::uint32_t(data[p+i])<<(i*8);return v;}
};
struct Snapshot {
    SourceObjectSpatialPool collidable{512},noncollidable{512,false};
    std::vector<SourceObjectAdmissionObject> objects;
    unsigned authored_clusters{},raw_object_bytes{};
};
void pool(SourceObjectSpatialDatumPool &out,const Bytes &header,const Bytes &raw,const Bytes &occupied){
    require(header.data.size()==76 && occupied.data.size()==256 && header.word(0x20)==2048,"invalid literal source spatial header");
    const auto stride=header.word(0x24);require(stride==12 || stride==32,"invalid literal source spatial stride");
    require(raw.data.size()==stride*2048,"incomplete original spatial data-array records");
    out.record_words=stride/4;out.cursor=header.word(0x34);out.highwater=header.word(0x38);out.active_count=header.word(0x3c);
    out.next_generation=std::uint16_t(header.word(0x40,2));out.debug_fill=(header.word(0x2a,2)&8)!=0;
    for(unsigned i=0;i<2048;++i)for(unsigned j=0;j<stride/4;++j)out.records[i][j]=raw.word(i*stride+j*4);
    for(unsigned i=0;i<64;++i)out.occupied[i]=occupied.word(i*4);
}
Snapshot load(const std::filesystem::path &path){
    std::ifstream file(path,std::ios::binary);require(bool(file),"restored admission snapshot unavailable");Bytes b;
    b.data.assign(std::istreambuf_iterator<char>(file),{});require(b.data.size()>=16,"missing restored admission snapshot header");
    require(std::string(b.data.begin(),b.data.begin()+8)=="H2OASNP1","unsupported restored admission snapshot version");b.at=8;
    Snapshot out;out.authored_clusters=b.take();require(out.authored_clusters==55 && b.take()==17,"unexpected restored bank shape");
    const auto object_header=b.block(),object_records=b.block(),object_occupied=b.block();
    require(object_header.word(0x20)==2048 && object_header.word(0x24)==12 && object_header.word(0x3c)==202,"incomplete restored object pool");
    require(object_records.data.size()==24576 && object_occupied.data.size()==256,"incomplete original object handle bank");
    for(auto *p:{&out.collidable,&out.noncollidable}){
        const auto heads=b.block();require(heads.data.size()==2048,"all original512heads required");
        for(unsigned i=0;i<512;++i)p->heads[i]=heads.word(i*4);
        for(auto *datum:{&p->entries,&p->references}){
            const auto header=b.block(),raw=b.block(),occupied=b.block();pool(*datum,header,raw,occupied);
        }
    }
    const auto count=b.take();require(count==202,"all202actual live owners required");
    for(unsigned i=0;i<count;++i){
        const auto id=b.take(),address=b.take();const auto raw=b.block();const auto slot=id&65535;
        require(slot<2048 && (object_occupied.word((slot/32)*4)&(1u<<(slot&31))),"owner missing actual occupied slot");
        require(object_records.word(slot*12,2)==(id>>16) && object_records.word(slot*12+8)==address && object_records.word(slot*12+6,2)==raw.data.size(),"restored full handle/address/size differs from literal header");
        const auto scalar=[&](unsigned off){return std::bit_cast<float>(raw.word(off));};
        out.objects.push_back({id,raw.word(4),std::int8_t(raw.word(0xaa,1)),{{scalar(0x30),scalar(0x34),scalar(0x38)},scalar(0x3c)}});
        out.raw_object_bytes+=unsigned(raw.data.size());
    }
    require(b.at==b.data.size() && out.raw_object_bytes==127296,"restored object body collection incomplete");
    require(out.collidable.entries.active_count==187 && out.collidable.references.active_count==187 && out.noncollidable.entries.active_count==37 && out.noncollidable.references.active_count==37,"restored spatial pressure differs");
    return out;
}
std::vector<std::uint32_t> words(const std::string &text){std::istringstream s(text);std::vector<std::uint32_t> out;std::uint32_t v;while(s>>std::hex>>v)out.push_back(v);return out;}
std::vector<std::uint32_t> probe(const Snapshot &snapshot,const std::vector<std::uint32_t> &input){
    std::size_t at=0;const auto take=[&](){require(at<input.size(),"truncated restored visibility input");return input[at++];};
    const auto scalar=[&](){return std::bit_cast<float>(take());};SourceObjectAdmissionInput in;in.capacity=take();in.flags=take();
    for(auto &v:in.clipping_plane)v=scalar();SourcePortalVisibilitySet visible;visible.groups.resize(take());
    const auto count=take();for(unsigned i=0;i<count;++i){SourceClusterFrustumRanges c;c.source_cluster=std::int16_t(take());for(auto &v:c.counts)v=std::int16_t(take());for(auto &v:c.first_indices)v=std::int16_t(take());visible.clusters.push_back(c);}
    const auto frusta=take();for(unsigned i=0;i<frusta;++i){SourceCameraFrustum f;for(auto &v:f.bounds)v=scalar();for(auto &p:f.planes)for(auto &v:p)v=scalar();visible.frustums.push_back(f);}
    const auto clusters=take();require(clusters==512,"literal snapshot heads require explicit512byte fog capacity");std::vector<std::uint8_t> fog(clusters);for(auto &v:fog)v=std::uint8_t(take());in.containing_fog=fog;
    const auto associations=take();std::vector<std::array<std::uint32_t,4>> prior(associations);for(auto &mask:prior)for(auto &v:mask)v=take();require(at==input.size(),"trailing restored visibility input");
    const auto out=source_object_admission(visible,snapshot.collidable,snapshot.noncollidable,snapshot.objects,in,prior);
    std::vector<std::uint32_t> result{unsigned(out.objects.size())};for(const auto &r:out.objects)result.insert(result.end(),{r.id,r.flags,r.other,r.association});
    result.push_back(unsigned(out.associations.size()));for(const auto &mask:out.associations)result.insert(result.end(),mask.begin(),mask.end());return result;
}
void print(const auto &v){for(auto w:v)std::cout<<std::hex<<w<<' ';std::cout<<'\n';}
}
int main(int argc,char **argv){try{
    if(argc==3 && std::string(argv[1])=="--probe"){
        const auto snapshot=load(argv[2]);std::string line;while(std::getline(std::cin,line))if(!line.empty())print(probe(snapshot,words(line)));return 0;
    }
    require(argc==3,"expected restored snapshot and original admission fixture paths");
    if(!std::filesystem::exists(argv[1])){std::cout<<"original restored snapshot fixture unavailable\n";return 77;}
    const auto snapshot=load(argv[1]);std::ifstream fixture(argv[2]);require(bool(fixture),"restored source queue fixture unavailable");
    std::string line;unsigned cases=0,compared=0;
    while(std::getline(fixture,line)){if(line.empty() || line[0]=='#')continue;const auto split=line.find('|');require(split!=std::string::npos,"missing original queue words");
        const auto actual=probe(snapshot,words(line.substr(0,split))),expected=words(line.substr(split+1));
        if(actual!=expected){std::cerr<<"restored original admission case "<<cases<<" differs\n";print(actual);throw FormatError("actual-pool admission differs from untouched source");}
        ++cases;compared+=unsigned(expected.size());
    }
    require(cases>0,"empty restored source admission fixture");
    std::cout<<std::dec<<cases<<" fresh original admission cases / "<<compared<<" words exact over202raw owners and224actual spatial entries; no re-registration or saved-collector equivalence claim\n";return 0;
}catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}}
