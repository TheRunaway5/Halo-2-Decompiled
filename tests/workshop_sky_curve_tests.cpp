#include "halo2/cache.hpp"
#include "halo2/shader_animation.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace halo2;
int main(int argc,char** argv)try {
    if(argc!=3)throw FormatError("Supply literal TU1.5 sky curve outputs and authenticated function tables");
    std::ifstream samples(argv[1]),file(argv[2],std::ios::binary);
    if(!samples || !file)throw FormatError("Missing required sky curve reference inputs");
    const std::vector<unsigned char> raw{std::istreambuf_iterator<char>(file),{}};
    std::uint64_t fingerprint=14695981039346656037ull;
    for(auto value:raw)fingerprint=(fingerprint^value)*1099511628211ull;
    if(raw.size()!=28*1024 || fingerprint!=0x22708b4cc478ddb7ull)
        throw FormatError("Sky curve lookup tables differ from authenticated original samples");
    ShaderFunctionTables tables;
    for(unsigned row=0;row<28;++row)for(unsigned i=0;i<1024;++i)
        (row<12?tables.periodic[row][i]:tables.transition[row-12][i])=raw[row*1024+i];
    unsigned cases{};std::string line;
    while(std::getline(samples,line)) {
        if(line.empty() || line[0]=='#')continue;
        std::istringstream row(line);std::string hex;std::uint32_t input,range,expected;
        if(!(row>>hex>>std::hex>>input>>range>>expected) || hex.size()%2 || hex.size()>8192)
            throw FormatError("Malformed original sky curve case");
        std::vector<std::byte> curve;
        for(unsigned n=0;n<hex.size();n+=2)curve.push_back(std::byte(std::stoul(hex.substr(n,2),nullptr,16)));
        const auto actual=std::bit_cast<std::uint32_t>(shader_function_fraction(curve,
            std::bit_cast<float>(input),std::bit_cast<float>(range),tables));
        if(actual!=expected) {
            std::cerr<<"case="<<cases<<" input="<<std::hex<<input<<" range="<<range
                <<" expected="<<expected<<" actual="<<actual<<'\n';
            throw FormatError("Native sky curve differs from unchanged TU1.5 execution");
        }
        ++cases;
    }
    if(cases!=432)throw FormatError("Incomplete original sky curve cohort");
    std::cout<<"PASS "<<cases<<" unchanged TU1.5 authored sky curve outputs, exact float bits\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
