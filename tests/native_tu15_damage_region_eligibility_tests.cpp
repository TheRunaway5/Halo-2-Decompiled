#include "gameplay/objects/source_damage_region_eligibility_native_tu15_internal.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

namespace {
using namespace halo2;using namespace halo2::physics_world_runtime_detail;using J=nlohmann::json;
void check(bool yes,const char* why){if(!yes)throw std::runtime_error(why);}
struct Memory final:SourcePhysicsContactMemory {
    std::uint32_t base{},pc{},global{};std::vector<std::byte> data;std::array<std::byte,4> global_data{};
    mutable J accesses=J::array(),fault=nullptr;J stores=J::array();
    std::span<std::byte> view(std::uint32_t p,std::size_t size) {
        if(p==global&&size==4)return global_data;
        if(p>=base&&std::uint64_t(p-base)+size<=data.size())return std::span(data).subspan(p-base,size);
        return {};
    }
    void fail(std::uint32_t p,std::size_t size,const char* kind)const {
        fault=J{{"pc",pc},{"kind",kind},{"address",p},{"size",size}};throw std::runtime_error("Original declared source-memory fault");
    }
    void read(std::uint32_t p,std::span<std::byte> out)const override {
        const auto bytes=const_cast<Memory*>(this)->view(p,out.size());if(bytes.empty())fail(p,out.size(),"read");
        accesses.push_back(J{{"pc",pc},{"kind","read"},{"address",p},{"size",out.size()}});
        std::copy(bytes.begin(),bytes.end(),out.begin());
    }
    void write(std::uint32_t p,std::span<const std::byte> in)override {
        auto bytes=view(p,in.size());if(bytes.empty())fail(p,in.size(),"write");
        accesses.push_back(J{{"pc",pc},{"kind","write"},{"address",p},{"size",in.size()}});
        std::uint32_t value{};for(unsigned n=0;n<in.size();++n)value|=std::to_integer<unsigned>(in[n])<<(n*8u);
        stores.push_back(J::array({pc,p,in.size(),value}));std::copy(in.begin(),in.end(),bytes.begin());
    }
    void initialize(std::uint32_t p,const std::string& hex) {
        auto bytes=view(p,hex.size()/2);check(bytes.size()==hex.size()/2,"Declared BEFORE escapes bank");
        for(unsigned n=0;n<bytes.size();++n)bytes[n]=std::byte(std::stoul(hex.substr(n*2,2),nullptr,16));
    }
};
}
int main(int argc,char** argv){J report{{"passed",false}};try{
 check(argc==4,"Supply original numeric fixture, count vectors and output");std::ifstream in(argv[1]);const auto ref=J::parse(in);
 std::ifstream vectors(argv[2],std::ios::binary);std::array<char,12> header{};check(bool(vectors.read(header.data(),12)),"Truncated count header");
 check(std::string_view(header.data(),8)=="REG15V01","Wrong region-count vector authority");
 for(unsigned word=0;word<65536;++word){std::array<unsigned char,4> bytes{};check(bool(vectors.read(reinterpret_cast<char*>(bytes.data()),4)),"Truncated count reference");
  std::uint32_t expected{};for(unsigned n=0;n<4;++n)expected|=std::uint32_t(bytes[n])<<(n*8u);
  const auto actual=native_tu15_object_region_count(std::bit_cast<std::int16_t>(std::uint16_t(word)));
  check(std::uint32_t(actual)==expected,"Signed extent division differs from unchanged original");
 }
 check(vectors.peek()==std::char_traits<char>::eof(),"Trailing count data");
 unsigned cases{},faults{},reads{},writes{};
 for(const auto& row:ref.at("cases"))for(bool selected:{false,true})for(unsigned live_values:{0u,2u}) {
  report["active_case"]=row.at("input");report["selected_observation"]=selected;
  Memory memory;memory.base=ref.at("bank");memory.global=ref.at("global_pointer");memory.data.resize(ref.at("size"));
  const auto seed=row.at("input").at("seed").get<unsigned>();for(unsigned n=0;n<memory.data.size();++n)memory.data[n]=std::byte((n*37u+seed)&255u);
  for(const auto& item:row.at("before_writes"))memory.initialize(item[0],item[1]);
  auto expected_data=memory.data;auto expected_global=memory.global_data;
  SourcePhysicsContactRuntimeAbi abi{};abi.gp=row.at("before_gp").get<decltype(abi.gp)>();abi.eflags=row.at("before_eflags");
  abi.floating_point=SourcePhysicsFactoryFloatingPoint{0x027f,0x1f80};abi.x87_depth=live_values;abi.x87[0]=1.25L;abi.x87[1]=-3.0L;
  for(unsigned i=0;i<8;++i)for(unsigned n=0;n<4;++n)abi.xmm[i][n]=0xa5760000u+i*4u+n;
  const SourcePhysicsWorldRuntimeServices services{};const SourcePhysicsContactRecordObserver observer{};
  WorldState state(memory,observer,services,SourcePhysicsContactImage::TitleUpdate15);state.load(abi);J trace=J::array();
  Memory other_bank;std::optional<SourceDamageRegionEligibilityObservationTu15> observation;
  SourceDamageRegionEligibilityObservationScopeTu15 observed(selected?&memory:&other_bank,[&](const auto& value){
    check(!observation,"Eligibility observation emitted twice");observation=value;
  });
  auto site=[&](std::uint32_t canonical,std::uint32_t actual){memory.pc=actual;trace.push_back(actual);return selected?actual:canonical;};
  try {
   if(row.at("input").at("entry")==0xbc6c0u)object_regions_native_tu15(state,[&](auto actual){return site(actual-0x1140u,actual);});
   else damage_region_eligibility_native_tu15(state,site);
  }catch(const std::exception&){if(memory.fault.is_null())throw;}
  const auto after=state.save();
  const bool complete_eligibility=memory.fault.is_null()&&row.at("input").at("entry")==0xdc4c0u;
  check(bool(observation)==(selected&&complete_eligibility),"Eligibility observation crossed a bank or an incomplete call");
  if(observation){
    check(observation->allowed==bool(after.gp[0]&255u),"Eligibility observation differs from actual source AL");
    check(observation->section==abi.gp[0]&&observation->response==abi.gp[1]&&observation->damage_info==abi.gp[2],"Eligibility observation lost its actual inputs");
  }
  for(const auto& store:row.at("stores")) {
   const auto address=store[1].get<std::uint32_t>(),width=store[2].get<unsigned>(),value=store[3].get<std::uint32_t>();
   for(unsigned n=0;n<width;++n)if(address==memory.global)expected_global[n]=std::byte(value>>(n*8u));else expected_data.at(address-memory.base+n)=std::byte(value>>(n*8u));
  }
  const bool gp=after.gp==row.at("after_gp").get<decltype(abi.gp)>();
  const bool flags=(after.eflags&row.at("defined_flags_mask").get<unsigned>())==(row.at("after_eflags").get<unsigned>()&row.at("defined_flags_mask").get<unsigned>());
  report.update(J{{"gp_equal",gp},{"flags_equal",flags},{"trace_equal",trace==row.at("trace")},{"accesses_equal",memory.accesses==row.at("accesses")},{"stores_equal",memory.stores==row.at("stores")},{"fault_equal",memory.fault==row.at("fault")},{"bank_equal",memory.data==expected_data&&memory.global_data==expected_global}});
  if(!(gp&&flags&&trace==row.at("trace")&&memory.accesses==row.at("accesses")&&memory.stores==row.at("stores")&&memory.fault==row.at("fault")&&memory.data==expected_data&&memory.global_data==expected_global)) {
   report.update(J{{"native_gp",after.gp},{"original_gp",row.at("after_gp")},{"native_trace",trace},{"original_trace",row.at("trace")},{"native_accesses",memory.accesses},{"original_accesses",row.at("accesses")},{"native_stores",memory.stores},{"original_stores",row.at("stores")},{"native_fault",memory.fault},{"original_fault",row.at("fault")}});
   throw std::runtime_error("Native region query/eligibility differs from original");
  }
  check(after.xmm==abi.xmm&&after.x87_depth==abi.x87_depth&&after.x87_status==abi.x87_status&&after.floating_point&&
    after.floating_point->x87_control_word==abi.floating_point->x87_control_word&&after.floating_point->mxcsr==abi.floating_point->mxcsr,"Unused FP/SIMD state changed");
  for(unsigned n=0;n<live_values;++n)check(std::memcmp(&after.x87[n],&abi.x87[n],10)==0,"Live x87 value changed");
  ++cases;faults+=!memory.fault.is_null();for(const auto& access:memory.accesses)reads+=access.at("kind")=="read";writes+=memory.stores.size();
 }
 report=J{{"passed",true},{"word_cases",65536},{"adapter_cases",cases},{"fault_prefixes",faults},{"ordered_reads",reads},{"ordered_stores",writes},{"original_gp_capture",false},{"full_gameplay_or_audio_parity",false}};
 }catch(const std::exception& error){report["error"]=error.what();std::cerr<<error.what()<<'\n';}
 if(argc>=4){std::ofstream out(argv[3]);out<<report.dump(2)<<'\n';}std::cout<<report.dump(2)<<'\n';return report.at("passed").get<bool>()?0:1;
}
