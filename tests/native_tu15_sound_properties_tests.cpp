// Differential test: the native sound property kernels (18FE80, 18FF60,
// 1974B0, 21FEB0, 2290C0) against the lifted TU1.5 routines they replace,
// from identical randomized game states across x87 controls and MXCSR modes.
// Path coverage comes from the lifted routine's own instruction trace: every
// basic block of each routine must run in some state that does not fault.
#include "halo2/native_tu15_sound_manager.hpp"
#include "halo2/source_sound_properties_tu15.hpp"
#include "native_tu15_sound_test_support.hpp"
#include <algorithm>
#include <set>
#include <vector>

using namespace halo2;
using namespace halo2::sound_test;
namespace {
// Retail globals the routines read (game runtime data, not .rdata; the
// routines read no .rdata constants).
constexpr std::uint32_t tag_instances=0x4f0414u,sound_gestalt=0x52d664u,sound_globals=0x4ecc1cu,
    global_scenario=0x4ecc20u,property_override=0x51df90u,sound_group=0x736e6421u;
constexpr std::uint32_t tags=0x01000000u,gestalt=0x01100000u,playbacks=0x01110000u,definitions=0x01200000u,
    globals=0x01300000u,global_rows=0x01310000u,scenario_data=0x01320000u,collections=0x01400000u,
    collection_rows=0x01480000u,filter_lists=0x01500000u,filter_tags=0x01540000u,filter_rows=0x01580000u,
    override_record=0x01600000u,record=0x01700000u,unmapped=0x7e000000u;
// Filter names (string ids) 1974B0 maps, their neighbours and the signed extremes.
constexpr std::array<std::uint32_t,12> names{0x1300012du,0x14000135u,0x1600013cu,0x1600013du,0x1600013eu,
    0x1600013fu,0x1300012cu,0x1300012eu,0x1600013bu,0x16000140u,0x80000000u,0x7fffffffu};

struct Builder {
    Random &r;Memory &m;
    std::vector<std::uint32_t> pool;
    unsigned definitions_used=0,collections_used=0,filters_used=0;
    std::uint32_t definition=0;
    Builder(Random &random,Memory &memory):r(random),m(memory) {
        // Distinct tag indices per state; 0xFFF0-0xFFFF exercise the
        // sign-extended group check against the zero-extended data read.
        for(std::uint32_t i=0;i<16;++i){pool.push_back(i);pool.push_back(0xfff0u+i);}
        std::shuffle(pool.begin(),pool.end(),r.rng);
    }
    // Rarely an unmapped pointer, so both sides must fault at the same read.
    std::uint32_t maybe_unmapped(std::uint32_t p){return r.below(80)?p:unmapped+(r.word()&0xfff0u);}
    std::uint32_t datum(std::uint32_t index){return (r.word()&0xffff0000u)|index;}
    // A tag-instance row (+0 group, +8 data). For indices from 0x8000 the
    // group is checked in the row the sign-extended index addresses and the
    // data read from the zero-extended one; the other halves hold decoys.
    std::uint32_t tag(std::uint32_t group,std::uint32_t data) {
        const auto index=pool.back();pool.pop_back();
        const auto checked=tags+(std::uint32_t(std::int32_t(std::int16_t(index)))<<4),read=tags+(index<<4);
        m.put32(checked,group);m.put32(read+8u,data);
        if(checked!=read){m.put32(read,group==sound_group?r.word():sound_group);m.put32(checked+8u,r.word());}
        return r.below(64)?datum(index):datum(r.word()&0xffffu);   // rarely an index without a row
    }
    std::uint32_t count() {
        switch(r.below(8)){case 0:return 0u;case 1:return 0xffffffffu;case 2:return 0x80000000u;
            case 3:return 0x7fffffffu;case 4:return r.word();default:return 1u+r.below(4);}
    }
    std::uint32_t name(){return r.below(8)?names[r.below(names.size())]:r.word();}
    // A filter list: +4 a filter tag datum (-1 for none); the filter tag's
    // +0x14 count and +0x18 rows, whose +8 names the property.
    std::uint32_t filter_list() {
        const auto list=filter_lists+filters_used*0x100u,data=filter_tags+filters_used*0x100u,
            rows=filter_rows+filters_used*0x100u;
        ++filters_used;
        m.put32(data+0x14u,count());m.put32(data+0x18u,maybe_unmapped(rows));m.put32(rows+8u,name());
        m.put32(list+4u,r.below(6)?tag(r.word(),maybe_unmapped(data)):0xffffffffu);
        return maybe_unmapped(list);
    }
    // A 0x34-byte custom playback: +0x2C filter count, +0x30 filter list.
    void playback(std::uint32_t row){m.put32(row+0x2cu,count());m.put32(row+0x30u,filter_list());}
    // A sound tag ('snd!' mostly) whose definition +0xD names its custom
    // playback row (-1 for none).
    std::uint32_t sound() {
        definition=definitions+definitions_used++*0x20u;
        const auto custom=std::uint8_t(r.below(3)==0?0xffu:r.below(2)?r.below(8):r.word());
        m.put8(definition+0xdu,custom);
        if(custom!=0xffu)playback(playbacks+std::uint32_t(std::int32_t(std::int8_t(custom)))*0x34u);
        return tag(r.below(6)?sound_group:r.word(),maybe_unmapped(definition));
    }
    // A sound effect collection tag (-1 for none): +4 rows of 0x38 bytes, the
    // effect's custom playback at row +4.
    std::uint32_t collection(std::uint32_t row) {
        if(!r.below(8))return 0xffffffffu;
        const auto data=collections+collections_used*0x100u,rows=collection_rows+collections_used*0x10000u;
        ++collections_used;
        m.put32(data+4u,rows);
        if(row<0x100u)playback(rows+row*0x38u+4u);
        return tag(r.word(),maybe_unmapped(data));
    }
    // An effect reference: -1, or bits 30-31 source (0 the sound's own,
    // 1 globals collection, 2 scenario collection, 3 none) and a row index.
    std::uint32_t effect() {
        m.put32(sound_globals,maybe_unmapped(globals));
        m.put32(global_scenario,maybe_unmapped(scenario_data));
        if(!r.below(5))return 0xffffffffu;
        const auto row=r.below(6)?r.below(16):r.word()&0x3fffffffu;
        m.put32(globals+0xc0u,r.below(6)?1u+r.below(3):0u);
        m.put32(globals+0xc4u,r.below(6)?maybe_unmapped(global_rows):0u);
        m.put32(global_rows+0xcu,collection(row));
        m.put32(scenario_data+0x324u,collection(row));
        return (r.below(4)<<30)|row;
    }
};

Case scenario(Random &r,std::uint32_t entry) {
    auto c=base_case(r);auto &m=c.memory;auto &a=c.abi;
    Builder b(r,m);
    m.put32(tag_instances,b.maybe_unmapped(tags));
    m.put32(sound_gestalt,b.maybe_unmapped(gestalt));m.put32(gestalt+0x34u,playbacks);
    switch(entry) {
    case 0x21feb0u:b.sound();a.gp[0]=b.maybe_unmapped(b.definition);break;
    case 0x18ff60u:a.gp[1]=r.below(8)?b.sound():0xffffffffu;break;
    case 0x18fe80u:a.gp[1]=r.below(8)?b.sound():0xffffffffu;a.gp[2]=b.effect();break;
    case 0x1974b0u:a.gp[1]=b.name();break;
    case 0x2290c0u: {
        const auto effect=b.effect();
        a.gp[7]=r.below(10)?b.sound():0xffffffffu;
        m.put32(record,(r.word()&~0x100u)|(r.below(4)?0x100u:0u));
        m.put32(record+0x98u,effect);
        m.put16(record+0x8eu,std::uint16_t(r.word()));m.put8(record+0x90u,std::uint8_t(r.word()));
        m.put32(property_override,b.maybe_unmapped(override_record));
        m.put16(override_record+6u,r.below(4)?std::uint16_t(r.word()):0xffffu);
        a.gp[3]=b.maybe_unmapped(record);
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
    source_run_sound_properties_tu15(m,entry,a,fp,services,o);
}

// Basic-block leaders of each lifted routine (its first instruction, branch
// targets and fall-throughs, and call returns). 18FE80 tail-jumps into
// 21FEB0. 2290C0's block 229110 (no filter rows when the chosen custom
// playback's re-read filter count is not positive) is unreachable: the count
// was positive when that playback was chosen and nothing is stored between.
const std::map<std::uint32_t,std::vector<std::uint32_t>> leaders{
    {0x18fe80u,{0x18fe80u,0x18fe88u,0x18fe8cu,0x18fea1u,0x18feb5u,0x18feb7u,0x18feb9u,0x18feccu,0x18fed6u,
        0x18fed9u,0x18fedcu,0x18fee8u,0x18feedu,0x18ff11u,0x18ff21u,0x18ff2bu,0x18ff30u,0x18ff49u,
        0x21feb0u,0x21feb7u,0x21febau}},
    {0x18ff60u,{0x18ff60u,0x18ff67u,0x18ff7eu,0x18ff92u,0x18ff95u,0x18ffa6u}},
    {0x1974b0u,{0x1974b0u,0x1974bbu,0x1974bdu,0x1974c5u,0x1974cdu,0x1974d5u,0x1974dbu,0x1974e1u,0x1974e7u,
        0x1974edu,0x1974f5u,0x1974f8u,0x1974feu,0x197503u}},
    {0x21feb0u,{0x21feb0u,0x21feb7u,0x21febau}},
    {0x2290c0u,{0x2290c0u,0x2290c7u,0x2290cfu,0x2290d2u,0x2290dau,0x2290e3u,0x2290e9u,0x2290f0u,0x2290f2u,
        0x2290f6u,0x2290fdu,0x2290ffu,0x229104u,0x22910bu,0x229112u,0x22911au,0x229133u,0x22913fu,
        0x22914fu,0x229159u,0x229161u,0x229167u,0x229173u}},
};
} // namespace

int main() try {
    Random r;unsigned compared=0,faults=0;
    for(const auto entry:{0x18fe80u,0x18ff60u,0x1974b0u,0x21feb0u,0x2290c0u}) {
        std::set<std::uint32_t> covered;
        const auto entry_faults=faults;
        for(unsigned n=0;n<8000;++n) {
            const auto c=scenario(r,entry);
            const auto before=faults;
            if(!agree(c,entry,reference,faults,"sound properties",n))return 1;
            if(faults==before) {   // the original's path through a state that did not fault
                auto run=c;run.memory=c.memory.clone();std::set<std::uint32_t> path;
                trace=&path;reference(run.memory,entry,run.abi,run.fp,{});trace=nullptr;
                covered.insert(path.begin(),path.end());
            }
            ++compared;
        }
        const auto &blocks=leaders.at(entry);
        for(const auto pc:blocks)if(!covered.contains(pc)) {
            std::cerr<<std::hex<<entry<<" path coverage incomplete: block "<<pc<<" never ran\n";return 1;
        }
        std::cout<<std::hex<<entry<<std::dec<<": 8000 states, "<<faults-entry_faults<<" identical faults, "
            <<blocks.size()<<" blocks covered\n";
    }
    std::cout<<"PASS native sound property kernels: "<<compared<<" randomized states identical to the lifted routines ("
        <<faults<<" identical faults)\n";
    return 0;
} catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
