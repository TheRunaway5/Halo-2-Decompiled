#include "halo2/bsp_atmosphere.hpp"
#include "halo2/menu_fog_state.hpp"
#include <array>
#include <bit>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
struct Fixture {
    int tick{},cluster{};
    unsigned classification{};
    halo2::Vec3 eye{},forward{};
    std::array<std::uint32_t,33> expected{};
};
std::vector<Fixture> fixtures(const std::filesystem::path &path) {
    std::ifstream input(path);
    std::vector<Fixture> result;
    std::string line;
    while(std::getline(input,line)) {
        if(line.empty() || line.front()=='#')continue;
        std::istringstream row(line);
        Fixture value;
        row>>value.tick>>value.cluster>>value.classification;
        std::array<std::uint32_t,6> camera{};
        for(auto &word:camera)row>>std::hex>>word;
        for(auto &word:value.expected)row>>std::hex>>word;
        if(!row)throw halo2::FormatError("truncated original title fog state");
        std::string extra;
        if(row>>extra)throw halo2::FormatError("extra original title fog state field");
        value.eye={std::bit_cast<float>(camera[0]),std::bit_cast<float>(camera[1]),std::bit_cast<float>(camera[2])};
        value.forward={std::bit_cast<float>(camera[3]),std::bit_cast<float>(camera[4]),std::bit_cast<float>(camera[5])};
        result.push_back(value);
    }
    if(result.size()!=11)throw halo2::FormatError("original title fog fixture coverage changed");
    return result;
}
std::array<std::uint32_t,33> words(const halo2::BspFogSample &sample) {
    std::array<std::uint32_t,33> result{};
    unsigned cursor=0;
    const auto scalar=[&](float v){result.at(cursor++)=std::bit_cast<std::uint32_t>(v);};
    const auto color=[&](halo2::Vec3 v){scalar(v.x);scalar(v.y);scalar(v.z);};
    const auto atmospheric=[&](const halo2::AtmosphericFog &fog){
        color(fog.color);scalar(fog.maximum_density);scalar(fog.start);scalar(fog.opaque);
    };
    atmospheric(sample.primary);atmospheric(sample.secondary);
    if(sample.patchy) {
        color(sample.patchy->color);scalar(0);scalar(0);scalar(0);
        for(float v:sample.patchy->density)scalar(v);
        for(float v:sample.patchy->distance)scalar(v);
    }else for(unsigned i=0;i<10;++i)scalar(0);
    color(sample.planar.color);scalar(sample.planar.maximum_density);
    scalar(sample.planar.opaque_distance);scalar(sample.planar.opaque_depth);
    scalar(sample.eye_height);scalar(sample.eye_fraction);scalar(sample.eye_offset);
    scalar(sample.transition);scalar(sample.interpolation);
    return result;
}
}

int main(int argc,char **argv) {
    try {
        if(argc<3 || argc>4)throw halo2::FormatError("title fog state test requires original map, fixture and optional --raw-loader");
        if(!std::filesystem::exists(argv[1]) || !std::filesystem::exists(argv[2]))return 77;
        const bool raw=argc==4 && std::string(argv[3])=="--raw-loader";
        if(argc==4 && !raw)throw halo2::FormatError("unknown title fog state test mode");
        const auto cases=fixtures(argv[2]);
        halo2::Cache cache(argv[1]);
        const auto authored=cache.load_menu_atmosphere();
        if(!authored.patchy)throw halo2::FormatError("original title patchy definition missing");
        // This is the current production input seam. Only compare primary/
        // patchy authored values here; classification already has its own
        // camera-dependent kernel and is not represented by this raw loader.
        halo2::BspFogSample authored_sample;
        authored_sample.primary=authored.atmospheric;
        authored_sample.patchy=authored.patchy;
        const auto authored_words=words(authored_sample);
        unsigned raw_differences=0,raw_checked=0;
        for(const auto &f:cases)if(f.tick>0) {
            for(unsigned i=0;i<22;++i)if(i<6 || i>=12) {
                ++raw_checked;
                if(authored_words[i]!=f.expected[i]) {
                    ++raw_differences;
                    std::cout<<"Raw title input tick "<<f.tick<<", word "<<i<<": "
                        <<std::hex<<authored_words[i]<<" versus original "<<f.expected[i]<<std::dec<<'\n';
                }
            }
        }
        if(raw_differences!=4 || raw_checked!=96)
            throw halo2::FormatError("raw-versus-produced original title fog evidence changed");
        std::cout<<raw_differences<<" of "<<raw_checked<<" raw title input words differ from original aggregation\n";
        if(raw)return 1;
        // Production integration must query the original authored BSP. No
        // fixture values are supplied to the sampling implementation.
        const auto bank=cache.load_bsp_atmosphere(0);
        if(bank.skies.size()!=7 || bank.clusters.size()!=70)
            throw halo2::FormatError("mainmenu authored sky/cluster indexing changed");
        unsigned differences=0,checked=0;
        auto title_frame=authored;
        const auto *noise_pixels=title_frame.patchy->noise.front().pixels.data();
        for(const auto &f:cases) {
            const auto sample=bank.sample(f.eye,f.forward);
            if(sample.cluster!=f.cluster || sample.classification!=f.classification)
                throw halo2::FormatError("original title camera cluster or fog classification differs");
            const auto actual=words(sample);
            halo2::update_menu_fog_inputs(title_frame,sample);
            halo2::BspFogSample submitted;
            submitted.primary=title_frame.atmospheric;
            submitted.patchy=title_frame.patchy;
            const auto submitted_words=words(submitted);
            for(unsigned i=0;i<22;++i)if(i<6 || i>=12)
                if(submitted_words[i]!=f.expected[i])
                    throw halo2::FormatError("production title fog inputs differ from original aggregation");
            if(title_frame.patchy->noise.front().pixels.data()!=noise_pixels)
                throw halo2::FormatError("per-frame title fog update replaced the authored noise texture");
            for(unsigned i=0;i<actual.size();++i) {
                ++checked;
                if(actual[i]!=f.expected[i]) {
                    if(differences<10)std::cerr<<"Produced title input tick "<<f.tick<<", word "<<i<<": "
                        <<std::hex<<actual[i]<<" versus original "<<f.expected[i]<<std::dec<<'\n';
                    ++differences;
                }
            }
        }
        if(differences)throw halo2::FormatError(std::to_string(differences)+" original title fog bank words differ");
        std::cout<<cases.size()<<" original mainmenu camera/cluster cases and "<<checked<<" fog-bank words match byte for byte\n";
        return 0;
    }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
