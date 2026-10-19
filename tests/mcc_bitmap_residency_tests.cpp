#include "halo2/cache.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>

using namespace halo2;
namespace {void check(bool value,const char* why){if(!value)throw std::runtime_error(why);}}
int main(int argc,char**argv)try {
    if(argc!=2 || !std::filesystem::is_regular_file(argv[1]))return 77;
    Cache cache(argv[1]);check(cache.is_mcc(),"Bitmap residency fixture requires the owned MCC cache");
    const Tag* selected=nullptr;Texture first;
    const auto tags=cache.select_tags();
    for(const auto& tag:tags)if(tag.group=="bitm" && tag.address) {
        try {auto value=cache.load_bitmap(tag.datum,0);if(value.pixels.empty())continue;selected=&tag;first=std::move(value);break;}
        catch(const FormatError&){}
    }
    check(selected,"No supported actual MCC bitmap owner exercised");
    const auto second=cache.load_bitmap(selected->datum,0);
    check(std::as_const(first).pixels.data()==second.pixels.data(),"Repeated MCC bitmap load duplicates completed decoded storage");
    check(first.width==second.width && first.height==second.height && first.depth==second.depth &&
        first.layers==second.layers && first.format==second.format && first.pixels==second.pixels,
        "MCC bitmap residency changed source metadata or bytes");
    const auto before=second.pixels[0];first.pixels[0]=std::byte(std::to_integer<unsigned>(before)^1u);
    check(second.pixels[0]==before && std::as_const(first).pixels.data()!=second.pixels.data(),
        "MCC bitmap writer changed another retained consumer");
    const auto third=cache.load_bitmap(selected->datum,0);
    check(third.pixels.data()==second.pixels.data() && third.pixels[0]==before,"MCC writer changed the cache's immutable bitmap");
    bool rejected=false;try{(void)cache.load_bitmap(selected->datum,std::numeric_limits<unsigned>::max());}catch(const FormatError&){rejected=true;}
    check(rejected,"MCC bitmap cache admitted an invalid subresource index");
    rejected=false;try{(void)cache.load_bitmap(selected->datum^0x10000u,0);}catch(const FormatError&){rejected=true;}
    check(rejected,"MCC bitmap cache admitted a stale salted handle");
    std::cout<<"MCC decoded bitmap residency/mutable isolation/2 rejection controls: "<<selected->name<<' '
        <<second.width<<'x'<<second.height<<" bytes="<<second.pixels.size()<<'\n';return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
