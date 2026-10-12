#include "halo2/source_instance_accelerator.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

using namespace halo2;
namespace {
void require(bool ok,const std::string&message){if(!ok)throw std::runtime_error(message);}
std::vector<std::byte> bytes(const std::string&hex) {
    require(hex.size()%2==0,"odd serialized accelerator hex length");std::vector<std::byte> out;
    for(std::size_t i=0;i<hex.size();i+=2)out.push_back(std::byte(std::stoul(hex.substr(i,2),nullptr,16)));
    return out;
}
template<class F> void rejects(F f,const char*message){bool rejected=false;try{f();}catch(const FormatError&){rejected=true;}require(rejected,message);}
void controls() {
    SourceInstanceAccelerator b;b.offset_scale={0,0,0,65536};b.code={std::byte{0x50},std::byte{0x2a}};
    auto visit=source_instance_accelerator_visit(b,{},{1,0,0},1,[](auto key,float){require(key==42,"literal terminal key changed");return .25f;});
    require(visit.keys==std::vector<std::uint32_t>{42}&&visit.fraction==.25f,"terminal collector fraction changed");
    b.code={std::byte{0x08}};rejects([&]{source_instance_accelerator_inventory(b);},"unknown original opcode admitted");
    rejects([&]{source_instance_accelerator_visit(b,{},{},1,[](auto,float f){return f;});},"unknown visitor opcode admitted");
    for(unsigned op=0;op<256;++op) {
        const bool unsupported=op==8||(op>=0x0c&&op<=0x0f)||(op>=0x1d&&op<=0x1f)||
            (op>=0x2c&&op<=0x2f)||(op>=0x54&&op<=0x5f)||op>=0x6c;
        if(!unsupported)continue;
        b.code={std::byte(op)};bool precise=false;
        try{source_instance_accelerator_inventory(b);}catch(const FormatError&e){precise=std::string(e.what()).find("unsupported original")!=std::string::npos;}
        require(precise,"unknown opcode did not produce the source admission diagnostic");
    }
    b.code={std::byte{0x10},std::byte{0},std::byte{255},std::byte{1},std::byte{0x30},std::byte{8}};
    rejects([&]{source_instance_accelerator_inventory(b);},"unknown right branch was hidden by current-ray admission");
    b.code={std::byte{0x53},std::byte{0}};rejects([&]{source_instance_accelerator_inventory(b);},"truncated terminal admitted");
    b.code={std::byte{5},std::byte{255}};rejects([&]{source_instance_accelerator_inventory(b);},"jump outside authored bytecode admitted");
    b.code={std::byte{0}};require(source_instance_accelerator_inventory(b).size()==1,"source empty terminal lost");
    std::vector<std::byte> raw(49);raw[0x20]=std::byte{2};rejects([&]{decode_source_instance_accelerator(raw,1);},"code exceeding authored capacity admitted");
    raw[0x20]=std::byte{1};rejects([&]{decode_source_instance_accelerator(raw,1);},"nonpositive source scale admitted");
}
}
int main(int argc,char**argv){try {
    require(argc==2,"literal accelerator fixture required");std::ifstream file(argv[1]);require(bool(file),"accelerator fixture missing");
    std::map<std::string,SourceInstanceAccelerator> banks;std::set<unsigned> opcodes,inventory_opcodes;unsigned count=0;std::string line;
    unsigned expected_cases=29,expected_banks=2,expected_opcodes=13;
    while(std::getline(file,line)) {
        if(line.empty()||line[0]=='#')continue;std::istringstream in(line);std::string kind,name;in>>kind>>name;
        if(kind=="C") {expected_cases=std::stoul(name);in>>expected_banks>>expected_opcodes;continue;}
        if(kind=="B") {unsigned capacity;std::string hex;in>>capacity>>hex;const auto raw=bytes(hex);
            auto bank=decode_source_instance_accelerator(raw,capacity);for(auto&i:source_instance_accelerator_inventory(bank))inventory_opcodes.insert(i.opcode);
            banks.emplace(name,std::move(bank));continue;}
        require(kind=="Q","unknown accelerator fixture record");std::string bank;in>>bank>>std::hex;
        const auto real=[&]{std::uint32_t w;require(bool(in>>w),"query word missing");return std::bit_cast<float>(w);};
        const Vec3 origin{real(),real(),real()},ray{real(),real(),real()};const auto fraction=real();unsigned n;
        require(bool(in>>n),"query callback count missing");struct Callback{std::uint32_t key,before,after;};std::vector<Callback> callbacks(n);
        for(auto&r:callbacks)require(bool(in>>r.key>>r.before>>r.after),"query callback missing");
        require(bool(in>>n),"query instruction count missing");std::vector<SourceInstanceAcceleratorInstruction> instructions(n);
        for(auto&r:instructions)require(bool(in>>r.byte_offset>>r.opcode),"query instruction missing");
        std::uint32_t expected;require(bool(in>>expected),"query final fraction missing");std::string tail;require(!(in>>tail),"query trailing fields");
        unsigned callback=0;const auto visit=source_instance_accelerator_visit(banks.at(bank),origin,ray,fraction,[&](auto key,float before) {
            require(callback<callbacks.size(),name+": unexpected callback key "+std::to_string(key));const auto&e=callbacks[callback++];
            require(key==e.key,name+": callback key differs");require(std::bit_cast<std::uint32_t>(before)==e.before,name+": callback fraction differs");return std::bit_cast<float>(e.after);
        });
        require(callback==callbacks.size(),name+": missing source callback");
        if(visit.instructions!=instructions) {
            std::cerr<<name<<" native trace:";for(const auto&i:visit.instructions)std::cerr<<' '<<std::hex<<i.byte_offset<<':'<<i.opcode;
            std::cerr<<"\nsource trace:";for(const auto&i:instructions)std::cerr<<' '<<std::hex<<i.byte_offset<<':'<<i.opcode;std::cerr<<'\n';
            throw std::runtime_error(name+": original bytecode trace differs");
        }
        require(std::bit_cast<std::uint32_t>(visit.fraction)==expected,name+": final fraction differs");
        for(auto&i:visit.instructions)opcodes.insert(i.opcode);++count;
    }
    require(count==expected_cases&&banks.size()==expected_banks&&opcodes.size()==expected_opcodes,"literal accelerator proof coverage changed");controls();
    std::cout<<count<<" source callback/instruction traces exact; "<<banks.size()<<" authored banks; "<<opcodes.size()<<" exercised / "<<inventory_opcodes.size()<<" reachable opcodes\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
