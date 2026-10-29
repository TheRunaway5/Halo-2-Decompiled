#include "halo2/mcc_resource_reader.hpp"
#include "halo2/mcc_cache.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <functional>
#include <iostream>
#include <thread>
#include <zlib.h>
#if defined(__linux__)
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
using namespace halo2;
namespace {
using Buffer=std::vector<std::byte>;
unsigned checks{},negatives{};
thread_local unsigned decodes{};
#if defined(__linux__)
thread_local std::function<void()> during_decode,during_pread,after_stat;
#endif
void check(bool value,const char* why){++checks;if(!value)throw FormatError(why);}
template<class F>void reject(F&& f){bool threw=false;try{f();}catch(const FormatError&){threw=true;}check(threw,"Invalid/stale MCC resource source admitted");++negatives;}
void word(Buffer& b,unsigned at,unsigned value){for(unsigned i=0;i<4;++i)b.at(at+i)=std::byte(value>>(i*8));}
Buffer source(unsigned seed){Buffer b(8192);word(b,0,0x68656164);word(b,4,13);word(b,8,b.size());for(unsigned i=896;i<b.size();++i)b[i]=std::byte((i*37+seed)%256);return b;}
Buffer compress(const Buffer& input,bool raw){Buffer b(input.begin(),input.begin()+896);word(b,28,1);word(b,776,8192);word(b,780,904);word(b,784,896);word(b,788,1);Buffer block;if(raw)block.assign(input.begin()+896,input.end());else{uLongf n=compressBound(input.size()-896);block.resize(n);check(compress2(reinterpret_cast<Bytef*>(block.data()),&n,reinterpret_cast<const Bytef*>(input.data()+896),input.size()-896,Z_BEST_SPEED)==Z_OK,"Fixture compression failed");block.resize(n);}b.resize(904);word(b,896,raw?unsigned(-int(input.size()-896)):unsigned(block.size()));word(b,900,904);b.insert(b.end(),block.begin(),block.end());return b;}
void save(const std::filesystem::path& p,const Buffer& b){std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),std::streamsize(b.size()));check(bool(f),"Fixture save failed");}
Buffer direct(std::span<const std::byte> b,unsigned offset,unsigned size){const auto v=b.subspan(offset,size);return {v.begin(),v.end()};}
#if defined(__linux__)
void touch(const std::filesystem::path& p,long ns){const std::array<timespec,2> t{{{0,UTIME_OMIT},{100,ns}}};check(::utimensat(AT_FDCWD,p.c_str(),t.data(),0)==0,"Fixture timestamp update failed");}
#endif
void portable(const std::filesystem::path& root){
    const auto a=source(1),b=source(7);const auto ap=root/"a.map",bp=root/"b.map";
    for(unsigned mode=0;mode<3;++mode){const auto encoded=mode==0?a:compress(a,mode==1);const auto p=root/("mode"+std::to_string(mode)+".map");save(p,encoded);const auto expected=decode_mcc_cache(encoded);const unsigned before=decodes;for(const auto [offset,size]:{std::pair{0u,896u},std::pair{895u,3u},std::pair{896u,1u},std::pair{1023u,2048u},std::pair{8191u,1u}})check(read_mcc_resource_range(p,offset,size)==direct(expected,offset,size),"Logical resource/header bytes differ from unchanged full decoder");
#if defined(__linux__)
        check(decodes==before+1,"Unchanged container decoded repeatedly");
#else
        (void)before;
#endif
        auto owned=read_mcc_resource_range(p,900,10);owned[0]^=std::byte{255};check(read_mcc_resource_range(p,900,10)==direct(expected,900,10),"Caller mutated immutable decoded owner");
        reject([&]{(void)read_mcc_resource_range(p,0,0);});reject([&]{(void)read_mcc_resource_range(p,0,128u*1024*1024+1);});reject([&]{(void)read_mcc_resource_range(p,8192,1);});reject([&]{(void)read_mcc_resource_range(p,8191,2);});reject([&]{(void)read_mcc_resource_range(p,UINT64_MAX,1);});
    }
    save(ap,a);save(bp,b);check(read_mcc_resource_range(ap,900,32)==direct(a,900,32),"First mod path failed");check(read_mcc_resource_range(bp,900,32)==direct(b,900,32),"Different mod file reused old data");check(read_mcc_resource_range(ap,900,32)==direct(a,900,32),"Last-container switch failed");
    const auto sub=root/"mods";std::filesystem::create_directory(sub);save(sub/"a.map",b);check(read_mcc_resource_range(sub/"a.map",900,32)==direct(b,900,32),"Same basename in mod folder collided");
    const auto old=read_mcc_resource_range(ap,900,32);const auto replacement=root/"replacement.map";save(replacement,b);std::filesystem::rename(replacement,ap);check(read_mcc_resource_range(ap,900,32)==direct(b,900,32) && old==direct(a,900,32),"Replaced inode reused stale bytes or changed returned ownership");
    auto bad=a;word(bad,4,12);const auto malformed=root/"malformed.map";save(malformed,bad);reject([&]{(void)read_mcc_resource_range(malformed,900,1);});bad=compress(a,false);bad.pop_back();save(malformed,bad);reject([&]{(void)read_mcc_resource_range(malformed,900,1);});save(malformed,Buffer(100));reject([&]{(void)read_mcc_resource_range(malformed,0,1);});reject([&]{(void)read_mcc_resource_range(root/"missing.map",900,1);});reject([&]{(void)read_mcc_resource_range(root,0,1);});
#if defined(__linux__)
    const auto changed=root/"changed.map";save(changed,a);touch(changed,1);const auto first=read_mcc_resource_range(changed,900,32);struct stat original{};check(::stat(changed.c_str(),&original)==0,"Fixture initial stat failed");
    // Preserve mtime deliberately; ctime must distinguish the in-place write.
    struct stat later{};for(unsigned attempt=0;attempt<100;++attempt){std::this_thread::sleep_for(std::chrono::milliseconds(2));save(changed,b);touch(changed,1);check(::stat(changed.c_str(),&later)==0,"Fixture changed stat failed");if(later.st_ctim.tv_sec!=original.st_ctim.tv_sec || later.st_ctim.tv_nsec!=original.st_ctim.tv_nsec)break;}
    check(later.st_mtim.tv_sec==original.st_mtim.tv_sec && later.st_mtim.tv_nsec==original.st_mtim.tv_nsec && (later.st_ctim.tv_sec!=original.st_ctim.tv_sec || later.st_ctim.tv_nsec!=original.st_ctim.tv_nsec),"Fixture could not isolate changed ctime");check(read_mcc_resource_range(changed,900,32)==direct(b,900,32) && first==direct(a,900,32),"Preserved mtime hid changed source");
    const auto ns_before=decodes;touch(changed,2);check(read_mcc_resource_range(changed,900,1)==direct(b,900,1),"Nanosecond mtime change corrupted data");check(decodes==ns_before+1,"Nanosecond identity change did not invalidate");
    const auto hard=root/"hard.map";std::filesystem::create_hard_link(changed,hard);(void)read_mcc_resource_range(changed,900,1);const auto hard_before=decodes;(void)read_mcc_resource_range(hard,900,1);check(decodes==hard_before,"Same admitted inode through hardlink was treated as a different data owner");
    const auto during=root/"during.map";save(during,a);during_decode=[&]{save(during,b);touch(during,3);};reject([&]{(void)read_mcc_resource_range(during,900,32);});check(read_mcc_resource_range(during,900,32)==direct(b,900,32),"Changed-during-decode owner was published");
    save(during,a);touch(during,4);during_decode=[&]{std::filesystem::resize_file(during,100);};reject([&]{(void)read_mcc_resource_range(during,900,32);});save(during,b);touch(during,5);check(read_mcc_resource_range(during,900,32)==direct(b,900,32),"Truncated source contaminated future owner");
    after_stat=[&]{save(during,a);touch(during,6);};reject([&]{(void)read_mcc_resource_range(during,900,32);});check(read_mcc_resource_range(during,900,32)==direct(a,900,32),"Cache-hit mutation published stale source");
    save(ap,a);touch(ap,8);const auto link=root/"link.map";
    std::filesystem::create_symlink(ap,link);
    reject([&]{(void)read_mcc_resource_range(link,900,32);});
    const auto directory_link=root/"linked-directory";
    std::filesystem::create_directory_symlink(sub,directory_link);
    reject([&]{(void)read_mcc_resource_range(directory_link/"a.map",900,32);});
    const auto threaded=root/"threaded.map";save(threaded,a);std::array<unsigned,2> counts{};std::array<bool,2> equal{};
    std::array<std::thread,2> workers{std::thread([&]{const auto begin=decodes;const auto x=read_mcc_resource_range(threaded,900,32),y=read_mcc_resource_range(threaded,900,32);counts[0]=decodes-begin;equal[0]=x==y && x==direct(a,900,32);}),std::thread([&]{const auto begin=decodes;const auto x=read_mcc_resource_range(threaded,900,32),y=read_mcc_resource_range(threaded,900,32);counts[1]=decodes-begin;equal[1]=x==y && x==direct(a,900,32);})};for(auto& w:workers)w.join();check(counts==std::array{1u,1u} && equal[0] && equal[1],"Threads shared a mutable or unowned last-container cache");
#endif
}
}
#if defined(__linux__)
std::vector<std::byte> real_decode(std::span<const std::byte>) asm("__real__ZN5halo216decode_mcc_cacheESt4spanIKSt4byteLm18446744073709551615EE");
std::vector<std::byte> wrap_decode(std::span<const std::byte>) asm("__wrap__ZN5halo216decode_mcc_cacheESt4spanIKSt4byteLm18446744073709551615EE");
std::vector<std::byte> wrap_decode(std::span<const std::byte> bytes){++decodes;auto action=std::move(during_decode);during_decode={};if(action)action();return real_decode(bytes);}
extern "C" ssize_t __real_pread(int,void*,size_t,off_t);
extern "C" ssize_t __wrap_pread(int fd,void* bytes,size_t count,off_t offset){auto action=std::move(during_pread);during_pread={};if(action)action();return __real_pread(fd,bytes,count,offset);}
extern "C" int __real_fstat(int,struct stat*);
extern "C" int __wrap_fstat(int fd,struct stat* value){const auto result=__real_fstat(fd,value);auto action=std::move(after_stat);after_stat={};if(action)action();return result;}
#endif
int main(int argc,char** argv)try{
    const auto root=std::filesystem::temp_directory_path()/("halo2-resource-range-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));std::filesystem::create_directories(root);try{portable(root);}catch(...){std::filesystem::remove_all(root);throw;}std::filesystem::remove_all(root);
    std::size_t actual_bytes{};
    if(argc>1){if(argc!=2)throw FormatError("Supply one optional MCC shared-container path");if(std::string(argv[1]).empty() || !std::filesystem::is_regular_file(argv[1]))return 77;const auto full=read_mcc_cache(argv[1]);for(std::size_t at=0;at<full.size();at+=8*1024*1024){const auto n=unsigned(std::min<std::size_t>(8*1024*1024,full.size()-at));const auto range=read_mcc_resource_range(argv[1],at,n);check(range.size()==n && std::equal(range.begin(),range.end(),full.begin()+at),"Actual MCC logical bytes differ from unchanged full decoder");actual_bytes+=n;}}
    std::cout<<"PASS "<<checks<<" checks / "<<negatives<<" negative cases; "<<actual_bytes<<" actual logical bytes exact; opened-FD identity, replacement/mod paths and thread ownership\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
