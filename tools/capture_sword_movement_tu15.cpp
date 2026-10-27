// Test-only --wrap observer. The real native movement executes unchanged.
#include "halo2/source_sandbox_special_tu15.hpp"
#include "halo2/source_runtime_memory.hpp"
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

using namespace halo2;
namespace {
using Json=nlohmann::json;
constexpr auto symbol="_ZN5halo231source_run_sandbox_special_tu15ERNS_26SourcePhysicsContactMemoryEjRNS_30SourcePhysicsContactRuntimeAbiENS_33SourcePhysicsFactoryFloatingPointERKNS_35SourcePhysicsContactRuntimeServicesERKSt8functionIFvRKNS_31SourcePhysicsContactRecordStoreEEE";
std::string hex(std::span<const std::byte> bytes) {
  constexpr char digits[]="0123456789abcdef";std::string result;result.reserve(bytes.size()*2);
  for(auto b:bytes){const auto v=std::to_integer<unsigned>(b);result+=digits[v>>4];result+=digits[v&15];}
  return result;
}
Json abi_json(const SourcePhysicsWorldRuntimeAbi &abi) {
  Json values=Json::array();
  for(unsigned n=0;n<abi.x87_depth;++n) {
    std::array<std::byte,10> raw{};std::memcpy(raw.data(),&abi.x87[n],10);values.push_back(hex(raw));
  }
  Json result{{"gp",abi.gp},{"xmm",abi.xmm},{"eflags",abi.eflags},
      {"fs_base",abi.fs_base},{"x87",values},{"x87_depth",abi.x87_depth},
      {"x87_status",abi.x87_status},{"active_mmx",bool(abi.active_mmx)}};
  if(abi.floating_point)result["floating_point"]={{"x87_control_word",abi.floating_point->x87_control_word},
      {"mxcsr",abi.floating_point->mxcsr}};
  return result;
}
class ObservedMemory final:public SourcePhysicsContactMemory {
public:
  SourceRuntimeMemory &actual;
  mutable std::map<std::uint32_t,std::byte> initial;
  explicit ObservedMemory(SourceRuntimeMemory &memory):actual(memory){}
  void remember(std::uint32_t address,std::span<const std::byte> data)const {
    for(std::size_t n=0;n<data.size();++n)initial.try_emplace(address+std::uint32_t(n),data[n]);
    if(initial.size()>1024u*1024u)throw std::runtime_error("Sword movement capture exceeded its 1MiB touched-byte bound");
  }
  void read(std::uint32_t address,std::span<std::byte> out)const override {
    actual.read(address,out);remember(address,out);
  }
  void write(std::uint32_t address,std::span<const std::byte> input)override {
    std::vector<std::byte> before(input.size());actual.read(address,before);remember(address,before);
    actual.write(address,input);
  }
  Json banks(bool final)const {
    Json result=Json::array();
    for(auto current=initial.begin();current!=initial.end();) {
      const auto address=current->first;std::vector<std::byte> bytes;
      do{bytes.push_back(current->second);++current;}while(current!=initial.end() && current->first==address+bytes.size());
      if(final)actual.read(address,bytes);
      result.push_back({{"address",address},{"hex",hex(bytes)}});
    }
    return result;
  }
};
std::uint32_t word(const SourceRuntimeMemory &memory,std::uint32_t address) {
  std::array<std::byte,4> raw{};memory.read(address,raw);std::uint32_t value{};
  for(unsigned n=0;n<4;++n)value|=std::to_integer<std::uint32_t>(raw[n])<<(8*n);return value;
}
std::filesystem::path capture_path() {
  const char *path=std::getenv("HALO2_SWORD_MOVEMENT_CAPTURE");if(!path || !*path)return {};
  const char *scratch=std::getenv("HALO2_TEST_SCRATCH");
  if(!scratch || !*scratch)throw std::runtime_error("Sword movement capture requires tools/run_test.py owned scratch");
  const auto root=std::filesystem::weakly_canonical(scratch);
  const auto output=std::filesystem::weakly_canonical(path);
  const auto relative=output.lexically_relative(root);
  if(relative.empty() || relative.is_absolute() || *relative.begin()=="..")
    throw std::runtime_error("Sword movement raw capture must remain inside HALO2_TEST_SCRATCH");
  return output;
}
}

extern "C" void real_sword_movement(SourcePhysicsContactMemory &,std::uint32_t,
    SourcePhysicsWorldRuntimeAbi &,SourcePhysicsFactoryFloatingPoint,
    const SourcePhysicsWorldRuntimeServices &,const SourcePhysicsContactRecordObserver &)
    __asm__("__real__ZN5halo231source_run_sandbox_special_tu15ERNS_26SourcePhysicsContactMemoryEjRNS_30SourcePhysicsContactRuntimeAbiENS_33SourcePhysicsFactoryFloatingPointERKNS_35SourcePhysicsContactRuntimeServicesERKSt8functionIFvRKNS_31SourcePhysicsContactRecordStoreEEE");
