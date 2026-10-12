#include "halo2/source_bitmap_manager_transition.hpp"
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
    if(hex.size()%2)throw std::invalid_argument("bitmap manager fixture odd hex byte length");
    const auto nibble=[](char c)->unsigned{
        if(c>='0' && c<='9')return unsigned(c-'0');
        if(c>='a' && c<='f')return unsigned(c-'a'+10);
        if(c>='A' && c<='F')return unsigned(c-'A'+10);
        throw std::invalid_argument("bitmap manager fixture nonhex byte");
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
        if(final.size()!=region.bytes.size())throw std::runtime_error("bitmap manager bank size changed");
        while(at<final.size()){
            if(final[at]==region.bytes[at]){++at;continue;}
            const auto start=at;
            while(at<final.size() && final[at]!=region.bytes[at])++at;
            patches.push_back({name,start,{final.begin()+std::ptrdiff_t(start),final.begin()+std::ptrdiff_t(at)}});
        }
    }
    return patches;
}
void apply_patch_bytes(Table &table,const Patch &patch){
    auto &bytes=table.at(patch.name).bytes;
    if(patch.offset>bytes.size() || patch.bytes.size()>bytes.size()-patch.offset)
        throw std::invalid_argument("bitmap manager frozen patch outside bank");
    std::copy(patch.bytes.begin(),patch.bytes.end(),bytes.begin()+std::ptrdiff_t(patch.offset));
}
unsigned negatives(const Table &initial,SourceBitmapManagerContext context){
    unsigned controls=0;
    const auto reject=[&](auto &&action){bool failed=false;try{action();}catch(const std::invalid_argument&){failed=true;}
        if(!failed)throw std::runtime_error("unproved bitmap manager input admitted");
        ++controls;};
    auto table=initial;std::vector<SourceBitmapManagerRegion> owners;auto bank=banks(table,owners);
    const auto get=[](std::span<const std::byte> bytes,unsigned at){std::uint32_t value=0;
        for(unsigned i=0;i<4;++i)value|=std::uint32_t(std::to_integer<unsigned>(bytes[at+i]))<<(i*8);
        return value;};
    const auto put=[](std::span<std::byte> bytes,unsigned at,std::uint32_t value){
        for(unsigned i=0;i<4;++i)bytes[at+i]=std::byte((value>>(i*8))&255u);};
    const auto salt=std::to_integer<unsigned>(bank.image_rows.bytes[0])|
        std::to_integer<unsigned>(bank.image_rows.bytes[1])<<8;
    const auto handle=salt<<16;
    reject([&]{source_bitmap_image_state(bank,65535);});
    reject([&]{source_bitmap_manager_delete(bank,handle^0x10000,context);});
    bank.image_pool.bytes[0x2a]|=std::byte{8};
    reject([&]{source_bitmap_image_delete(bank,handle,context);});
    bank.image_pool.bytes[0x2a]&=std::byte{0xf7};
    auto no_fence=context;no_fence.fence.reset();
    reject([&]{source_bitmap_image_busy(bank,handle,no_fence);});
    bank.image_rows.bytes[2]=std::byte{2};
    auto no_override=context;no_override.cache_busy_override.reset();
    reject([&]{source_bitmap_image_busy(bank,handle,no_override);});
    bank.image_rows.bytes[2]=std::byte{1};
    if(!source_bitmap_image_busy(bank,handle,{}))throw std::runtime_error("bitmap manager pinned shortcut consumed inactive inputs");
    ++controls;bank.image_rows.bytes[2]=std::byte{};
    put(bank.image_rows.bytes,0x14,0x50001);
    reject([&]{source_bitmap_image_busy(bank,handle,context);});
    put(bank.image_rows.bytes,0x14,0x40001);
    const auto before=table;
    if(source_bitmap_manager_delete(bank,handle,context)!=SourceBitmapManagerBoundary::NeedsWorkMutexAcquire ||
       !differences(before,table).empty())
        throw std::runtime_error("bitmap manager busy deletion fabricated completion writes");
    ++controls;
    const auto head=get(bank.manager.bytes,0x3c);
    put(bank.physical_rows.bytes,(head&65535u)*24+0xc,head);
    reject([&]{source_bitmap_manager_delete(bank,handle,context);});
    return controls;
}
}
int main(int argc,char **argv){try{
    const bool probe=argc==2 && std::string_view(argv[1])=="--probe";
    std::ifstream file;if(!probe){if(argc!=2)throw std::invalid_argument("bitmap manager fixture required");file.open(argv[1]);
        if(!file)throw std::invalid_argument("bitmap manager fixture missing");}
    auto &input=probe?std::cin:file;Table initial;SourceBitmapManagerContext context;
    std::string line;unsigned cases=0;std::size_t checked_bytes=0;
    while(std::getline(input,line)){
        if(line.empty() || line[0]=='#')continue;
        std::istringstream header(line);std::string tag;header>>tag;
        if(tag=="B"){
            std::string name,hex;std::uint32_t address;header>>name>>std::hex>>address>>hex;
            if(!header || !initial.emplace(name,Region{address,decode(hex)}).second)
                throw std::invalid_argument("bitmap manager fixture bank malformed/duplicate");
            continue;
        }
        if(tag=="G"){unsigned override;std::uint32_t queued,completed;header>>std::hex>>override>>queued>>completed;
            if(!header || override>255)throw std::invalid_argument("bitmap manager context malformed");
            context.cache_busy_override=std::uint8_t(override);context.fence=SourceBitmapManagerFence{queued,completed};continue;}
        if(tag!="K" || !context.fence)throw std::invalid_argument("bitmap manager fixture kind/context missing");
        const auto divider=line.find('|');std::istringstream row(line.substr(0,divider));
        std::string kind,replacement,override;std::uint32_t handle;row>>tag>>kind>>std::hex>>handle>>replacement>>override;
        if(!row)throw std::invalid_argument("bitmap manager fixture row malformed");
        auto actual=initial;auto case_context=context;
        if(replacement!="-")apply_patch_bytes(actual,{"image_rows",(handle&65535u)*40,decode(replacement)});
        if(override!="-")case_context.cache_busy_override=std::uint8_t(std::stoul(override,nullptr,16));
        const auto before=actual;std::vector<SourceBitmapManagerRegion> owners;auto bank=banks(actual,owners);
        auto boundary=SourceBitmapManagerBoundary::Complete;std::uint32_t value=0;
        if(kind=="state")value=source_bitmap_image_state(bank,handle);
        else if(kind=="busy")value=source_bitmap_image_busy(bank,handle,case_context);
        else if(kind=="delete")boundary=source_bitmap_image_delete(bank,handle,case_context);
        else if(kind=="manager_delete")boundary=source_bitmap_manager_delete(bank,handle,case_context);
        else throw std::invalid_argument("unknown bitmap manager fixture kind");
        if(probe){const auto patches=differences(before,actual);
            std::cout<<std::dec<<std::uint32_t(boundary)<<' '<<std::hex<<std::uppercase<<std::setw(2)<<std::setfill('0')<<value
                <<' '<<std::dec<<patches.size();
            for(const auto &patch:patches)std::cout<<' '<<patch.name<<' '<<std::hex<<patch.offset<<' '<<encode(patch.bytes);
            std::cout<<'\n';continue;}
        if(divider==std::string::npos)throw std::invalid_argument("bitmap manager fixture separator missing");
        std::istringstream output(line.substr(divider+1));unsigned expected_boundary,patch_count;std::uint32_t expected_value;
        output>>std::dec>>expected_boundary>>std::hex>>expected_value>>std::dec>>patch_count;
        auto expected=before;
        for(unsigned i=0;i<patch_count;++i){Patch patch;std::string hex;output>>patch.name>>std::hex>>patch.offset>>hex;
            if(!output)throw std::invalid_argument("bitmap manager output patch malformed");
            patch.bytes=decode(hex);apply_patch_bytes(expected,patch);}
        std::string extra;if(!output || output>>extra || expected_boundary!=std::uint32_t(boundary) || expected_value!=value)
            throw std::runtime_error("original bitmap manager result differs at case "+std::to_string(cases));
        for(const auto &[name,region]:expected){
            if(actual.at(name).bytes!=region.bytes)throw std::runtime_error("original bitmap manager bank "+name+" differs at case "+std::to_string(cases));
            checked_bytes+=region.bytes.size();
        }
        ++cases;
    }
    if(!probe){
        if(cases!=2024 || checked_bytes!=131681440)
            throw std::runtime_error("bitmap manager frozen corpus incomplete");
        std::cout<<cases<<" source bitmap manager cases; "<<checked_bytes<<" bank bytes exact; "<<negatives(initial,context)<<" admission/boundary controls\n";
    }
    return 0;
}catch(const std::exception &error){std::cerr<<error.what()<<'\n';return 1;}}
