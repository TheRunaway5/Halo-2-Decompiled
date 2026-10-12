#include "halo2/source_bitmap_reservation.hpp"
#include <algorithm>
#include <bit>
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
    if(hex.size()%2)throw std::invalid_argument("reservation fixture odd hex byte length");
    const auto nibble=[](char c)->unsigned{
        if(c>='0' && c<='9')return unsigned(c-'0');
        if(c>='a' && c<='f')return unsigned(c-'a'+10);
        if(c>='A' && c<='F')return unsigned(c-'A'+10);
        throw std::invalid_argument("reservation fixture nonhex byte");
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
SourceBitmapManagerBanks banks(Table &table,std::vector<SourceBitmapManagerRegion> &owners){
    const auto get=[&](const std::string &name){auto &region=table.at(name);return SourceBitmapManagerRegion{region.address,region.bytes};};
    for(auto &[name,region]:table)if(name.starts_with("owner_"))owners.push_back({region.address,region.bytes});
    return {get("manager"),get("image_pool"),get("physical_pool"),get("image_rows"),get("physical_rows"),
        get("image_bits"),get("physical_bits"),owners};
}
std::vector<Patch> differences(const Table &before,const Table &after){
    std::vector<Patch> patches;
    for(const auto &[name,region]:before){
        const auto &final=after.at(name).bytes;std::size_t at=0;
        if(final.size()!=region.bytes.size())throw std::runtime_error("reservation bank size changed");
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
        throw std::invalid_argument("reservation frozen patch outside bank");
    std::copy(change.bytes.begin(),change.bytes.end(),bytes.begin()+std::ptrdiff_t(change.offset));
}
void read_patches(std::istringstream &input,Table &table){
    unsigned count;input>>std::dec>>count;
    if(!input)throw std::invalid_argument("reservation fixture patch count missing");
    for(unsigned i=0;i<count;++i){Patch change;std::string hex;
        input>>change.name>>std::hex>>change.offset>>hex;
        if(!input)throw std::invalid_argument("reservation fixture patch malformed");
        change.bytes=decode(hex);patch(table,change);
    }
}
void put(std::span<std::byte> bytes,unsigned at,std::uint32_t value){
    for(unsigned i=0;i<4;++i)bytes[at+i]=std::byte((value>>(8*i))&255u);
}
unsigned negatives(const Table &initial,const SourceBitmapManagerContext &context){
    unsigned controls=0;
    const auto reject=[&](auto &&configure,const SourceBitmapReservationInput &request=SourceBitmapReservationInput{5504,2}){
        auto table=initial;std::vector<SourceBitmapManagerRegion> owners;auto bank=banks(table,owners);configure(bank);
        const auto before=table;bool failed=false;
        try{source_bitmap_reserve(bank,request,context);}catch(const std::invalid_argument&){failed=true;}
        if(!failed || !differences(before,table).empty())throw std::runtime_error("unproved reservation input admitted or mutated");
        ++controls;
    };
    const auto unchanged=[](auto &){};
    reject(unchanged,{0,2});reject(unchanged,{-1,2});reject(unchanged,{5504,1});reject(unchanged,{5504,8});
    reject([](auto &bank){put(bank.manager.bytes,0x2c,0);});
    reject([](auto &bank){put(bank.manager.bytes,0x2c,1);});
    reject([](auto &bank){put(bank.manager.bytes,0x34,11);});
    reject([](auto &bank){put(bank.manager.bytes,0x64,0);});
    reject([](auto &bank){put(bank.physical_pool.bytes,0x3c,0);});
    reject([](auto &bank){bank.image_bits.bytes[0]^=std::byte{1};});
    reject([](auto &bank){put(bank.physical_rows.bytes,0xc,0);});
    reject([](auto &bank){put(bank.physical_rows.bytes,4,0);});
    reject([](auto &bank){bank.physical_pool.bytes[0x40]=bank.physical_pool.bytes[0x41]=std::byte{0xff};});
    // A successful physical reservation has not run the caller's16B910 image
    // binding. A second paired-bank call must reject that incomplete phase.
    auto table=initial;std::vector<SourceBitmapManagerRegion> owners;auto bank=banks(table,owners);
    const auto result=source_bitmap_reserve(bank,{5504,2},context);
    if(result.boundary!=SourceBitmapManagerBoundary::Complete || result.handle==0xffffffffu)
        throw std::runtime_error("retained reservation did not create physical phase");
    const auto before=table;bool failed=false;
    try{source_bitmap_reserve(bank,{5504,2},context);}catch(const std::invalid_argument&){failed=true;}
    if(!failed || !differences(before,table).empty())throw std::runtime_error("unbound physical phase admitted or mutated");
    ++controls;return controls;
}
}
int main(int argc,char **argv){try{
    const bool probe=argc==2 && std::string_view(argv[1])=="--probe";
    std::ifstream file;
    if(!probe){if(argc!=2)throw std::invalid_argument("reservation fixture required");file.open(argv[1]);
        if(!file)throw std::invalid_argument("reservation fixture missing");}
    auto &input=probe?std::cin:file;Table initial;SourceBitmapManagerContext base_context;
    std::string line;unsigned cases=0;std::size_t checked_bytes=0;
    while(std::getline(input,line)){
        if(line.empty() || line[0]=='#')continue;
        std::istringstream header(line);std::string tag;header>>tag;
        if(tag=="B"){
            std::string name,hex;std::uint32_t address;header>>name>>std::hex>>address>>hex;
            if(!header || !initial.emplace(name,Region{address,decode(hex)}).second)
                throw std::invalid_argument("reservation fixture bank malformed/duplicate");
            continue;
        }
        if(tag=="G"){
            unsigned override;std::uint32_t queued,completed;header>>std::hex>>override>>queued>>completed;
            if(!header || override>255)throw std::invalid_argument("reservation context malformed");
            base_context.cache_busy_override=std::uint8_t(override);base_context.fence=SourceBitmapManagerFence{queued,completed};continue;
        }
        if(tag!="R" || !base_context.fence)throw std::invalid_argument("reservation fixture kind/context missing");
        const auto divider=line.find('|');std::istringstream row(line.substr(0,divider));
        std::uint32_t bytes,age,queued,completed;unsigned override;
        row>>tag>>std::hex>>bytes>>age>>override>>queued>>completed;
        if(!row || override>255)throw std::invalid_argument("reservation fixture input malformed");
        auto actual=initial;read_patches(row,actual);std::string extra;
        if(row>>extra)throw std::invalid_argument("reservation fixture unexpected input");
        const auto before=actual;std::vector<SourceBitmapManagerRegion> owners;auto bank=banks(actual,owners);
        const SourceBitmapManagerContext context{std::uint8_t(override),SourceBitmapManagerFence{queued,completed}};
        const auto result=source_bitmap_reserve(bank,{std::bit_cast<std::int32_t>(bytes),std::bit_cast<std::int32_t>(age)},context);
        if(probe){
            const auto patches=differences(before,actual);
            std::cout<<std::dec<<std::uint32_t(result.boundary)<<' '<<std::hex<<std::uppercase
                <<std::setw(8)<<std::setfill('0')<<result.handle<<' '<<std::setw(8)<<result.pending_delete<<' '<<std::dec<<patches.size();
            for(const auto &change:patches)std::cout<<' '<<change.name<<' '<<std::hex<<change.offset<<' '<<encode(change.bytes);
            std::cout<<'\n';continue;
        }
        if(divider==std::string::npos)throw std::invalid_argument("reservation fixture separator missing");
        std::istringstream output(line.substr(divider+1));unsigned expected_boundary;std::uint32_t expected_handle,expected_pending;
        output>>std::dec>>expected_boundary>>std::hex>>expected_handle>>expected_pending;
        auto expected=before;read_patches(output,expected);
        if(output>>extra || expected_boundary!=std::uint32_t(result.boundary) ||
           expected_handle!=result.handle || expected_pending!=result.pending_delete)
            throw std::runtime_error("original reservation result differs at case "+std::to_string(cases));
        for(const auto &[name,region]:expected){
            if(actual.at(name).bytes!=region.bytes)throw std::runtime_error("original reservation bank "+name+" differs at case "+std::to_string(cases));
            checked_bytes+=region.bytes.size();
        }
        ++cases;
    }
    if(!probe){
        if(cases!=392 || checked_bytes!=25503520)throw std::runtime_error("reservation frozen corpus incomplete");
        std::cout<<cases<<" source reservation cases; "<<checked_bytes<<" bank bytes exact; "<<negatives(initial,base_context)<<" admission/phase controls\n";
    }
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
