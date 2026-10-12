#include "halo2/texture.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>

using namespace halo2;
namespace {
void require(bool value,const char *message){if(!value)throw FormatError(message);}
}
int main(){try{
    Cache cache("assets/disc/maps/zanzibar.map");
    std::ifstream fixture("tests/fixtures/retail_source_bitmap_partitions.tsv");
    require(bool(fixture),"missing original bitmap partition fixture");
    unsigned cases=0,metadata_words=0,levels=0;std::size_t payload_bytes=0;std::set<std::uint32_t> bitmaps;
    std::string line;
    while(std::getline(fixture,line)){
        if(line.empty()||line[0]=='#')continue;
        const auto divider=line.find('|');require(divider!=std::string::npos,"partition fixture separator missing");
        std::istringstream in(line.substr(0,divider)),out(line.substr(divider+1));in>>std::hex;out>>std::hex;
        std::vector<std::uint32_t> values,header;std::uint32_t word;while(in>>word)values.push_back(word);while(out>>word)header.push_back(word);
        require(in.eof()&&out.eof()&&values.size()==12&&header.size()==5,"original partition fixture malformed");
        const auto partition=values[1];require(partition<3,"unknown original bitmap partition");
        const auto resource=cache.load_bitmap_resource(values[0],0);bitmaps.insert(values[0]);
        require(resource.width==values[2]&&resource.height==values[3]&&resource.depth==values[4]&&resource.type==values[5]&&resource.format==values[6],"actual bitmap metadata differs");
        require(resource.type==0&&resource.depth==1&&values[7]==0x203&&(resource.format==14||resource.format==16),"partition test scope differs from actual energy blade");
        require(resource.levels[partition].size()==values[10],"actual authored resource partition byte count differs");
        require(header[0]==0x40001&&header[1]==(values[11]&0x0fffffffu)&&header[2]==0&&header[4]==0,"original declared D3D header fields differ");
        const auto format=header[3];
        require(((format>>4)&15)==2&&((format>>8)&255)==(resource.format==14?12u:15u),"original NV2A texture dimensionality/format differs");
        const auto width=std::uint32_t(resource.width)>>partition,height=std::uint32_t(resource.height)>>partition;
        const auto additional=retail_mip_count(width,height,1,resource.format,false,values[8]-partition);
        require(width==(1u<<((format>>20)&15))&&height==(1u<<((format>>24)&15))&&1==(1u<<((format>>28)&15))&&additional+1==((format>>16)&15),"original12D6D0/359E0 partition dimensions/mip count differs");
        metadata_words+=4;
        const auto texture=prepare_texture(Bytes(resource.levels[partition]),width,height,resource.format,false,additional);
        const auto full=cache.load_bitmap(values[0],0);
        require(texture.mips.size()==additional+1&&full.mips.size()==texture.mips.size()+partition,"native partition mip chain length differs");
        for(unsigned mip=0;mip<texture.mips.size();++mip){
            const auto &actual=texture.mips[mip],&authored=full.mips[mip+partition];
            require(actual.width==authored.width&&actual.height==authored.height&&actual.size==authored.size,"native partition mip metadata differs from authored tail");
            require(std::equal(texture.pixels.begin()+actual.offset,texture.pixels.begin()+actual.offset+actual.size,full.pixels.begin()+authored.offset),"native authored partition payload differs from full mip tail");
            ++levels;payload_bytes+=actual.size;
        }
        ++cases;
    }
    require(fixture.eof()&&cases==12&&bitmaps.size()==4&&metadata_words==48&&levels==63,"original bitmap partition coverage incomplete");
    std::cout<<"12 original partition headers;48 native metadata words and63 authored BC mip tails/"<<payload_bytes<<" bytes exact\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
