#include "halo2/cache.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>

using namespace halo2;
namespace {
void require(bool value,const char *message){if(!value)throw FormatError(message);}
std::vector<std::byte> unhex(const std::string &text){
    if(text=="-")return {};
    require(text.size()%2==0,"odd source payload hex length");
    std::vector<std::byte> result;result.reserve(text.size()/2);
    for(std::size_t i=0;i<text.size();i+=2){
        const auto byte=std::stoul(text.substr(i,2),nullptr,16);
        result.push_back(std::byte(byte));
    }
    return result;
}
std::uint64_t fnv(std::span<const std::byte> bytes){
    std::uint64_t value=14695981039346656037ull;
    for(const auto byte:bytes)value=(value^std::to_integer<unsigned>(byte))*1099511628211ull;
    return value;
}
bool same_texture(const Texture &a,const Texture &b){
    if(a.width!=b.width || a.height!=b.height || a.format!=b.format ||
       a.layers!=b.layers || a.depth!=b.depth || a.pixels!=b.pixels ||
       a.mips.size()!=b.mips.size())return false;
    for(std::size_t i=0;i<a.mips.size();++i){
        const auto &x=a.mips[i],&y=b.mips[i];
        if(x.width!=y.width || x.height!=y.height || x.depth!=y.depth ||
           x.offset!=y.offset || x.size!=y.size)return false;
    }
    return true;
}
template<class F>void rejected(F action){
    bool threw=false;try{action();}catch(const FormatError&){threw=true;}
    require(threw,"unproved/malformed low-detail resource was admitted");
}
std::filesystem::path map_path(const std::string &name){
    auto path=std::filesystem::path("assets/disc/maps")/(name+".map");
    if(!std::filesystem::exists(path))path=std::filesystem::path("assets/map-pack/maps")/(name+".map");
    return path;
}
struct PrivateMap {
    std::filesystem::path directory,path;
    std::vector<std::byte> original;
    PrivateMap(){
        directory=std::filesystem::temp_directory_path()/
            ("halo2-low-detail-controls-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        require(std::filesystem::create_directory(directory),"cannot create private bitmap control directory");
        path=directory/"zanzibar.map";
        std::filesystem::copy_file(map_path("zanzibar"),path);
        std::ifstream file(path,std::ios::binary|std::ios::ate);
        require(bool(file),"missing private source map");const auto size=file.tellg();
        require(size>0,"empty private source map");original.resize(std::size_t(size));file.seekg(0);
        require(bool(file.read(reinterpret_cast<char*>(original.data()),size)),"short private source map read");
    }
    ~PrivateMap(){std::error_code error;std::filesystem::remove_all(directory,error);}
    void store(std::size_t at,std::uint32_t value,unsigned size=4){
        require(at+size<=original.size() && (size==2 || size==4),"invalid private patch range");
        std::fstream file(path,std::ios::binary|std::ios::in|std::ios::out);file.seekp(std::streamoff(at));
        for(unsigned i=0;i<size;++i)file.put(char(value>>(8*i)));
        require(bool(file),"private source patch failed");
    }
    void restore(std::size_t at,unsigned size=4){
        std::fstream file(path,std::ios::binary|std::ios::in|std::ios::out);file.seekp(std::streamoff(at));
        require(bool(file.write(reinterpret_cast<const char*>(original.data()+at),size)),"private source restore failed");
    }
};
}
int main(){try{
    std::ifstream fixture("tests/fixtures/retail_source_bitmap_low_detail_assets.tsv");
    require(bool(fixture),"missing original all-map low-detail fixture");
    std::unique_ptr<Cache> cache;std::string current_map,line;
    std::set<std::string> maps,identities;std::set<std::uint64_t> cube_hashes;
    unsigned cases=0,present=0,absent=0,planar=0,cubes=0,volumes=0,header_words=0;
    std::size_t stored_bytes=0,admitted_bytes=0,decoded_bytes=0;
    std::uint32_t floor_record=0,floor_blob=0,floor_offset=0;
    std::uint32_t cube_record=0,cube_datum=0,cube_index=0;
    std::set<std::string> authored_cubes;
    while(std::getline(fixture,line)){
        if(line.empty() || line[0]=='#')continue;
        const auto first=line.find('|'),second=line.find('|',first+1);
        require(first!=std::string::npos && second!=std::string::npos,"missing asset fixture separator");
        std::istringstream input(line.substr(0,first)),output(line.substr(first+1,second-first-1)),payload_text(line.substr(second+1));
        char kind;std::string name,payload_hex;input>>kind>>name>>std::hex;output>>std::hex;
        std::vector<std::uint32_t> v;std::vector<std::uint64_t> h;
        std::uint32_t word;std::uint64_t wide;while(input>>word)v.push_back(word);while(output>>wide)h.push_back(wide);
        require(input.eof() && output.eof() && v.size()==12 && h.size()==9 && (kind=='P' || kind=='N'),"malformed original asset fixture");
        require(bool(payload_text>>payload_hex),"missing source payload bytes");std::string extra;require(!(payload_text>>extra),"trailing source payload data");
        require(identities.insert(name+":"+std::to_string(v[0])+":"+std::to_string(v[1])).second,"duplicate authored bitmap record");
        if(name!=current_map){
            require(maps.insert(name).second,"source map records are not contiguous");
            cache=std::make_unique<Cache>(map_path(name));current_map=name;
        }
        const auto authored=unhex(payload_hex);
        const auto resource=cache->load_bitmap_low_detail_resource(v[0],v[1]);
        if(kind=='N'){
            require(!resource && authored.empty() && !cache->load_bitmap_low_detail(v[0],v[1]),"absent authored low-detail bitmap became present");
            require(std::all_of(h.begin(),h.begin()+5,[](auto value){return value==0;}),"absent descriptor acquired an original header");
            ++absent;++cases;continue;
        }
        require(bool(resource),"present authored low-detail bitmap absent");const auto &r=*resource;
        require(r.type==v[5] && r.flags==v[6] && r.shared_offset==v[8] && r.pixels==authored && r.pixels.size()==v[9],"raw resource differs from exact padded source slot");
        require(r.format==(v[10]&0xffff) && r.width==(v[10]>>16) && r.height==(v[11]&0xffff) && r.depth==(v[11]>>16),"raw resource description differs from immutable source fields");
        require(v[7]==0 && h[0]==0x40001 && h[1]==((0x08100000u+r.shared_offset)&0x0fffffffu) && h[2]==0 && h[4]==0,"original Common/Data/Lock/Size header differs");
        const auto format=std::uint32_t(h[3]);
        require(((format>>16)&15)==1 && (1u<<((format>>20)&15))==r.width &&
            (1u<<((format>>24)&15))==r.height && (1u<<((format>>28)&15))==r.depth,"original one-level header dimensions differ");
        require(((format>>4)&15)==(r.type==1?3u:2u) && bool(format&4)==(r.type==2),"original source header type differs");
        const auto nv=r.format==11?6u:r.format==14?12u:r.format==15?14u:15u;
        require(((format>>8)&255)==nv && h[5]==r.pixels.size() && h[6]<=h[5],"original header format or stored/admitted byte count differs");
        header_words+=5;stored_bytes+=r.pixels.size();admitted_bytes+=std::size_t(h[6]);++present;
        if(r.type==1){
            require(r.format==11 && r.width==4 && r.height==4 && r.depth==4 && r.pixels.size()==256 && h[6]==256 && h[7]==0 && h[8]==0,"unsupported source volume scope changed");
            rejected([&]{cache->load_bitmap_low_detail(v[0],v[1]);});++volumes;
        }else{
            const auto texture=cache->load_bitmap_low_detail(v[0],v[1]);require(bool(texture),"proved source texture absent");const auto &t=*texture;
            require(t.width==r.width && t.height==r.height && t.format==r.format && t.depth==0 && t.pixels.size()==h[7] && fnv(t.pixels)==h[8],"decoded source texture differs from frozen pixel expectation");
            decoded_bytes+=t.pixels.size();
            if(r.type==0){
                const auto size=std::size_t((r.width+3)/4)*((r.height+3)/4)*(r.format==14?8:16);
                require(t.layers==1 && t.mips.size()==1 && t.mips[0].offset==0 && t.mips[0].size==size && size==h[6] && size==t.pixels.size() &&
                    std::equal(t.pixels.begin(),t.pixels.end(),authored.begin()),"one-level planar BC payload differs");++planar;
            }else{
                require(r.type==2 && r.format==11 && r.width==4 && r.height==4 && r.depth==1 && r.pixels.size()==768 && h[6]==384 && t.layers==6 && t.mips.size()==6,"proved source cube face scope differs");
                for(unsigned face=0;face<6;++face)require(t.mips[face].offset==face*64 && t.mips[face].size==64,"cube retained source padding as pixels or changed face order");
                cube_hashes.insert(h[8]);authored_cubes.insert(name+":"+std::to_string(v[0])+":"+std::to_string(v[1]));++cubes;
                if(name=="zanzibar" && !cube_record){cube_record=v[2];cube_datum=v[0];cube_index=v[1];}
            }
            for(const auto &mip:t.mips)require(mip.width==r.width && mip.height==r.height && mip.depth==1,"one-level source mip description differs");
        }
        if(name=="zanzibar" && v[0]==0xf4f91373 && v[1]==0){floor_record=v[2];floor_blob=v[3];floor_offset=v[8];}
        ++cases;
    }
    require(fixture.eof() && maps.size()==21 && cases==35442 && present==23157 && absent==12285 &&
        planar==22486 && cubes==524 && volumes==147 && header_words==115785 && cube_hashes.size()>1,
        "all-map original low-detail coverage incomplete");
    require(floor_record && floor_blob && cube_record,"source controls missing actual Zanzibar records");
    std::ifstream layouts("tests/fixtures/retail_source_bitmap_low_detail_cube_layouts.tsv");
    require(bool(layouts),"missing complete original cube-face layout fixture");
    unsigned faces=0;std::set<std::string> face_identities;
    while(std::getline(layouts,line)){
        if(line.empty() || line[0]=='#')continue;
        const auto divider=line.find('|');require(divider!=std::string::npos,"cube layout separator missing");
        std::istringstream input(line.substr(0,divider)),output(line.substr(divider+1));std::string name;
        input>>name>>std::hex;output>>std::hex;
        std::vector<std::uint32_t> v,h;std::uint32_t word;while(input>>word)v.push_back(word);while(output>>word)h.push_back(word);
        require(input.eof() && output.eof() && v.size()==10 && h.size()==5 && v[2]<6,"malformed original cube-face fixture");
        const auto identity=name+":"+std::to_string(v[0])+":"+std::to_string(v[1]);
        require(authored_cubes.contains(identity) && face_identities.insert(identity+":"+std::to_string(v[2])).second,"cube-face fixture identity differs from authored resource");
        require(v[3]==0x08100000 && v[5]==0x40001 && v[6]==((v[3]+v[4])&0x0fffffffu) && v[7]==0 && v[8]==0x0221062d && v[9]==0,
            "original cube-face input header changed");
        require(h[0]==((v[3]+v[4])|0x80000000u)+v[2]*128 && h[1]==16 && h[2]==64 && h[3]==v[8] && h[4]==0,
            "original complete XDK cube-face address/pitch differs");
        ++faces;
    }
    require(layouts.eof() && faces==3144 && face_identities.size()==authored_cubes.size()*6,"original cube-face coverage incomplete");

    Cache original(map_path("zanzibar"));
    constexpr std::array<std::uint32_t,5> bitmaps{0xf4f91373,0xe2e1016c,0xe2e2016d,0xe2e3016e,0xe2e4016f};
    std::vector<Texture> hardware;
    for(const auto bitmap:bitmaps)hardware.push_back(original.load_bitmap(bitmap,0));
    for(std::size_t i=0;i<bitmaps.size();++i){
        const auto low=original.load_bitmap_low_detail(bitmaps[i]);require(bool(low),"source sword/floor low-detail resource missing");
        bool different=true;
        for(const auto &mip:hardware[i].mips)if(mip.width==low->width && mip.height==low->height && mip.size==low->pixels.size())
            different=!std::equal(low->pixels.begin(),low->pixels.end(),hardware[i].pixels.begin()+mip.offset);
        require(different,"low-detail authored blob silently became a hardware mip tail");
        require(same_texture(hardware[i],original.load_bitmap(bitmaps[i],0)),"low-detail request changed full hardware texture");
    }
    std::ifstream special_formats("tests/fixtures/retail_source_bitmap_low_detail_cube_formats.tsv");
    require(bool(special_formats),"missing original special cube-format fixture");
    unsigned special_headers=0;
    struct SpecialCube {std::uint32_t datum,index,record,flags,format;};
    std::vector<SpecialCube> special_controls;
    while(std::getline(special_formats,line)){
        if(line.empty() || line[0]=='#')continue;
        const auto divider=line.find('|');require(divider!=std::string::npos,"special cube-format separator missing");
        std::istringstream input(line.substr(0,divider)),output(line.substr(divider+1));std::string name;
        input>>name>>std::hex;output>>std::hex;
        std::vector<std::uint32_t> v,h;std::uint32_t word;while(input>>word)v.push_back(word);while(output>>word)h.push_back(word);
        require(input.eof() && output.eof() && name=="zanzibar" && v.size()==5 && h.size()==5,
            "malformed original special cube-format fixture");
        require(v[0]==0xe17d0008 && v[1]==0 && (v[4]==10 || v[4]==11) &&
            h[0]==0x40001 && h[2]==0 && h[4]==0,"special cube-format source identity changed");
        const auto nv=(h[3]>>8)&255;
        require(nv==((v[3]&0x20)?0x33u:(v[4]==11?6u:7u)),"source special format mapping changed");
        if(v[3]&0x20)special_controls.push_back({v[0],v[1],v[2],v[3],v[4]});
        ++special_headers;
    }
    require(special_formats.eof() && special_headers==4 && special_controls.size()==2,
        "original special cube-format controls incomplete");
    unsigned controls=0;
    require(!original.load_bitmap_low_detail_resource(0xffffffff) && !original.load_bitmap_low_detail(0xffffffff),"NONE bitmap acquired a low-detail image");++controls;
    rejected([&]{original.load_bitmap_low_detail_resource(0x12345678);});++controls;
    rejected([&]{original.load_bitmap_low_detail_resource(0xf4f91373,65536);});++controls;
    PrivateMap private_map;
    const auto low=*original.load_bitmap_low_detail(0xf4f91373);
    auto absent_control=[&](std::size_t at){
        private_map.store(at,0);Cache modified(private_map.path);
        require(!modified.load_bitmap_low_detail_resource(0xf4f91373) && !modified.load_bitmap_low_detail(0xf4f91373),"absent blob/descriptor did not stay optional");
        private_map.restore(at);++controls;
    };
    absent_control(0x158);absent_control(0x15c);absent_control(floor_record+0x64);
    auto bad_raw=[&](std::size_t at,std::uint32_t value,unsigned size=4){
        private_map.store(at,value,size);Cache modified(private_map.path);
        rejected([&]{modified.load_bitmap_low_detail_resource(0xf4f91373);});
        private_map.restore(at,size);++controls;
    };
    bad_raw(0x158,std::uint32_t(private_map.original.size()-1));
    bad_raw(0x15c,0xffffffff);bad_raw(floor_record+0x60,0xffffffff);
    bad_raw(floor_record+0x64,0x7fffffff);bad_raw(floor_record+0x64,0x80000000);
    bad_raw(floor_record+0x5c,0x40001);bad_raw(floor_record+0x6a,0,2);
    bad_raw(floor_record+0x6a,4097,2);bad_raw(floor_record+0x6e,0,2);
    bad_raw(floor_record+0xa,3,2);
    auto bad_decode=[&](std::size_t at,std::uint32_t value,unsigned size=2){
        private_map.store(at,value,size);Cache modified(private_map.path);
        require(bool(modified.load_bitmap_low_detail_resource(0xf4f91373)),"raw API discarded an in-range unproved descriptor");
        rejected([&]{modified.load_bitmap_low_detail(0xf4f91373);});
        private_map.restore(at,size);++controls;
    };
    bad_decode(floor_record+0x68,0xffff);bad_decode(floor_record+0x6a,3);
    bad_decode(floor_record+0x6e,2);bad_decode(floor_record+0xe,0x213);
    bad_decode(floor_record+0x64,63,4);
    private_map.store(floor_blob+floor_offset+64,0xa5a5a5a5);
    {Cache modified(private_map.path);require(same_texture(low,*modified.load_bitmap_low_detail(0xf4f91373)),"planar padding changed admitted source pixels");}
    private_map.restore(floor_blob+floor_offset+64);++controls;
    private_map.store(cube_record+0x64,640);
    {Cache modified(private_map.path);
        require(bool(modified.load_bitmap_low_detail_resource(cube_datum,cube_index)),"raw cube control absent");
        rejected([&]{modified.load_bitmap_low_detail(cube_datum,cube_index);});
    }
    private_map.restore(cube_record+0x64);++controls;
    for(const auto &control:special_controls){
        private_map.store(control.record+0xe,control.flags,2);
        private_map.store(control.record+0x68,control.format,2);
        {Cache modified(private_map.path);
            const auto raw=modified.load_bitmap_low_detail_resource(control.datum,control.index);
            require(raw && raw->flags==control.flags && raw->format==control.format,
                "raw API discarded unproved special cube descriptor");
            rejected([&]{modified.load_bitmap_low_detail(control.datum,control.index);});
        }
        private_map.restore(control.record+0xe,2);private_map.restore(control.record+0x68,2);++controls;
    }
    require(controls==25,"low-detail control coverage changed");
    std::cout<<"21 maps; 35,442 raw records; 115,785 original header words and 15,720 cube-face layout words; 22,486 planar and 524 cube textures / "
        <<decoded_bytes<<" bytes exact; 147 volume decodes explicitly rejected; "<<stored_bytes
        <<" stored versus "<<admitted_bytes<<" admitted bytes; 20 special cube-format header words; 25 controls and five full hardware textures preserved\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
