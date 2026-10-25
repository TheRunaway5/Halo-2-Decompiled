// Differential test: every native sound manager kernel against the lifted
// TU1.5 routine it replaces, from identical randomized game states, across
// x87 precision/rounding controls, MXCSR modes, special floats and branches.
#include "halo2/native_tu15_sound_manager.hpp"
#include "halo2/source_sound_random_scalar_tu15.hpp"
#include "halo2/source_sound_random_short_tu15.hpp"
#include "halo2/source_sound_permutation_tu15.hpp"
#include "halo2/source_sound_range_selection_tu15.hpp"
#include "halo2/source_sound_scalar_tu15.hpp"
#include <bit>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <vector>

using namespace halo2;
namespace {
struct Memory final : SourcePhysicsContactMemory {
    std::map<std::uint32_t,std::shared_ptr<std::array<std::byte,4096>>> pages;
    Memory clone() const {
        Memory copy;for(const auto &[n,p]:pages)copy.pages[n]=std::make_shared<std::array<std::byte,4096>>(*p);
        return copy;
    }
    std::byte *at(std::uint32_t p) const {
        const auto found=pages.find(p>>12);
        if(found==pages.end()){char t[40];std::snprintf(t,sizeof t,"unmapped %08x",p);throw std::out_of_range(t);}
        return found->second->data()+(p&4095u);
    }
    void read(std::uint32_t p,std::span<std::byte> out) const override {for(auto &b:out)b=*at(p++);}
    void write(std::uint32_t p,std::span<const std::byte> in) override {for(auto b:in)*at(p++)=b;}
    void map(std::uint32_t p,std::size_t bytes) {
        for(auto n=p>>12;n<=(p+bytes-1)>>12;++n)
            if(!pages.contains(n))pages[n]=std::make_shared<std::array<std::byte,4096>>();
    }
    void put32(std::uint32_t p,std::uint32_t v){map(p,4);for(unsigned i=0;i<4;++i)*at(p+i)=std::byte(v>>(i*8));}
    void put16(std::uint32_t p,std::uint16_t v){map(p,2);for(unsigned i=0;i<2;++i)*at(p+i)=std::byte(v>>(i*8));}
    void put8(std::uint32_t p,std::uint8_t v){map(p,1);*at(p)=std::byte(v);}
};
struct Random {
    std::mt19937 rng{0x5eed1234u};
    std::uint32_t word(){return rng();}
    std::uint32_t below(std::uint32_t n){return rng()%n;}
    float scalar() {
        switch(below(12)) {
        case 0:return std::bit_cast<float>(0x7fc00000u|(word()&0x3fffffu));  // quiet NaN
        case 1:return std::bit_cast<float>(0x7f800001u|(word()&0x3fffffu));  // signalling NaN
        case 2:return below(2)?INFINITY:-INFINITY;
        case 3:return std::bit_cast<float>(word()&0x807fffffu);              // denormal
        case 4:return below(2)?0.f:-0.f;
        case 5:return std::bit_cast<float>(word());                          // any bits
        default:return std::ldexp(float(std::int32_t(word()))/2147483648.f,int(below(40))-20);
        }
    }
};

constexpr std::uint32_t stack=0x60080000u;
constexpr std::array<std::uint16_t,6> controls{0x27f,0x7f,0x37f,0x67f,0xa7f,0xe7f};
constexpr std::array<std::uint32_t,5> csrs{0x1f80u,0x9f80u,0x1fc0u,0xdfc0u,0x3f80u};

struct Case {
    Memory memory;SourcePhysicsContactRuntimeAbi abi{};SourcePhysicsFactoryFloatingPoint fp{};std::uint32_t entry{};
};
Case scenario(Random &r,std::uint32_t entry) {
    Case c;c.entry=entry;auto &m=c.memory;
    const std::uint32_t tags=0x01000000u,gestalt=0x01100000u,rows=0x01110000u,random=0x01200000u,
        globals=0x01300000u,reference=0x01310000u,class_tag=0x01320000u,class_rows=0x01330000u,
        definition=0x01400000u,request=0x01500000u,sound=0x01600000u;
    // Retail .rdata words the lifted routines read (TU1.5 XBE): 1/65535, 2^32,
    m.put32(0x46a160u,0x37800080u);m.put32(0x46a0c8u,0x4f800000u);m.put32(0x45695cu,0x7f7fffffu); // FLT_MAX
    m.put32(0x46a0ccu,0x3f000000u);m.put32(0x46a0d0u,0x3f800000u);   // 0.5, 1.0
    m.put32(0x4f0414u,tags);m.put32(0x52d664u,gestalt);m.put32(gestalt+4u,rows);
    m.put32(0x4f3cd8u,random);m.put32(random+4u,r.word());
    m.put32(0x4ecc1cu,globals);m.put32(globals+0xc0u,1u+r.below(3));m.put32(globals+0xc4u,reference);
    const std::uint32_t class_datum=0xe0000000u|(r.word()&0xffff0000u)|7u,sound_datum=(r.word()&0xffff0000u)|11u;
    m.put32(reference+4u,class_datum);m.put32(tags+7u*16u+8u,class_tag);m.put32(class_tag+4u,class_rows);
    m.put32(tags+11u*16u+8u,sound);
    for(unsigned i=0;i<64;++i) {                       // playback rows
        const auto row=rows+i*0x38u;
        for(unsigned w=0;w<0x38u;w+=4)m.put32(row+w,std::bit_cast<std::uint32_t>(r.scalar()));
        m.put16(row+0x18u,std::uint16_t(r.word()));m.put16(row+0x1au,std::uint16_t(r.word()));
    }
    for(unsigned i=0;i<256;++i)                        // signed 8-bit class indices: +/-128 rows
        for(unsigned w=0;w<0x5cu;w+=4)m.put32(class_rows-128u*0x5cu+i*0x5cu+w,std::bit_cast<std::uint32_t>(r.scalar()));
    m.put16(definition+6u,std::uint16_t(r.below(64)));
    m.put8(request,std::uint8_t(r.word()));m.put32(request+0x38u,std::bit_cast<std::uint32_t>(r.scalar()));
    m.put8(sound+1u,std::uint8_t(r.word()));m.put8(sound+2u,std::uint8_t(r.word()));m.put16(sound+6u,std::uint16_t(r.below(64)));
    // Pitch ranges: definition +8 first range, +0xA count; 12-byte range rows
    // (+2 parameter row, +0xA permutations); 10-byte parameter rows.
    const std::uint32_t parameters=0x01700000u,ranges=0x01710000u;
    m.put32(gestalt+0x1cu,parameters);m.put32(gestalt+0x24u,ranges);
    std::vector<std::int16_t> bounds;
    for(unsigned i=0;i<48;++i) {
        const auto row=parameters+i*10u;
        const auto lower=std::int16_t(int(r.below(4000))-2000),upper=std::int16_t(lower+int(r.below(3000))-200);
        m.put16(row+2u,std::uint16_t(int(r.below(4000))-2000));m.put16(row+4u,std::uint16_t(int(r.below(4000))-2000));
        m.put16(row+6u,std::uint16_t(lower));m.put16(row+8u,std::uint16_t(upper));
        bounds.push_back(lower);bounds.push_back(upper);
    }
    for(unsigned i=0;i<48;++i) {
        m.put16(ranges+i*12u+2u,std::uint16_t(r.below(48)));m.put16(ranges+i*12u+0xau,std::uint16_t(int(r.below(5))-1));
    }
    // Permutations: 16-byte rows (+2 skip fraction); used masks in the
    // gestalt runtime bank (+0x3C) at bit offset range +6; sound classes
    // keep history unless flag bit 5 of +0xA is set.
    const std::uint32_t permutations=0x01720000u,used=0x01730000u;
    m.put32(gestalt+0x2cu,permutations);m.put32(gestalt+0x3cu,used);
    for(unsigned i=0;i<96;++i)m.put16(permutations+i*16u+2u,std::uint16_t(r.below(3)==0?0:r.word()));
    for(unsigned i=0;i<64;++i)m.put8(used+i,std::uint8_t(r.word()));
    for(unsigned i=0;i<48;++i) {
        const auto row=ranges+i*12u;
        m.put16(row+6u,std::uint16_t(r.below(256)));                 // used-mask bit offset
        m.put16(row+8u,std::uint16_t(r.below(60)));                  // first permutation
        m.put16(row+0xau,std::uint16_t(r.below(8)==0?int(r.below(3))-1:int(r.below(33))));
        m.put8(row+4u,std::uint8_t(r.below(4)==0?0xff:r.below(40)));    // last
        m.put8(row+5u,std::uint8_t(r.below(3)==0?r.below(40):0xff));    // forced
    }
    for(unsigned i=0;i<256;++i) {                                      // class history flag
        const auto row=class_rows-128u*0x5cu+i*0x5cu+0xau;
        m.put8(row,std::uint8_t((std::to_integer<unsigned>(*m.at(row))&~0x20u)|(r.below(4)==0?0x20u:0u)));
    }
    m.put8(definition+2u,std::uint8_t(r.word()));
    const auto count=r.below(10)?int(r.below(9)):-int(r.below(3));
    m.put8(definition+0xau,std::uint8_t(count));m.put16(definition+8u,std::uint16_t(r.below(40)));
    m.map(stack-0x2000u,0x3000u);m.put32(stack,0x00abcdefu);m.put32(stack+4u,r.word());
    auto &a=c.abi;
    for(auto &g:a.gp)g=r.word();
    a.gp[4]=stack;
    for(auto &x:a.xmm)for(auto &l:x)l=r.word();
    a.eflags=0x202u;
    if(entry==0x127e00u){a.gp[0]=definition;a.gp[1]=random+4u;}
    if(entry==0x127d70u)a.gp[0]=definition;
    if(entry==0x127b20u){a.gp[0]=sound_datum;a.gp[1]=request;}
    if(entry==0x21f960u) {
        a.gp[1]=definition;
        const auto choice=r.below(4);
        const auto previous=choice==0?-1:choice==1?int(r.below(12))-2:std::max(0,int(r.below(std::uint32_t(std::max(count,1)))));
        a.gp[0]=(r.word()&0xffff0000u)|std::uint16_t(previous);
        const auto pitch=r.below(4)==0?r.scalar():r.below(3)==0?float(bounds[r.below(std::uint32_t(bounds.size()))]):
            float(int(r.below(6000))-3000)+float(r.below(4))*0.25f;
        a.xmm[1][0]=std::bit_cast<std::uint32_t>(pitch);
    }
    if(entry==0x21fa80u) {
        // Callers select permutations only from ranges that have some; with
        // none, a skip fraction of 1.0 never ends the original's rerolls.
        a.gp[7]=definition;
        const auto index=r.below(std::uint32_t(std::max(count,1)));
        c.memory.put32(stack+4u,(r.word()&0xffff0000u)|std::uint16_t(index));
        const auto first=std::bit_cast<std::int16_t>(std::array{*c.memory.at(definition+8u),*c.memory.at(definition+9u)});
        c.memory.put16(ranges+std::uint32_t(first+int(index))*12u+0xau,std::uint16_t(1u+r.below(32)));
    }
    if(entry==0x228220u)c.memory.put32(stack+4u,(r.word()&0xffff0000u)|std::uint16_t(std::int16_t(int(r.below(256))-128)));
    a.x87_depth=r.below(5);
    for(unsigned i=0;i<a.x87_depth;++i)a.x87[i]=static_cast<long double>(r.scalar());
    a.x87_status=std::uint16_t((r.below(8)<<11)|(r.word()&0x473fu));
    c.fp={controls[r.below(controls.size())],csrs[r.below(csrs.size())]|(r.word()&0x3fu)};
    a.floating_point=c.fp;
    return c;
}
void reference(SourcePhysicsContactMemory &m,std::uint32_t entry,SourcePhysicsContactRuntimeAbi &a,
    SourcePhysicsFactoryFloatingPoint fp,const SourcePhysicsContactRecordObserver &o) {
    SourcePhysicsWorldRuntimeServices services;
    services.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("unowned callee");};
    if(entry==0x127e00u)source_run_sound_random_short_tu15(m,entry,a,fp,services,o);
    else if(entry==0x127d70u)source_run_sound_random_scalar_tu15(m,entry,a,fp,services,o);
    else if(entry==0x21f960u)source_run_sound_range_selection_tu15(m,entry,a,fp,services,o);
    else if(entry==0x21fa80u)source_run_sound_permutation_tu15(m,entry,a,fp,services,o);
    else source_run_sound_scalar_tu15(m,entry,a,fp,services,o);
}
} // namespace

