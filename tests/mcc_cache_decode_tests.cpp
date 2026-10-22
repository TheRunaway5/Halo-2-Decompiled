#include "halo2/cache.hpp"
#include "halo2/mcc_cache.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <future>
#include <iostream>
#include <random>
#include <zlib.h>

#ifdef HALO2_DECODER_REFERENCE
namespace halo2 {std::vector<std::byte> decode_mcc_cache_reference(std::span<const std::byte>);}
#endif
namespace {
using namespace halo2;
using Buffer=std::vector<std::byte>;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
void word(Buffer& out,std::size_t at,std::uint32_t x){for(unsigned i=0;i<4;++i)out.at(at+i)=std::byte(x>>(i*8));}
struct Fixture {Buffer encoded,expected;};
Fixture fixture(unsigned count,unsigned chunk_size,int level,int strategy,bool short_chunks) {
    Fixture f;f.encoded.resize(896+count*8);word(f.encoded,0,0x68656164);word(f.encoded,4,13);
    word(f.encoded,28,1);word(f.encoded,776,chunk_size);word(f.encoded,784,896);word(f.encoded,788,count);
    std::mt19937 random(19);Buffer payload;
    for(unsigned i=0;i<count;++i) {
        const auto size=short_chunks?1+random()%chunk_size:chunk_size;
        Buffer part(size);for(auto& b:part)b=std::byte(random()%((i%3)?8:256));
        if(part.size()>2){part[0]=std::byte{0x78};part[1]=std::byte{0x9c};}
        payload.insert(payload.end(),part.begin(),part.end());Buffer stored;
        if(i%3==0)stored=part;
        else {
            z_stream z{};check(deflateInit2(&z,level,Z_DEFLATED,15,8,strategy)==Z_OK,"deflate init");
            stored.resize(deflateBound(&z,part.size()));z.next_in=reinterpret_cast<Bytef*>(part.data());z.avail_in=part.size();
            z.next_out=reinterpret_cast<Bytef*>(stored.data());z.avail_out=stored.size();
            check(deflate(&z,Z_FINISH)==Z_STREAM_END,"deflate");stored.resize(z.total_out);deflateEnd(&z);
        }
        word(f.encoded,896+i*8,i%3?std::uint32_t(stored.size()):std::uint32_t(-std::int32_t(stored.size())));
        word(f.encoded,900+i*8,f.encoded.size());f.encoded.insert(f.encoded.end(),stored.begin(),stored.end());
    }
    word(f.encoded,8,896+payload.size());f.expected.assign(f.encoded.begin(),f.encoded.begin()+896);
    f.expected.insert(f.expected.end(),payload.begin(),payload.end());return f;
}
unsigned comparisons{},rejections{};
void compare(const Buffer& input,const Buffer* expected=nullptr) {
    Buffer actual,reference;bool rejected=false,reference_rejected=false;
    try {actual=decode_mcc_cache(input);}catch(const FormatError&){rejected=true;}
#ifdef HALO2_DECODER_REFERENCE
    try {reference=decode_mcc_cache_reference(input);}catch(const FormatError&){reference_rejected=true;}
    check(rejected==reference_rejected,"decoder admission changed");
    if(!rejected)check(actual==reference,"reference decoded bytes changed");
#endif
    if(expected)check(!rejected && actual==*expected,"expected decoded bytes changed");
    else check(rejected,"malformed input accepted");
    ++comparisons;if(rejected)++rejections;
}
}
int main(int argc,char** argv) {
    try {
        for(auto strategy:{Z_DEFAULT_STRATEGY,Z_FILTERED,Z_HUFFMAN_ONLY,Z_RLE,Z_FIXED})
            for(auto level:{0,1,6,9})for(auto count:{3u,67u})for(bool short_chunks:{false,true}) {
                const auto f=fixture(count,count==3?211u:32768u,level,strategy,short_chunks);compare(f.encoded,&f.expected);
            }
        const auto f=fixture(19,262144,1,Z_DEFAULT_STRATEGY,true);compare(f.encoded,&f.expected);
        for(auto at:{0u,4u,8u,776u,788u,896u,900u}) {
            auto bad=f.encoded;word(bad,at,0);compare(bad);
        }
        for(auto n:{0u,1u,895u,896u,unsigned(f.encoded.size()-1)}) {
            auto bad=f.encoded;bad.resize(n);compare(bad);
        }
        for(auto bound:{1u,128u,262143u}) {
            auto full=fixture(19,262144,1,Z_DEFAULT_STRATEGY,false);word(full.encoded,776,bound);compare(full.encoded);
        }
        auto malformed=f.encoded;const Bytes b(f.encoded);const auto size=b.u32(904),address=b.u32(908);
        for(auto value:{size-1,size+1}){malformed=f.encoded;word(malformed,904,value);compare(malformed);}
        malformed=f.encoded;malformed.at(address+size-1)^=std::byte{1};compare(malformed);
        // Empty zlib payload is never an admitted map chunk.
        auto empty=fixture(2,1,1,Z_DEFAULT_STRATEGY,false);Buffer stream(32);uLongf length=stream.size();
        check(compress2(reinterpret_cast<Bytef*>(stream.data()),&length,nullptr,0,1)==Z_OK,"empty compression");
        word(empty.encoded,904,length);word(empty.encoded,908,empty.encoded.size());
        empty.encoded.insert(empty.encoded.end(),stream.begin(),stream.begin()+length);compare(empty.encoded);
        // Simultaneous callers own separate decode buffers and accelerator state.
        std::vector<std::future<Buffer>> jobs;
        for(unsigned i=0;i<8;++i)jobs.push_back(std::async(std::launch::async,[&]{return decode_mcc_cache(f.encoded);}));
        for(auto& job:jobs)check(job.get()==f.expected,"concurrent decoder bytes changed");
        std::cout<<"contracts "<<comparisons<<" rejections "<<rejections<<" concurrent 8\n";
#ifdef HALO2_DECODER_REFERENCE
        for(int i=1;i<argc;++i) {
            std::ifstream file(argv[i],std::ios::binary|std::ios::ate);check(bool(file),"map open");Buffer input(file.tellg());file.seekg(0);file.read(reinterpret_cast<char*>(input.data()),input.size());
            double before{},after{};
            for(unsigned repetition=0;repetition<3;++repetition) {
                auto start=std::chrono::steady_clock::now();auto reference=decode_mcc_cache_reference(input);
                before+=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
                start=std::chrono::steady_clock::now();auto actual=decode_mcc_cache(input);
                after+=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();check(actual==reference,"installed map bytes changed");
            }
            std::cout<<argv[i]<<" reference_seconds "<<before/3<<" accelerated_seconds "<<after/3<<"\n";
        }
#endif
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
