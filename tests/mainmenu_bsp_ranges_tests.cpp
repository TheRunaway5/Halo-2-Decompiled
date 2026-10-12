#include "halo2/mainmenu_bsp_ranges.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>

using namespace halo2;
namespace {
void require(bool ok,const std::string &message){if(!ok)throw FormatError(message);}
struct Reader {
    std::ifstream stream;
    explicit Reader(const char *path):stream(path,std::ios::binary){require(bool(stream),"mainmenu original ranges fixture missing");}
    std::uint32_t word() {
        std::array<unsigned char,4> b{};stream.read(reinterpret_cast<char*>(b.data()),4);
        require(bool(stream),"truncated original mainmenu range word");
        return unsigned(b[0])|(unsigned(b[1])<<8)|(unsigned(b[2])<<16)|(unsigned(b[3])<<24);
    }
    std::vector<std::uint32_t> bank() {
        const auto count=word();require(count<1000000,"original mainmenu range bank exceeds bounds");
        std::vector<std::uint32_t> out(count);for(auto &v:out)v=word();return out;
    }
};
// Independent literal index-bank access avoids the renderer's vertex-format
// admission and verifies original submitted ushort values, not only counts.
struct Indices {
    SourceTagMetadata owned;Bytes metadata;
    std::vector<std::vector<std::byte>> resources;
    std::map<std::pair<bool,unsigned>,std::vector<std::uint16_t>> memo;
    explicit Indices(const Cache &cache):owned(cache.load_source_bsp_metadata(0)),
        metadata(owned.bytes_at(owned.source_address(),owned.size())),resources(cache.load_bsp_resources(0)) {}
    std::size_t offset(unsigned p) const {
        require(p>=owned.source_address(),"literal index metadata pointer precedes allocation");return p-owned.source_address();
    }
    const std::vector<std::uint16_t> &get(bool instance,unsigned section) {
        const auto key=std::pair{instance,section};const auto found=memo.find(key);if(found!=memo.end())return found->second;
        const auto count=metadata.u32(16+(instance?0x138:0x9c));require(section<count,"literal index section escapes its bank");
        const auto at=offset(metadata.u32(16+(instance?0x138:0x9c)+4))+section*(instance?0xc8:0xb0);
        const Bytes raw(resources.at((instance?metadata.u32(16+0x9c):0)+section));const auto base=metadata.u32(at+48)+8;
        const auto entries=offset(metadata.u32(at+60));std::vector<std::uint16_t> out;bool present=false;
        for(unsigned i=0;i<metadata.u32(at+56);++i) {
            const auto r=entries+i*16;if(metadata.u16(r+4)!=32 || metadata.u16(r+6)!=2)continue;
            require(!present,"literal index resource ambiguous");present=true;
            const auto bytes=raw.slice(base+metadata.u32(r+12),metadata.u32(r+8));require(bytes.size()%2==0,"literal ushort index resource truncated");
            const Bytes values(bytes);for(unsigned j=0;j<bytes.size()/2;++j)out.push_back(values.u16(j*2));
        }
        require(present,"literal original index resource absent");return memo.emplace(key,std::move(out)).first->second;
    }
};
void incomplete(const MainmenuBspRangeFrame &result,const std::string &name) {
    require(!result.complete && !result.diagnostic.empty() && result.units.empty() && result.zero_alpha_instances.empty() &&
        result.arena.used==0 && std::all_of(result.arena.words.begin(),result.arena.words.end(),[](auto v){return v==0;}),
        name+" exposed a partial range schedule/arena");
}
void failure_controls(MainmenuBspRangeProducer &producer,const MainmenuBspAdmissionFrame &frame) {
    auto bad=frame;bad.location_query_succeeded=false;incomplete(producer.prepare(bad),"outside frame");
    bad=frame;require(!bad.admission.clusters.empty(),"original control needs clusters");
    bad.admission.cluster_entries[bad.admission.clusters.back().id]=0xff;
    incomplete(producer.prepare(bad),"missing constructed entry");
    bad=frame;bad.admission.instances.push_back({2048,0xfc00,0xffff,0});
    incomplete(producer.prepare(bad),"late unowned instance");
    bad=frame;require(!bad.visibility.frustums.empty(),"original control needs frustums");
    bad.visibility.frustums.front().planes[0][0]=std::numeric_limits<float>::quiet_NaN();
    incomplete(producer.prepare(bad),"unavailable consumed accelerator planes");
    // Cache warming/failure does not contaminate a later ordinary source frame.
    require(producer.prepare(frame).complete,"failed schedule contaminated valid subsequent frame");
}
}
int main(int argc,char **argv){try {
    require(argc==3,"mainmenu BSP ranges test requires original mainmenu map and original producer fixture");
    if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]))return 77;
    Cache cache(argv[1]);MainmenuBspFrameProducer frame_producer(cache,0);MainmenuBspRangeProducer producer(cache,0);Indices indices(cache);
    Reader in(argv[2]);std::array<char,8> magic{};in.stream.read(magic.data(),magic.size());
    require(magic==std::array<char,8>{'H','2','M','P','R','0','0','1'},"unknown mainmenu original range fixture schema");
    std::array<char,64> provenance{};in.stream.read(provenance.data(),provenance.size());require(bool(in.stream),"original range provenance missing");
    const auto count=in.word();require(count==4,"original mainmenu range camera coverage differs");
    const std::array<unsigned,4> ticks{0,181,293,840};
    unsigned units_checked=0,ranges_checked=0,empty_parts=0,multiple_parts=0,zero_alpha=0,faded=0,partial=0;
    std::size_t indices_checked=0;bool instance58=false;
    for(unsigned case_index=0;case_index<count;++case_index) {
        const auto tick=in.word();require(tick==ticks[case_index],"original mainmenu range case order differs");
        const auto input=in.bank();require(input.size()==32,"original full mainmenu frame input extent differs");
        const auto real=[&](unsigned at){return std::bit_cast<float>(input[at]);};
        Camera camera;camera.position={real(0),real(1),real(2)};camera.forward={real(3),real(4),real(5)};camera.up={real(6),real(7),real(8)};
        camera.projection_vertical_angle=real(9);camera.horizontal_field_of_view=true;camera.oriented=true;
        camera.projection_clip_scale={544.f/640.f,408.f/480.f};
        const BspAtmosphere::CameraLocation previous{std::bit_cast<std::int32_t>(input[10]),std::bit_cast<std::int32_t>(input[11])};
        SourcePortalMaskState portals;for(unsigned i=0;i<16;++i)portals.words[i]=input[12+i];
        const SourceBspShaderLodAuxiliary cinematic{bool(input[28]),bool(input[29])};
        const std::optional<std::uint32_t> override_flags=input[30]?std::optional(input[31]):std::nullopt;
        const auto frame=frame_producer.prepare({camera,previous,portals,cinematic,override_flags});
        const auto actual=producer.prepare(frame);const auto label="original mainmenu range tick "+std::to_string(tick);
        require(actual.complete && actual.diagnostic.empty(),label+" incomplete: "+actual.diagnostic);
        const auto used=in.word();const auto arena=in.bank();require(arena.size()==512,"original full arena bank missing");
        require(actual.arena.used==used && std::equal(arena.begin(),arena.end(),actual.arena.words.begin()),label+" full source arena differs");
        const auto skipped=in.bank();require(skipped==actual.zero_alpha_instances,label+" source alpha-zero skip queue differs");zero_alpha+=unsigned(skipped.size());
        for(const auto id:skipped)require(!actual.unit_for_geometry(-2-int(id)),label+" skipped alpha-zero instance was scheduled");
        const auto units=in.word();require(units==actual.units.size(),label+" source queue unit count differs");
        for(unsigned i=0;i<units;++i) {
            const auto geometry=std::bit_cast<std::int32_t>(in.word());const auto section=in.word(),flags=in.word(),coverage=in.word(),alpha=in.word(),mask=in.word();
            const auto transform=in.bank();const auto &unit=actual.units[i];
            require(unit.geometry==geometry && unit.section==section && unit.flags==flags && unit.coverage==coverage && unit.alpha==alpha &&
                unit.mask_word_offset.value_or(0xffffffffu)==mask,label+" unit "+std::to_string(i)+" source schedule fields differ");
            require(actual.unit_for_geometry(geometry)==&unit,label+" geometry lookup changed source queue identity");
            require(transform.size()==(unit.transform?13:0),label+" literal owner transform presence differs");
            if(unit.transform) {
                const auto words=std::bit_cast<std::array<std::uint32_t,13>>(*unit.transform);
                require(std::equal(transform.begin(),transform.end(),words.begin()),label+" literal owner transform bytes differ");
            }
            require((geometry>=0 && !unit.transform) || (geometry<=-2 && unit.transform),label+" source collector geometry/transform ownership differs");
            const auto &source_indices=indices.get(geometry<=-2,section);
            const auto parts=in.word();require(parts==unit.parts.size(),label+" authored part count differs");
            for(unsigned p=0;p<parts;++p) {
                const auto part=in.word(),returned=in.word(),draws=in.word();const auto &actual_part=unit.parts[p];
                require(part==actual_part.part && draws==actual_part.ranges.size(),label+" ordered part/submission count differs");
                unsigned sum=0;
                for(unsigned d=0;d<draws;++d) {
                    const auto primitive=in.word(),first=in.word(),size=in.word();const auto values=in.bank();const auto range=actual_part.ranges[d];
                    require((primitive==5 || primitive==6) && actual_part.topology==(primitive==5?MenuWorldSourceTopology::triangle_list:MenuWorldSourceTopology::triangle_strip) &&
                        first==range.first && size==range.count && values.size()==size,label+" original primitive/ordered range differs");
                    require(first<=source_indices.size() && size<=source_indices.size()-first,label+" submitted index range escapes original allocation");
                    for(unsigned j=0;j<size;++j)require(source_indices[first+j]==values[j],label+" original submitted ushort index differs");
                    ++ranges_checked;indices_checked+=size;sum+=size;
                }
                require(sum==returned,label+" original4DAA0 returned index count differs");empty_parts+=draws==0;multiple_parts+=draws>1;
                if(tick==293 && geometry==-60 && part==2) {
                    require((unit.flags&0x40) && actual_part.ranges==std::vector<MenuWorldSourceIndexRange>{{702,522}},"partial instance58 original part2 regression");instance58=true;
                }
            }
            faded+=unit.alpha<255;partial+=(geometry<=-2 && (unit.flags&0x40));++units_checked;
        }
        failure_controls(producer,frame);
    }
    require(in.stream.peek()==std::char_traits<char>::eof(),"original mainmenu range fixture trailing bytes");
    require(units_checked==344 && ranges_checked==851 && indices_checked==316896 && empty_parts==85 && multiple_parts==14 &&
        zero_alpha==9 && faded==4 && partial>0 && instance58,"original end-to-end producer coverage changed");
    std::cout<<"4 source camera/frame to BSP-prefix schedule/range cases: "<<units_checked<<" units, 2048 arena words, "<<ranges_checked
        <<" ranges, "<<indices_checked<<" ushort indices; actual metric/alpha/transform bytes, 9alpha-zero skips, 4faded units and atomic incomplete controls passed\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