int main() try {
    Random r;unsigned compared=0,faults=0;
    for(const auto entry:{0x127e00u,0x127d70u,0x127b20u,0x228220u,0x21f960u,0x21fa80u}) {
        unsigned branches[3]{},ranges[3]{};
        for(unsigned n=0;n<4000;++n) {
            auto c=scenario(r,entry);
            if(entry==0x127b20u) {
                const auto flags=std::to_integer<unsigned>(*c.memory.at(0x01500000u));
                const auto sound=std::to_integer<unsigned>(*c.memory.at(0x01600001u));
                ++branches[(flags&8u)?0:(sound&8u)?1:2];
            }
            // The oracle predicts natively over a copy-on-write view, then
            // runs the lifted routine and compares every output.
            std::string why;
            try {
                why=native_tu15_sound_manager_compare(c.memory,entry,c.abi,c.fp,{},
                    [&](auto &m,auto &a,const auto &o){reference(m,entry,a,c.fp,o);});
            } catch(const std::exception &error) {
                // Faults must match too: the original faults at the same address.
                auto lifted=c.memory.clone();auto abi=c.abi;std::string other="returned";
                try{reference(lifted,entry,abi,c.fp,{});}catch(const std::exception &e){other=e.what();}
                if(other!=error.what()) {
                    std::cerr<<"entry "<<std::hex<<entry<<std::dec<<" case "<<n<<": native "<<error.what()
                        <<", lifted "<<other<<'\n';return 1;
                }
                ++faults;continue;
            }
            if(!why.empty()){std::cerr<<"case "<<n<<": "<<why<<'\n';return 1;}
            if(entry==0x21f960u) {  // kept previous / none / searched
                const auto result=std::int16_t(c.abi.gp[0]);
                ++ranges[result==-1?1:c.abi.gp[1]==0xffffffffu?0:2];
            }
            // Committed native execution leaves memory identical to the reference.
            auto native=scenario(r,entry),lifted=native;lifted.memory=native.memory.clone();
            std::string native_fault,lifted_fault;
            try{native_tu15_sound_manager_run(native.memory,entry,native.abi,native.fp);}catch(const std::exception &e){native_fault=e.what();}
            try{reference(lifted.memory,entry,lifted.abi,lifted.fp,{});}catch(const std::exception &e){lifted_fault=e.what();}
            if(native_fault!=lifted_fault){std::cerr<<"committed fault differs: "<<native_fault<<" / "<<lifted_fault<<'\n';return 1;}
            for(const auto &[page,bytes]:lifted.memory.pages) {
                const auto base=page<<12;
                for(unsigned i=0;i<4096;++i) {
                    const auto p=base+i;
                    if(p<stack+8u && p>=stack-0x1000u)continue;   // dead callee scratch and arguments
                    if((*bytes)[i]!=(*native.memory.pages.at(page))[i]) {
                        std::cerr<<"committed store differs at "<<std::hex<<p<<'\n';return 1;
                    }
                }
            }
            compared+=2;
        }
        if(entry==0x21f960u && (!ranges[0]||!ranges[1]||!ranges[2])) {
            std::cerr<<"21F960 path coverage incomplete\n";return 1;
        }
        if(entry==0x127b20u && (!branches[0]||!branches[1]||!branches[2])) {
            std::cerr<<"127B20 branch coverage incomplete\n";return 1;
        }
    }
    std::cout<<"PASS native sound manager kernels: "<<compared<<" randomized states identical to the lifted routines ("<<faults<<" identical faults)\n";
    return 0;
} catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
