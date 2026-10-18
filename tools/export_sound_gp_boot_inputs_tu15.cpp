#include "halo2/sound_gp_image_tu15.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
void write(const std::filesystem::path &p,const void *v,std::size_t n){std::ofstream f(p,std::ios::binary);if(!f.write(static_cast<const char*>(v),n))throw std::runtime_error("Cannot retain image input");}
void write_words(const std::filesystem::path &p,const std::vector<std::uint32_t> &words){
 std::vector<std::byte> bytes(words.size()*4);
 for(std::size_t i=0;i<bytes.size();++i)bytes[i]=std::byte(words[i/4]>>(8*(i%4)));
 write(p,bytes.data(),bytes.size());
}
int main(int argc,char **argv)try {
 if(argc!=3)throw std::runtime_error("Supply owned executable and fresh output directory");
 auto image=halo2::load_sound_gp_image_tu15(argv[1]);const std::filesystem::path out=argv[2];if(!std::filesystem::create_directory(out))throw std::runtime_error("Preserve previous image input");
 // Declared offline backing, not the SDK allocation/relocation implementation.
 // The original control code clears X/Y and loads program/data itself.
 std::vector<std::byte> scratch(image.user_scratch_base+image.user_scratch_bytes);
 std::copy(image.decoded_resource.begin(),image.decoded_resource.end(),scratch.begin());
 for(std::size_t i=0;i<image.control_program.size()*4;++i)scratch[i]=std::byte(image.control_program[i/4]>>(8*(i%4)));
 write(out/"initial-scratch.bin",scratch.data(),scratch.size());
 write_words(out/"expected-effects.bin",image.effect_program);
 std::vector<std::uint32_t> states;for(const auto &e:image.effects)states.push_back(e.state_word);
 write_words(out/"effect-states.bin",states);
 std::cout<<"PASS owned executable -> original decoded image; request word retained, no captured RAM or readiness result\n";
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