extern "C" void wrap_sword_movement(SourcePhysicsContactMemory &memory,std::uint32_t entry,
    SourcePhysicsWorldRuntimeAbi &abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices &services,const SourcePhysicsContactRecordObserver &observe)
    __asm__("__wrap__ZN5halo231source_run_sandbox_special_tu15ERNS_26SourcePhysicsContactMemoryEjRNS_30SourcePhysicsContactRuntimeAbiENS_33SourcePhysicsFactoryFloatingPointERKNS_35SourcePhysicsContactRuntimeServicesERKSt8functionIFvRKNS_31SourcePhysicsContactRecordStoreEEE");
extern "C" void wrap_sword_movement(SourcePhysicsContactMemory &memory,std::uint32_t entry,
    SourcePhysicsWorldRuntimeAbi &abi,SourcePhysicsFactoryFloatingPoint fp,
    const SourcePhysicsWorldRuntimeServices &services,const SourcePhysicsContactRecordObserver &observe) {
  if(entry!=0x1f39d0u || !std::getenv("HALO2_SWORD_MOVEMENT_CAPTURE")) {
    real_sword_movement(memory,entry,abi,fp,services,observe);return;
  }
  auto *actual=dynamic_cast<SourceRuntimeMemory *>(&memory);
  if(!actual)throw std::runtime_error("Sword movement observer requires the actual retained runtime bank");
  const auto path=capture_path();if(path.empty()){real_sword_movement(memory,entry,abi,fp,services,observe);return;}
  const auto tick=word(*actual,word(*actual,0x51df54u)+8u);
  if(const char *minimum=std::getenv("HALO2_SWORD_MOVEMENT_CAPTURE_MIN_TICK")) {
    if(tick<std::stoul(minimum)){real_sword_movement(memory,entry,abi,fp,services,observe);return;}
  }
  static unsigned count{};static bool truncated{};unsigned limit=256;
  if(const char *value=std::getenv("HALO2_SWORD_MOVEMENT_CAPTURE_MAX"))limit=unsigned(std::stoul(value));
  if(limit==0 || limit>1024)throw std::runtime_error("Invalid sword movement capture call bound");
  if(count>=limit) {
    if(!truncated) {
      std::ofstream receipt(path,std::ios::app);
      receipt<<Json{{"type","truncated"},{"clock_tick",tick},{"calls_recorded",count}}.dump()<<'\n';
      receipt.flush();if(!receipt)throw std::runtime_error("Cannot write sword capture truncation receipt");
      truncated=true;
    }
    real_sword_movement(memory,entry,abi,fp,services,observe);return;
  }
  if(abi.active_mmx)throw std::runtime_error("Sword movement capture cannot serialize active MMX");
  const auto output_address=word(*actual,abi.gp[4]+8u);
  // Original1ED990/1ED994 passes physical component+10 as the first
  // argument. Its preceding word at component+4 is the genuine unit datum.
  const auto physical_component=word(*actual,abi.gp[4]+4u)-0x10u;
  ObservedMemory watched(*actual);Json row{{"entry",entry},{"index",count},
      {"clock_tick",tick},
      {"movement_state_address",abi.gp[0]},
      {"movement_parent",word(*actual,abi.gp[0]+0xcu)},
      {"movement_active_byte",word(*actual,abi.gp[0]+0xbcu)&255u},
      {"physical_component",physical_component},{"unit_datum",word(*actual,physical_component+4u)},
      {"physical_mode",word(*actual,physical_component)&255u},
      {"output_address",output_address},{"output_flags_before",word(*actual,output_address)},
      {"entry_abi",abi_json(abi)},{"fp",{{"x87_control_word",fp.x87_control_word},{"mxcsr",fp.mxcsr}}}};
  std::vector<std::uint32_t> instructions;Json stores=Json::array();
  auto observed=services;
  observed.instruction=[&](std::uint32_t pc){instructions.push_back(pc);if(services.instruction)services.instruction(pc);};
  // Replaced callback intentionally takes the complete diagnostic path.
  observed.instruction_cursor={};
  const SourcePhysicsContactRecordObserver writes=[&](const auto &store) {
    stores.push_back({{"instruction",store.instruction},{"address",store.address},
        {"before",hex(store.before)},{"written",hex(store.written)}});
    if(observe)observe(store);
  };
  real_sword_movement(watched,entry,abi,fp,observed,writes);
  row["output_flags_after"]=word(*actual,output_address);
  row["exit_abi"]=abi_json(abi);row["initial_banks"]=watched.banks(false);row["final_banks"]=watched.banks(true);
  row["instructions"]=instructions;row["stores"]=stores;
  std::filesystem::create_directories(path.parent_path());
  std::ofstream output(path,count?std::ios::app:std::ios::trunc);
  if(!output)throw std::runtime_error("Cannot open owned sword movement capture");
  if(!count)output<<Json{{"type","header"},{"version",1},{"entry",entry},{"wrapped_symbol",symbol},
      {"original_sha256","21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"},
      {"scope","Actual live native movement inputs and outputs; read-only test observer; no supplied owner results"}}.dump()<<'\n';
  output<<row.dump()<<'\n';output.flush();if(!output)throw std::runtime_error("Cannot write owned sword movement capture");
  ++count;
}
