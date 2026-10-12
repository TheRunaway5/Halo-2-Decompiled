#include "halo2/source_bitmap_image_binding.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace halo2;
namespace {
struct Region {std::uint32_t address;std::vector<std::byte> bytes;};
using Table=std::map<std::string,Region>;
struct Patch {std::string name;std::size_t offset;std::vector<std::byte> bytes;};
std::vector<std::byte> decode(const std::string &hex){
    if(hex.size()%2)throw std::invalid_argument("image binding fixture odd hex byte length");
    const auto nibble=[](char c)->unsigned{
        if(c>='0' && c<='9')return unsigned(c-'0');
        if(c>='a' && c<='f')return unsigned(c-'a'+10);
        if(c>='A' && c<='F')return unsigned(c-'A'+10);
        throw std::invalid_argument("image binding fixture nonhex byte");
    };
    std::vector<std::byte> bytes;
    for(std::size_t i=0;i<hex.size();i+=2)bytes.push_back(std::byte(nibble(hex[i])*16+nibble(hex[i+1])));
    return bytes;
}
std::string encode(std::span<const std::byte> bytes){
    constexpr char hex[]="0123456789abcdef";std::string out;
    for(auto byte:bytes){const auto value=std::to_integer<unsigned>(byte);out+=hex[value>>4];out+=hex[value&15];}
    return out;
}
SourceBitmapImageBindingBank banks(Table &table){
    const auto get=[&](const std::string &name){auto &region=table.at(name);return SourceBitmapManagerRegion{region.address,region.bytes};};
    return {get("image_pool"),get("image_rows"),get("image_bits")};
}
std::vector<Patch> differences(const Table &before,const Table &after){
    std::vector<Patch> patches;
    for(const auto &[name,region]:before){
        const auto &final=after.at(name).bytes;std::size_t at=0;
        if(final.size()!=region.bytes.size())throw std::runtime_error("image binding bank size changed");
        while(at<final.size()){
            if(final[at]==region.bytes[at]){++at;continue;}
            const auto start=at;
            while(at<final.size() && final[at]!=region.bytes[at])++at;
            patches.push_back({name,start,{final.begin()+std::ptrdiff_t(start),final.begin()+std::ptrdiff_t(at)}});
        }
    }
    return patches;
}
void patch(Table &table,const Patch &change){
    auto &bytes=table.at(change.name).bytes;
    if(change.offset>bytes.size() || change.bytes.size()>bytes.size()-change.offset)
        throw std::invalid_argument("image binding frozen patch outside bank");
    std::copy(change.bytes.begin(),change.bytes.end(),bytes.begin()+std::ptrdiff_t(change.offset));
}
void read_patches(std::istringstream &input,Table &table){
    unsigned count;input>>std::dec>>count;
    if(!input)throw std::invalid_argument("image binding fixture patch count missing");
    for(unsigned i=0;i<count;++i){Patch change;std::string hex;
        input>>change.name>>std::hex>>change.offset>>hex;
        if(!input)throw std::invalid_argument("image binding fixture patch malformed");
        change.bytes=decode(hex);patch(table,change);
    }
}
void put(std::span<std::byte> bytes,unsigned at,std::uint32_t value){
    for(unsigned i=0;i<4;++i)bytes[at+i]=std::byte((value>>(8*i))&255u);
}
unsigned negatives(const Table &initial){
    unsigned controls=0;
    constexpr std::uint32_t free_handle=0xe80c011eu;
    const auto reject=[&](auto &&configure){
        auto table=initial;auto bank=banks(table);configure(bank);const auto before=table;bool failed=false;
        try{source_bitmap_bind_image(bank,free_handle);}catch(const std::invalid_argument&){failed=true;}
        if(!failed || !differences(before,table).empty())throw std::runtime_error("unproved image binding storage admitted or mutated");
        ++controls;
    };
    reject([](auto &bank){bank.pool.bytes=bank.pool.bytes.first(0x4b);});
    reject([](auto &bank){bank.pool.address=0;});
    reject([](auto &bank){bank.pool.address=0xffffffe0u;});
    reject([](auto &bank){put(bank.pool.bytes,0x20,511);});
    reject([](auto &bank){put(bank.pool.bytes,0x24,24);});
    reject([](auto &bank){bank.rows.bytes=bank.rows.bytes.first(512*40-1);});
    reject([](auto &bank){bank.rows.address=0;});
    reject([](auto &bank){put(bank.pool.bytes,0x44,0);});
    reject([](auto &bank){bank.bits.bytes=bank.bits.bytes.first(63);});
    reject([](auto &bank){bank.bits.address=0;});
    reject([](auto &bank){put(bank.pool.bytes,0x48,0);});
    reject([](auto &bank){bank.rows.address=bank.pool.address;put(bank.pool.bytes,0x44,bank.rows.address);});
    reject([](auto &bank){bank.bits.address=bank.pool.address+0x24;put(bank.pool.bytes,0x48,bank.bits.address);});
    reject([](auto &bank){bank.bits.address=bank.rows.address;put(bank.pool.bytes,0x48,bank.bits.address);});
    reject([](auto &bank){bank.bits.bytes=bank.rows.bytes.first(64);});
    // Invalid indices return before accessing row or bit storage. A nonzero
    // row salt returns before consuming the bit bank, irrespective of its bit.
    for(bool invalid_index:{true,false}){
        auto table=initial;auto bank=banks(table);
        if(invalid_index)bank.rows={};
        bank.bits={};const auto before=table;
        const auto result=source_bitmap_bind_image(bank,invalid_index?0xe80c0200u:0x00000000u);
        if(result!=0xffffffffu || !differences(before,table).empty())
            throw std::runtime_error("image binding unused storage changed early source return");
        ++controls;
    }
    return controls;
}
}
int main(int argc,char **argv){try{
    const bool probe=argc==2 && std::string_view(argv[1])=="--probe";
    std::ifstream file;
    if(!probe){if(argc!=2)throw std::invalid_argument("image binding fixture required");file.open(argv[1]);
        if(!file)throw std::invalid_argument("image binding fixture missing");}
    auto &input=probe?std::cin:file;Table initial;std::string line;unsigned cases=0;std::size_t checked_bytes=0;
    while(std::getline(input,line)){
        if(line.empty() || line[0]=='#')continue;
        std::istringstream header(line);std::string tag;header>>tag;
        if(tag=="B"){
            std::string name,hex;std::uint32_t address;header>>name>>std::hex>>address>>hex;
            if(!header || !initial.emplace(name,Region{address,decode(hex)}).second)
                throw std::invalid_argument("image binding fixture bank malformed/duplicate");
            continue;
        }
        if(tag!="I" || initial.size()!=3)throw std::invalid_argument("image binding fixture kind/banks missing");
        const auto divider=line.find('|');std::istringstream row(line.substr(0,divider));
        std::uint32_t handle;unsigned repeats;row>>tag>>std::hex>>handle>>std::dec>>repeats;
        if(!row || !repeats || repeats>2)throw std::invalid_argument("image binding fixture input malformed");
        auto actual=initial;read_patches(row,actual);std::string extra;
        if(row>>extra)throw std::invalid_argument("image binding fixture unexpected input");
        const auto before=actual;auto bank=banks(actual);std::uint32_t result=0;
        for(unsigned i=0;i<repeats;++i)result=source_bitmap_bind_image(bank,handle);
        if(probe){
            const auto patches=differences(before,actual);
            std::cout<<std::hex<<std::uppercase<<std::setw(8)<<std::setfill('0')<<result<<' '<<std::dec<<patches.size();
            for(const auto &change:patches)std::cout<<' '<<change.name<<' '<<std::hex<<change.offset<<' '<<encode(change.bytes);
            std::cout<<'\n';continue;
        }
        if(divider==std::string::npos)throw std::invalid_argument("image binding fixture separator missing");
        std::istringstream output(line.substr(divider+1));std::uint32_t expected_result;output>>std::hex>>expected_result;
        auto expected=before;read_patches(output,expected);
        if(!output || expected_result!=result || (output>>extra))
            throw std::runtime_error("original image binding result differs at case "+std::to_string(cases));
        for(const auto &[name,region]:expected){
            if(actual.at(name).bytes!=region.bytes)
                throw std::runtime_error("original image binding bank "+name+" differs at case "+std::to_string(cases));
            checked_bytes+=region.bytes.size();
        }
        ++cases;
    }
    if(!probe){
        if(cases!=787 || checked_bytes!=16227940)throw std::runtime_error("image binding frozen corpus incomplete");
        std::cout<<cases<<" source image binding cases; "<<checked_bytes<<" bank bytes exact; "<<negatives(initial)<<" admission/early-return controls\n";
    }
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
