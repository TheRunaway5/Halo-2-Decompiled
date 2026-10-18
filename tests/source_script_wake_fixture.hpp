#pragma once
#include "halo2/scripts.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace halo2;
namespace halo2::script_wake_fixture {
std::uint32_t u32(std::istream &f) {
    std::uint32_t result{};
    for(unsigned i=0;i<4;++i){const auto c=f.get();if(c<0)throw std::runtime_error("truncated fixture");result|=std::uint32_t(c)<<(8*i);}
    return result;
}
std::uint16_t u16(std::istream &f) {
    const auto lo=f.get(),hi=f.get();if(lo<0||hi<0)throw std::runtime_error("truncated fixture");
    return std::uint16_t(lo|(hi<<8));
}
ScriptProgram load(const std::filesystem::path &path) {
    std::ifstream f(path,std::ios::binary);char magic[4];f.read(magic,4);
    if(std::string(magic,4)!="HSW1")throw std::runtime_error("fixture magic");
    const auto nodes=u32(f),scripts=u32(f),strings=u32(f);ScriptProgram p;
    for(unsigned i=0;i<nodes;++i)p.nodes.push_back({u16(f),u16(f),u16(f),u16(f),u32(f),u32(f),u32(f)});
    for(unsigned i=0;i<scripts;++i){char name[32];f.read(name,32);p.scripts.push_back({std::string(name,std::find(name,name+32,'\0')),u16(f),u16(f),u32(f)});}
    p.strings.resize(strings);f.read(p.strings.data(),strings);if(!f)throw std::runtime_error("truncated strings");return p;
}
std::string read(const std::filesystem::path &path) {
    std::ifstream f(path);if(!f)throw std::runtime_error("missing expected events");return {std::istreambuf_iterator<char>(f),{}};
}
}
