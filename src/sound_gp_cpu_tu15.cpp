#include "halo2/sound_gp_cpu_tu15.hpp"
#include "sound_gp_cpu_tu15_ffi.h"
#include <algorithm>
#include <cstddef>
#include <openssl/evp.h>
#include <exception>
#include <stdexcept>
#include <utility>

namespace halo2 {
struct SoundGpCpuTu15::Impl {
  std::array<std::uint32_t,0xc00> x{};
  std::array<std::uint32_t,0x400> mix{};
  std::array<std::uint32_t,0x800> y{};
  std::array<std::uint32_t,0x1000> program{};
  SoundGpCpuInputTu15 input;
  void *core{};
  std::exception_ptr failure;
  bool executing{},booted{},program_dirty{},block_running{},stop_requested{};
  void stop(){stop_requested=true;if(block_running)halo2_gp_core_request_stop(core);}
  std::uint32_t current_pc{};
  std::array<std::uint32_t,64> current_registers{};
  void sync_state(){halo2_gp_core_state(core,&current_pc,current_registers.data());}

  static std::uint32_t peripheral_read(void *opaque,std::uint32_t address) noexcept {
    auto &self=*static_cast<Impl*>(opaque);
    // Never unwind through Rust/JIT code. A failed callback permanently
    // faults this owner; its partially executed instruction is never
    // reported as a completed frame or ready device.
    try{self.valid();const auto value=self.input.read_peripheral(address);
      if(value>0xffffff)throw std::runtime_error("GP peripheral read exceeds 24 bits");
      return value;
    }catch(...){if(!self.failure)self.failure=std::current_exception();self.stop();return 0;}
  }
  static void peripheral_write(void *opaque,std::uint32_t address,std::uint32_t value) noexcept {
    auto &self=*static_cast<Impl*>(opaque);
    try{self.valid();self.input.write_peripheral(address,value);}
    catch(...){if(!self.failure)self.failure=std::current_exception();self.stop();}
  }
  static std::uint32_t unmapped_read(void *opaque,std::uint32_t) noexcept {
    auto &self=*static_cast<Impl*>(opaque);
    try{throw std::out_of_range("GP instruction reads unmapped memory");}
    catch(...){if(!self.failure)self.failure=std::current_exception();self.stop();}return 0;
  }
  static void unmapped_write(void *opaque,std::uint32_t address,std::uint32_t) noexcept {unmapped_read(opaque,address);}
  void valid() const {if(failure)std::rethrow_exception(failure);}
  void supported_modes(const std::array<std::uint32_t,64> &registers) const {
    // The pinned core explicitly leaves these modes unimplemented. Reject
    // rather than silently executing them as ordinary GP arithmetic.
    if((registers[57]&((1u<<17)|(1u<<13)|(1u<<14)))||
       (registers[58]&((1u<<20)|(1u<<7))))
      throw std::runtime_error("GP execution reached an unsupported DSP mode");
  }
  explicit Impl(SoundGpCpuInputTu15 value):input(std::move(value)) {
    if(!input.read_peripheral||!input.write_peripheral)throw std::invalid_argument("GP execution requires both peripheral owners");
    if(!std::ranges::all_of(input.registers,[](auto v){return v<=0xffffff;}))throw std::invalid_argument("GP initial register exceeds 24 bits");
    supported_modes(input.registers);
    core=halo2_gp_core_create(x.data(),mix.data(),y.data(),program.data(),
        {this,peripheral_read,peripheral_write,unmapped_read,unmapped_write});
    if(!core)throw std::runtime_error("Cannot construct native GP instruction core");
    halo2_gp_core_initial_registers(core,input.registers.data());sync_state();
  }
  ~Impl(){if(core)halo2_gp_core_destroy(core);}
  std::uint32_t &word(SoundGpSpaceTu15 space,std::uint32_t address) {
    switch(space) {
    case SoundGpSpaceTu15::x:
      if(address<x.size())return x[address];
      if(address>=0xc00&&address<0x1000)return mix[address-0xc00];
      if(address>=0x1400&&address<0x1800)return mix[address-0x1400];
      break;
    case SoundGpSpaceTu15::y:if(address<y.size())return y[address];break;
    case SoundGpSpaceTu15::program:if(address<program.size())return program[address];break;
    }
    throw std::out_of_range("GP RAM access is outside its actual mapped extent");
  }
};
SoundGpCpuTu15::SoundGpCpuTu15(SoundGpCpuInputTu15 input):impl_(std::make_unique<Impl>(std::move(input))){}
SoundGpCpuTu15::~SoundGpCpuTu15()=default;
void SoundGpCpuTu15::bootstrap(std::span<const std::uint32_t> words) {
  auto &self=*impl_;self.valid();
  if(self.executing||self.booted)throw std::logic_error("GP bootstrap cannot replace a running owner");
  if(words.size()!=0x800||!std::ranges::all_of(words,[](auto v){return v<=0xffffff;}))throw std::invalid_argument("GP bootstrap needs exactly 0x800 24-bit words");
  std::copy(words.begin(),words.end(),self.program.begin());
  halo2_gp_core_invalidate(self.core);self.booted=true;
}
void SoundGpCpuTu15::step() {
  auto &self=*impl_;self.valid();
  if(self.executing||!self.booted)throw std::logic_error("GP instruction execution has no completed bootstrap or re-enters its owner");
  self.supported_modes(self.current_registers);
  if(self.current_pc>=self.program.size())throw std::out_of_range("GP instruction PC exceeds physical PRAM");
  self.executing=true;halo2_gp_core_step(self.core);self.executing=false;self.valid();
  if(self.program_dirty){halo2_gp_core_invalidate(self.core);self.program_dirty=false;}
  self.sync_state();self.supported_modes(self.current_registers);
}
SoundGpCpuTu15::RunResult SoundGpCpuTu15::run_original_gp(std::uint32_t maximum) {
  auto &self=*impl_;self.valid();
  if(self.executing||!self.booted||!maximum||maximum>0x7fffffff)throw std::logic_error("Invalid original GP execution slice");
  self.supported_modes(self.current_registers);
  if(self.current_pc>=self.program.size())throw std::out_of_range("GP execution PC exceeds PRAM");
  constexpr char pin[]="a5b3859a22b68c6a90b14419b442471d51fb8475836371ea804cd50dad96241f";
  std::array<std::byte,15684> bytes{};
  for(std::size_t i=0;i<bytes.size();++i)bytes[i]=std::byte(self.program[i/4]>>(8*(i%4)));
  std::array<unsigned char,32> digest{};unsigned length{};
  if(EVP_Digest(bytes.data(),bytes.size(),digest.data(),&length,EVP_sha256(),nullptr)!=1||length!=digest.size())throw std::runtime_error("Cannot authenticate running GP program");
  const auto digit=[](char c){return c<='9'?unsigned(c-'0'):unsigned(c-'a'+10);};
  for(unsigned i=0;i<digest.size();++i)if(digest[i]!=((digit(pin[i*2])<<4)|digit(pin[i*2+1])))throw std::runtime_error("Block execution requires the exact original TU1.5 GP program");
  self.stop_requested=false;self.executing=self.block_running=true;
  const auto cycles=halo2_gp_core_run(self.core,maximum);
  self.executing=self.block_running=false;self.valid();
  if(self.program_dirty){halo2_gp_core_invalidate(self.core);self.program_dirty=false;}
  self.sync_state();self.supported_modes(self.current_registers);
  return {cycles,self.stop_requested};
}
void SoundGpCpuTu15::request_stop(){
  auto &self=*impl_;self.valid();if(!self.executing)throw std::logic_error("GP stop must come from its executing owner");self.stop();
}
std::uint32_t SoundGpCpuTu15::cycles() const {
  impl_->valid();if(impl_->executing)throw std::logic_error("GP callback may not inspect executing core state");return halo2_gp_core_cycles(impl_->core);
}
std::uint32_t SoundGpCpuTu15::pc() const {
  impl_->valid();if(impl_->executing)throw std::logic_error("GP callback may access RAM, not executing core state");
  return impl_->current_pc;
}
std::array<std::uint32_t,64> SoundGpCpuTu15::registers() const {
  impl_->valid();if(impl_->executing)throw std::logic_error("GP callback may access RAM, not executing core state");
  return impl_->current_registers;
}
SoundGpExecutionStateTu15 SoundGpCpuTu15::execution_state() const {
  impl_->valid();if(impl_->executing)throw std::logic_error("GP callback may not inspect executing instruction state");
  SoundGpExecutionStateTu15 state;std::array<std::uint32_t,32> stack{};std::uint32_t repeating{},pc_on_repeat{};
  halo2_gp_core_execution_state(impl_->core,&state.pc,&state.cycles,state.registers.data(),stack.data(),&repeating,&pc_on_repeat);
  for(unsigned half=0;half<2;++half)std::copy_n(stack.begin()+half*16,16,state.stack[half].begin());
  state.repeating=repeating;state.pc_on_repeat=pc_on_repeat;return state;
}
std::uint32_t SoundGpCpuTu15::read(SoundGpSpaceTu15 space,std::uint32_t address) const {impl_->valid();return impl_->word(space,address);}
void SoundGpCpuTu15::write(SoundGpSpaceTu15 space,std::uint32_t address,std::uint32_t value) {
  auto &self=*impl_;self.valid();if(value>0xffffff)throw std::invalid_argument("GP RAM write exceeds 24 bits");
  auto &word=self.word(space,address);if(word==value)return;word=value;
  if(space==SoundGpSpaceTu15::program){if(self.executing){self.program_dirty=true;self.stop();}else halo2_gp_core_invalidate(self.core);}
}
} // namespace halo2
