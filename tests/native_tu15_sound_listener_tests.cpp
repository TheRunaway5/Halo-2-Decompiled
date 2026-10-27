// Differential test: the native listener selection kernels (12A810, 12A930,
// 2287B0, 253070, 2531E0) against the lifted TU1.5 routines they replace,
// from identical randomized game states across x87 controls, MXCSR modes,
// caller x87 stack depths and special floats.
// Path coverage comes from the lifted routine's own instruction trace: every
// basic block an entry can reach (its own and its callees') must run in some
// state that does not fault.
#include "halo2/native_tu15_sound_manager.hpp"
#include "halo2/source_sound_listener_selection_tu15.hpp"
#include "native_tu15_sound_test_support.hpp"
#include <algorithm>
#include <set>
#include <vector>

using namespace halo2;
using namespace halo2::sound_test;
namespace {
// Retail globals the routines read (game runtime data). The .rdata constants
// they read (0x45695C FLT_MAX, 0x46A148 255.0, 0x46A26C 256.0, 0x46A2D8
// 1/253, 0x46A2D4 256/127) are mapped with their TU1.5 XBE values.
constexpr std::uint32_t listener_globals=0x4f2c50u,local_players=0x4f54f0u,structure_bsp=0x4ecc18u;
constexpr std::uint32_t listeners=0x02000000u,players=0x02100000u,structure=0x02200000u,scale_table=0x02300000u,
    cluster_bytes=0x02400000u,scale_bytes=0x02500000u,source=0x02600000u,result=0x02700000u,unmapped=0x7e000000u;

struct Builder {
    Random &r;Memory &m;
    // Rarely an unmapped pointer, so both sides must fault at the same access.
    std::uint32_t maybe_unmapped(std::uint32_t p){return r.below(100)?p:unmapped+(r.word()&0xfff0u);}
    float coordinate(){return r.below(10)?r.finite(100.f):r.scalar();}
    float positive(float scale){return r.below(8)?std::fabs(r.finite(scale)):r.scalar();}
    // A cluster: mostly small, sometimes -1 (none), rarely any word.
    std::uint16_t cluster() {
        if(!r.below(40))return std::uint16_t(r.word());
        return r.below(8)?std::uint16_t(r.below(8)):0xffffu;
    }
    std::uint8_t cell(){switch(r.below(6)){case 0:return 0;case 1:return 0xff;default:return std::uint8_t(r.word());}}
    // The structure's cluster tables: +0x224 scale rows (signed), +0x9C
    // clusters, +0xE8 cluster bytes; the scale table's +4..+8 range and
    // +0x28 bytes. Pair rows stay within +/-0x200 of the tables for the
    // clusters above (a rare any-word cluster or count faults).
    void structure_tables() {
        m.put32(structure_bsp,maybe_unmapped(structure));
        std::uint32_t rows;
        switch(r.below(6)){case 0:rows=0;break;case 1:rows=0u-(1u+r.below(3));break;case 2:rows=0x80000000u;break;
            default:rows=1u+r.below(40);}
        m.put32(structure+0x224u,rows);
        m.put32(structure+0x9cu,r.below(30)?r.below(12):r.word());
        m.put32(structure+0xe8u,maybe_unmapped(cluster_bytes));
        m.put32(structure+0x228u,maybe_unmapped(scale_table));
        m.putf(scale_table+4u,positive(300.f));m.putf(scale_table+8u,positive(300.f));
        m.put32(scale_table+0x28u,maybe_unmapped(scale_bytes));
        m.map(cluster_bytes-0x200u,0x400u);m.map(scale_bytes-0x200u,0x400u);
        for(std::uint32_t i=0;i<0x400u;++i) {
            *m.at(cluster_bytes-0x200u+i)=std::byte(cell());*m.at(scale_bytes-0x200u+i)=std::byte(cell());
        }
    }
    // Listener rows (0x48 bytes from +0x88): location +0, cluster +4, active
    // byte +6, position +0x30/+0x34/+0x38.
    void listener_rows(unsigned count) {
        m.put32(listener_globals,maybe_unmapped(listeners));
        for(unsigned i=0;i<count;++i) {
            const auto row=listeners+0x88u+i*0x48u;
            m.put32(row,r.below(4));m.put16(row+4u,cluster());
            m.put8(row+6u,r.below(4)?std::uint8_t(1u+r.below(255)):0u);
            m.putf(row+0x30u,coordinate());m.putf(row+0x34u,coordinate());m.putf(row+0x38u,coordinate());
        }
    }
    // A source: flags word +0, column byte +2, mode nibble +3, position
    // +0xC/+0x10/+0x14, location +0x28, cluster +0x2C, column height +0x30.
    void source_record(std::uint32_t mode) {
        m.put16(source,std::uint16_t(r.word()));
        m.put8(source+2u,r.below(2)?0u:std::uint8_t(r.word()));
        m.put8(source+3u,std::uint8_t((r.word()&0xf0u)|mode));
        m.putf(source+0xcu,coordinate());m.putf(source+0x10u,coordinate());m.putf(source+0x14u,coordinate());
        m.put32(source+0x28u,r.below(4));m.put16(source+0x2cu,cluster());
        m.putf(source+0x30u,positive(60.f));
    }
    std::uint32_t mode(){return r.below(3)?1u:r.below(16);}
    void local_player_slots() {
        m.put32(local_players,maybe_unmapped(players));
        for(unsigned i=0;i<4;++i)m.put32(players+0xcu+i*4u,r.below(2)?0xffffffffu:r.word());
    }
};

Case scenario(Random &r,std::uint32_t entry) {
    auto c=base_case(r);auto &m=c.memory;auto &a=c.abi;
    m.put32(0x45695cu,0x7f7fffffu);m.put32(0x46a148u,0x437f0000u);m.put32(0x46a26cu,0x43800000u);
    m.put32(0x46a2d8u,0x3b81848eu);m.put32(0x46a2d4u,0x40010204u);
    Builder b{r,m};
    switch(entry) {
    case 0x2531e0u: {
        b.structure_tables();
        a.gp[0]=(r.word()&0xffff0000u)|b.cluster();
        a.gp[1]=(r.word()&0xffff0000u)|(r.below(6)?b.cluster():std::uint16_t(a.gp[0]));
        a.gp[7]=b.maybe_unmapped(structure);
        break;
    }
    case 0x253070u: {
        b.structure_tables();
        a.gp[1]=std::uint32_t(std::int32_t(std::int16_t(b.cluster())));
        switch(r.below(8)) {
        case 0:a.gp[0]=a.gp[1];break;                                    // the same cluster
        case 1:a.gp[0]=(r.word()&0xffff0000u)|(a.gp[1]&0xffffu);break;   // same low word: 2531E0 gives 0
        default:a.gp[0]=std::uint32_t(std::int32_t(std::int16_t(b.cluster())));
        }
        a.gp[2]=b.maybe_unmapped(structure);
        break;
    }
    case 0x2287b0u: {
        b.structure_tables();b.listener_rows(4);b.source_record(b.mode());
        a.gp[1]=r.below(5)?r.below(4):r.below(20)?0xffffffffu:r.word();
        a.gp[2]=b.maybe_unmapped(source);
        m.putf(stack+4u,b.positive(3.f));                                 // distance over its limit
        break;
    }
    case 0x12a930u: {
        b.listener_rows(8);b.source_record(r.below(3)?r.below(2):r.below(16));
        a.gp[1]=b.maybe_unmapped(source);
        a.gp[2]=r.below(40)?r.below(8):r.word();
        break;
    }
    case 0x12a810u: {
        b.structure_tables();b.listener_rows(4);b.local_player_slots();
        b.source_record(r.below(4)?1u:r.below(16));
        m.put32(stack+4u,b.maybe_unmapped(source));
        m.putf(stack+8u,b.positive(400.f));                               // distance limit
        m.put32(stack+0xcu,r.below(4)?b.maybe_unmapped(result):0u);       // result pointer
        m.put32(result,r.word());
        break;
    }
    default:break;
    }
    return c;
}

std::set<std::uint32_t> *trace=nullptr;
void reference(SourcePhysicsContactMemory &m,std::uint32_t entry,SourcePhysicsContactRuntimeAbi &a,
    SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsContactRecordObserver &o) {
    SourcePhysicsWorldRuntimeServices services;
    services.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("unowned callee");};
    if(trace)services.instruction=[](std::uint32_t pc){trace->insert(pc);};
    source_run_sound_listener_selection_tu15(m,entry,a,fp,services,o);
}

