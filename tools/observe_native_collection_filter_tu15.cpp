// Owned independent native/physical comparison, with generated original operands.
int main(int argc,char**argv){J report{{"passed",false},{"supplied_original_outcomes",false}};try{
 check(argc==6,"Supply original ELF, BEFORE manifest, declared cases, case, report");
 std::ifstream mi(argv[2]),ci(argv[3]);const auto manifest=J::parse(mi),cases=J::parse(ci);J input;
 for(const auto& item:cases)if(item.at("name")==argv[4])input=item;check(!input.is_null(),"Unknown declared collection case");
 Child original;original.start(argv[1],0);const auto initial_regs=original.regs();const auto initial_fp=original.fp();
 constexpr std::uint32_t graph=0x6200c000u,first=graph,second=graph+64u,first_root=graph+128u,second_root=graph+192u;
 constexpr std::uint32_t filter=graph+0x200u,shape=graph+0x400u,wrapped=graph+0x500u,twice=graph+0x600u;
 constexpr std::uint32_t table=graph+0x700u,wrapper_table=graph+0x800u,twice_table=graph+0x900u;
 constexpr std::uint32_t collection=graph+0xa00u,collection_table=graph+0xb00u,children=graph+0xc00u;
 constexpr std::uint32_t user=graph+0xd00u,body=graph+0xe00u,motion=graph+0xf00u,motion_table=graph+0x1000u;
 constexpr std::uint32_t output=graph+0x1100u,native_geometry=graph+0x1200u,context=graph+0x1400u;
 auto put=[&](std::uint32_t p,std::uint32_t value){original.write(p,{reinterpret_cast<const std::byte*>(&value),4});};
 auto literal_owner=[](unsigned kind)->std::uint32_t{switch(kind){case 1:return 0xad740;case 4:return 0x2e8cf0;case 5:return 0x31dd70;case 6:return 0x31edf0;case 7:return 0x31ede0;case 8:return 0xad390;case 10:return 0xad710;case 21:return 0xae690;case 23:return 0xa9520;case 24:return 0xab4b0;default:throw std::runtime_error("No real constant method for declared class");}};
 original.write(graph,std::vector<std::byte>(0x1800));
 const bool parents=input.value("parents",false),special=input.value("special",false);
 const auto a=parents?first_root:first,b=parents?second_root:second;
 if(parents){put(first+12u,first_root);put(second+12u,second_root);}
 std::uint32_t row=0x55120003u,column=0x66130004u;
 if(input.value("same_group",false))column=0x55120004u;
 if(input.value("zero_group",false)){row=0;column=1;}
 if(input.value("bit31",false)){row=31;column=31;}
 put(a+28u,column);put(b+28u,row);put(filter-12u+(row&31u)*4u+28u,input.value("reject",false)?0:1u<<(column&31u));
 put(b+24u,input.value("root_kind",1u));put(b+32u,input.value("null_body",false)?0:body);
 put(0x48b860u,special?(input.value("null_body",false)?0:body):0x12345678u);
 put(body+60u,motion);put(motion,motion_table);put(motion_table+24u,literal_owner(input.value("motion",7u)));
 put(shape,table);put(table+20u,literal_owner(input.value("kind",10u)));
 auto selected=shape;const auto wrapper=input.value("wrapper",0u);
 if(wrapper==23||wrapper==2123){put(wrapped,wrapper_table);put(wrapper_table+20u,literal_owner(23));put(wrapped+48u,selected);selected=wrapped;}
 if(wrapper==21||wrapper==2123){put(twice,twice_table);put(twice_table+20u,literal_owner(21));put(twice+12u,selected);selected=twice;}
 const auto userdata=input.value("negative_user",false)?0x80003000u:input.value("user",user);
 put(shape+8u,userdata);if(userdata==user||userdata==0x80003000u){const std::byte flags{static_cast<unsigned char>(input.value("flag",1u))};original.write(userdata+31u,std::span(&flags,1));}
 put(first,selected);put(collection,collection_table);put(collection+12u,children);put(collection_table+52u,0x306220u);put(children,selected);
 put(output,0xaabbccddu);const auto index=input.value("no_key",false)?0xffffffffu:0u;
 struct Bank{std::uint32_t address;std::vector<std::byte> before,after;};std::vector<Bank> banks;
 for(const auto& region:manifest.at("regions")){const auto address=region.at("address").get<std::uint32_t>();banks.push_back({address,original.read(address,region.at("size")),{}});}
 banks.push_back({graph,original.read(graph,0x1800),{}});
 constexpr std::uint32_t sp=0x6200ef00u;const std::array<std::uint32_t,7> args{0x1ccaca,output,context,second,first,collection,index};
 original.write(sp,{reinterpret_cast<const std::byte*>(args.data()),sizeof(args)});auto entered=initial_regs;entered.rsp=sp;entered.rip=0x1cbe70;entered.rcx=filter;request(PTRACE_SETREGS,original.pid,nullptr,&entered);
 J reads=J::array(),stores=J::array(),calls=J::array();std::size_t count{};std::uint32_t original_geometry{};
 auto normalize=[](std::uint32_t p,std::uint32_t geometry){return geometry&&p>=geometry&&p<geometry+256u?0xfff10000u+p-geometry:p;};
 auto bytes=[](const auto& values){J out=J::array();for(auto b:values)out.push_back(std::to_integer<unsigned>(b));return out;};
 for(;;){const auto r=original.regs();const auto pc=std::uint32_t(r.rip);if(pc==0x1ccaca)break;check(++count<1024,"Collection filter left bounded original owners");
  std::uint32_t address{},target{},receiver{},return_pc{};unsigned width{};bool read=false,write=false,call=false;J arguments=J::array();
  switch(pc){/* ORIGINAL_OPERANDS */}
  if(read)reads.push_back(J::array({address,width}));
  if(call){
   if(pc==0x1cbebd){original_geometry=arguments[1];arguments[1]=normalize(original_geometry,original_geometry);}
   const auto before=fp_words(original.fp());original.step();original.go(return_pc);
   calls.push_back(J{{"entry",target},{"this",receiver},{"arguments",arguments},{"returned",std::uint32_t(original.regs().rax)},{"before_fp",before},{"after_fp",fp_words(original.fp())}});
  }else original.step();
  if(write)stores.push_back(J::array({address,bytes(original.read(address,width))}));
 }
 const auto returned=std::uint32_t(original.regs().rax);const auto final_fp=original.fp();for(auto& bank:banks)bank.after=original.read(bank.address,bank.before.size());report["original_completed"]=true;
 Child native;native.start(argv[1],0);for(const auto& bank:banks)native.write(bank.address,bank.before);Memory memory(native);Bridge bridge{native,0x62008000u,initial_fp,report};
 NativeTu15CollectionFilterServices services;
 services.classify_shape=[&](auto shape,auto owner,auto& fp){return bridge.call(owner.identity,shape.identity,{},0x1ccaca,fp);};
 services.resolve_child=[&](auto shape,auto owner,auto key,auto out,auto& fp){return NativeTu15ContactChild{bridge.call(owner.identity,shape.identity,{key,out},0x1ccaca,fp)};};
 services.motion_type=[&](auto motion,auto owner,auto& fp){return bridge.call(owner.identity,motion,{},0x1ccaca,fp);};
 NativeTu15PhysicsFrameFp fp{initial_fp.cwd,initial_fp.swd,initial_fp.mxcsr};
 const auto result=native_tu15_filter_collection_child(memory,filter,output,{second},{first},{collection},index,{native_geometry},fp,services);
 J native_calls=J::array();for(const auto& call:bridge.calls){auto arguments=call.at("arguments");if(arguments.size()==2)arguments[1]=normalize(arguments[1].get<std::uint32_t>(),native_geometry);native_calls.push_back(J{{"entry",call.at("entry")},{"this",call.at("this")},{"arguments",arguments},{"returned",call.at("returned_eax")},{"before_fp",call.at("before_fp")},{"after_fp",call.at("after_fp")}});}
 std::size_t compared{},different{};J differences=J::array();for(const auto& bank:banks){const auto actual=native.read(bank.address,bank.after.size());compared+=actual.size();for(std::size_t n=0;n<actual.size();++n)if(actual[n]!=bank.after[n]){++different;if(differences.size()<24)differences.push_back(J{{"address",bank.address+n},{"native",std::to_integer<unsigned>(actual[n])},{"original",std::to_integer<unsigned>(bank.after[n])}});}}
 const bool reads_equal=reads==memory.ledger,stores_equal=stores==memory.stores,calls_equal=calls==native_calls,fp_equal=fp_words(final_fp)==J::array({fp.control_word,fp.status,fp.mxcsr});
 report.update(J{{"passed",result==returned&&reads_equal&&stores_equal&&calls_equal&&fp_equal&&different==0},{"case",input},{"returned_original",returned},{"returned_native",result},{"original_owner_instructions",count},{"ordered_reads_equal",reads_equal},{"ordered_stores_equal",stores_equal},{"service_calls_equal",calls_equal},{"fp_equal",fp_equal},{"persistent_compared_bytes",compared},{"different_bytes",different},{"first_differences",differences},{"original_reads",reads},{"native_reads",memory.ledger},{"original_stores",stores},{"native_stores",memory.stores},{"original_calls",calls},{"native_calls",native_calls}});
 }catch(const std::exception& error){report["error"]=error.what();std::cerr<<error.what()<<'\n';}
 if(argc>=6){std::ofstream out(argv[5]);out<<report.dump(2)<<'\n';}return report.at("passed").get<bool>()?0:1;}
