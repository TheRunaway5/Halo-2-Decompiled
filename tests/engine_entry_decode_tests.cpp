#include "halo2/source_engine_entry_decode.hpp"
#include "halo2/source_engine_entity.hpp"
#include <nlohmann/json.hpp>
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace halo2;
using Json=nlohmann::json;
namespace {
void check(bool value,const char* text){if(!value)throw std::runtime_error(text);}
std::vector<std::uint8_t> bytes(const std::string& text) {
  check(text.size()%2==0,"Odd original byte string");std::vector<std::uint8_t> out(text.size()/2);
  for(unsigned i=0;i<out.size();++i)out[i]=std::uint8_t(std::stoul(text.substr(i*2,2),nullptr,16));
  return out;
}
std::string hex(std::span<const std::uint8_t> data) {
  constexpr char digits[]="0123456789abcdef";std::string out;
  for(auto value:data){out+=digits[value>>4];out+=digits[value&15];}return out;
}
template<class T>T decode(std::span<const std::uint8_t> data) {
  check(data.size()==sizeof(T),"Original object width differs");T value;std::memcpy(&value,data.data(),sizeof(value));return value;
}
template<class T>std::string object_hex(const T& value) {
  return hex(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(&value),sizeof(value)));
}
void append(std::vector<std::uint8_t>& out,std::uint32_t value,unsigned width) {
  for(unsigned i=0;i<width;++i)out.push_back(std::uint8_t(value>>(i*8)));
}
void unavailable_inputs() {
  SourceEngineEntryRetail entry;entry.prefix.entity=5;entry.prefix.definition=0;entry.state_size=40;
  SourceEngineEntryAllocator allocator;std::array<SourceEngineEntryDecoder,1> definitions;
  std::array<std::uint8_t,40> live{},private_bank{};std::array<std::uint8_t,8> data{};
  SourceEngineBitstreamState state;state.capacity_bytes=4;state.mode=3;SourceEngineBitstream stream(state,data);
  std::uint32_t mask=0xaabbccdd;std::int32_t produced=0;std::array<SourceEngineEntryBlock,1> blocks;
  auto rejected=[&](auto action){try{action();return false;}catch(const std::runtime_error&){return true;}};
  auto run=[&](std::uint32_t handle){return source_engine_entry_update_decode(entry,handle,definitions,live,allocator,mask,0,produced,blocks,stream);};
  check(run(6)==3 && mask==0xaabbccdd && produced==0 && allocator.outstanding==0,"Stale identity used unavailable providers");
  check(rejected([&]{run(5);}) && allocator.outstanding==0,"Missing allocation owner became success");
  allocator.allocate=[&](std::int32_t size){check(size==40,"Original allocation size differs");return SourceEngineEntryAllocation{1,private_bank};};
  live[0]=0x79;
  check(rejected([&]{run(5);}) && private_bank[0]==0x79 && allocator.outstanding==1,"Missing read owner did not preserve prior copy/counter");
  allocator.outstanding=0;state.mode=4;
  check(rejected([&]{run(5);}) && private_bank[0]==0 && allocator.outstanding==1,"Missing init owner did not preserve zeroing/counter");
}
}
int main(int argc,char** argv)try {
  check(argc==2,"Supply whole original entry decode fixture");std::ifstream input(argv[1]);Json fixture;input>>fixture;
  check(fixture.at("instruction_patches").empty() && fixture.at("supplied_returns").empty(),"Original decoder proof substitutes returns");
  unsigned calls=0,successes=0,overflow_successes=0,reads=0,initializations=0,releases=0,compactions=0;
  for(const auto& [version,data]:fixture.at("versions").items()) {
    for(unsigned n=0;n<fixture.at("inputs").size();++n)try {
      const auto& row=fixture.at("inputs")[n];const auto& expected=data.at("results")[n];
      auto live=bytes(row.at("live_hex")),wire=bytes(row.at("data_hex"));const auto original_live=live,original_wire=wire;
      auto state=decode<SourceEngineBitstreamState>(bytes(row.at("stream_hex")));SourceEngineBitstream stream(state,wire);
      std::vector<std::uint8_t> entry=bytes(row.at("prefix_hex"));
      if(version!="retail")append(entry,row.at("secondary_pending"),4);
      append(entry,row.at("creation_size"),4);append(entry,row.at("creation_address"),4);
      append(entry,std::uint32_t(row.value("state_size",40)),4);append(entry,fixture.at("live_address"),4);
      check(hex(std::span<const std::uint8_t>(entry))==expected.at("entry_hex").get<std::string>(),"Native constructed a different input entry");
      SourceEngineEntryAllocator allocator;allocator.outstanding=expected.at("allocator_count_before");
      // Original allocator outputs are explicit host-backing inputs at this
      // module boundary. This test ports the decoder, not the arena allocator.
      std::array<std::uint8_t,40> private_bank{};
      Json events=Json::array();const auto observe=[&](const char* kind) -> Json& {
        events.push_back(Json{{"kind",kind},{"count",allocator.outstanding},{"stream_hex",object_hex(state)}});return events.back();
      };
      const auto private_address=expected.at("private_address").get<std::uint32_t>();
      allocator.allocate=[&](std::int32_t size) {
        observe("allocate")["size"]=std::bit_cast<std::uint32_t>(size);
        return SourceEngineEntryAllocation{private_address,private_bank};
      };
      allocator.compact=[&]{observe("compact");++compactions;};
      allocator.inspect=[&](const auto& block) {
        auto& event=observe("info");event["private_address"]=block.address;event["bank_hex"]=hex(block.bytes);check(block.bytes.data()==private_bank.data(),"Inspected a different private bank");
      };
      allocator.release=[&](const auto& block) {
        auto& event=observe("release");event["private_address"]=block.address;event["bank_hex"]=hex(block.bytes);++releases;
      };
      const auto& identity=data.at("identities").at(row.at("selected").get<unsigned>());
      SourceEngineEntityCallbacks engine;engine.identifier=identity.at("identifier");
      engine.initialize=[&](std::uint32_t size,std::span<std::uint8_t> bank) {
        check(size==40 && bank.size()==40,"Original initializer received a different bank width");
        SourceKingReplicationBank value;source_king_replication_initialize(value);std::memcpy(bank.data(),&value,40);
      };
      const auto* selected=identity.at("present").get<bool>() ? &engine : nullptr;
      std::array<SourceEngineEntryDecoder,4> definitions;
      definitions[3].initialize=[&](std::int32_t creation_size,std::uint32_t creation_address,std::int32_t size,std::span<std::uint8_t> bank) {
        auto& event=observe("initialize");event["creation_size"]=std::bit_cast<std::uint32_t>(creation_size);event["creation_address"]=creation_address;
        event["state_size"]=std::uint32_t(size);event["private_address"]=private_address;event["bank_hex"]=hex(bank);++initializations;
        return source_engine_entity_initialize(selected,data.at("definition_identifier"),std::uint32_t(size),bank);
      };
      definitions[3].read=[&](bool initial,std::uint32_t& mask,std::int32_t size,std::span<std::uint8_t> bank,SourceEngineBitstream& received_stream) {
        check(!initial && &received_stream==&stream,"Original reader initial/stream input differs");
        auto& event=observe("read");event["state_size"]=std::uint32_t(size);event["initial_mask"]=mask;
        event["private_address"]=private_address;event["bank_hex"]=hex(bank);++reads;
        auto value=decode<SourceKingReplicationBank>(bank);const bool ok=source_king_wire_read(stream,value,mask);std::memcpy(bank.data(),&value,40);return ok;
      };
      std::array<SourceEngineEntryBlock,5> blocks;
      for(auto& block:blocks){block.size=block.type=std::bit_cast<std::int16_t>(std::uint16_t(0xa5a5));block.data_address=0xa5a5a5a5;}
      std::uint32_t mask=row.at("initial_mask");std::int32_t produced=row.at("initial_count");std::int32_t status;
      const auto invoke=[&](auto& value) {
        const auto result=source_engine_entry_update_decode(value,row.at("handle"),definitions,live,allocator,mask,
          std::bit_cast<std::int32_t>(row.at("capacity").get<std::uint32_t>()),produced,blocks,stream);
        check(object_hex(value)==expected.at("entry_hex").get<std::string>(),"Decoder changed entry metadata");return result;
      };
      if(version=="retail"){auto value=decode<SourceEngineEntryRetail>(entry);status=invoke(value);}
      else{auto value=decode<SourceEngineEntryTitleUpdate>(entry);status=invoke(value);}
      check(status==expected.at("status") && mask==expected.at("mask") && produced==expected.at("produced_count"),"Original status/mask/count differs");
      check(allocator.outstanding==expected.at("allocator_count_after"),"Original wrapper counter differs");
      check(object_hex(state)==expected.at("stream_hex").get<std::string>() && stream.overflowed()==expected.at("overflow"),"Original stream state/overflow differs");
      check(live==original_live && wire==original_wire,"Decoder changed live bank or packet bytes");
      check(events==expected.at("events"),"Original allocator/initializer/reader order or prestate differs");
      std::vector<std::uint8_t> output;
      for(const auto& block:blocks){append(output,std::uint16_t(block.size),2);append(output,std::uint16_t(block.type),2);append(output,block.data_address,4);}
      check(hex(std::span<const std::uint8_t>(output))==expected.at("blocks_hex").get<std::string>(),"Original typed output/untouched slots differ");
      if(!expected.at("bank_hex").is_null())check(hex(std::span<const std::uint8_t>(private_bank))==expected.at("bank_hex").get<std::string>(),"Original private bank differs");
      if(status==0){check(blocks[unsigned(produced-1)].data.data()==private_bank.data(),"Successful output does not own decoded bank");++successes;overflow_successes+=stream.overflowed();}
      ++calls;
    }catch(const std::exception& error){throw std::runtime_error(version+" input "+std::to_string(n)+": "+error.what());}
  }
  unavailable_inputs();
  std::cout<<calls<<" original entry decodes, "<<successes<<" successful, "<<overflow_successes<<" successful overflow, "
    <<reads<<" real readers, "<<initializations<<" initialization gates, "<<releases<<" releases, "<<compactions<<" compactions matched\n";
  return 0;
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
