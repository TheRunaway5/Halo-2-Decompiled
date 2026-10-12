#include "halo2/source_effect_light_opaque.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
using namespace halo2;
namespace {
void require(bool condition,const char* why){if(!condition)throw FormatError(why);}
struct Reader {std::span<const std::uint32_t> data;unsigned at{};std::uint32_t word(){require(at<data.size(),"source light range fixture truncated");return data[at++];}};
using Key=std::pair<MenuWorldSourceSectionKind,unsigned>;
std::vector<std::uint16_t> indices(const Cache& cache,Key key) {
    const auto owned=cache.load_source_bsp_metadata(0);const auto base=owned.source_address();const Bytes m(owned.bytes_at(base,owned.size()));
    const bool instance=key.first==MenuWorldSourceSectionKind::bsp_instance_definition;
    const auto field=16+(instance?0x138:0x9c);require(key.second<m.u32(field),"source light range index section missing");
    const auto at=m.u32(field+4)-base+key.second*(instance?200:176);const auto allocations=cache.load_bsp_resources(0);const Bytes raw(allocations.at((instance?m.u32(16+0x9c):0)+key.second));
    const auto resource_base=m.u32(at+48)+8;const auto count=m.u32(at+56),resources=m.u32(at+60)-base;std::vector<std::uint16_t> out;bool present=false;
    for(unsigned i=0;i<count;++i) {
        const auto entry=resources+i*16;if(m.u16(entry+4)!=32 || m.u16(entry+6)!=2)continue;
        require(!present,"source light range index resource ambiguous");present=true;const auto bytes=raw.slice(resource_base+m.u32(entry+12),m.u32(entry+8));require(bytes.size()%2==0,"source light range index resource incomplete");const Bytes b(bytes);for(unsigned j=0;j<bytes.size()/2;++j)out.push_back(b.u16(j*2));
    }
    require(present,"source light range index resource absent");return out;
}
void same(const SourceOpaquePartRanges& a,const SourceOpaquePartRanges& b) {
    require(a.topology==b.topology && a.filtered==b.filtered && a.ranges==b.ranges,"source ordered range record differs");
}
}
int main(int argc,char** argv)try {
    const char* fixture=argc>1?argv[1]:HALO2_SOURCE_DIR "/tests/fixtures/effect_light_draw_ranges.original.bin";
    const char* map=argc>2?argv[2]:HALO2_SOURCE_DIR "/assets/disc/maps/lockout.map";
    const Cache cache(map);std::ifstream f(fixture,std::ios::binary|std::ios::ate);require(bool(f),"source light range fixture missing");const auto size=f.tellg();require(size>=12 && size%4==0,"source light range fixture extent");std::vector<std::uint32_t> data(std::size_t(size)/4);f.seekg(0);f.read(reinterpret_cast<char*>(data.data()),size);require(bool(f),"source light range fixture read failed");Reader r{data};require(r.word()==0x474e524c && r.word()==1,"source light range fixture schema");const auto cases=r.word();
    std::map<Key,SourceBspRangeGeometry> geometries;std::map<Key,std::vector<std::uint16_t>> index_banks;
    SourcePartCommandBank commands;commands.storage.resize(1024);commands.header[1]=1024;
    std::vector<SourceCullDrawUnit> units(850);std::vector<SourceOpaqueSortRecord> sorted;std::vector<SourceOpaquePartRanges> expected;std::map<unsigned,Key> unit_keys;std::vector<unsigned> expected_classes;
    SourceBspRangeArena original_arena;std::uint32_t original_base=0;std::uint64_t draws=0,index_words=0;
    for(unsigned c=0;c<cases;++c) {
        const auto command=r.word(),index=r.word(),vertex_class=r.word();const auto kind=r.word(),section=r.word();require(kind<2,"source light range geometry kind invalid");const Key key{kind?MenuWorldSourceSectionKind::bsp_instance_definition:MenuWorldSourceSectionKind::bsp_cluster,section};
        SourceCullDrawUnit unit;for(auto& v:unit.words)v=r.word();const auto part=r.word(),flags=r.word(),subparts=r.word(),arena_base=r.word();SourceBspRangeArena arena;for(auto& v:arena.words)v=r.word();
        if(!geometries.contains(key)){geometries.emplace(key,load_source_bsp_range_geometry(cache,0,key.first,key.second));index_banks.emplace(key,indices(cache,key));}
        auto geometry=geometries.at(key);require(part<geometry.parts.size(),"source light range part missing");const auto half=std::uint16_t(flags);std::memcpy(geometry.parts[part].data()+2,&half,2);geometry.subpart_count=std::bit_cast<std::int32_t>(subparts);
        const auto selected=source_effect_light_part_ranges(unit,geometry,part,arena,arena_base);const auto topology=r.word(),filtered=r.word(),returned=r.word(),n=r.word();require(topology<2 && selected.topology==(topology?MenuWorldSourceTopology::triangle_strip:MenuWorldSourceTopology::triangle_list) && selected.filtered==bool(filtered),"source light range flags differ");require(selected.ranges.size()==n,"source light range draw count differs");
        unsigned sum=0;for(unsigned i=0;i<n;++i) {
            const auto primitive=r.word(),first=r.word(),count=r.word();require(primitive==(topology?6u:5u) && selected.ranges[i]==MenuWorldSourceIndexRange{first,count},"source light ordered index interval differs");const auto& ib=index_banks.at(key);require(first<=ib.size() && count<=ib.size()-first,"source light original range escapes map indices");for(unsigned j=0;j<count;++j){require(ib[first+j]==r.word(),"source light submitted index differs from independent Cache");++index_words;}sum+=count;++draws;
        }
        require(sum==returned,"source light returned index count differs");
        if(c<13) {
            require(command<commands.storage.size() && index<units.size(),"source light actual bank ordinal invalid");commands.storage[command].words[1]=0x4d0b0;commands.storage[command].words[3]=(index<<8)|part;units[index]=unit;unit_keys[index]=key;SourceOpaqueSortRecord record;record.words[0]=command;record.words[2]=vertex_class;sorted.push_back(record);expected.push_back(selected);expected_classes.push_back(vertex_class);original_arena=arena;original_base=arena_base;
        }
    }
    require(r.at==data.size(),"source light range trailing fixture words");
    for(unsigned mode=0;mode<3;++mode) {
        unsigned consumed=0;const auto result=source_effect_light_ordered_ranges(commands,units,sorted,original_arena,original_base,[&](unsigned command,unsigned unit,unsigned vertex) {
            const auto ordinal=consumed++;require(ordinal<sorted.size() && command==std::uint16_t(sorted[ordinal].words[0]) && vertex==expected_classes[ordinal],"source light ordered geometry binding call differs");const bool bound=mode==0 || (mode==1 && ordinal%2==0);return SourceOpaqueRangeGeometry{bound?&geometries.at(unit_keys.at(unit)):nullptr,bound};
        });
        require(consumed==13,"source light sorted binding order lost a record");unsigned out=0;
        for(unsigned i=0;i<13;++i)if(mode==0 || (mode==1 && i%2==0)) {require(out<result.size(),"source light ordered output truncated");const auto& draw=result[out++];require(draw.command==std::uint16_t(sorted[i].words[0]) && draw.vertex_class==expected_classes[i],"source light ordered command regrouped");same(draw.selected,expected[i]);}
        require(out==result.size(),"source light failed vertex binding emitted a range");
    }
    std::cout<<"PASS "<<cases<<" original source light range cases / "<<draws<<" ordered indexed calls / "<<index_words<<" exact Cache indices / 3 sorted binding schedules\n";
} catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
