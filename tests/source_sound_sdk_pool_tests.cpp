#include "halo2/source_sound_sdk_pool.hpp"
#include "halo2/source_runtime_sound_sdk_resources.hpp"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace halo2;using Json=nlohmann::json;
namespace {
void check(bool v,const char*m){if(!v)throw std::runtime_error(m);}
Json load(const std::filesystem::path&p){std::ifstream f(p);check(bool(f),"Missing original DSP reference input");return Json::parse(f);}
std::vector<std::byte> bytes(const std::string&hex){check(hex.size()%2==0,"Malformed DSP reference bytes");std::vector<std::byte> out(hex.size()/2);const auto digit=[](char c){return unsigned(c<='9'?c-'0':c-'a'+10);};for(std::size_t i=0;i<out.size();++i)out[i]=std::byte((digit(hex[i*2])<<4)|digit(hex[i*2+1]));return out;}
std::string hex(std::span<const std::byte>b){constexpr char d[]="0123456789abcdef";std::string out;out.reserve(b.size()*2);for(auto v:b){auto n=std::to_integer<unsigned>(v);out+=d[n>>4];out+=d[n&15];}return out;}
SourcePhysicsWorldRuntimeAbi import(const Json&j){SourcePhysicsWorldRuntimeAbi a{};a.fs_base=j.at("fs_base");a.gp=j.at("gp").get<decltype(a.gp)>();a.xmm=j.at("xmm").get<decltype(a.xmm)>();a.eflags=j.at("eflags");a.x87_status=j.at("x87_status");a.x87_depth=j.at("x87_depth");a.floating_point=SourcePhysicsFactoryFloatingPoint{j.at("x87_control"),j.at("mxcsr")};check(a.x87_depth<=8,"Sparse original x87 input unsupported");for(unsigned i=0;i<a.x87_depth;++i){auto raw=bytes(j.at("x87_logical_hex")[i]);check(raw.size()==10,"Original x87 extent differs");std::memcpy(&a.x87[i],raw.data(),10);}return a;}
std::vector<std::uint32_t> instructions;
void save(const std::filesystem::path&p,std::uint32_t pc,const SourcePhysicsWorldRuntimeAbi&a,const SourceRuntimeMemory&m){
 Json raw=Json::array(),regions=Json::array();for(unsigned i=0;i<8;++i)raw.push_back(hex({reinterpret_cast<const std::byte*>(&a.x87[i]),10}));
 m.visit_committed([&](auto address,auto b){regions.push_back({{"address",address},{"size",b.size()},{"writable",!m.is_read_only(address,b.size())},{"hex",hex(b)}});});
 Json j{{"instructions",instructions},{"pc",pc},{"abi",{{"gp",a.gp},{"fs_base",a.fs_base},{"xmm",a.xmm},{"eflags",a.eflags},{"x87_depth",a.x87_depth},{"x87_status",a.x87_status},{"x87_control",a.floating_point->x87_control_word},{"mxcsr",a.floating_point->mxcsr},{"x87_tag",a.x87_depth==8?0:65535},{"x87_logical_hex",raw}}},{"regions",regions}};
 std::ofstream f(p);f<<j.dump()<<'\n';check(bool(f),"Cannot retain native DSP prefix state");
}
struct EndPoolTest {};
std::uint32_t word(const SourceRuntimeMemory&m,std::uint32_t p){auto b=m.view(p,4);std::uint32_t v{};for(unsigned i=0;i<4;++i)v|=std::to_integer<std::uint32_t>(b[i])<<(8*i);return v;}
void put(SourceRuntimeMemory&m,std::uint32_t p,std::uint32_t v){std::array<std::byte,4>b;for(unsigned i=0;i<4;++i)b[i]=std::byte(v>>(i*8));m.write(p,b);}
}
int main(int argc,char**argv)try{
 check(argc==4,"Supply ORIGINAL_SDK_CAPTURE ORIGINAL_POOL_RULE_CAPTURE OUTPUT_DIRECTORY");const std::filesystem::path sdk(argv[1]),original(argv[2]),out(argv[3]);if(!std::filesystem::is_regular_file(original/"report.json")||!std::filesystem::is_regular_file(sdk/"call-02.json"))return 77;
 const auto reference=load(original/"report.json"),allocation=load(sdk/"call-02.json").at("before"),query=load(sdk/"call-03.json").at("before");
 check(reference.at("original_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"&&reference.at("source_factory_returned_normally").get<bool>(),"Wrong pool reference version");
 for(const auto*key:{"original_code_patches","original_memory_writes","original_register_writes","supplied_callee_returns"})check(reference.at(key).empty(),"Pool reference was supplied a result");
 SourceRuntimeMemory bank;for(const auto&r:allocation.at("regions"))bank.mount(r.at("address"),bytes(r.at("hex")),r.at("writable").get<bool>()?SourceRuntimeMemory::Access::ReadWrite:SourceRuntimeMemory::Access::ReadOnly,"actual original raw-pool call input");
 auto allocation_abi=import(allocation.at("abi")),query_abi=import(query.at("abi"));SourceRuntimeSoundSdkResources resources(bank,allocation_abi.fs_base);resources.require_quiescent();
 std::vector<std::uint32_t> sizes;for(std::uint32_t n=0;n<=4056;++n)sizes.push_back(n);for(auto n:{4057u,4095u,4096u,4097u,8191u,8192u,8193u,40964u,65535u,65536u,65537u})sizes.push_back(n);
 Json cases=Json::array();unsigned original_positive_matches{};bool corruption_rejected=false,overflow_rejected=false;
 try{(void)source_sound_sdk_pool_layout(0xffffffffu);}catch(const std::exception&){overflow_rejected=true;}check(overflow_rejected,"Unrepresentable native backing became a successful32-bit allocation");
 try{SourceRuntimeSoundSdkResources::CallScope scope(&resources);
  for(const auto n:sizes){
   auto a=allocation_abi;put(bank,a.gp[4],0x38797bu);put(bank,a.gp[4]+4u,n);put(bank,a.gp[4]+8u,n==40964u?0x61645344u:0x626f5344u);
   check(resources.dispatch(bank,{0x387975u,word(bank,0x41b79cu),false},a),"Actual pool allocation request was not dispatched");const auto pointer=a.gp[0];const auto layout=source_sound_sdk_pool_layout(n);
   check(pointer&&layout.client_capacity>=n&&bank.view(pointer,layout.client_capacity).size()==layout.client_capacity,"Pool client lacks its real requested backing");
   check((pointer&0xfffu)==0?!layout.client_offset:layout.client_offset==8u,"Pool pointer violates the original small/page query distinction");
   auto q=query_abi;put(bank,q.gp[4],0x387988u);put(bank,q.gp[4]+4u,pointer);check(resources.dispatch(bank,{0x387982u,word(bank,0x41b7ecu),false},q),"Actual pool capacity request was not dispatched");check(q.gp[0]==layout.client_capacity,"Pool query did not report actual owned capacity");
   cases.push_back({{"requested",n},{"pointer",pointer},{"backing_bytes",layout.backing_bytes},{"client_offset",layout.client_offset},{"client_capacity",layout.client_capacity},{"alignment",layout.alignment},{"capacity_query_result",q.gp[0]},{"prefix8_hex",layout.client_offset?Json(hex(bank.view(pointer-8u,8))):Json(nullptr)}});
   for(std::size_t i=0;i+1<reference.at("rows").size();++i){const auto&r=reference.at("rows")[i];if(r.at("kind")=="allocate"&&r.at("arguments")[0]==n){const auto&next=reference.at("rows")[i+1];check(next.at("kind")=="capacity"&&next.at("arguments")[0]==r.at("result")&&next.at("result")==q.gp[0],"Native capacity differs from actual original whole-call result");++original_positive_matches;}}
   if(n==16u){auto metadata=bank.mutable_view(pointer-5u,1);const auto actual=metadata[0];metadata[0]=std::byte{0};auto invalid=query_abi;put(bank,invalid.gp[4],0x387988u);put(bank,invalid.gp[4]+4u,pointer);try{resources.dispatch(bank,{0x387982u,word(bank,0x41b7ecu),false},invalid);}catch(const std::exception&){corruption_rejected=true;}metadata[0]=actual;}
  }
  check(resources.live_allocations()==sizes.size(),"Actual raw allocations were not retained");throw EndPoolTest{};
 }catch(const EndPoolTest&){}
 check(resources.live_allocations()==0&&resources.lock_depth()==0&&!resources.quiescent(),"Pool abort leaked native blocks or admitted partial source state");check(corruption_rejected&&original_positive_matches==5,"Pool metadata/captured positive coverage is incomplete");
 std::filesystem::create_directories(out);Json report{{"original_sha256",reference.at("original_sha256")},{"original_pool_report",std::filesystem::absolute(original/"report.json").string()},{"cases",cases},{"small_request_domain",{{"first",0},{"last",4056},{"cases",4057}}},{"actual_original_full_call_matches",original_positive_matches},{"real_native_allocations",cases.size()},{"native_allocations_after_abort",resources.live_allocations()},{"invalid_size_metadata_rejected",corruption_rejected},{"unrepresentable_backing_rejected",overflow_rejected},{"fixture_callee_results_used",false},{"kernel_free_lists_physical_availability_or_raw_metadata_emulated",false},{"native_device_dsp_or_pcm_verified",false}};
 std::ofstream f(out/"report.json");f<<report.dump(2)<<'\n';check(bool(f),"Cannot retain native pool capacity cases");std::cout<<"PASS real native pool capacities: "<<cases.size()<<" allocations; five original complete calls matched; metadata/overflow/cleanup guards passed"<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
