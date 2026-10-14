#include "halo2/source_runtime_dsp_section.hpp"
#include "halo2/cache.hpp"
#include <algorithm>
#include <array>
#include <exception>
#include <fstream>
#include <openssl/evp.h>
namespace halo2 {namespace {
std::uint32_t word(std::span<const std::byte> b,std::size_t p){
 if(p>b.size()||b.size()-p<4)throw FormatError("Truncated owned DSP section metadata");
 std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[p+i])<<(8*i);return v;
}
std::string header_string(std::span<const std::byte> b,std::uint32_t base,std::uint32_t p){
 if(p<base||p-base>=b.size())throw FormatError("Owned DSP section name is outside its actual image");
 std::string out;for(auto i=std::size_t(p-base);i<b.size();++i){auto c=std::to_integer<unsigned>(b[i]);if(!c)return out;if(c>127)throw FormatError("Non-ASCII owned section-name domain is unavailable");out+=char(c);}
 throw FormatError("Unterminated owned DSP section name");
}
std::uint32_t source_word(const SourcePhysicsContactMemory &m,std::uint32_t p){std::array<std::byte,4>b;m.read(p,b);return word(b,0);}
}
struct SourceRuntimeDspSection::Impl {
 SourceRuntimeMemory &bank;
 struct Backing {std::uint32_t address{};std::vector<std::byte> bytes;};
 std::shared_ptr<const Backing> resident;
 std::vector<std::shared_ptr<const Backing>> retained;
 Impl(SourceRuntimeMemory &memory,const std::filesystem::path &path):bank(memory){
  std::ifstream f(path,std::ios::binary|std::ios::ate);if(!f||f.tellg()<0||f.tellg()>16*1024*1024)throw FormatError("Owned TU1.5 DSP image file is unavailable");
  std::vector<std::byte> file(std::size_t(f.tellg()));f.seekg(0);if(!f.read(reinterpret_cast<char*>(file.data()),file.size()))throw FormatError("Truncated owned TU1.5 DSP image");
  std::array<unsigned char,32> digest;unsigned size{};
  if(EVP_Digest(file.data(),file.size(),digest.data(),&size,EVP_sha256(),nullptr)!=1||size!=digest.size())throw FormatError("Cannot authenticate native DSP resource");
  constexpr char hex[]="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd";
  static_assert(sizeof(hex)==65);
  const auto digit=[](char c){return c<='9'?unsigned(c-'0'):unsigned(c-'a'+10);};
  for(unsigned i=0;i<digest.size();++i)if(digest[i]!=((digit(hex[i*2])<<4)|digit(hex[i*2+1])))throw FormatError("Native DSP ownership requires the authenticated TU1.5 executable");
  if(word(file,0)!=0x48454258u)throw FormatError("Invalid owned DSP image header");
  const auto base=word(file,0x104),count=word(file,0x11c),table=word(file,0x120);if(table<base||count>128)throw FormatError("Invalid owned DSP section table");
  for(unsigned i=0;i<count;++i){const auto p=std::uint64_t(table)-base+std::uint64_t(i)*56;
   if(header_string(file,base,word(file,p+20))!="DSPImage")continue;
   if(resident)throw FormatError("Ambiguous owned DSPImage resource");
   const auto flags=word(file,p),va=word(file,p+4),extent=word(file,p+8),raw=word(file,p+12),length=word(file,p+16);
   if(!(flags&2u)||(flags&1u)||!extent||length>extent||raw>file.size()||length>file.size()-raw)throw FormatError("Owned DSPImage does not have its authenticated preload/read-only layout");
   auto backing=std::make_shared<Backing>();backing->address=va;backing->bytes.resize(extent);std::copy_n(file.begin()+raw,length,backing->bytes.begin());resident=std::move(backing);
  }
  if(!resident)throw FormatError("Owned TU1.5 DSPImage is unavailable");validate();
 }
 void validate()const{
  if(!bank.is_read_only(resident->address,resident->bytes.size())||!std::ranges::equal(bank.view(resident->address,resident->bytes.size()),resident->bytes))throw FormatError("Native DSP resource lost its authenticated read-only source binding");
 }
};
SourceRuntimeDspSection::SourceRuntimeDspSection(SourceRuntimeMemory &m,const std::filesystem::path&p):impl_(std::make_unique<Impl>(m,p)){}
SourceRuntimeDspSection::~SourceRuntimeDspSection()=default;
std::uint32_t SourceRuntimeDspSection::retain(){impl_->validate();impl_->retained.push_back(impl_->resident);return impl_->resident->address;}
void SourceRuntimeDspSection::release(){impl_->validate();if(impl_->retained.empty())throw FormatError("Native DSP unload has no matching retained request");impl_->retained.pop_back();}
std::span<const std::byte> SourceRuntimeDspSection::retained_bytes()const{impl_->validate();if(impl_->retained.empty())throw FormatError("Native DSP data requires its real retained request");return impl_->retained.back()->bytes;}
std::uint32_t SourceRuntimeDspSection::address()const noexcept{return impl_->resident->address;}
std::size_t SourceRuntimeDspSection::active_references()const noexcept{return impl_->retained.size();}
std::size_t SourceRuntimeDspSection::resident_references()const noexcept{return std::size_t(impl_->resident.use_count());}
void SourceRuntimeDspSection::require_quiescent()const{impl_->validate();if(!impl_->retained.empty())throw FormatError("Outstanding native DSP section requests cannot cross a world checkpoint");}
void SourceRuntimeDspSection::discard_new_references(std::size_t n)noexcept{if(impl_->retained.size()>n)impl_->retained.resize(n);}
SourceRuntimeDspSection::CallScope::CallScope(SourceRuntimeDspSection *owner):owner_(owner),references_(owner?owner->active_references():0),exceptions_(std::uncaught_exceptions()){}
SourceRuntimeDspSection::CallScope::~CallScope(){if(owner_&&std::uncaught_exceptions()>exceptions_)owner_->discard_new_references(references_);}
bool SourceRuntimeDspSection::dispatch(SourcePhysicsContactMemory &memory,const SourcePhysicsContactRuntimeCall &call,SourcePhysicsWorldRuntimeAbi &abi){
 if(call.target!=0x2df851u&&call.target!=0x2df86cu)return false;
 if(&memory!=&impl_->bank||call.tail||source_word(memory,abi.gp[4])!=call.instruction+5u)throw FormatError("Native DSP section caller/bank ABI differs");
 const auto name=source_word(memory,abi.gp[4]+4);std::array<std::byte,9> raw;memory.read(name,raw);
 constexpr std::array expected{std::byte{'D'},std::byte{'S'},std::byte{'P'},std::byte{'I'},std::byte{'m'},std::byte{'a'},std::byte{'g'},std::byte{'e'},std::byte{0}};
 if(raw!=expected)throw FormatError("Native section service supports only its reached DSPImage request domain");
 if(call.target==0x2df851u)abi.gp[0]=retain();else{release();abi.gp[0]=1;}
 abi.gp[4]+=8;return true;
}
}
