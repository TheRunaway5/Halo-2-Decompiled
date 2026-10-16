#include "halo2/native_tu15_sound_ep_module0_numeric.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <span>
#include <vector>
#include <stdexcept>
using namespace halo2::tu15_ep_module0_numeric;
using Json=nlohmann::json;
namespace {
void check(bool v,const char*why){if(!v)throw std::runtime_error(why);}
template<class Sections>void histories(std::vector<std::uint32_t>&v,const Sections&sections){for(const auto&section:sections)for(auto sample:section.samples)v.push_back(sample.bits);}
template<class Sections>void residues(std::vector<std::uint32_t>&v,const Sections&sections){for(const auto&section:sections){v.push_back(section.residual_high.bits);v.push_back(section.residual_low.bits);}}
std::vector<std::uint32_t> words(const Result&r,const State&s){std::vector<std::uint32_t>v;for(const auto&channel:r.output)for(auto sample:channel)v.push_back(sample.bits);for(auto sample:r.peak)v.push_back(sample.bits);for(auto sample:r.energy_root)v.push_back(sample.bits);histories(v,s.left);histories(v,s.right);histories(v,s.surround);residues(v,s.left);residues(v,s.right);residues(v,s.surround);return v;}
std::string hex(std::span<const std::byte>bytes){std::string result;for(auto byte:bytes){const auto v=std::to_integer<unsigned>(byte);result+="0123456789abcdef"[v>>4];result+="0123456789abcdef"[v&15];}return result;}
}
int main(int argc,char**argv)try{
    check(argc==4,"Expected actual source case descriptions, source observations and report");Json fixtures;std::ifstream(argv[1])>>fixtures;std::ifstream source(argv[2]);std::string line;std::size_t calls=0,compared=0;State state;
    for(const auto&c:fixtures.at("cases")){
        state=State{};
        for(unsigned phase=0;phase<fixtures.at("frame_count").get<unsigned>();phase++){
            check(bool(std::getline(source,line)),"Missing original module0 observation");const Json expected=Json::parse(line);
            check(expected.at("case")==calls/fixtures.at("frame_count").get<unsigned>()&&expected.at("half_phase")==(phase&1)*256,"Source case/phase order differs");
            Inputs inputs;for(unsigned channel=0;channel<5;channel++)for(unsigned sample=0;sample<256;sample++)inputs[channel][sample]=Word24(phase<2?c.at("rings")[channel][(phase&1)*256+sample].get<std::uint32_t>():0);
            Controls controls{Word24(expected.at("numeric_entry_SR").get<std::uint32_t>()),{}};controls.address_modes.fill(AddressingMode::linear);
            const Parameters parameters{Word24((phase==0||phase==fixtures.at("reset_frame").get<std::uint32_t>())?c.at("cold").get<std::uint32_t>():0),Word24(c.at("level").get<std::uint32_t>())};
            const auto result=process(controls,parameters,inputs,state);const auto actual=words(result,state);const auto wanted=expected.at("words").get<std::vector<std::uint32_t>>();
            if(actual!=wanted){for(std::size_t i=0;i<actual.size();i++)if(actual[i]!=wanted[i]){std::cerr<<"call "<<calls<<" word "<<i<<" actual "<<actual[i]<<" expected "<<wanted[i]<<'\n';break;}throw std::runtime_error("Module0 native arithmetic/history differs");}
            check(result.scale_coefficient.bits==expected.at("scale_coefficient"),"Gain table scalar differs from original MOVEM");
            const auto packed=pack_interleaved_s16(result);check(hex(packed)==expected.at("fifo_hex").get<std::string>(),"Original module0 FIFO packing differs");compared+=actual.size();++calls;
        }
    }
    check(!std::getline(source,line),"Trailing original module0 observation");Json report={{"passed",true},{"retained_native_calls",calls},{"exact_output_diagnostic_history_words",compared},{"history_refilled_between_calls",false},{"actual_packed_bytes",calls*1024},{"Ready",false}};std::ofstream(argv[3])<<report.dump(2)<<'\n';std::cout<<report.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
