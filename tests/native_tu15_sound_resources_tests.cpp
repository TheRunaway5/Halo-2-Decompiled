// Differential test: the native sound cache binding, query and chunk
// readiness kernels against the lifted TU1.5 routines they replace, from
// identical randomized game states (x87/MXCSR environments, every branch, and
// occasional out-of-range indices that must fault at the same address).
// 21F260 and 128970 call the original allocation (21F420) and yield
// (2DBF74) routines; the services below simulate both deterministically and
// log every register and stack argument they see.
#include "halo2/native_tu15_sound_manager.hpp"
#include "halo2/source_sound_binding_tu15.hpp"
#include "halo2/source_sound_cache_query_tu15.hpp"
#include "halo2/source_sound_chunk_readiness_tu15.hpp"
#include "native_tu15_sound_test_support.hpp"
#include <cstdlib>

using namespace halo2;
using namespace halo2::sound_test;
namespace {
using Mem=SourcePhysicsContactMemory;
using Abi=SourcePhysicsContactRuntimeAbi;
using Observer=SourcePhysicsContactRecordObserver;
using Services=SourcePhysicsContactRuntimeServices;

// Test layout. Every table sits alone in a 16 MB region, so a large index
// lands on an unmapped page instead of another table.
constexpr std::uint32_t tags=0x01000000u,definitions=0x02000000u,gestalt=0x03000000u,
    ranges=0x04001000u,permutations=0x05001000u,chunks=0x06001000u,cache=0x07000000u,
    entries=0x07100000u,allocator=0x08000000u,block_owner=0x08100000u,blocks=0x08200000u,
    table=0x09000000u,bindings=0x0a000000u,
    // Simulated callees: +0 chunk row the yield completes, +4 yields left,
    // +8 handle 21F420 allocates (-1 fails), +0xC loaded byte of a new entry,
    // +0x10 call count, +0x20 call log (12 words per call).
    control=0x0b000000u;

std::uint32_t get(const Mem &m,std::uint32_t p,unsigned n=4) {
    std::array<std::byte,4> b{};m.read(p,std::span(b).first(n));
    std::uint32_t v=0;for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(i*8);return v;
}
// A callee store, published to the observer like an original store.
void put(Mem &m,const Observer &o,std::uint32_t site,std::uint32_t p,std::uint32_t v,unsigned n=4) {
    SourcePhysicsContactRecordStore x{site,p,std::vector<std::byte>(n),std::vector<std::byte>(n)};
    m.read(p,x.before);
    for(unsigned i=0;i<n;++i)x.written[i]=std::byte((v>>(i*8))&255u);
    m.write(p,x.written);if(o)o(x);
}
std::uint32_t cache_entry(const Mem &m,std::uint32_t handle) {
    return ((handle&0xffffu)<<4)+get(m,get(m,0x50ebdcu)+0x44u);
}

// The original callees, deterministically. Each logs its call site, ESP,
// all general registers and its return address (and 21F420 its stack
// argument; neither reads further into its caller's frame), then:
//  21F420 (ret 4): allocates handle [control+8] for the chunk row in EBX
//   (unless -1), resetting the new entry with loaded byte [control+0xC];
//  2DBF74 (ret 0): yields; after [control+4] more calls it marks the entry
//   of chunk row [control] loaded. The loop around it ends only then.
// Both return scratch EAX/ECX/EDX and XMM2 lane 0 derived from the call.
void simulate(Mem &m,const SourcePhysicsContactRuntimeCall &call,Abi &a,const Observer &o) {
    const bool allocate=call.target==0x21f420u && call.instruction==0x21f2a3u;
    const bool yield=call.target==0x2dbf74u && call.instruction==0x21f300u;
    if(call.tail || (!allocate && !yield))throw std::runtime_error("unexpected callee");
    const auto count=get(m,control+0x10u);
    if(count>=8)throw std::runtime_error("callee log full");
    const auto record=control+0x20u+count*48u;
    const std::uint32_t words[]{call.instruction,a.gp[4],a.gp[0],a.gp[1],a.gp[2],a.gp[3],a.gp[5],a.gp[6],a.gp[7],
        get(m,a.gp[4]),allocate?get(m,a.gp[4]+4u):0u};
    put(m,o,call.instruction,control+0x10u,count+1u);
    for(unsigned i=0;i<std::size(words);++i)put(m,o,call.instruction,record+i*4u,words[i]);
    if(allocate) {
        const auto row=a.gp[3],handle=get(m,control+8u);
        if(handle!=0xffffffffu) {
            put(m,o,call.instruction,row+8u,handle);
            const auto e=cache_entry(m,handle);
            put(m,o,call.instruction,e+2u,get(m,control+0xcu,1),1);
            for(const auto at:{3u,4u,5u})put(m,o,call.instruction,e+at,0u,1);
        }
        a.gp[4]+=8u;
    } else {
        const auto left=get(m,control+4u);
        if(left)put(m,o,call.instruction,control+4u,left-1u);
        else put(m,o,call.instruction,cache_entry(m,get(m,get(m,control)+8u))+2u,1u,1);
        a.gp[4]+=4u;
    }
    const auto h=((count+1u)*0x9e3779b9u)^a.gp[6]^call.target;
    a.gp[0]=h;a.gp[1]=h*0x85ebca6bu;a.gp[2]=h^0xc2b2ae35u;a.xmm[2][0]=h;
}
Services callees() {
    Services services;
    services.required_owner=[](Mem &m,const SourcePhysicsContactRuntimeCall &call,Abi &a,const Observer &o){
        simulate(m,call,a,o);
    };
    return services;
}

void reference(Mem &m,std::uint32_t entry,Abi &a,SourcePhysicsFactoryFloatingPoint fp,const Services &services,
    const Observer &o) {
    if(source_sound_binding_tu15_owns_entry(entry))source_run_sound_binding_tu15(m,entry,a,fp,services,o);
    else if(source_sound_cache_query_tu15_owns_entry(entry))source_run_sound_cache_query_tu15(m,entry,a,fp,services,o);
    else if(source_sound_chunk_readiness_tu15_owns_entry(entry))
        source_run_sound_chunk_readiness_tu15(m,entry,a,fp,services,o);
    else throw std::runtime_error("entry has no lifted owner");
}

// agree() from the shared harness, with the simulated callees passed to the
// oracle, the native kernel and the lifted routine alike.
bool agree_with(const Case &c,std::uint32_t entry,const Services &services,unsigned &faults,unsigned n) {
    auto oracle=c;oracle.memory=c.memory.clone();
    try {
        const auto why=native_tu15_sound_manager_compare(oracle.memory,entry,oracle.abi,oracle.fp,{},
            [&](auto &m,auto &a,const auto &o){reference(m,entry,a,oracle.fp,services,o);},services);
        if(!why.empty()){std::cerr<<"case "<<n<<": "<<why<<'\n';return false;}
    } catch(const std::exception &error) {
        auto lifted=c;lifted.memory=c.memory.clone();std::string other="returned";
        try{reference(lifted.memory,entry,lifted.abi,lifted.fp,services,{});}catch(const std::exception &e){other=e.what();}
        if(other!=error.what()) {
            std::cerr<<std::hex<<entry<<std::dec<<" case "<<n<<": native "<<error.what()<<", lifted "<<other<<'\n';
            return false;
        }
        ++faults;return true;
    }
    auto native=c,lifted=c;native.memory=c.memory.clone();lifted.memory=c.memory.clone();
    std::string native_fault,lifted_fault;
    try{native_tu15_sound_manager_run(native.memory,entry,native.abi,native.fp,{},services);}
    catch(const std::exception &e){native_fault=e.what();}
    try{reference(lifted.memory,entry,lifted.abi,lifted.fp,services,{});}catch(const std::exception &e){lifted_fault=e.what();}
    if(native_fault!=lifted_fault) {
        std::cerr<<std::hex<<entry<<std::dec<<" case "<<n<<": committed fault "<<native_fault<<" / "<<lifted_fault<<'\n';
        return false;
    }
    if(native.abi.gp!=lifted.abi.gp) {
        std::cerr<<std::hex<<entry<<std::dec<<" case "<<n<<": committed registers differ\n";return false;
    }
    const auto dead_end=lifted.abi.gp[4];
    for(const auto &[page,bytes]:lifted.memory.pages) {
        const auto &other=*native.memory.pages.at(page);
        for(unsigned i=0;i<4096;++i) {
            const auto p=(page<<12)+i;
            if(p<dead_end && p>=stack-0x1000u)continue;
            if((*bytes)[i]!=other[i]) {
                char t[96];std::snprintf(t,sizeof t,"%x case %u: committed store differs at %08x",entry,n,p);
                std::cerr<<t<<'\n';return false;
            }
        }
    }
    return true;
}

std::uint32_t chunk_handle(Random &r,bool rare) {
    if(r.below(4)==0)return 0xffffffffu;
    return (r.word()&0xffff0000u)|(rare?(r.word()&0xffffu):r.below(48));
}
Case scenario(Random &r,std::uint32_t entry) {
    auto c=base_case(r);auto &m=c.memory;
    // Roughly one state in 48 per field takes an out-of-range index, so the
    // original faults; the native kernel must fault at the same address.
    const auto rare=[&]{return r.below(48)==0;};
    m.put32(0x4f0414u,tags);
    for(unsigned i=0;i<16;++i) {
        // Sound definitions: +4/+5 format bytes, +8 first pitch range,
        // +0xA pitch range count.
        const auto d=definitions+i*0x100u;
        m.put32(tags+i*16u+8u,d);
        m.put8(d+4u,std::uint8_t(r.below(4)?r.below(4):r.word()));
        m.put8(d+5u,std::uint8_t(r.below(4)?r.below(3):r.word()));
        m.put16(d+8u,std::uint16_t(rare()?r.word():r.below(8)));
        m.put8(d+0xau,std::uint8_t(r.below(4)?1u+r.below(8):0u));
    }
    m.put32(0x52d664u,gestalt);
    m.put32(gestalt+0x24u,ranges);m.put32(gestalt+0x2cu,permutations);m.put32(gestalt+0x44u,chunks);
    for(int i=-48;i<80;++i) {
        const auto k=std::uint32_t(i);
        m.put16(ranges+k*12u+8u,std::uint16_t(rare()?r.word():r.below(8)));     // first permutation
        m.put16(ranges+k*12u+0xau,std::uint16_t(r.below(3)?1u+r.below(8):0u));  // permutation count
        m.put16(permutations+k*16u+0xcu,std::uint16_t(rare()?r.word():r.below(16)));  // first chunk
        m.put32(chunks+k*12u+4u,r.word());
        m.put32(chunks+k*12u+8u,chunk_handle(r,rare()));                       // cache handle
    }
    m.put32(0x50ebdcu,cache);m.put32(cache+0x44u,entries);
    for(unsigned i=0;i<48;++i) {
        const auto e=entries+i*16u;
        m.put8(e+2u,std::uint8_t(r.below(2)?0u:1u+r.below(255)));   // loaded
        m.put8(e+3u,std::uint8_t(r.below(2)?0u:r.word()));          // used
        m.put8(e+4u,std::uint8_t(r.below(3)==0?0u:r.word()));       // locks (0 wraps to 0xFF)
        m.put8(e+5u,std::uint8_t(r.below(3)?0u:r.word()));          // held
        m.put32(e+8u,r.word());m.put32(e+0xcu,r.word());
    }
    m.put32(0x50ebe4u,allocator);m.put32(allocator+0x38u,r.word());m.put32(allocator+0x64u,block_owner);
    m.put32(block_owner+0x44u,blocks);m.map(blocks,48u*24u);
    m.put32(0x4f2c4cu,table);m.put32(table+0x44u,bindings);
    for(unsigned i=0;i<16;++i) {
        const auto b=bindings+i*0xbcu;
        m.put8(b+4u,std::uint8_t(r.word()));
        m.put32(b+0xcu,(r.word()&0xffff0000u)|(rare()?(r.word()&0xffffu):r.below(16)));
        m.put8(b+0x9cu,std::uint8_t(int(r.below(8))-2));
        m.put8(b+0x9du,std::uint8_t(int(r.below(8))-2));
        m.put16(b+0x9eu,std::uint16_t(int(r.below(8))-2));
    }
    m.put32(0x48b8fcu,r.below(2)?0u:(r.word()|1u));                    // .data global
    // The yield loop of 21F260 ends only when its entry loads; the simulated
    // yield always loads it within four calls, so no state loops forever.
    m.put32(control+4u,r.below(4));
    m.put32(control+8u,r.below(4)==0?0xffffffffu:(r.word()&0xffff0000u)|r.below(48));
    m.put8(control+0xcu,std::uint8_t(r.below(2)?0u:1u+r.below(255)));
    m.put32(control+0x10u,0u);m.map(control+0x20u,8u*48u);
    auto &a=c.abi;
    const auto binding=bindings+(rare()?r.word()&0xffffu:r.below(16))*0xbcu;
    switch(entry) {
    case 0x129470u:a.gp[1]=binding;break;
    case 0x1293f0u:a.gp[0]=binding;break;
    case 0x128970u:m.put32(stack+4u,binding);break;
    case 0x12b1c0u:
        a.gp[0]=(r.word()&0xffff0000u)|(rare()?r.word()&0xffffu:r.below(16));
        for(unsigned i=1;i<=4;++i)m.put32(stack+i*4u,r.word());
        break;
    case 0x1296e0u:a.gp[0]=(r.word()&0xffff0000u)|(rare()?r.word()&0xffffu:r.below(16));break;
    case 0x21f3b0u:m.put32(stack+4u,(r.word()&0xffff0000u)|(rare()?r.word()&0xffffu:r.below(48)));break;
    case 0x21f260u: {
        const auto row=chunks+std::uint32_t(int(r.below(128))-48)*12u;
        m.put32(stack+4u,r.below(4)==0?0xffffffffu:r.word());   // sound datum
        m.put32(stack+8u,row);
        m.put32(stack+0xcu,r.word());                             // request bits (upper bits reach EBX)
        m.put32(control,row);
        break;
    }
    }
    return c;
}
} // namespace

