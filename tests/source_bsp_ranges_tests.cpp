#include "halo2/source_bsp_ranges.hpp"
#include "halo2/source_tag_metadata.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <numeric>
#include <string>

using namespace halo2;
namespace {
void require(bool ok,const std::string &message) {if(!ok)throw FormatError(message);}
template<class F>void rejected(F action,const std::string &message) {
    bool threw=false;try{action();}catch(const FormatError &){threw=true;}
    require(threw,message);
}
struct Reader {
    std::ifstream stream;
    explicit Reader(const char *path):stream(path,std::ios::binary){require(bool(stream),"original range fixture missing");}
    std::vector<std::byte> raw(std::size_t count) {
        require(count<16000000,"original range fixture byte extent invalid");
        std::vector<std::byte> out(count);
        stream.read(reinterpret_cast<char*>(out.data()),std::streamsize(count));
        require(bool(stream),"truncated original range fixture bytes");return out;
    }
    std::uint32_t word(){const auto value=raw(4);return Bytes(value).u32(0);}
    std::vector<std::byte> raw(){return raw(word());}
    std::vector<std::uint32_t> bank() {
        const auto count=word();require(count<1000000,"original range fixture word extent invalid");
        std::vector<std::uint32_t> out(count);for(auto &value:out)value=word();return out;
    }
};
template<std::size_t N>std::array<std::uint32_t,N> fixed(Reader &in) {
    const auto bank=in.bank();require(bank.size()==N,"original range fixed bank extent differs");
    std::array<std::uint32_t,N> out;std::copy(bank.begin(),bank.end(),out.begin());return out;
}
using Key=std::pair<unsigned,unsigned>;
struct LiteralGeometry {
    std::vector<std::byte> header,parts,subparts,accelerator,mapping,indices;
};
struct NativeGeometry {SourceBspRangeGeometry geometry;std::vector<std::uint16_t> indices;};
struct ExpectedDraw {unsigned primitive{},first{},count{};std::vector<std::uint32_t> indices;};
struct ExpectedPart {unsigned part{},returned{};std::vector<ExpectedDraw> draws;};
struct Unit {
    std::int32_t geometry{};Key key;unsigned mask_offset{};
    SourceCullDrawUnit before,after;
    std::optional<Transform> transform;
    std::vector<std::uint32_t> mask;
    std::vector<ExpectedPart> parts;
};
struct Allocation {std::int32_t subparts{};unsigned before{},after{},address{},offset{};};
struct Case {
    unsigned tick{};SourceBspFrustumCullContext context;
    SourcePortalVisibilitySet visibility;
    std::vector<std::array<std::uint32_t,4>> associations;
    SourceBspRangeArena initial,before,after,final;
    std::vector<Allocation> allocations;std::vector<Unit> units;
};
struct Words {
    std::vector<std::uint32_t> bank;std::size_t at{};
    std::uint32_t take(){require(at<bank.size(),"original visibility truncated");return bank[at++];}
    std::int16_t short_value(){const auto v=take();require(v<=65535,"original visibility short invalid");return std::bit_cast<std::int16_t>(std::uint16_t(v));}
};
Case read_case(Reader &in) {
    Case c;c.tick=in.word();auto &context=c.context;
    context.main_prefix=in.word();context.draw_count=in.word();context.mode=in.word();
    context.query_mode=std::bit_cast<std::int32_t>(in.word());context.mask_arena_address=in.word();
    require(in.bank().size()==10,"original camera input extent differs");
    const auto groups=in.word();require(groups<=6,"original visibility group extent differs");
    for(unsigned i=0;i<groups;++i)c.visibility.groups.push_back(std::bit_cast<SourceFrustumCamera>(fixed<111>(in)));
    Words data{in.bank()};const auto clusters=data.take();require(clusters<=128,"original cluster extent differs");
    for(unsigned i=0;i<clusters;++i) {
        SourceClusterFrustumRanges cluster;cluster.source_cluster=data.short_value();
        for(auto &value:cluster.counts)value=data.short_value();
        for(auto &value:cluster.first_indices)value=data.short_value();
        c.visibility.clusters.push_back(cluster);
    }
    for(unsigned i=0;i<clusters;++i)c.visibility.sorted_cluster_indices.push_back(data.take());
    const auto frustums=data.take();require(frustums<=512,"original frustum extent differs");
    for(unsigned i=0;i<frustums;++i) {
        std::array<std::uint32_t,66> words{};
        // Existing original capture excludes unconsumed process-pointer word3.
        for(unsigned j=0;j<66;++j)if(j!=3)words[j]=data.take();
        c.visibility.frustums.push_back(std::bit_cast<SourceCameraFrustum>(words));
    }
    require(data.at==data.bank.size(),"original visibility trailing words");
    const auto associations=in.word();require(associations<=512,"original association extent differs");
    for(unsigned i=0;i<associations;++i)c.associations.push_back(fixed<4>(in));
    for(auto *arena:{&c.initial,&c.before,&c.after,&c.final}) {
        const auto used=in.word();require(used<=512,"original arena used exceeds backing");
        arena->used=std::uint16_t(used);arena->words=fixed<512>(in);
    }
    const auto allocations=in.word();require(allocations<=850,"original allocation extent differs");
    for(unsigned i=0;i<allocations;++i)c.allocations.push_back({std::bit_cast<std::int32_t>(in.word()),in.word(),in.word(),in.word(),in.word()});
    const auto units=in.word();require(units<=850,"original draw-unit extent differs");
    for(unsigned i=0;i<units;++i) {
        Unit unit;unit.geometry=std::bit_cast<std::int32_t>(in.word());
        unit.key={in.word(),in.word()};unit.mask_offset=in.word();
        unit.before.words=fixed<8>(in);unit.after.words=fixed<8>(in);
        const auto transform=in.bank();require(transform.empty() || transform.size()==13,"original transform extent differs");
        if(!transform.empty()) {
            std::array<std::uint32_t,13> words;std::copy(transform.begin(),transform.end(),words.begin());
            unit.transform=std::bit_cast<Transform>(words);
        }
        unit.mask=in.bank();const auto parts=in.word();require(parts<=2048,"original part extent differs");
        for(unsigned p=0;p<parts;++p) {
            ExpectedPart part;part.part=in.word();part.returned=in.word();const auto draws=in.word();
            require(draws<=2048,"original submitted range extent differs");
            for(unsigned d=0;d<draws;++d) {
                ExpectedDraw draw;draw.primitive=in.word();draw.first=in.word();draw.count=in.word();draw.indices=in.bank();
                require(draw.indices.size()==draw.count,"original submitted index extent differs");part.draws.push_back(std::move(draw));
            }
            unit.parts.push_back(std::move(part));
        }
        c.units.push_back(std::move(unit));
    }
    return c;
}
// Validate every literal fixture geometry resource against the independently
// loaded map, including sections unused at these four camera ticks. This uses
// no native range/visibility result as an expected value.
void validate_map(const Cache &cache,const std::map<Key,LiteralGeometry> &literal) {
    const auto owned=cache.load_source_bsp_metadata(0);const Bytes m(owned.bytes_at(owned.source_address(),owned.size()));
    const auto pointer=[&](unsigned value){require(value>=owned.source_address(),"original metadata pointer invalid");return std::size_t(value-owned.source_address());};
    const auto clusters=m.u32(16+0x9c),definitions=m.u32(16+0x138);
    require(clusters==70 && definitions==73 && literal.size()==clusters+definitions,"actual original BSP section coverage changed");
    const auto raw=cache.load_bsp_resources(0);require(raw.size()==literal.size(),"actual raw section allocation count differs");
    for(const auto &[key,expected]:literal) {
        const auto [kind,section]=key;require(kind<=1 && section<(kind?definitions:clusters),"fixture original section identity invalid");
        const auto at=pointer(m.u32(16+(kind?0x138:0x9c)+4))+section*(kind?0xc8:0xb0);
        const Bytes bytes(raw.at(kind?clusters+section:section));const auto base=std::size_t(m.u32(at+48))+8;
        const auto header=bytes.slice(8,base-8);
        require(std::equal(header.begin(),header.end(),expected.header.begin(),expected.header.end()),"actual raw geometry header differs from original fixture");
        std::map<std::pair<unsigned,unsigned>,std::vector<std::byte>> resources;
        const auto offset=pointer(m.u32(at+60));
        for(unsigned i=0;i<m.u32(at+56);++i) {
            const auto r=offset+i*16;const auto payload=bytes.slice(base+m.u32(r+12),m.u32(r+8));
            require(resources.emplace(std::pair{m.u16(r+4),m.u16(r+6)},std::vector<std::byte>(payload.begin(),payload.end())).second,"duplicate actual raw resource identity");
        }
        for(const auto &[identity,value]:std::array<std::pair<std::pair<unsigned,unsigned>,const std::vector<std::byte>*>,5>{{
            {{0,72},&expected.parts},{{8,8},&expected.subparts},{{40,65535},&expected.accelerator},{{48,2},&expected.mapping},{{32,2},&expected.indices}}}) {
            const auto found=resources.find(identity);
            require(found!=resources.end() && found->second==*value,"actual raw range resource differs from original fixture");
        }
    }
}
NativeGeometry load_native(const Cache &cache,Key key,const LiteralGeometry &literal) {
    NativeGeometry n;n.geometry=load_source_bsp_range_geometry(cache,0,
        key.first?MenuWorldSourceSectionKind::bsp_instance_definition:MenuWorldSourceSectionKind::bsp_cluster,key.second);
    const Bytes header(literal.header);const auto &g=n.geometry;
    require(g.part_count==std::int32_t(header.u32(0)) && g.subpart_count==std::int32_t(header.u32(8)) &&
        g.accelerator_count==std::int32_t(header.u32(0x30)) && g.index_count==header.u32(0x20),"range-only loader literal counts differ");
    std::vector<std::byte> parts,subparts,mapping;
    for(const auto &value:g.parts)parts.insert(parts.end(),value.begin(),value.end());
    for(const auto &value:g.subparts)subparts.insert(subparts.end(),value.begin(),value.end());
    for(const auto value:g.mapping){mapping.push_back(std::byte(value));mapping.push_back(std::byte(value>>8));}
    require(parts==literal.parts && subparts==literal.subparts && g.accelerator==literal.accelerator && mapping==literal.mapping,
        "range-only loader literal resources differ");
    const Bytes indices(literal.indices);require(indices.size()==g.index_count*2,"original index count differs");
    for(unsigned i=0;i<g.index_count;++i)n.indices.push_back(indices.u16(i*2));return n;
}
void equal_arena(const SourceBspRangeArena &a,const SourceBspRangeArena &b,const std::string &name) {
    require(a.used==b.used,name+" used count differs");
    for(unsigned i=0;i<512;++i)require(a.words[i]==b.words[i],name+" word "+std::to_string(i)+" differs");
}
std::vector<SourceCullDrawUnit> input_units(const Case &c) {
    std::vector<SourceCullDrawUnit> out;for(const auto &unit:c.units)out.push_back(unit.before);return out;
}
void equal_units(std::span<const SourceCullDrawUnit> a,std::span<const SourceCullDrawUnit> b,const std::string &name) {
    require(a.size()==b.size(),name+" count differs");
    for(unsigned i=0;i<a.size();++i)for(unsigned j=0;j<8;++j)
        require(a[i].words[j]==b[i].words[j],name+" unit "+std::to_string(i)+" word "+std::to_string(j)+" differs");
}
void allocation_controls() {
    SourceBspRangeArena a;for(unsigned i=0;i<512;++i)a.words[i]=0x6a000000u+i;
    const auto words=a.words;
    require(!a.allocate(0) && !a.allocate(-1) && a.used==0 && a.words==words,"nonpositive original allocation changed arena");
    require(a.allocate(33)==0 && a.used==2 && a.words==words,"original allocation cleared its retained storage");
    a.used=510;require(a.allocate(32)==510 && a.used==511 && a.words==words,"last nonoverflow source allocation differs");
    require(a.allocate(1)==0 && a.used==511 && a.words==words,"exact-end512 original allocation did not alias arena beginning");
    a.words[0]|=1u<<17;const auto aliased=a.words;
    require(a.allocate(64)==0 && a.used==511 && a.words==aliased,"repeated overflow allocation lost shared alias writes");
    a.used=500;require(a.allocate(385)==0 && a.used==500 && a.words==aliased,"greater-than512 overflow changed arena ownership");
}
void atomic_controls(const Case &c,std::span<const SourceBspFrustumDrawBank> source_banks) {
    std::vector<unsigned> eligible;
    for(unsigned i=0;i<c.units.size();++i)if((c.units[i].before.words[0]&0x41)==0x40)eligible.push_back(i);
    require(eligible.size()>1,"original case lacks a multi-unit atomic control");
    const auto fail=[&](std::vector<SourceBspFrustumDrawBank> banks,SourceBspFrustumCullContext context) {
        auto units=input_units(c);const auto original=units;auto arena=c.before;
        rejected([&]{source_bsp_frustum_cull(units,banks,arena,context,c.visibility,c.associations);},"unsupported range cull accepted");
        equal_units(units,original,"atomic rejected cull");equal_arena(arena,c.before,"atomic rejected cull arena");
    };
    auto banks=std::vector<SourceBspFrustumDrawBank>(source_banks.begin(),source_banks.end());
    banks[eligible.back()].geometry_resolved=false;fail(banks,c.context);
    banks.assign(source_banks.begin(),source_banks.end());banks[eligible.back()].geometry=nullptr;fail(banks,c.context);
    banks.assign(source_banks.begin(),source_banks.end());auto context=c.context;context.query_mode=1;fail(banks,context);
    // Corrupt a late consumed stream only; prior complete unit mutations must
    // still be rolled back. No oracle expectation comes from this mutation.
    auto corrupt=*banks[eligible.back()].geometry;unsigned tree=0;
    if((c.units[eligible.back()].before.words[0]&0x1000) && std::int64_t(corrupt.accelerator_count)>2ll*corrupt.subpart_count)
        tree=(Bytes(corrupt.accelerator).u32(0x20)+15)&~15u;
    require(tree+48<corrupt.accelerator.size(),"actual accelerator control stream missing");
    corrupt.accelerator[tree+48]=std::byte{0xff};banks[eligible.back()].geometry=&corrupt;fail(banks,c.context);
    auto units=input_units(c);for(auto &unit:units)unit.words[0]|=1;const auto original=units;auto arena=c.before;
    source_bsp_frustum_cull(units,{},arena,context,{},{});
    equal_units(units,original,"already admitted unread services");equal_arena(arena,c.before,"already admitted unread arena");
    const auto &g=*source_banks[eligible.front()].geometry;
    std::vector<std::uint32_t> mask((unsigned(g.subpart_count)+31)/32,0x10101010u);const auto original_mask=mask;
    std::uint32_t flags=0x40;const std::array<std::uint32_t,2> direct{0,0xffff};
    rejected([&]{source_bsp_expand_accelerator_lists(g,false,direct,{},mask,flags);},"invalid subpart list accepted");
    require(mask==original_mask && flags==0x40,"invalid expansion partially wrote flags/mask");
    // Original45019 reaches neither owner planes nor mask storage with zero
    // groups. Original458D0/45A80 also leave a null mask unread when a complete
    // accelerator query returns both lists empty.
    std::vector<SourceCullDrawUnit> unread{c.units[eligible.front()].before};
    unread[0].words[5]=0;unread[0].words[7]=1;const auto no_groups_input=unread;
    auto no_groups=c.visibility;no_groups.groups.clear();context=c.context;
    context.main_prefix=context.draw_count=1;context.mask_arena_address=0;
    std::vector<SourceBspFrustumDrawBank> one_bank{source_banks[eligible.front()]};one_bank[0].transform.reset();
    arena=c.before;source_bsp_frustum_cull(unread,one_bank,arena,context,no_groups,c.associations);
    equal_units(unread,no_groups_input,"zero groups unread owner/mask");equal_arena(arena,c.before,"zero groups unread arena");
    auto empty_geometry=*one_bank[0].geometry;unsigned selected=0;
    if((unread[0].words[0]&0x1000) && std::int64_t(empty_geometry.accelerator_count)>2ll*empty_geometry.subpart_count)
        selected=(Bytes(empty_geometry.accelerator).u32(0x20)+15)&~15u;
    require(selected+48<empty_geometry.accelerator.size(),"actual empty-query control stream missing");
    empty_geometry.accelerator[selected+48]=std::byte{};one_bank[0].geometry=&empty_geometry;
    unread[0].words[7]=0;const auto empty_input=unread;
    source_bsp_frustum_cull(unread,one_bank,arena,context,c.visibility,c.associations);
    equal_units(unread,empty_input,"complete empty query unread mask");equal_arena(arena,c.before,"complete empty query unread arena");
}
}
int main(int argc,char **argv) {try {
    allocation_controls();
    require(argc==3,"source BSP ranges test requires original mainmenu map and binary original fixture");
    if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]))return 77;
    Cache cache(argv[1]);Reader in(argv[2]);const auto magic=in.raw(8);
    const std::array<std::byte,8> expected_magic{std::byte{'H'},std::byte{'2'},std::byte{'B'},std::byte{'R'},std::byte{'G'},std::byte{'0'},std::byte{'0'},std::byte{'1'}};
    require(std::equal(magic.begin(),magic.end(),expected_magic.begin()),"unknown original range fixture schema");
    (void)in.raw(64); // Retained original manifest SHA and literal map SHA.
    const auto bank_count=in.word(),case_count=in.word();require(bank_count==143 && case_count==4,"original range fixture coverage changed");
    std::map<Key,LiteralGeometry> literals;
    for(unsigned i=0;i<bank_count;++i) {
        const Key key{in.word(),in.word()};LiteralGeometry g;
        g.header=in.raw();g.parts=in.raw();g.subparts=in.raw();g.accelerator=in.raw();g.mapping=in.raw();g.indices=in.raw();
        require(literals.emplace(key,std::move(g)).second,"duplicate original range fixture geometry");
    }
    validate_map(cache,literals);std::map<Key,NativeGeometry> native;
    unsigned unit_count=0,draw_count=0,empty_parts=0,multiple_parts=0;std::size_t index_count=0;bool instance58=false;
    const std::array<unsigned,4> ticks{0,181,293,840};
    for(unsigned case_index=0;case_index<case_count;++case_index) {
        const auto c=read_case(in);const auto label="original BSP ranges tick "+std::to_string(c.tick);
        require(c.tick==ticks[case_index],"original camera case order changed");
        auto allocated=c.initial;
        for(const auto &a:c.allocations) {
            require(allocated.used==a.before,label+" allocation initial used differs");
            const auto offset=allocated.allocate(a.subparts);
            require(offset && *offset==a.offset && allocated.used==a.after && a.address==c.context.mask_arena_address+4*a.offset,
                label+" actual1332B0 allocation differs");
        }
        equal_arena(allocated,c.before,label+" allocation retained arena");
        std::vector<SourceBspFrustumDrawBank> banks;
        for(const auto &unit:c.units) {
            if(!native.contains(unit.key))native.emplace(unit.key,load_native(cache,unit.key,literals.at(unit.key)));
            banks.push_back({&native.at(unit.key).geometry,true,unit.transform});
            require((unit.before.words[7]!=0)==bool(unit.transform),label+" actual transform presence differs");
        }
        auto units=input_units(c);auto arena=c.before;
        source_bsp_frustum_cull(units,banks,arena,c.context,c.visibility,c.associations);
        std::vector<SourceCullDrawUnit> expected;for(const auto &unit:c.units)expected.push_back(unit.after);
        equal_units(units,expected,label);equal_arena(arena,c.after,label+" post44720 arena");
        equal_arena(arena,c.final,label+" post4DAA0 retained arena");
        for(unsigned i=0;i<units.size();++i) {
            const auto &unit=c.units[i];const auto &g=native.at(unit.key);std::span<const std::uint32_t> mask;
            if(units[i].words[5]) {
                require(unit.mask_offset<512 && units[i].words[5]==c.context.mask_arena_address+4*unit.mask_offset,label+" actual mask pointer differs");
                mask=std::span<const std::uint32_t>(arena.words).subspan(unit.mask_offset,(unsigned(g.geometry.subpart_count)+31)/32);
                require(std::equal(mask.begin(),mask.end(),unit.mask.begin(),unit.mask.end()),label+" actual unit mask differs");
            } else require(unit.mask.empty() && unit.mask_offset==0xffffffffu,label+" null source mask has fabricated storage");
            require(unit.parts.size()==g.geometry.parts.size(),label+" original part coverage differs");
            for(const auto &part:unit.parts) {
                const Bytes authored(g.geometry.parts.at(part.part));
                // Exact4D5E2..4D617 caller inputs; primitive5=list,6=strip.
                const unsigned primitive=(units[i].words[0]&0x200) && !(authored.u16(2)&8)?6:5;
                const auto topology=primitive==6?MenuWorldSourceTopology::triangle_strip:MenuWorldSourceTopology::triangle_list;
                const bool filtered=(units[i].words[0]&0x40) && units[i].words[5] && g.geometry.subpart_count!=0;
                const auto ranges=source_bsp_part_ranges(g.geometry,part.part,topology,filtered,mask);
                require(ranges.size()==part.draws.size(),label+" unit "+std::to_string(i)+" part "+std::to_string(part.part)+" submission count differs");
                unsigned returned=0;
                for(unsigned d=0;d<ranges.size();++d) {
                    const auto &range=ranges[d];const auto &draw=part.draws[d];
                    require(draw.primitive==primitive && range.first==draw.first && range.count==draw.count,label+" source-ordered range/topology differs");
                    require(range.first<=g.indices.size() && range.count<=g.indices.size()-range.first,label+" submitted index range escapes its source");
                    for(unsigned j=0;j<range.count;++j)require(g.indices[range.first+j]==draw.indices[j],label+" literal submitted index differs");
                    returned+=range.count;++draw_count;index_count+=range.count;
                }
                require(returned==part.returned,label+" original4DAA0 returned count differs");
                empty_parts+=ranges.empty();multiple_parts+=ranges.size()>1;
                if(c.tick==293 && unit.geometry==-60 && part.part==2) {
                    require(filtered && ranges==std::vector<MenuWorldSourceIndexRange>{{702,522}},"partial original instance58 part2 regression");instance58=true;
                }
            }
        }
        atomic_controls(c,banks);unit_count+=unsigned(units.size());
    }
    require(in.stream.peek()==std::char_traits<char>::eof(),"original range fixture trailing bytes");
    require(unit_count==344 && draw_count==851 && index_count==316896 && empty_parts==85 && multiple_parts==14 && instance58,
        "integrated original range coverage changed");
    std::cout<<"4 unchanged original BSP cases; "<<unit_count<<" exact unit banks, 2048 arena words, "<<draw_count
        <<" ordered ranges, "<<index_count<<" literal indices; 1332B0 alias/no-clear and atomic rejection controls passed\n";
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
