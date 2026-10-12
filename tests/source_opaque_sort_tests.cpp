#include "halo2/source_opaque_sort.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace halo2;
int main(int argc,char** argv)try {
    const char* path=argc>1?argv[1]:HALO2_SOURCE_DIR "/tests/fixtures/effect_light_opaque_sort.original.bin";
    std::ifstream f(path,std::ios::binary|std::ios::ate);if(!f)throw std::runtime_error("opaque fixture unavailable");const auto size=f.tellg();if(size<12 || size%4)throw std::runtime_error("opaque fixture length");std::vector<std::uint32_t> data(std::size_t(size)/4);f.seekg(0);f.read(reinterpret_cast<char*>(data.data()),size);if(!f)throw std::runtime_error("opaque fixture read failed");unsigned cursor=0;
    const auto word=[&](){if(cursor==data.size())throw std::runtime_error("opaque fixture truncated");return data[cursor++];};
    if(word()!=0x5453504f || word()!=1)throw std::runtime_error("opaque fixture signature/version");const auto count=word();std::uint64_t checked=0,comparisons=0;
    for(unsigned c=0;c<count;++c) {
        const auto n=word();std::vector<SourceOpaqueSortRecord> rows(n);for(auto& r:rows)for(auto& v:r.words)v=word();
        const auto matrix=word();if(matrix!=(n<=64?n*n:0))throw std::runtime_error("opaque fixture matrix length");
        for(unsigned i=0;i<n && matrix;++i)for(unsigned j=0;j<n;++j) {if(unsigned(source_opaque_compare(rows[i],rows[j]))!=word())throw std::runtime_error("opaque comparator mismatch case"+std::to_string(c));++comparisons;}
        source_opaque_sort(rows);
        for(const auto& r:rows)for(const auto v:r.words) {if(v!=word())throw std::runtime_error("opaque order/payload mismatch case"+std::to_string(c)+" word"+std::to_string(checked));++checked;}
    }
    if(cursor!=data.size())throw std::runtime_error("opaque fixture trailing words");std::cout<<"PASS "<<count<<" original opaque sorts / "<<checked<<" exact words / "<<comparisons<<" comparator calls\n";
} catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
