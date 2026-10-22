#include "platform/startup_texture_fingerprints.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
#include <string_view>
int main()try {
  using namespace halo2::detail;
  const std::string_view hello="hello";
  const auto text=std::as_bytes(std::span(hello.data(),hello.size()));
  std::vector<std::byte> large(4u*1024u*1024u);
  for(std::size_t i=0;i<large.size();++i)large[i]=std::byte((i*73u+i/257u)&255u);
  auto different=large;different[512]^=std::byte{1};
  auto duplicate=large;
  const std::span<const std::byte> empty;
  const std::array<std::span<const std::byte>,6> inputs{empty,text,large,large,duplicate,different};
  const auto expected=texture_bytes_fnv1a(large);
  {
    StartupTextureFingerprints bank(inputs);
    if(bank.get(text)!=0xa430d84680aabd0bull || bank.get(empty)!=0xcbf29ce484222325ull ||
       bank.get(large)!=expected || bank.get(duplicate)!=expected || bank.get(different)!=texture_bytes_fnv1a(different) ||
       bank.get(std::span(large).subspan(3,270))!=
           texture_bytes_fnv1a(std::span(large).subspan(3,270)))throw std::runtime_error("Original byte identity changed");
  }
  {StartupTextureFingerprints bank(inputs);} // Join even when no result is requested.
  large.clear();
  std::cout<<"PASS complete original FNV1a bytes, known vectors, duplicate/unplanned inputs and abandoned bank join\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
