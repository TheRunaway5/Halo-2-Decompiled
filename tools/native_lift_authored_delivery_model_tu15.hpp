#pragma once
#include <immintrin.h>

namespace {
struct AuthoredDeliveryObserverStop {};
std::uint16_t delivery_host_control(){std::uint16_t value;asm volatile("fnstcw %0":"=m"(value));return value;}
std::uint16_t delivery_host_status(){std::uint16_t value;asm volatile("fnstsw %0":"=am"(value));return value;}
std::uint32_t delivery_word(Bank&memory,std::uint32_t p){std::array<std::byte,4>b{};memory.read(p,b);std::uint32_t value{};std::memcpy(&value,b.data(),4);return value;}
void delivery_put(Bank&memory,std::uint32_t p,std::uint32_t value,bool extend=false){
  std::array<std::byte,4>b{};std::memcpy(b.data(),&value,4);
  if(extend)for(unsigned i=0;i<4;++i)memory.data[p+i]=b[i];else memory.write(p,b);
}

J run_authored_delivery_models(const J&fixture){
  unsigned complete{},prefixes{},unknown_motion{},unknown_wake{},private_alias{},bad_fp{};
  for(const auto&row:fixture.at("cases"))for(const auto&delivery:row.at("velocity_deliveries")){
    const auto label="typed delivery parent"+std::to_string(row.at("id").get<unsigned>())+
      " site"+address(delivery.at("callsite"));
    const bool angular=delivery.at("callsite")==0x1db64fu;
    check(delivery.at("entry_x87_empty"),label+" original child requires an unprovided live x87 quantity");
    const halo2::NativeTu15LiftVelocityDelivery request{
      angular?halo2::NativeTu15LiftVelocityPhase::set_angular:halo2::NativeTu15LiftVelocityPhase::add_linear,
      delivery.at("component"),delivery.at("slot"),delivery.at("vector"),0};
    const auto invoke=[&](Bank&memory,auto&fp,const auto&observer,auto&activity,const auto&input){
      constexpr std::uint32_t base=0x62000000u;
      const std::array owned{halo2::NativeTu15ContactPrivateRange{base,0x900u}};
      halo2::NativeTu15ContactCandidatePrivateData view(memory,owned,{});
      halo2::native_tu15_deliver_lift_body_velocity(view,input,fp,
        {base+0x20u,base+0x24u,base+0x40u,base+0x80u,0x800u},{},observer,&activity);
    };
    for(std::size_t mode=0;mode<=delivery.at("observer_prefixes").size();++mode){
      Bank memory(delivery.at("ordinary_before_banks"));
      const auto&before=delivery.at("entry_fp");
      halo2::NativeTu15PhysicsFrameFp fp{before[0],before[1],before[2]};
      halo2::NativeTu15ObjectActivationBindingActivity activity;
      J observed=J::array();const auto stop=mode?delivery.at("observer_prefixes")[mode-1].at("store_count").get<unsigned>():0u;
      const halo2::NativeTu15RetainedStoreObserver observer([&](const auto&e){
        const auto i=observed.size();const auto&wanted=delivery.at("stores").at(i);
        const J actual{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}};
        check(actual==wanted,label+" typed source store differs: "+actual.dump()+" expected "+wanted.dump());
        std::vector<std::byte> current(e.written.size());memory.read(e.address,current);check(current==e.written,label+" observer precedes committed bytes");
        observed.push_back(actual);
        _mm_setcsr(_mm_getcsr()^0x6000u);
        const std::uint16_t altered=0x37fu;asm volatile("fldcw %0"::"m"(altered));
        if(stop&&observed.size()==stop)throw AuthoredDeliveryObserverStop{};
      });
      const auto host_cw=delivery_host_control(),host_sw=delivery_host_status();const auto host_mx=_mm_getcsr();bool stopped{};
      try{invoke(memory,fp,observer,activity,request);}catch(const AuthoredDeliveryObserverStop&){stopped=true;}
      check(stopped==bool(stop),label+" observer prefix did not stop at exact committed store");
      check(host_cw==delivery_host_control()&&host_sw==delivery_host_status()&&host_mx==_mm_getcsr(),label+" host FP leaked through observer/unwind");
      const auto&expected_fp=mode?delivery.at("observer_prefixes")[mode-1].at("after_fp"):delivery.at("return_fp");
      check(J::array({fp.control_word,fp.status,fp.mxcsr})==expected_fp,label+" typed delivery/prefix FP differs");
      const auto&expected=mode?delivery.at("observer_prefixes")[mode-1].at("after_banks"):delivery.at("ordinary_after_banks");
      const std::set<std::uint32_t> private_bytes=delivery.at("private_original_bytes").get<std::set<std::uint32_t>>();
      for(const auto&bank:expected){const auto first=bank.at("address").get<std::uint32_t>();const auto value=bytes(bank.at("hex"));
        for(unsigned i=0;i<value.size();++i)if(!private_bytes.contains(first+i))check(memory.data.at(first+i)==value[i],label+" typed delivery/prefix retained bank differs at "+address(first+i));}
      check(observed.size()==(stop?stop:delivery.at("stores").size()),label+" source store prefix/count differs");
      check(!activity.residual_calls,label+" positive typed body delivery used an unqualified child");
      if(mode)++prefixes;else ++complete;
    }
    if(!unknown_motion&&!delivery.at("stores").empty()){
      Bank memory(delivery.at("ordinary_before_banks"));const auto cell=delivery_word(memory,request.component+0x70u)+request.slot*0x60u+0x40u;
      const auto body=delivery_word(memory,cell),motion=delivery_word(memory,body+0x3cu),table=delivery_word(memory,motion);
      delivery_put(memory,table+(angular?0x54u:0x50u),0xdeadbeefu);
      halo2::NativeTu15PhysicsFrameFp fp{delivery.at("entry_fp")[0],delivery.at("entry_fp")[1],delivery.at("entry_fp")[2]};
      halo2::NativeTu15ObjectActivationBindingActivity activity;bool refused{};
      try{invoke(memory,fp,halo2::NativeTu15RetainedStoreObserver{},activity,request);}catch(const std::runtime_error&e){refused=std::string(e.what())=="Native phantom velocity requires motion virtual=3735928559";}
      check(refused&&!activity.residual_calls,label+" unknown motion owner did not refuse natively");++unknown_motion;
    }
    if(!unknown_wake&&row.at("declared_input").at("body_branch")=="inactive_world"){
      Bank memory(delivery.at("ordinary_before_banks"));const auto cell=delivery_word(memory,request.component+0x70u)+request.slot*0x60u+0x40u;
      const auto body=delivery_word(memory,cell),world=delivery_word(memory,body+8u);
      // The zero-list positive path never reads its pointer. Declare this
      // new negative input explicitly instead of borrowing an afterimage.
      delivery_put(memory,world+0x78u,world+0x900u,true);delivery_put(memory,world+0x7cu,1,true);
      delivery_put(memory,world+0x900u,world+0x920u,true);delivery_put(memory,world+0x920u,world+0x940u,true);delivery_put(memory,world+0x944u,0xdeadbeefu,true);
      halo2::NativeTu15PhysicsFrameFp fp{delivery.at("entry_fp")[0],delivery.at("entry_fp")[1],delivery.at("entry_fp")[2]};
      halo2::NativeTu15ObjectActivationBindingActivity activity;bool refused{};
      try{invoke(memory,fp,halo2::NativeTu15RetainedStoreObserver{},activity,request);}catch(const std::runtime_error&e){refused=std::string(e.what())=="Native lift velocity requires native wake child=3735928559";}
      check(refused&&activity.residual_owners[0xdeadbeefu]==1,label+" unknown wake listener escaped native refusal");++unknown_wake;
    }
    if(!private_alias&&!delivery.at("stores").empty()){
      Bank memory(delivery.at("ordinary_before_banks"));auto aliased=request;aliased.caller_vector=0x62000000u;
      halo2::NativeTu15PhysicsFrameFp fp{delivery.at("entry_fp")[0],delivery.at("entry_fp")[1],delivery.at("entry_fp")[2]};
      halo2::NativeTu15ObjectActivationBindingActivity activity;bool refused{};
      try{invoke(memory,fp,halo2::NativeTu15RetainedStoreObserver{},activity,aliased);}catch(const std::runtime_error&e){refused=std::string(e.what()).find("aliases private DATA")!=std::string::npos;}
      check(refused,label+" private caller vector alias did not refuse");++private_alias;
    }
    if(!bad_fp&&!angular){
      for(bool trap:{false,true}){
        Bank memory(delivery.at("ordinary_before_banks"));const auto before=memory.data;auto rejected=request;
        halo2::NativeTu15PhysicsFrameFp fp{delivery.at("entry_fp")[0],delivery.at("entry_fp")[1],delivery.at("entry_fp")[2]};
        if(trap)fp.mxcsr&=~0x80u;else rejected.incoming_x87_depth=6;
        halo2::NativeTu15ObjectActivationBindingActivity activity;bool refused{};
        const auto host_cw=delivery_host_control(),host_sw=delivery_host_status();const auto host_mx=_mm_getcsr();
        try{invoke(memory,fp,halo2::NativeTu15RetainedStoreObserver{},activity,rejected);}catch(const std::runtime_error&e){refused=std::string(e.what())=="Native TU1.5 joint row requires its FP capacity/exception owner";}
        check(refused&&memory.data==before&&!activity.residual_calls,label+" FP capacity/trap owner refused after a store or admitted execution");
        check(host_cw==delivery_host_control()&&host_sw==delivery_host_status()&&host_mx==_mm_getcsr(),label+" refused FP owner changed host state");++bad_fp;
      }
    }
  }
  check(complete&&prefixes&&unknown_motion&&private_alias,"Typed child models lacked positive/prefix/negative controls");
  return J{{"complete",complete},{"observer_prefixes",prefixes},{"unknown_motion_refusals",unknown_motion},
           {"unknown_wake_refusals",unknown_wake},{"private_alias_refusals",private_alias},
           {"fp_capacity_trap_refusals",bad_fp},
           {"whole_original_point_or_parent_conversion_claimed",false}};
}
}
