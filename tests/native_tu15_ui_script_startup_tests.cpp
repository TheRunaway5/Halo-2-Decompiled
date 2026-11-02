#include "halo2/source_ui_script_startup_tu15.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace halo2;
using Json=nlohmann::json;
namespace {
constexpr std::uint32_t arena=0x81000000,arena_bytes=0x3fe000,object=0x2001000,stack=0x2008000,stop=0x2100000;
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void put(SourceRuntimeMemory& memory,std::uint32_t p,std::uint32_t value){std::array<std::byte,4> bytes{};for(unsigned i=0;i<4;++i)bytes[i]=std::byte(value>>(i*8));memory.write(p,bytes);}
std::uint32_t word(const SourceRuntimeMemory& memory,std::uint32_t p){const auto raw=memory.view(p,4);std::uint32_t value=0;for(unsigned i=0;i<4;++i)value|=std::to_integer<std::uint32_t>(raw[i])<<(i*8);return value;}
std::vector<std::byte> unhex(const std::string& text){
 check(!(text.size()%2),"Odd reference byte encoding");
 auto nibble=[](char c)->unsigned {if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;throw std::runtime_error("Invalid reference byte encoding");};
 std::vector<std::byte> raw(text.size()/2);for(std::size_t i=0;i<raw.size();++i)raw[i]=std::byte((nibble(text[i*2])<<4)|nibble(text[i*2+1]));return raw;
}
void same(const SourceRuntimeMemory& memory,std::uint32_t address,std::size_t bytes,const Json& expected){
 const auto raw=unhex(expected.get<std::string>());check(raw.size()==bytes&&std::ranges::equal(memory.view(address,bytes),raw),"Native constructor bytes differ from unchanged TU1.5 execution");
}
template<class F>void reject(F action){bool rejected=false;try{action();}catch(const std::exception&){rejected=true;}check(rejected,"Invalid or partial constructor backing was admitted");}
void scalar(const Json& fixture) {
 check(fixture.at("xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd" &&
     fixture.at("table_address")==0x502248 && fixture.at("table_bytes")==0x3c10 &&
     fixture.at("instruction_patches").empty() && fixture.at("supplied_results").empty(),
     "Scalar constructor reference is not unchanged TU1.5 evidence");
 unsigned cases=0;
 for(const auto& row:fixture.at("cases")) {
  SourceRuntimeMemory memory;mount_native_tu15_ui_script_startup_data(memory);
  check(std::ranges::all_of(memory.view(0x502248,0x3c10),[](auto b){return b==std::byte{};}),
      "Complete scalar-cast lookup storage must start loader-zeroed");
  const auto source_stack=fixture.at("stack").get<std::uint32_t>();
  memory.mount(source_stack-128,unhex(row.at("stack_before")),SourceRuntimeMemory::Access::ReadWrite,
      "declared original scalar constructor caller stack");
  SourcePhysicsWorldRuntimeAbi abi{};abi.gp=row.at("initial_gp").get<decltype(abi.gp)>();
  abi.eflags=row.at("flags");const auto control=row.at("control").get<std::uint16_t>();
  abi.floating_point={control,0x1f80};
  for(unsigned n=0;n<8;++n)for(unsigned j=0;j<4;++j)for(unsigned b=0;b<4;++b)
      abi.xmm[n][j]|=((cases+n+j*4+b)&255)<<(b*8);
  const auto before=abi;
  Json stores=Json::array();std::vector<std::uint32_t> trace;
  SourcePhysicsWorldRuntimeServices services;
  services.instruction=[&](auto pc){trace.push_back(pc);};
  services.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("Scalar constructor reached a dependency");};
  source_run_ui_script_startup_tu15(memory,0x212470,abi,{control,0x1f80},services,[&](const auto& store){
   std::uint64_t value=0;for(std::size_t n=0;n<store.written.size();++n)value|=std::uint64_t(std::to_integer<unsigned>(store.written[n]))<<(n*8);
   stores.push_back({store.instruction,store.address,store.written.size(),value});
  });
  check(abi.gp==row.at("gp").get<decltype(abi.gp)>() && abi.eflags==before.eflags,
      "Scalar constructor returned different original registers/flags");
  check(abi.xmm==before.xmm && abi.x87_depth==before.x87_depth && abi.x87_status==before.x87_status &&
      abi.floating_point && before.floating_point &&
      abi.floating_point->x87_control_word==before.floating_point->x87_control_word &&
      abi.floating_point->mxcsr==before.floating_point->mxcsr,"Scalar constructor changed its FP/SIMD ABI");
  check(trace==row.at("trace").get<decltype(trace)>() && stores==row.at("stores"),
      "Scalar constructor original instruction/store order differs");
  same(memory,0x502248,0x3c10,row.at("table_after"));
  same(memory,source_stack-128,256,row.at("stack_after"));++cases;
 }
 check(cases==4,"Scalar constructor reference lost flags/direction coverage");
 std::cout<<"PASS four unchanged original scalar constructors; all3844 lookup words, stores, stack and ABI\n";
}
void run(const Json& expected){
 SourceRuntimeMemory memory;mount_native_tu15_ui_script_startup_data(memory);
 const auto fill=expected.at("fill").get<unsigned>();
 memory.mount(arena,std::vector<std::byte>(arena_bytes,std::byte(fill)),SourceRuntimeMemory::Access::ReadWrite,"declared prior native arena");
 memory.mount(0x2000000,std::vector<std::byte>(0x20000),SourceRuntimeMemory::Access::ReadWrite,"declared caller and arena object");
 memory.mount(0x4f2950,std::array<std::byte,16>{},SourceRuntimeMemory::Access::ReadWrite,"declared previous arena bounds and CRC");
 memory.mount(0x51df2c,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"declared previous arena object binding");
 memory.mount(0x45f1d8,std::array<std::byte,4>{std::byte{0x10},std::byte{0x72},std::byte{0x12},std::byte{}},SourceRuntimeMemory::Access::ReadOnly,"original arena allocation vtable slot");
 memory.mount(0x474cb8,std::array<std::byte,8>{std::byte{0x34},std::byte{0x60},std::byte{0x48},std::byte{},std::byte{0x38},std::byte{0x60},std::byte{0x48},std::byte{}},SourceRuntimeMemory::Access::ReadOnly,"original generic pool default owner identities");
 memory.mount(0x486034,std::array<std::byte,4>{std::byte{0x48},std::byte{0xf0},std::byte{0x45},std::byte{}},SourceRuntimeMemory::Access::ReadOnly,"original generic allocator virtual object");
 memory.mount(0x45f048,std::array<std::byte,4>{std::byte{0xf0},std::byte{0xf3},std::byte{0x11},std::byte{}},SourceRuntimeMemory::Access::ReadOnly,"original generic allocator vtable slot");
 constexpr std::array<char,16> format{"%s reference"};memory.mount(0x466e00,std::as_bytes(std::span(format)),SourceRuntimeMemory::Access::ReadOnly,"authored object-reference CRT format");
 memory.mount(0x474c20,std::array<std::byte,4>{std::byte{0xff},std::byte{0xff},std::byte{0xff},std::byte{0xff}},SourceRuntimeMemory::Access::ReadWrite,"declared prior no-scenario startup");
 memory.mount(0x4ecc20,std::array<std::byte,4>{},SourceRuntimeMemory::Access::ReadWrite,"declared prior unloaded scenario binding");
 memory.mount(0x41b6d4,std::array<std::byte,4>{std::byte{0xb8},std::byte{},std::byte{},std::byte{0x80}},SourceRuntimeMemory::Access::ReadOnly,"original NtAllocateVirtualMemory import");
 memory.mount(0x56e225,std::array<std::byte,1>{},SourceRuntimeMemory::Access::ReadWrite,"original CRC cold flag");
 memory.mount(0x56e258,std::vector<std::byte>(0x400,std::byte(fill)),SourceRuntimeMemory::Access::ReadWrite,"original CRC table backing");
 put(memory,0x51df2c,object);put(memory,object,0x45f1d8);put(memory,0x4f2950,arena);put(memory,0x4f2954,0x100);put(memory,0x4f2958,0x200);put(memory,0x4f295c,0x12345678);
 NativeTu15StorageFormatting formatting(memory);
 Json allocations=Json::array();
 SourcePhysicsPoolHostStorage host(memory,0x10000000,[&](const auto& event){
  check(event.allocation,"Original startup unexpectedly released a raw allocation");
  std::ranges::fill(memory.mutable_view(event.result[1],event.result[2]),std::byte(fill));
  allocations.push_back({{"source",event.result[1]},{"requested",event.request[2]},{"reserved",event.result[2]}});
 });
 SourcePhysicsWorldRuntimeAbi abi{};for(unsigned i=0;i<8;++i)abi.gp[i]=0x11000000+i*0x111111;
 abi.eflags=0x202;abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x27f,0x1f80};
 Json stores=Json::array();SourcePhysicsContactRecordObserver observer=[&](const auto& store){std::uint64_t value=0;check(store.written.size()<=8,"Unexpected original store width");for(std::size_t i=0;i<store.written.size();++i)value|=std::uint64_t(std::to_integer<unsigned>(store.written[i]))<<(i*8);stores.push_back({store.instruction,store.address,store.written.size(),value});};
 std::size_t instructions=0;SourcePhysicsWorldRuntimeServices services;services.instruction=[&](auto){++instructions;};
 services.required_owner=[](auto&,const auto&,auto&,const auto&){throw std::runtime_error("Original startup reached undeclared dependency");};
 reject([&]{native_tu15_ui_script_startup_views(memory);});
 Json steps=Json::array();steps.push_back(expected.at("prior_static_constructor"));for(const auto& step:expected.at("steps"))steps.push_back(step);
 for(const auto& step:steps){
  put(memory,stack,stop);abi.gp[4]=stack;const auto before=instructions;
  if(step.at("entry")==0x381390)source_run_ui_script_startup_prior_constructor(memory,abi,{0x27f,0x1f80},services,observer);
  else source_run_ui_script_startup_with_native_storage(memory,step.at("entry").get<std::uint32_t>(),abi,{0x27f,0x1f80},services,host,formatting,observer);
  check(abi.gp==step.at("returned_gp").get<decltype(abi.gp)>(),"Native original startup returned different registers");
  check((abi.eflags&0xcc5)==(step.at("returned_eflags").get<std::uint32_t>()&0xcc5),"Native original startup returned different defined flags");
  check(instructions-before==step.at("instructions"),"Native original startup instruction count differs");
  check(word(memory,0x4f2954)==step.at("arena_cursor")&&word(memory,0x4f2958)==step.at("arena_top")&&word(memory,0x4f295c)==step.at("crc"),"Native original startup arena/CRC state differs");
 }
 check(instructions==expected.at("instructions"),"Native startup total instruction count differs");
 check(stores==expected.at("source_stores"),"Native startup original ordered CPU stores differ");
 check(allocations.size()==1&&host.live_allocations()==1&&formatting.calls()==1,"Native startup raw resource count differs");
 for(std::size_t i=0;i<allocations.size();++i)for(const auto* field:{"source","requested","reserved"})check(allocations[i].at(field)==expected.at("allocations")[i].at(field),"Native raw resource request differs");
 for(const auto& field:expected.at("fields"))same(memory,field.at("address"),field.at("bytes"),field.at("hex"));
 for(const auto& field:expected.at("allocations_bytes"))same(memory,field.at("source"),field.at("bytes"),field.at("hex"));
 same(memory,arena,arena_bytes,expected.at("arena_hex"));same(memory,0x56e258,0x400,expected.at("crc_table_hex"));
 const auto views=native_tu15_ui_script_startup_views(memory);check(views.regions.size()==22,"Native startup wide storage region count differs");
 for(const auto& region:views.regions)check(region.native.value>0xffffffffu&&memory.source_address(region.native,region.bytes)==region.source,"Native startup storage truncates or detaches wide backing");
 const auto original=word(memory,0x51df50);put(memory,0x51df50,0);reject([&]{native_tu15_ui_script_startup_views(memory);});put(memory,0x51df50,original);
 auto before=abi;SourceRuntimeMemory foreign;SourcePhysicsPoolHostStorage other(foreign);reject([&]{source_run_ui_script_startup_with_native_storage(memory,0x31d400,abi,{0x27f,0x1f80},services,other,formatting);});check(abi.gp==before.gp,"Rejected foreign native storage changed caller registers");
 std::cout<<"Original startup51..58 fill "<<fill<<": "<<instructions<<" instructions, "<<views.regions.size()<<" wide storage regions\n";
}
}
int main(int argc,char** argv){try{check(argc==2,"Expected original startup fixture");std::ifstream stream(argv[1]);Json fixture;stream>>fixture;if(fixture.value("schema","")=="H2_TU15_HS_SCALAR_CASTS_V1"){scalar(fixture);return 0;}check(fixture.at("passed")==true&&fixture.at("original_xbe_sha256")=="21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd","Original startup fixture is not authenticated TU1.5 evidence");for(const auto& row:fixture.at("cases"))run(row);return 0;}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
