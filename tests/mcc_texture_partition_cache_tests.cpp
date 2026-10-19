#include "halo2/mcc_cache.hpp"
#include <zlib.h>
#include <algorithm>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
#include <array>
#include <vector>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
static std::atomic<unsigned> inflates{};
extern "C" int __real_inflate(z_streamp,int);
extern "C" int __wrap_inflate(z_streamp stream,int flush){++inflates;return __real_inflate(stream,flush);}
namespace {
void need(bool okay,const char*message){if(!okay)throw std::runtime_error(message);}
void word(std::vector<std::byte>&out,std::uint32_t n){for(unsigned i=0;i<4;++i)out.push_back(std::byte(n>>(i*8)));}
void save(const std::filesystem::path&p,const std::vector<std::byte>&b){std::ofstream f(p,std::ios::binary|std::ios::trunc);f.write(reinterpret_cast<const char*>(b.data()),b.size());need(bool(f),"Fixture write failed");}
std::vector<std::byte> partition(const std::vector<std::byte>&bytes){uLongf n=compressBound(bytes.size());std::vector<std::byte> zipped(n);need(compress2(reinterpret_cast<Bytef*>(zipped.data()),&n,reinterpret_cast<const Bytef*>(bytes.data()),bytes.size(),Z_BEST_SPEED)==Z_OK,"Fixture compression failed");zipped.resize(n);std::vector<std::byte> out;word(out,1);word(out,zipped.size());out.insert(out.end(),zipped.begin(),zipped.end());return out;}
}
int main(int argc,char**argv)try{
 const auto* scratch=argc==2?argv[1]:std::getenv("HALO2_TEST_SCRATCH");need(scratch,"Supply disposable scratch");auto root=std::filesystem::path(scratch);auto path=root/"partition.dat";
 std::vector<std::byte> original(1024*1024);for(std::size_t i=0;i<original.size();++i)original[i]=std::byte((i*37+(i>>10))&255);auto raw=partition(original);save(path,raw);
 auto first=halo2::read_mcc_texture_partition(path,0,raw.size());need(first==original,"First decode changed bytes");const auto before=inflates.load();
 for(unsigned i=0;i<12;++i)need(halo2::read_mcc_texture_partition(path,0,raw.size())==original,"Repeated partition changed bytes");
 const auto repeated=inflates.load()-before;need(!repeated,"Repeated immutable MCC partition was decompressed again");
 first[0]^=std::byte{127};need(halo2::read_mcc_texture_partition(path,0,raw.size())==original,"Writable caller result contaminated residency");
 auto changed=original;changed[changed.size()/2]^=std::byte{3};const auto replacement=partition(changed);save(path,replacement);need(halo2::read_mcc_texture_partition(path,0,replacement.size())==changed,"Same-path changed data reused stale pixels");
 auto broken=raw;broken.back()^=std::byte{255};save(path,broken);unsigned rejected{};
 for(unsigned i=0;i<2;++i){try{halo2::read_mcc_texture_partition(path,0,broken.size());}catch(const std::exception&){++rejected;}}need(rejected==2,"Malformed bytes acquired a successful cached result");
 save(path,raw);std::array<std::thread,4> threads;std::array<bool,4> okay{};for(unsigned i=0;i<4;++i)threads[i]=std::thread([&,i]{okay[i]=halo2::read_mcc_texture_partition(path,0,raw.size())==original;});for(auto&t:threads)t.join();for(auto v:okay)need(v,"Concurrent partition lookup changed data");
 // Simultaneous first readers may race to decode, but must publish only
 // immutable byte-identical values and leave subsequent hits decode-free.
 auto cold=original;cold.back()^=std::byte{9};const auto cold_raw=partition(cold);save(root/"cold.dat",cold_raw);
 for(unsigned i=0;i<4;++i)threads[i]=std::thread([&,i]{okay[i]=halo2::read_mcc_texture_partition(root/"cold.dat",0,cold_raw.size())==cold;});for(auto&t:threads)t.join();for(auto v:okay)need(v,"Concurrent cold publication changed output");
 const auto warm=inflates.load();need(halo2::read_mcc_texture_partition(root/"cold.dat",0,cold_raw.size())==cold && inflates.load()==warm,"Cold publication did not become resident");
 // Exceed the residency budget using distinct compressed inputs, then revisit
 // the first key. Eviction must preserve decoding and invalid-input behavior.
 std::vector<std::byte> block(2*1024*1024,std::byte{23});for(unsigned i=0;i<68;++i){block[0]=std::byte(i);const auto b=partition(block);save(root/"eviction.dat",b);need(halo2::read_mcc_texture_partition(root/"eviction.dat",0,b.size())==block,"Eviction changed output");}
 save(path,raw);const auto evicted=inflates.load();need(halo2::read_mcc_texture_partition(path,0,raw.size())==original && inflates.load()>evicted,"Oldest partition was not evicted or could not decode again");
 std::cout<<"{\"status\":\"GREEN\",\"repeated_decode_calls\":"<<repeated<<",\"malformed_rejections\":"<<rejected<<",\"concurrent_readers\":4,\"eviction_inputs\":68,\"independent_writes_and_changed_files\":true}\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
