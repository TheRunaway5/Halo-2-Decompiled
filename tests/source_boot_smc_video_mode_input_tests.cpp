#include "halo2/source_boot_smc_video_mode_input.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>

using namespace halo2;
namespace {
void check(bool ok,const char *why) { if(!ok)throw std::runtime_error(why); }
std::vector<std::byte> file(const std::filesystem::path &path) {
  std::ifstream input(path,std::ios::binary|std::ios::ate);
  check(bool(input),"Actual input file is unavailable");
  const auto size=input.tellg();check(size>=0,"Input size unavailable");
  std::vector<std::byte> bytes(static_cast<std::size_t>(size));input.seekg(0);
  input.read(reinterpret_cast<char*>(bytes.data()),size);check(bool(input),"Input file is truncated");
  return bytes;
}
std::uint32_t word(std::span<const std::byte> bytes,std::size_t offset=0) {
  check(offset<=bytes.size() && bytes.size()-offset>=4,"Source DWORD exceeds its owner");
  std::uint32_t value=0;for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(bytes[offset+i])<<(8*i);
  return value;
}
std::vector<std::byte> original(std::span<const std::byte> image,std::uint32_t address,unsigned size) {
  check(word(image)==0x48454258u,"Expected original XBE image");
  const auto base=word(image,0x104),count=word(image,0x11c),table=word(image,0x120);
  check(count<=128 && table>=base,"Invalid original section table");
  for(unsigned index=0;index<count;++index) {
    const auto header=std::size_t(table-base)+index*56;
    const auto va=word(image,header+4),raw=word(image,header+12),bytes=word(image,header+16);
    if(va<=address && std::uint64_t(address)+size<=std::uint64_t(va)+bytes) {
      const auto offset=std::size_t(raw)+address-va;
      check(offset<=image.size() && size<=image.size()-offset,"Truncated original section");
      return {image.begin()+offset,image.begin()+offset+size};
    }
  }
  throw std::runtime_error("Original code/import address is absent");
}
}
int main(int argc,char **argv)try {
  check(argc==5,"Supply TU15_XBE ACTUAL_SMC_INPUT CAPTURE_REPORT OUTPUT");
  for(unsigned i=1;i<=3;++i)if(!std::filesystem::is_regular_file(argv[i]))return 77;
  const auto image=file(argv[1]),record=file(argv[2]);
  std::ifstream source(argv[3]);nlohmann::json captured;source>>captured;
  check(captured.at("guest_code_patches").empty() && captured.at("guest_memory_writes").empty() &&
      captured.at("guest_register_writes").empty() && captured.at("supplied_service_results").empty(),
      "Actual capture changed source execution");
  const auto inputs=SourceBootSmcVideoModeInput::load(argv[2]);const auto &observed=captured.at("actual_external_inputs");
  check(inputs.inputs()==std::array<std::uint32_t,3>{observed.at("prior_word"),observed.at("status"),observed.at("response_word")},
      "Native input file differs from the actual transaction");
  check(inputs.published_value()==captured.at("observed_publication"),"Native publication differs from the actual kernel");
  auto code=original(image,0x33c29du,8),slot=original(image,0x41b784u,4);
  SourceRuntimeMemory memory;memory.mount(0x33c29du,code,SourceRuntimeMemory::Access::ReadOnly,"original getter");
  memory.mount(0x41b784u,slot,SourceRuntimeMemory::Access::ReadOnly,"original import");
  bool unbound=false;try{memory.view(0x80000164u,4);}catch(const std::exception&){unbound=true;}
  check(unbound,"An absent platform input acquired a default");
  source_mount_tu15_boot_smc_video_mode_input(memory,inputs);
  check(word(memory.view(0x80000164u,4))==captured.at("observed_publication") && memory.is_read_only(0x80000164u,4),
      "Native import lost its computed immutable owner");
  check(std::ranges::equal(memory.view(0x33c29du,8),code) && std::ranges::equal(memory.view(0x41b784u,4),slot),
      "Binding modified original code or its import");
  bool immutable=false,duplicate=false,owner=false;
  try{memory.write(0x80000164u,std::array{std::byte{0}});}catch(const std::exception&){immutable=true;}
  try{source_mount_tu15_boot_smc_video_mode_input(memory,inputs);}catch(const std::exception&){duplicate=true;}
  try{SourceBootSmcVideoModeInput(0,0,0,"");}catch(const std::exception&){owner=true;}
  check(immutable && duplicate && owner,"Boot input ownership was replaceable or anonymous");
  check(word(memory.view(0x80000164u,4))==captured.at("observed_publication"),"Rejected mutation changed the input");
  for(bool change_code:{false,true}) {
    auto altered_code=code,altered_slot=slot;
    (change_code?altered_code:altered_slot)[0]^=std::byte{1};
    SourceRuntimeMemory altered;altered.mount(0x33c29du,altered_code,SourceRuntimeMemory::Access::ReadOnly,"altered code control");
    altered.mount(0x41b784u,altered_slot,SourceRuntimeMemory::Access::ReadOnly,"altered import control");
    bool rejected=false;try{source_mount_tu15_boot_smc_video_mode_input(altered,inputs);}catch(const std::exception&){rejected=true;}
    check(rejected,"Foreign code/import acquired a boot input");
    bool untouched=false;try{altered.view(0x80000164u,4);}catch(const std::exception&){untouched=true;}
    check(untouched,"Rejected ownership partially published a value");
  }
  const std::filesystem::path out=argv[4];std::filesystem::create_directories(out);
  for(unsigned variant=0;variant<3;++variant) {
    auto malformed=record;if(variant==0)malformed.pop_back();else if(variant==1)malformed.push_back(std::byte{0});else malformed[0]^=std::byte{1};
    const auto path=out/("malformed-"+std::to_string(variant));
    {std::ofstream f(path,std::ios::binary);f.write(reinterpret_cast<const char*>(malformed.data()),malformed.size());}
    bool rejected=false;try{SourceBootSmcVideoModeInput::load(path);}catch(const std::exception&){rejected=true;}
    std::filesystem::remove(path);check(rejected,"Malformed input selected a mode");
  }
  nlohmann::json result{{"passed",true},{"actual_inputs",inputs.inputs()},{"native_publication",inputs.published_value()},
      {"actual_kernel_publication",captured.at("observed_publication")},{"original_getter_and_import_unchanged",true},
      {"immutable_publication",true},{"foreign_owner_and_malformed_inputs_rejected",true},
      {"publication_result_not_loaded_from_record",true},{"native_SMBus_transport_implemented",false},
      {"audio_device_initialized",false},{"physical_xbox_verified",false},{"full_weapon_parity",false}};
  std::ofstream(out/"report.json")<<result.dump(2)<<'\n';
  std::cout<<"PASS actual boot transaction publication, original import ownership, immutable data and rejected malformed inputs\n";return 0;
}catch(const std::exception &failure){std::cerr<<failure.what()<<'\n';return 1;}
