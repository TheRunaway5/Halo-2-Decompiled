#include "halo2/source_king_wire.hpp"
#include <nlohmann/json.hpp>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
std::vector<std::uint8_t> bytes(const std::string& hex) {
  check(hex.size()%2==0,"Odd original hex record");std::vector<std::uint8_t> out(hex.size()/2);
  for(unsigned i=0;i<out.size();++i)out[i]=std::uint8_t(std::stoul(hex.substr(i*2,2),nullptr,16));
  return out;
}
template<class T>T decode(const Json& hex) {
  const auto raw=bytes(hex.get<std::string>());check(raw.size()==sizeof(T),"Original record width differs");
  T value;std::memcpy(&value,raw.data(),sizeof(value));return value;
}
template<class T>void equal(const T& actual,const Json& hex,const char* message) {
  const auto raw=bytes(hex.get<std::string>());
  check(raw.size()==sizeof(T) && std::memcmp(&actual,raw.data(),sizeof(T))==0,message);
}
void equal(const std::vector<std::uint8_t>& actual,const Json& hex,const char* message) {
  check(actual==bytes(hex.get<std::string>()),message);
}
void result(const SourceEngineBitstream& stream,const SourceKingReplicationBank& bank,std::uint32_t flags,bool returned,const Json& expected) {
  equal(stream.state,expected.at("stream_hex"),"Original whole bitstream state differs");
  equal(std::vector<std::uint8_t>(stream.backing.begin(),stream.backing.end()),expected.at("data_hex"),"Original buffer/padding bytes differ");
  equal(bank,expected.at("bank_hex"),"Original whole update bank differs");
  check(flags==expected.at("flags"),"Original output mask differs");
  check(unsigned(returned)==expected.at("returned_al"),"Original codec success differs");
  check(stream.overflowed()==expected.at("overflow"),"Original stream overflow predicate differs");
}
}
int main(int argc,char** argv)try {
  check(argc==2,"Supply original King wire fixture");std::ifstream file(argv[1]);Json fixture;file>>fixture;
  check(fixture.at("instruction_patches").empty() && fixture.at("supplied_returns").empty(),"Wire evidence replaced original services");
  unsigned writes=0,reads=0,sizes=0,overflow_success=0,zero_mask_success=0,rollbacks=0;
  for(const auto& [version,rows]:fixture.at("versions").items()) {
    for(unsigned n=0;n<fixture.at("writers").size();++n)try {
      const auto& input=fixture.at("writers")[n];const auto& expected=rows.at("written")[n];
      auto bank=decode<SourceKingReplicationBank>(input.at("bank_hex"));
      auto state=decode<SourceEngineBitstreamState>(input.at("stream_hex"));auto storage=bytes(input.at("data_hex"));
      SourceEngineBitstream stream(state,storage);auto flags=input.at("initial_written").get<std::uint32_t>();
      const bool ok=source_king_wire_write(stream,input.at("requested"),flags,bank,input.at("reserve"));
      result(stream,bank,flags,ok,expected);++writes;rollbacks+=unsigned(state.discard_count);
      if(ok && !flags)++zero_mask_success;
      // Reproduce the independently declared original read ABI, including its
      // unchanged padding/error and initial bank. This read is not acceptance.
      auto read_state=decode<SourceEngineBitstreamState>(input.at("stream_hex"));read_state.mode=3;
      read_state.error_and_padding=(read_state.error_and_padding&0xffffff00u)|(n%2);
      std::array<std::uint8_t,40> initial_bank;initial_bank.fill(0xa5);
      SourceKingReplicationBank decoded;std::memcpy(&decoded,initial_bank.data(),initial_bank.size());
      SourceEngineBitstream read_stream(read_state,storage);flags=0xffffffff;
      const bool read_ok=source_king_wire_read(read_stream,decoded,flags);
      result(read_stream,decoded,flags,read_ok,rows.at("decoded")[n]);++reads;
      if(read_ok && read_stream.overflowed())++overflow_success;
    }catch(const std::exception&){std::cerr<<version<<" writer/roundtrip="<<n<<'\n';throw;}
    for(unsigned n=0;n<fixture.at("readers").size();++n)try {
      const auto& input=fixture.at("readers")[n];const auto& expected=rows.at("random_read")[n];
      auto bank=decode<SourceKingReplicationBank>(input.at("bank_hex"));
      auto state=decode<SourceEngineBitstreamState>(input.at("stream_hex"));auto storage=bytes(input.at("data_hex"));
      SourceEngineBitstream stream(state,storage);auto flags=input.at("initial_read").get<std::uint32_t>();
      const bool ok=source_king_wire_read(stream,bank,flags);result(stream,bank,flags,ok,expected);++reads;
      if(ok && stream.overflowed())++overflow_success;
    }catch(const std::exception&){std::cerr<<version<<" read="<<n<<'\n';throw;}
    for(unsigned mask=0;mask<128;++mask) {
      check(source_king_wire_minimum_bits(mask)==rows.at("sizes")[mask],"Original scheduling size hint differs");++sizes;
    }
  }
  SourceEngineBitstreamState unavailable;unavailable.capacity_bytes=1;
  std::array<std::uint8_t,1> missing_padding{};bool rejected=false;
  try{SourceEngineBitstream stream(unavailable,missing_padding);}catch(const std::runtime_error&){rejected=true;}
  check(rejected,"Native stream fabricated missing original guard/word padding");
  std::cout<<"Original King wire: "<<writes<<" writes, "<<reads<<" reads, "<<sizes<<" size hints, "<<rollbacks
    <<" rollbacks; "<<zero_mask_success<<" successful zero-mask writes and "<<overflow_success
    <<" successful overflowed reads remain distinct from packet acceptance\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