int main() try {
    Random r(0x50c4e7u);const auto services=callees();
    unsigned compared=0,faults=0;
    for(const auto entry:{0x129470u,0x12b1c0u,0x1296e0u,0x128970u,0x1293f0u,0x21f260u,0x21f3b0u}) {
        unsigned paths[10]{};
        for(unsigned n=0;n<8000;++n) {
            const auto c=scenario(r,entry);
            if(!agree_with(c,entry,services,faults,n))return 1;
            ++compared;
            // Path coverage from a committed native run.
            auto run=c;run.memory=c.memory.clone();
            try{native_tu15_sound_manager_run(run.memory,entry,run.abi,run.fp,{},services);}catch(const std::exception &){continue;}
            const auto &m=run.memory;
            const auto calls=m.get32(control+0x10u);
            unsigned allocations=0,yields=0;
            for(unsigned i=0;i<calls;++i)(m.get32(control+0x20u+i*48u)==0x21f2a3u?allocations:yields)++;
            const auto held=[&](std::uint32_t binding){return (std::to_integer<unsigned>(*c.memory.at(binding+4u))>>1)&1u;};
            switch(entry) {
            case 0x129470u:++paths[held(c.abi.gp[1])];break;
            case 0x1293f0u:++paths[held(c.abi.gp[0])];break;
            case 0x12b1c0u:++paths[held(bindings+(c.abi.gp[0]&0xffffu)*0xbcu)];break;
            case 0x1296e0u: {   // rejected format, codec global clear, empty ranges, playable
                const auto d=c.memory.get32(tags+(c.abi.gp[0]&0xffffu)*16u+8u);
                const auto encoding=std::to_integer<unsigned>(*c.memory.at(d+4u));
                const auto compression=std::to_integer<unsigned>(*c.memory.at(d+5u));
                if(compression!=1u && encoding!=2u)++paths[0];
                else if(encoding==2u && !c.memory.get32(0x48b8fcu))++paths[1];
                else ++paths[run.abi.gp[0]?3:2];
                break;
            }
            case 0x21f3b0u: {   // locked, held, not loaded, evictable
                const auto e=entries+(c.memory.get32(stack+4u)&0xffffu)*16u;
                const auto byte=[&](unsigned at){return std::to_integer<unsigned>(*c.memory.at(e+at));};
                ++paths[byte(4)?0:byte(5)?1:(run.abi.gp[0]&0xffu)?2:3];
                if((run.abi.gp[0]&0xffu)!=(byte(4)||byte(5)||!byte(2)?1u:0u))
                    throw std::runtime_error("21F3B0 classification differs");
                break;
            }
            case 0x21f260u: case 0x128970u: {
                // 128970 returns the ready bit; 21F260 the state bits.
                const auto state=run.abi.gp[0];
                if(allocations)++paths[0];
                if(yields)++paths[1];
                if(yields>1)++paths[2];
                if(entry==0x21f260u) {
                    if(state&1u)++paths[3];
                    if(state&2u)++paths[4];
                    if(state&4u)++paths[5];
                    if(!state)++paths[6];
                } else {
                    ++paths[3+(state&1u)];
                    // A ready or pending chunk takes the binding's lock.
                    const auto binding=c.memory.get32(stack+4u);
                    if(!held(binding) && (std::to_integer<unsigned>(*m.at(binding+4u))&2u))++paths[5];
                    if(held(binding))++paths[6];
                }
                break;
            }
            }
        }
        const auto need=[&](std::initializer_list<unsigned> list,const char *what) {
            for(const auto p:list)if(!paths[p]) {
                std::cerr<<std::hex<<entry<<std::dec<<" path coverage incomplete: "<<what<<" path "<<p<<'\n';
                std::exit(1);
            }
        };
        if(entry==0x129470u||entry==0x12b1c0u||entry==0x1293f0u)need({0,1},"lock held / free");
        if(entry==0x1296e0u)need({0,1,2,3},"format / codec global / empty / playable");
        if(entry==0x21f3b0u)need({0,1,2,3},"locked / held / unloaded / evictable");
        if(entry==0x21f260u)need({0,1,2,3,4,5,6},"allocate / yield / yields / pending / ready / locked / none");
        if(entry==0x128970u) {
            need({0,3,4,5,6},"allocate / not ready / ready / lock taken / already held");
            if(paths[1])throw std::runtime_error("128970 yielded although it never waits");
        }
    }
    std::cout<<"PASS native sound resource kernels: "<<compared<<" randomized states identical to the lifted routines ("
        <<faults<<" identical faults)\n";
    return 0;
} catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}
