#include "halo2/source_object_body_storage.hpp"
#include "halo2/original_data.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

namespace {
using namespace halo2;
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class F> void rejects(F f){bool stopped=false;try{f();}catch(const std::exception&){stopped=true;}
    require(stopped,"unowned storage operation admitted");}
std::uint32_t word(const std::string& s){return std::stoul(s,nullptr,16);}
std::vector<std::byte> state(const NativeObjectLifetimes& lifetimes,const SourceObjectBodyStorage& bodies){
    std::vector<std::byte> result;
    const auto push=[&](std::uint32_t v){for(unsigned j=0;j<4;++j)result.push_back(std::byte(v>>(8*j)));};
    const auto& pool=lifetimes.pool();
    push(pool.cursor());push(pool.high_water());push(pool.count());push(pool.next_salt());
    for(auto v:pool.bitmap())push(v);
    for(auto v:pool.elements())result.push_back(std::byte(v));
    result.insert(result.end(),bodies.arena().begin(),bodies.arena().end());
    return result;
}
void native_ownership(){
    NativeObjectLifetimes lifetimes(8);SourceObjectBodyStorage bodies(512);
    const auto h=bodies.allocate(lifetimes,65);
    require(h!=DatumPool::none && lifetimes.owner(h).type==0xff && lifetimes.header(h)[3]==0,
        "storage reservation wrote an early source type");
    rejects([&]{lifetimes.publish_native(h,0);});
    rejects([&]{lifetimes.release(h);});
    auto raw=bodies.backing(lifetimes,h);require(raw.size()==80 && bodies.body(lifetimes,h).size()==65,
        "logical/physical storage extents conflated");
    for(std::size_t i=65;i<raw.size();++i)require(raw[i]==std::byte{0xcd},"base allocator cleared padding");
    const auto blocker=bodies.allocate(lifetimes,80);require(blocker!=DatumPool::none,"relocation blocker absent");
    raw[79]=std::byte{0x57};
    // Source resize copies physical padding; zero-sized append needs no grow.
    auto pool=bodies.objects(lifetimes);require(pool.grow(h,160),"real relocation failed");
    require(bodies.backing(lifetimes,h)[79]==std::byte{0x57} && bodies.body(lifetimes,h).size()==65,
        "resize lost physical padding or committed the caller's logical extent");
    auto saved_lifetimes=lifetimes;auto saved_bodies=bodies;const auto saved=state(lifetimes,bodies);
    const auto saved_scratch=std::vector<std::byte>(bodies.scratch().begin(),bodies.scratch().end());
    bodies.release(lifetimes,h);bodies.release(lifetimes,blocker);
    require(!bodies.contains(h) && !lifetimes.contains(h),"storage retirement retained a datum");
    rejects([&]{bodies.release(lifetimes,h);});rejects([&]{bodies.append(lifetimes,h,0,4,0);});
    const auto reused=bodies.allocate(lifetimes,64);require(reused!=h && std::uint16_t(reused)==std::uint16_t(h),
        "storage retirement did not use shared salt reuse");
    lifetimes=saved_lifetimes;bodies=saved_bodies;
    require(state(lifetimes,bodies)==saved && std::equal(bodies.scratch().begin(),bodies.scratch().end(),saved_scratch.begin()),
        "value restore lost arena, private frame, header bytes or allocator metadata");
    require(bodies.append(lifetimes,h,0,23,2),"restored storage closure points at a discarded owner");
    require(state(saved_lifetimes,saved_bodies)==saved,"copy mutation aliased saved storage");
    // Native category identities and recovered storage share the same pool.
    const auto native=lifetimes.reserve(5);lifetimes.publish_native(native,DatumPool::none);
    require(!bodies.contains(native) && lifetimes.contains(native),"body bank inferred a native category body");
    rejects([&]{bodies.release(lifetimes,native);});lifetimes.release(native);
    NativeObjectLifetimes oversized(2049);SourceObjectBodyStorage bounded;
    rejects([&]{bounded.allocate(oversized,64);});require(oversized.pool().count()==0,"capacity rejection mutated header pool");
    rejects([&]{bodies.memory(lifetimes,0,4);});
}
}
int main(int argc,char** argv){
    try{
        require(argc==5,"supply storage fixture, extent fixture and selected images");
        for(int i=1;i<argc;++i)if(!std::filesystem::exists(argv[i]))return 77;
        native_ownership();
        std::unique_ptr<NativeObjectLifetimes> lifetimes;std::unique_ptr<SourceObjectBodyStorage> bodies;
        std::ifstream input(argv[1]);std::size_t compared=0;unsigned cases=0,operations=0,snapshots=0;
        std::string name;
        for(std::string line;std::getline(input,line);){
            std::istringstream row(line);char kind;std::string text;require(bool(row>>kind),"empty storage fixture");
            if(kind=='N'){
                unsigned pool,heap,initial;require(bool(row>>name>>pool>>heap>>initial),"incomplete storage input");
                lifetimes=std::make_unique<NativeObjectLifetimes>(pool);
                bodies=std::make_unique<SourceObjectBodyStorage>(heap,std::byte(initial));++cases;
            }else{
                require(bool(lifetimes)&&bool(bodies),"operation outside storage case");
                try{
                    if(kind=='A'){
                        int extent;require(bool(row>>extent>>text),"incomplete base allocation");
                        require(bodies->allocate(*lifetimes,std::int16_t(extent))==word(text),"base allocation result differs");
                    }else if(kind=='G'){
                        std::string h;int field,bytes;unsigned alignment,expected;
                        require(bool(row>>h>>field>>bytes>>alignment>>expected),"incomplete component append");
                        require(bodies->append(*lifetimes,word(h),std::int16_t(field),std::int16_t(bytes),alignment)==bool(expected),
                            "component append result differs");
                    }else if(kind=='P'){
                        unsigned n,seed;require(bool(row>>text>>n>>seed),"incomplete physical payload control");
                        auto b=bodies->backing(*lifetimes,word(text));require(n<=b.size(),"payload control exceeds physical backing");
                        for(unsigned i=0;i<n;++i)b[i]=std::byte((i*37+seed)&255);
                    }else if(kind=='W'){
                        unsigned offset,n,value;require(bool(row>>text>>offset>>n>>value),"incomplete header control");
                        auto h=lifetimes->header(word(text));require(offset>=2&&offset+n<=8&&n<=4,"unowned header control");
                        for(unsigned i=0;i<n;++i)h[offset+i]=std::uint8_t(value>>(8*i));
                    }else if(kind=='M'){
                        unsigned type;require(bool(row>>type>>text),"incomplete native category control");
                        require(lifetimes->reserve(type)==word(text),"shared category reservation differs");
                    }else if(kind=='D'){
                        require(bool(row>>text),"missing native category identity");lifetimes->release(word(text));
                    }else if(kind=='F'){
                        require(bool(row>>text),"missing storage identity");bodies->release(*lifetimes,word(text));
                    }else if(kind=='S'){
                        require(bool(row>>text),"missing full-state snapshot");
                        const auto path=std::filesystem::path(argv[1]).parent_path()/text;
                        std::ifstream f(path,std::ios::binary);require(bool(f),"source snapshot absent");
                        std::vector<char> expected((std::istreambuf_iterator<char>(f)),{});const auto actual=state(*lifetimes,*bodies);
                        require(expected.size()==actual.size(),"full storage state extent differs");
                        for(std::size_t i=0;i<actual.size();++i)if(std::byte(std::uint8_t(expected[i]))!=actual[i])
                            throw std::runtime_error("full storage byte differs at "+std::to_string(i));
                        for(auto h:bodies->handles())if(h!=DatumPool::none)
                            require(lifetimes->contains(h)&&bodies->contains(h),"body bank lost shared full identity");
                        compared+=actual.size();++snapshots;
                    }else throw std::runtime_error("unknown storage operation");
                }catch(const std::exception& error){throw std::runtime_error(name+" operation "+std::to_string(operations)+": "+error.what());}
                ++operations;
            }
            require(!(row>>text),"unexpected storage fixture operand");
        }
        require(cases>=20 && snapshots>=100 && compared>1000000,"incomplete full-state storage coverage");
        const std::array images{halo2::OriginalData::load_xbe(argv[3]),halo2::OriginalData::load_xbe(argv[4])};
        std::ifstream extents(argv[2]);unsigned rows=0;
        for(std::string line;std::getline(extents,line);){
            unsigned image,type,extent;std::istringstream row(line);
            require(bool(row>>image>>type>>extent)&&image>=1&&image<=2&&type<13,"invalid original extent row");
            require(rows==(image-1)*13+type,"original extent fixture out of order");
            const auto tables=images[image-1]->object_construction_tables;
            require(tables&&tables->base_extent(type)==extent,"selected type extent import differs");++rows;
            require(!(row>>name),"extra extent fixture operand");
        }
        require(rows==26,"incomplete selected type extent coverage");
        rejects([&]{SourceObjectConstructionTables{}.base_extent(0);});
        rejects([&]{images[0]->object_construction_tables->base_extent(-1);});
        rejects([&]{images[0]->object_construction_tables->base_extent(13);});
        std::cout<<"storageCases="<<cases<<" operations="<<operations<<" fullSnapshots="<<snapshots
            <<" comparedBytes="<<compared<<" selectedExtents="<<rows<<" sharedIdentityCopyRestore=pass\n";
        return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
