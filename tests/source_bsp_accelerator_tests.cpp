#include "halo2/source_bsp_accelerator.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>

using namespace halo2;
namespace {
void require(bool ok,const std::string&message){if(!ok)throw std::runtime_error(message);}
std::vector<std::byte> bytes(const std::string&hex) {
    require(hex.size()%2==0,"odd source accelerator fixture bytes");std::vector<std::byte> out;
    for(std::size_t i=0;i<hex.size();i+=2)out.push_back(std::byte(std::stoul(hex.substr(i,2),nullptr,16)));
    return out;
}
std::vector<std::byte> tree(std::initializer_list<unsigned> code) {
    std::vector<std::byte> out(48);const auto scale=std::bit_cast<std::uint32_t>(65536.f);
    for(unsigned i=0;i<4;++i)out[12+i]=std::byte(scale>>(8*i));
    for(auto op:code)out.push_back(std::byte(op));return out;
}
void controls() {
    const std::vector<std::uint32_t> initial_direct{42},initial_ranges{7,9};
    const auto reject=[&](std::vector<std::byte> code,std::vector<ShaderVector> planes={},std::size_t direct_cap=1024,std::size_t range_cap=1024) {
        auto direct=initial_direct,ranges=initial_ranges;
        const auto result=source_bsp_accelerator_query0(code,planes,direct,ranges,direct_cap,range_cap);
        require(!result.complete&&!result.diagnostic.empty(),"unsupported stream/context admitted");
        require(direct==initial_direct&&ranges==initial_ranges,"incomplete traversal partially mutated caller banks");
    };
    for(unsigned op=0;op<256;++op) {
        const bool unsupported=op==8||(op>=0x0c&&op<=0x0f)||(op>=0x1d&&op<=0x1f)||
            (op>=0x2c&&op<=0x2f)||(op>=0x54&&op<=0x5f)||op>=0x6c;
        if(unsupported)reject(tree({op}));
    }
    reject(tree({0x10,0,255,1,0x31,8})); // hidden right branch rejected before current plane admission
    reject(tree({0x53,0}));reject(tree({5,255}));reject(std::vector<std::byte>(47));
    auto bad_scale=tree({0});for(unsigned i=12;i<16;++i)bad_scale[i]=std::byte{};reject(bad_scale);
    reject(tree({0x31}),{},1); // source count==capacity requires original allocation
    reject(tree({0x26,0,1,0x31}),{},1024,3); // first endpoint fits, second does not; both banks unchanged
    reject(tree({0}),std::vector<ShaderVector>(33));
    reject(tree({0}),{{std::bit_cast<float>(0x7fc00000u),0,0,0}});
    reject(tree({0}),{},0);reject(tree({0}),{},1024,1);
    reject(tree({0}),{},std::size_t(0x80000000u));reject(tree({0}),{},1024,std::size_t(0x80000000u));
    auto direct=initial_direct,ranges=initial_ranges;
    auto empty=source_bsp_accelerator_query0(tree({0}),{},direct,ranges);
    require(empty.complete&&empty.diagnostic.empty()&&direct==initial_direct&&ranges==initial_ranges,"complete empty lost");
    ranges.push_back(1);const auto odd=ranges;
    require(!source_bsp_accelerator_query0(tree({0}),{},direct,ranges).complete&&ranges==odd,"odd input range bank admitted");
    auto header=tree({0x31});for(unsigned i=0x20;i<0x24;++i)header[i]=std::byte{0xff};ranges.clear();direct.clear();
    require(source_bsp_accelerator_query0(header,{},direct,ranges).complete&&direct==std::vector<std::uint32_t>{1},"header+20 incorrectly treated as bytecode length");
}
}
int main(int argc,char**argv){try {
    const bool sphere=argc==3&&std::string(argv[2])=="--sphere";
    require(argc==2||sphere,"literal original BSP accelerator fixture required");std::ifstream file(argv[1]);require(bool(file),"source fixture missing");
    std::map<std::string,std::vector<std::byte>> banks;unsigned count=0,expected_count=0,expected_banks=0;std::string line;
    while(std::getline(file,line)) {
        if(line.empty()||line[0]=='#')continue;std::istringstream in(line);std::string kind,name;require(bool(in>>kind>>name),"malformed fixture row");
        if(kind=="C"){expected_count=std::stoul(name);require(bool(in>>expected_banks),"fixture coverage missing");continue;}
        if(kind=="B"){std::string hex;require(bool(in>>hex),"fixture bank missing");require(banks.emplace(name,bytes(hex)).second,"duplicate fixture bank");continue;}
        require(kind=="Q","unknown fixture row");std::string bank;std::size_t direct_cap,range_cap;
        require(bool(in>>bank>>direct_cap>>range_cap),"query metadata missing");
        const auto read_bank=[&] {
            std::size_t size;require(bool(in>>std::dec>>size)&&size<100000,"fixture bank count missing");std::vector<std::uint32_t> values(size);
            for(auto&value:values)require(bool(in>>std::hex>>value),"fixture word missing");return values;
        };
        const auto words=read_bank();require(words.size()%4==0,"partial fixture plane");std::vector<ShaderVector> planes(words.size()/4);
        for(std::size_t i=0;i<words.size();++i)planes[i/4][i%4]=std::bit_cast<float>(words[i]);
        auto direct=read_bank(),ranges=read_bank();const auto expected_direct=read_bank(),expected_ranges=read_bank();
        std::string tail;require(!(in>>tail),"fixture trailing fields");
        require(!sphere||planes.size()==1,"sphere fixture must contain center/radius");
        const auto result=sphere?source_bsp_accelerator_sphere(banks.at(bank),
            {planes[0][0],planes[0][1],planes[0][2]},planes[0][3],direct,ranges,direct_cap,range_cap):
            source_bsp_accelerator_query0(banks.at(bank),planes,direct,ranges,direct_cap,range_cap);
        require(result.complete,name+": source query incomplete: "+result.diagnostic);
        if(direct!=expected_direct||ranges!=expected_ranges) {
            std::cerr<<name<<" direct native:";for(auto key:direct)std::cerr<<' '<<std::hex<<key;
            std::cerr<<" source:";for(auto key:expected_direct)std::cerr<<' '<<std::hex<<key;
            std::cerr<<"\nranges native:";for(auto key:ranges)std::cerr<<' '<<std::hex<<key;
            std::cerr<<" source:";for(auto key:expected_ranges)std::cerr<<' '<<std::hex<<key;std::cerr<<'\n';
            throw std::runtime_error(name+": original ordered direct/range output differs");
        }
        ++count;
    }
    require(count==expected_count&&banks.size()==expected_banks&&banks.size()==456&&
        (sphere?count>=5000:count==1398),"original fixture coverage changed");controls();
    if(sphere) {
        for(auto code:{tree({8}),tree({0x31}),tree({0x26,0,1,0x31})}) {
            std::vector<std::uint32_t> direct{42},ranges{7,9};
            const auto result=source_bsp_accelerator_sphere(code,{},512,direct,ranges,1,3);
            require(!result.complete&&direct==std::vector<std::uint32_t>{42}&&
                ranges==std::vector<std::uint32_t>{7,9},"incomplete sphere query changed caller lists");
        }
    }
    std::cout<<count<<" unchanged original "<<(sphere?"sphere":"query0")<<" lists exact; "<<banks.size()<<" literal banks; bounded rejection controls passed\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
