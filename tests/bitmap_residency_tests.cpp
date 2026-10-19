#include "halo2/cache.hpp"
#include "halo2/original_data.hpp"
#include <array>
#include <iostream>
#include <fstream>
#include <thread>

using namespace halo2;
namespace {
void check(bool ok,const char* message){if(!ok)throw FormatError(message);}
const std::byte* bytes(const Texture& image){return image.pixels.data();}
template<class F>void rejects(F call){bool rejected=false;try{call();}catch(const FormatError&){rejected=true;}
    check(rejected,"Invalid bitmap request entered the residency cache");}
}
int main(int argc,char** argv)try{
    if((argc!=3 && argc!=4) || !std::filesystem::is_regular_file(argv[1]) || !std::filesystem::is_regular_file(argv[2]))return 77;
    Cache cache(argv[2],std::filesystem::path(argv[2]).parent_path(),OriginalData::load_xbe(argv[1]));
    std::uint32_t selected=0xffffffffu,paletted=0xffffffffu;
    for(const auto& tag:cache.tags())if(tag.group=="bitm" && tag.address && tag.size>=76){
        if(!cache.bitmap_count(tag.datum))continue;
        const auto resource=cache.load_bitmap_resource(tag.datum,0);
        if(resource.width>512 || resource.height>512 || resource.type!=0)continue;
        if(resource.format==18){if(paletted==0xffffffffu)paletted=tag.datum;}
        else if(selected==0xffffffffu && (resource.format==11 ||
            (resource.format>=14 && resource.format<=17)))selected=tag.datum;
        if(selected!=0xffffffffu && paletted!=0xffffffffu)break;
    }
    check(selected!=0xffffffffu,"Owned intro map has no bounded nonpaletted bitmap fixture");
    auto first=cache.load_bitmap(selected,0);const auto second=cache.load_bitmap(selected,0);
    check(bytes(first)==bytes(second),"Repeated native bitmap loads decoded independent image allocations");
    check(first.width==second.width && first.height==second.height && first.format==second.format &&
        first.layers==second.layers && first.depth==second.depth && first.pixels==second.pixels,
        "Bitmap residency changed decoded dimensions, format or bytes");
    const auto remembered=first;auto* writable=first.pixels.data();
    const auto late_copy=first;writable[0]^=std::byte{1};
    check(late_copy.pixels==remembered.pixels && cache.load_bitmap(selected,0).pixels==remembered.pixels,
        "Writable bitmap output changed the cache or a later value snapshot");
    rejects([&]{cache.load_bitmap(selected,0xffffffffu);});
    rejects([&]{cache.load_bitmap(selected,0xffffffffu);});
    unsigned palette_cases{};
    if(paletted!=0xffffffffu){
        std::vector<std::byte> dark(1024),light(1024,std::byte{255});
        const auto a=cache.load_bitmap(paletted,0,dark),b=cache.load_bitmap(paletted,0,light);
        check(a.pixels!=b.pixels,"Distinct declared palette bytes aliased one decoded image");
        const auto again=cache.load_bitmap(paletted,0,dark);
        check(bytes(a)==bytes(again),"Identical palette input lost native image residency");
        dark[0]=std::byte{3};const auto changed=cache.load_bitmap(paletted,0,dark);
        (void)changed;std::vector<std::byte> original(1024);
        check(bytes(cache.load_bitmap(paletted,0,original))==bytes(a),"Cache key borrowed mutable caller palette storage");
        rejects([&]{cache.load_bitmap(paletted,0);});palette_cases=1;
    }
    std::array<std::thread,4> threads;std::array<bool,4> okay{};
    for(unsigned i=0;i<threads.size();++i)threads[i]=std::thread([&,i]{
        const auto value=cache.load_bitmap(selected,0);okay[i]=bytes(value)==bytes(second) && value.pixels==second.pixels;
    });
    for(auto& thread:threads)thread.join();for(bool ok:okay)check(ok,"Concurrent bitmap reads lost immutable native residency");
    Cache cold(argv[2],std::filesystem::path(argv[2]).parent_path(),cache.original_data());
    std::array<Texture,4> initial;
    for(unsigned i=0;i<threads.size();++i)threads[i]=std::thread([&,i]{initial[i]=cold.load_bitmap(selected,0);});
    for(auto& thread:threads)thread.join();
    for(const auto& value:initial)check(bytes(value)==bytes(initial[0]) && value.pixels==remembered.pixels,
        "Concurrent cold decodes published different native image residencies");
    if(argc==4){std::ofstream report(argv[3]);report<<"{\"passed\":true,\"palette_cases\":"<<palette_cases
        <<",\"bitmap\":"<<selected<<",\"pixel_bytes\":"<<remembered.pixels.size()<<"}\n";
        check(bool(report),"Cannot retain bitmap residency evidence");}
    std::cout<<"PASS owned intro bitmap residency, independent writable results, rejection and concurrent reads; palette_cases="<<palette_cases<<'\n';
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