// Basic-block leaders of each lifted routine (its first instruction, branch
// targets and fall-throughs, and call returns).
const std::vector<std::uint32_t> blocks_12a810{0x12a810u,0x12a822u,0x12a830u,0x12a835u,0x12a83eu,0x12a846u,
    0x12a850u,0x12a871u,0x12a876u,0x12a87fu,0x12a884u,0x12a889u,0x12a892u,0x12a8b3u,0x12a8b9u,0x12a8bcu,
    0x12a8c7u,0x12a8ceu,0x12a8e0u,0x12a8e8u,0x12a8edu,0x12a900u,0x12a905u,0x12a90eu,0x12a910u,0x12a912u,
    0x12a918u,0x12a920u,0x12a924u};
const std::vector<std::uint32_t> blocks_12a930{0x12a930u,0x12a946u,0x12a949u,0x12a974u,0x12a99au,0x12a9a2u,
    0x12a9a8u,0x12a9afu,0x12a9b2u,0x12a9bbu,0x12a9e0u,0x12a9eau,0x12aa1au};
const std::vector<std::uint32_t> blocks_2287b0{0x2287b0u,0x2287bau,0x2287c0u,0x2287cbu,0x2287e5u,0x2287f0u,
    0x2287f7u,0x2287fcu,0x22880du,0x228820u,0x228823u,0x228824u,0x228827u,0x22882eu};
