// Appended to the shared physical i386 bridge by the owned qualification tool.
int main(int argc,char**argv){
 J report{{"passed",false},{"instruction_patches",J::array()},{"supplied_original_outcomes",false},{"captured_original_gp_frame",false}};
 try{
  check(argc==5,"Supply declared-caller ELF, actual BEFORE manifest, case and report");
  std::ifstream in(argv[2]);const auto capture=J::parse(in);const std::string name=argv[3];
  Child original;original.start(argv[1],0);const auto entry=original.regs();const auto before=original.fp();
  check(before.ftw==0,"List qualification requires its declared empty x87 stack");
  const auto args=capture.at("input").at("arguments").get<std::array<std::uint32_t,4>>();
  const bool reversed=capture.at("input").at("owner")==0x2f81f0u;
  const auto first=args[0],second=args[1],context=args[2],user_data=args[3],collection=original.word(reversed?second:first);
  auto put=[&](Child& child,std::uint32_t address,std::uint32_t value){child.write(address,{reinterpret_cast<const std::byte*>(&value),4});};
  if(name=="zero_count")put(original,collection+16u,0);
  if(name=="negative_count")put(original,collection+16u,0xffffffffu);
  if(name=="one_child")put(original,collection+16u,1);
  if(name=="rejected"){
   auto root=reversed?first:second;while(const auto parent=original.word(root+12u))root=parent;
   const auto filter=original.word(context+4u),row=original.word(root+28u);
   put(original,filter-12u+(row&31u)*4u+28u,0);
  }
  struct Bank{std::uint32_t address;std::vector<std::byte> before,after;};std::vector<Bank> banks;
  for(const auto& r:capture.at("regions")){const auto address=r.at("address").get<std::uint32_t>();banks.push_back({address,original.read(address,r.at("size")),{}});}
  J reads=J::array(),stores=J::array(),calls=J::array();std::uint32_t payload{},output{};std::size_t instructions{};
  auto bytes=[](const auto& input){J values=J::array();for(auto b:input)values.push_back(std::to_integer<unsigned>(b));return values;};
  auto normalize=[](std::uint32_t address,std::uint32_t data,std::uint32_t out){
   if(data&&address>=data&&address<data+16u)return 0xffff0000u+address-data;
   if(out&&address==out)return 0xffff0020u;
   return address;
  };
  auto normalized_args=[&](J values,std::uint32_t data,std::uint32_t out){for(auto& v:values)v=normalize(v.get<std::uint32_t>(),data,out);return values;};
  for(;;){
   const auto r=original.regs();const auto pc=std::uint32_t(r.rip);if(pc==0x1ccaca)break;
   check(++instructions<4096,"List constructor left its bounded original path");
   std::uint32_t address{},target{},receiver{},return_pc{};unsigned width=4;bool read=false,write=false,call=false;J arguments=J::array();
   switch(pc){
    case 0x2f8046:case 0x2f8206:address=0x48c938;read=true;break;
    case 0x2f804c:case 0x2f820c:address=r.rcx;read=true;break;
    case 0x2f8052:address=r.rax+16u;read=true;call=true;target=original.word(address);receiver=r.rcx;return_pc=0x2f8055;arguments=J::array({original.word(r.rsp),original.word(r.rsp+4u)});break;
    case 0x2f8055:address=r.rax+4u;width=2;write=true;break;
    case 0x2f8213:address=r.rax+16u;read=true;call=true;target=original.word(address);receiver=r.rcx;return_pc=0x2f8216;arguments=J::array({original.word(r.rsp),original.word(r.rsp+4u)});break;
    case 0x2f8218:address=r.rsi+4u;width=2;write=true;break;
    case 0x2f8249:address=r.rsi;write=true;break;
    case 0x2f7d46:address=r.rbp+8u;write=true;break;
    case 0x2f7d4e:address=r.rbp;write=true;break;
    case 0x2f7d5c:address=r.rbx;write=true;break;
    case 0x2f7d5e:address=r.rbx+4u;write=true;break;
    case 0x2f7d61:address=r.rbx+8u;write=true;break;
    case 0x2f7d6c:address=r.rax+8u;read=true;break;
    case 0x2f7d6f:address=r.rax;read=true;break;
    case 0x2f7d71:payload=r.rsp+28u;output=r.rsp+68u;address=r.rsp+40u;write=true;break;
    case 0x2f7d75:address=r.rsp+36u;write=true;break;
    case 0x2f7d79:address=r.rsi;read=true;break;
    case 0x2f7d86:address=r.rax+40u;read=true;call=true;target=original.word(address);receiver=r.rcx;return_pc=0x2f7d89;break;
    case 0x2f7d8b:address=r.rbx+8u;read=true;break;
    case 0x2f7dab:address=r.rbx+4u;write=true;break;
    case 0x2f7dae:address=r.rbp+16u;read=true;break;
    case 0x2f7dc0:address=r.rsi+12u;read=true;break;
    case 0x2f7dc3:address=std::uint32_t(r.rcx+r.rdi*8u);read=true;break;
    case 0x2f7dc6:address=r.rbp+4u;read=true;break;
    case 0x2f7dca:address=r.rsp+32u;write=true;break;
    case 0x2f7dde:address=r.rsp+52u;write=true;break;
    case 0x2f7de2:address=r.rcx;read=true;break;
    case 0x2f7de5:address=r.rax;read=true;call=true;target=original.word(address);receiver=r.rcx;return_pc=0x2f7de7;for(unsigned n=0;n<6;++n)arguments.push_back(original.word(r.rsp+n*4u));break;
    case 0x2f7de7:address=r.rax;width=1;read=true;break;
    case 0x2f7dec:address=r.rsp+28u;read=true;break;
    case 0x2f7df0:address=r.rbp;read=true;break;
    case 0x2f7df3:address=r.rcx;read=true;break;
    case 0x2f7df9:address=r.rdx+20u;read=true;call=true;target=original.word(address);receiver=r.rcx;return_pc=0x2f7dfc;break;
    case 0x2f7e02:address=r.rax;read=true;break;
    case 0x2f7e04:address=r.rcx;read=true;break;
    case 0x2f7e06:address=r.rdx+20u;read=true;call=true;target=original.word(address);receiver=r.rcx;return_pc=0x2f7e09;break;
    case 0x2f7e09:case 0x2f7e3b:address=r.rbx;read=true;break;
    case 0x2f7e18:address=std::uint32_t(r.rax+r.rsi*4u+0x18cu);read=true;break;
    case 0x2f7e2e:call=true;target=r.rax;return_pc=0x2f7e30;for(unsigned n=0;n<4;++n)arguments.push_back(original.word(r.rsp+n*4u));report["original_child_payloads"].push_back(bytes(original.read(payload,16)));break;
    case 0x2f7e30:address=r.rsi;write=true;break;
    case 0x2f7e4d:address=r.rdx;write=true;break;
    case 0x2f7e53:address=r.rax+16u;read=true;break;
   }
   if(read)reads.push_back(J::array({normalize(address,payload,output),width}));
   if(call){
    const auto initial=original.fp();original.step();original.go(return_pc);auto result=std::uint32_t(original.regs().rax);
    // Nested cdecl factories do not consume ECX; its volatile input is
    // outside their native typed API, unlike actual virtual receivers.
    if(pc==0x2f7e2e)receiver=0;
    calls.push_back(J{{"entry",target},{"this",receiver},{"arguments",normalized_args(arguments,payload,output)},{"returned",normalize(result,payload,output)},{"before_fp",fp_words(initial)},{"after_fp",fp_words(original.fp())}});
   }else original.step();
   if(write)stores.push_back(J::array({normalize(address,payload,output),bytes(original.read(address,width))}));
  }
  const auto returned=std::uint32_t(original.regs().rax);const auto after_fp=original.fp();
  for(auto& bank:banks)bank.after=original.read(bank.address,bank.before.size());
  report["original_completed"]=true;
  Child native;native.start(argv[1],0);for(const auto& bank:banks)native.write(bank.address,bank.before);
  Memory memory(native);Bridge bridge{native,0x62008000u,before,report};
  NativeTu15ListContactServices services;const std::uint32_t native_payload=0x6200c000u,native_output=0x6200c020u;
  put(native,native_output,context);
  services.allocation.allocate=[&](const auto& request,auto& fp){return bridge.call(request.owner,request.receiver,{request.bytes,request.memory_class},0x2f8055,fp);};
  services.allocation.release=[&](const auto& request,auto& fp){bridge.call(request.owner,request.receiver,{request.data,request.bytes,request.memory_class},0x1ccaca,fp);};
  services.child_count=[&](auto child,auto owner,auto& fp){return bridge.call(owner.identity,child.identity,{},0x2f7d89,fp);};
  services.filter_child=[&](auto receiver,auto owner,auto out,auto settings,auto b,auto a,auto shape,auto index,auto& fp){return bridge.call(owner.identity,receiver,{out,settings.identity,b.identity,a.identity,shape.identity,index},0x2f7de7,fp);};
  unsigned classes{};
  services.dispatch.classify_shape=[&](auto child,auto owner,auto& fp){return bridge.call(owner.identity,child.identity,{},classes++%2==0?0x2f7dfcu:0x2f7e09u,fp);};
  services.dispatch.create_agent=[&](auto owner,auto a,auto b,auto settings,auto data,auto& fp){report["native_child_payloads"].push_back(bytes(native.read(a.identity,16)));return NativeTu15ContactAgent{bridge.call(owner.identity,0,{a.identity,b.identity,settings.identity,data},0x2f7e30,fp)};};
  if(name=="native_collection"){
   services.child_count=[&](auto child,auto owner,auto& state){
    check(owner.identity==0x3061f0u,"Require the actual native list-count owner");
    const auto initial=J::array({state.control_word,state.status,state.mxcsr});memory.trace=false;
    const auto count=native_tu15_collection_child_count(memory,child);memory.trace=true;
    bridge.calls.push_back(J{{"entry",owner.identity},{"this",child.identity},{"arguments",J::array()},{"returned_eax",count},{"before_fp",initial},{"after_fp",J::array({state.control_word,state.status,state.mxcsr})}});return count;
   };
   services.filter_child=[&](auto receiver,auto owner,auto out,auto settings,auto b,auto a,auto shape,auto index,auto& state){
    check(owner.identity==0x1cbe70u,"Require the actual native collection-filter owner");
    const auto initial=J::array({state.control_word,state.status,state.mxcsr});memory.trace=false;
    NativeTu15CollectionFilterServices filter;
    filter.classify_shape=[&](auto shape,auto method,auto& state){return bridge.call(method.identity,shape.identity,{},0x1ccaca,state);};
    filter.resolve_child=[&](auto shape,auto method,auto key,auto output,auto& state){return NativeTu15ContactChild{bridge.call(method.identity,shape.identity,{key,output},0x1ccaca,state)};};
    filter.motion_type=[&](auto motion,auto method,auto& state){return bridge.call(method.identity,motion,{},0x1ccaca,state);};
    const auto previous=bridge.calls.size();
    const auto result=native_tu15_filter_collection_child(memory,receiver,out,b,a,shape,index,{0x6200c100u},state,filter);
    bridge.calls.erase(bridge.calls.begin()+previous,bridge.calls.end());memory.trace=true;
    bridge.calls.push_back(J{{"entry",owner.identity},{"this",receiver},{"arguments",J::array({out,settings.identity,b.identity,a.identity,shape.identity,index})},{"returned_eax",result},{"before_fp",initial},{"after_fp",J::array({state.control_word,state.status,state.mxcsr})}});return result;
   };
  }
  NativeTu15PhysicsFrameFp fp{before.cwd,before.swd,before.mxcsr};
  const NativeTu15ListContactArguments arguments{[&]{return first;},[&]{return second;},[&]{return context;},[&]{return user_data;}};
  const auto result=reversed?
    native_tu15_create_reversed_list_contact_agent(memory,arguments,{native_payload,native_output},fp,services):
    native_tu15_create_list_contact_agent(memory,arguments,{native_payload,native_output},fp,services);
  J native_reads=J::array(),native_stores=J::array(),native_calls=J::array();
  for(auto item:memory.ledger){item[0]=normalize(item[0].get<std::uint32_t>(),native_payload,native_output);native_reads.push_back(item);}
  for(auto item:memory.stores){item[0]=normalize(item[0].get<std::uint32_t>(),native_payload,native_output);native_stores.push_back(item);}
  for(const auto& call:bridge.calls)native_calls.push_back(J{{"entry",call.at("entry")},{"this",call.at("this")},{"arguments",normalized_args(call.at("arguments"),native_payload,native_output)},{"returned",normalize(call.at("returned_eax"),native_payload,native_output)},{"before_fp",call.at("before_fp")},{"after_fp",call.at("after_fp")}});
  std::size_t compared{},different{};J differences=J::array();
  for(const auto& bank:banks){const auto actual=native.read(bank.address,bank.after.size());compared+=actual.size();for(std::size_t n=0;n<actual.size();++n)if(actual[n]!=bank.after[n]){++different;if(differences.size()<24)differences.push_back(J{{"address",bank.address+n},{"native",std::to_integer<unsigned>(actual[n])},{"original",std::to_integer<unsigned>(bank.after[n])}});}}
  auto normalized_payloads=[&](const char* key){auto values=report.value(key,J::array());for(auto& value:values){check(value.size()==16,"Expected actual sixteen-byte child DATA");}return values;};
  const bool same_reads=reads==native_reads,same_stores=stores==native_stores,same_calls=calls==native_calls;
  const bool same_fp=fp_words(after_fp)==J::array({fp.control_word,fp.status,fp.mxcsr});
  const bool same_payloads=normalized_payloads("original_child_payloads")==normalized_payloads("native_child_payloads");
  report.update(J{{"passed",different==0&&result.identity==returned&&same_reads&&same_stores&&same_calls&&same_fp&&same_payloads},{"case",name},{"returned_original_agent",returned},{"returned_native_agent",result.identity},{"fp_equal",same_fp},{"ordered_reads_equal",same_reads},{"ordered_stores_equal",same_stores},{"service_calls_equal",same_calls},{"child_payloads_equal",same_payloads},{"persistent_compared_bytes",compared},{"different_bytes",different},{"first_differences",differences},{"original_owner_instructions",instructions},{"ordered_original_reads",reads},{"ordered_native_reads",native_reads},{"ordered_original_stores",stores},{"ordered_native_stores",native_stores},{"original_calls",calls},{"native_calls",native_calls},{"remaining_services_execute_original_cpu",true},{"production_native_only_closure",false}});
 }catch(const std::exception& error){report["error"]=error.what();std::cerr<<error.what()<<'\n';}
 if(argc>=5){std::ofstream out(argv[4]);out<<report.dump(2)<<'\n';}
 return report.at("passed").get<bool>()?0:1;
}