const std::vector<std::uint32_t> blocks_253070{0x253070u,0x253082u,0x253087u,0x253091u,0x2530c9u,0x2530d0u,
    0x2530d4u,0x2530e0u,0x25310au,0x25310fu};
const std::vector<std::uint32_t> blocks_2531e0{0x2531e0u,0x2531e8u,0x2531eau,0x2531f0u,0x253219u};
// Unreachable through a caller: 12A810 calls 12A930 only for mode-1 sources
// (12A949 the x87 mode and 12AA1A mode 0 are not reached); 2287B0 calls
// 253070 only with distinct sign-extended clusters (253082 equal clusters
// and 253219 equal low words are not reached).
std::vector<std::uint32_t> blocks(std::uint32_t entry) {
    std::vector<std::uint32_t> all;
    const auto add=[&](const std::vector<std::uint32_t> &v,std::initializer_list<std::uint32_t> skip={}) {
        for(const auto pc:v)if(std::find(skip.begin(),skip.end(),pc)==skip.end())all.push_back(pc);
    };
    switch(entry) {
    case 0x12a810u:add(blocks_12a810);add(blocks_12a930,{0x12a949u,0x12aa1au});
        add(blocks_2287b0);add(blocks_253070,{0x253082u});add(blocks_2531e0,{0x253219u});break;
    case 0x12a930u:add(blocks_12a930);break;
    case 0x2287b0u:add(blocks_2287b0);add(blocks_253070,{0x253082u});add(blocks_2531e0,{0x253219u});break;
    case 0x253070u:add(blocks_253070);add(blocks_2531e0);break;
    case 0x2531e0u:add(blocks_2531e0);break;
    }
    return all;
}
} // namespace

int main() try {
    Random r;unsigned compared=0,faults=0;
    for(const auto entry:{0x12a810u,0x12a930u,0x2287b0u,0x253070u,0x2531e0u}) {
        if(!native_tu15_sound_manager_owns_entry(entry)){std::cerr<<std::hex<<entry<<" has no native kernel\n";return 1;}
        std::set<std::uint32_t> covered;
        const auto entry_faults=faults;
        for(unsigned n=0;n<8000;++n) {
            const auto c=scenario(r,entry);
            const auto before=faults;
            if(!agree(c,entry,reference,faults,"sound listener",n))return 1;
            if(faults==before) {   // the original's path through a state that did not fault
                auto run=c;run.memory=c.memory.clone();std::set<std::uint32_t> path;
                trace=&path;reference(run.memory,entry,run.abi,run.fp,{});trace=nullptr;
                covered.insert(path.begin(),path.end());
            }
            ++compared;
        }
        const auto required=blocks(entry);
        for(const auto pc:required)if(!covered.contains(pc)) {
            std::cerr<<std::hex<<entry<<" path coverage incomplete: block "<<pc<<" never ran\n";return 1;
        }
        std::cout<<std::hex<<entry<<std::dec<<": 8000 states, "<<faults-entry_faults<<" identical faults, "
            <<required.size()<<" blocks covered\n";
    }
    std::cout<<"PASS native sound listener kernels: "<<compared<<" randomized states identical to the lifted routines ("
        <<faults<<" identical faults)\n";
    return 0;
} catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
