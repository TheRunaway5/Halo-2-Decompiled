// Offline fragment combined with the existing producer's unchanged-original
// child/ABI helper. Original services execute their genuine bodies. Only
// declared input states are varied; no expected query result is installed.
namespace {
std::string query_hex(std::span<const std::byte>b){constexpr char d[]="0123456789abcdef";std::string out;for(auto v:b){const auto n=std::to_integer<unsigned>(v);out+=d[n>>4u];out+=d[n&15u];}return out;}
std::uint32_t query_reg(const user_regs_struct&r,const std::string&s){if(s.empty())return 0;if(s=="eax")return r.rax;if(s=="ebx")return r.rbx;if(s=="ecx")return r.rcx;if(s=="edx")return r.rdx;if(s=="esp")return r.rsp;if(s=="ebp")return r.rbp;if(s=="esi")return r.rsi;if(s=="edi")return r.rdi;throw std::runtime_error("Unknown original query address register");}
struct QueryMemory:Memory {
 std::uint32_t private_first,private_end,scratch,result,contact;mutable J ledger=J::array();
 QueryMemory(Child&c,std::uint32_t first,std::uint32_t end,std::uint32_t p,std::uint32_t r,std::uint32_t out):Memory(c),private_first(first),private_end(end),scratch(p),result(r),contact(out){}
 bool retained(std::uint32_t p,std::size_t n)const{return p<private_first||p>=private_end||(p>=scratch+0x18u&&std::uint64_t(p)+n<=scratch+0x70u)||(p==result&&n==1)||(p>=contact&&std::uint64_t(p)+n<=contact+80u);}
 void read(std::uint32_t p,std::span<std::byte>b)const override{Memory::read(p,b);if(retained(p,b.size()))ledger.push_back(J{{"kind","read"},{"address",p},{"bytes",b.size()},{"value",query_hex(b)}});}
 void write(std::uint32_t p,std::span<const std::byte>b)override{const auto before=child.read(p,b.size());Memory::write(p,b);if(retained(p,b.size())){check(!ledger.empty()&&ledger.back().at("kind")=="read"&&ledger.back().at("address")==p&&ledger.back().at("bytes")==b.size(),"Native query store observer before-read absent");ledger.erase(ledger.end()-1);ledger.push_back(J{{"kind","write"},{"address",p},{"bytes",b.size()},{"before",query_hex(before)},{"written",query_hex(b)}});}}
};
}
int main(int argc,char**argv){J report{{"passed",false},{"supplied_original_outcomes",false},{"instruction_patches",J::array()},{"production_native_only_closure",false}};try{
 check(argc==7,"Supply original ELF, capture.json, producer ordinal, input variant, metadata and report");
 std::ifstream input(argv[2]);const auto capture=J::parse(input);const auto ordinal=std::stoul(argv[3]),variation=std::stoul(argv[4]);
 Child original;original.start(argv[1],ordinal);original.go(0x2e4fe0u);const auto entry=original.regs();const auto before_fp=original.fp();check(before_fp.ftw==0,"Native query retained cohort requires empty incoming x87");const auto sp=std::uint32_t(entry.rsp),scratch=((sp-4u)&~15u)-0x70u;
 const std::array<std::uint32_t,5>args{original.word(sp+4u),original.word(sp+8u),original.word(sp+12u),original.word(sp+16u),original.word(sp+20u)};
 J patches=J::array();const auto put=[&](std::uint32_t p,std::uint32_t n){original.write(p,{reinterpret_cast<const std::byte*>(&n),4});patches.push_back(J::array({p,n}));};
 if(variation){put(std::uint32_t(entry.rcx)+0xf0u,0xffffffffu);}
 if(variation>=2){auto root=args[1];for(auto parent=original.word(root+0xcu);parent;parent=original.word(root+0xcu))root=parent;const auto body=original.word(root+0x20u),motion=original.word(body+0x3cu);put(motion+0x40u,variation==3?0x7f812345u:0x41000001u);put(motion+0x44u,0);put(motion+0x48u,0);put(motion+0x4cu,variation==4?0x7f812345u:0);if(variation>=5)put(original.word(args[1])+0x20u,0x3f800000u);}
 struct Bank{std::uint32_t address;std::vector<std::byte>before,after;};std::vector<Bank>banks;
 for(const auto&r:capture.at("regions")){const auto address=r.at("address").get<std::uint32_t>();banks.push_back({address,original.read(address,r.at("size")),{}});}
 const auto private_first=capture.at("private_returned_stack").at("first").get<std::uint32_t>(),private_end=capture.at("private_returned_stack").at("end").get<std::uint32_t>();QueryMemory original_view(original,private_first,private_end,scratch,args[0],args[4]);
 std::ifstream meta_input(argv[5]);const auto metadata=J::parse(meta_input);const auto ret=original.word(sp);J expected_ledger=J::array(),expected_stores=J::array();unsigned instructions{};
 std::map<std::uint32_t,std::pair<unsigned,std::uint32_t>>calls{{0x2e5010u,{0,0x2e5013u}},{0x2e512cu,{0,0x2e512fu}},{0x2e5292u,{0,0x2e5295u}},{0x2e52acu,{3,0x2e52b1u}},{0x2e52c5u,{3,0x2e52cau}},{0x2e52e8u,{5,0x2e52edu}}};
#if defined(HALO2_NATIVE_QUERY_CACHE)
 calls.erase(0x2e52acu);calls.erase(0x2e52c5u);
#if !defined(HALO2_NATIVE_QUERY_SHAPES)
 for(const auto&[pc,after]:std::map<std::uint32_t,std::uint32_t>{{0x30907au,0x30907du},{0x309089u,0x30908cu},{0x3090beu,0x3090c1u},{0x30910fu,0x309112u},{0x309134u,0x309137u},{0x30913eu,0x309141u},{0x30921au,0x30921du},{0x30926au,0x30926du},{0x309281u,0x309284u}})calls.emplace(pc,std::pair{0u,after});
#endif
#endif
#if defined(HALO2_NATIVE_QUERY_CONVEX)
 calls.erase(0x2e52e8u);calls.emplace(0x2f8765u,std::pair{0u,0x2f876au});
#endif
 for(;;){const auto state=original.regs();const auto pc=std::uint32_t(state.rip);if(pc==ret)break;check(++instructions<4096,"Original query parent instruction budget exceeded");const auto&site=metadata.at(std::to_string(pc));J pending=J::array();
  for(const auto&op:site.at("memory")){const auto address=query_reg(state,op.at("base"))+query_reg(state,op.at("index"))*op.at("scale").get<std::uint32_t>()+op.at("displacement").get<std::uint32_t>(),size=op.at("bytes").get<std::uint32_t>();if(!original_view.retained(address,size))continue;const auto data=original.read(address,size);if(op.at("read").get<bool>())expected_ledger.push_back(J{{"kind","read"},{"address",address},{"bytes",size},{"value",query_hex(data)}});if(op.at("write").get<bool>())pending.push_back(J{{"kind","write"},{"address",address},{"bytes",size},{"before",query_hex(data)},{"instruction",pc}});}
  if(auto call=calls.find(pc);call!=calls.end()){original.step();original.go(call->second.second);}else original.step();
  for(auto store:pending){store["written"]=query_hex(original.read(store.at("address"),store.at("bytes")));expected_stores.push_back(store);store.erase("instruction");expected_ledger.push_back(store);}
 }
 const auto expected_fp=original.fp();for(auto&bank:banks)bank.after=original.read(bank.address,bank.before.size());const auto expected_valid=original.read(args[0],1),expected_contact=original.read(args[4],80),expected_data=original.read(scratch+0x18u,0x58u);
 Child native;native.start(argv[1],ordinal);native.go(0x2e4fe0u);for(const auto&bank:banks)native.write(bank.address,bank.before);auto installed=entry;request(PTRACE_SETREGS,native.pid,nullptr,&installed);auto installed_fp=before_fp;request(PTRACE_SETFPREGS,native.pid,nullptr,&installed_fp);
 QueryMemory memory(native,private_first,private_end,scratch,args[0],args[4]);Bridge bridge{native,scratch,before_fp,report};NativeTu15ContactQueryServices services;unsigned classifications{};J native_stores=J::array();
#if defined(HALO2_NATIVE_QUERY_SHAPES)
 services.classify_shape=[&](auto,auto owner,auto&)->std::uint32_t{++classifications;if(owner.identity==0xa9520u)return native_tu15_contact_child_class_23();if(owner.identity==0xab4b0u)return native_tu15_contact_child_class_24();throw std::runtime_error("Native composed query requires its unsupported shape class owner");};
#else
 services.classify_shape=[&](auto shape,auto owner,auto&fp){const auto ret=classifications==0?0x2e5013u:classifications==1?0x2e512fu:0x2e5295u;++classifications;return bridge.call(owner.identity,shape.identity,{},ret,fp);};
#endif
#if defined(HALO2_NATIVE_QUERY_CACHE)
 J cache_calls=J::array();unsigned native_cache_parents{};
 services.initialize_cache=[&](auto owner,auto cache,auto first,auto second,auto transform,auto&fp){
  check(owner==0x309060u||owner==0x309150u,"Query selected unsupported cache owner");
  // Offline ABI maps each original owner's vector DATA to its real addresses.
  // The native cache receives only a typed data area; no GP/return postimage.
  const auto cache_data=scratch-(owner==0x309060u?0xa0u:0x90u);Bridge dependencies{native,cache_data,before_fp,report};NativeTu15ContactCacheServices cache_services;unsigned centers{},supports{},encodings{};
  const auto cache_observer=[&](const SourcePhysicsContactRecordStore&store){if(memory.retained(store.address,store.written.size()))native_stores.push_back(J{{"kind","write"},{"address",store.address},{"bytes",store.written.size()},{"before",query_hex(store.before)},{"written",query_hex(store.written)},{"instruction",store.instruction}});};
#if defined(HALO2_NATIVE_QUERY_SHAPES)
  cache_services.shape_center=[&](auto shape,auto method,auto output,auto&){++centers;native_tu15_contact_shape_center(memory,shape,method,output,cache_observer);};
  cache_services.support_vertex=[&](auto shape,auto method,auto direction,auto output,auto&state){++supports;const NativeTu15ContactShapeScratch child{cache_data-0x44u,cache_data-0x40u,nullptr};const NativeTu15ContactShapeScratch storage{cache_data-0x14u,0,&child};native_tu15_contact_shape_support(memory,shape,method,direction,output,storage,state,0,cache_observer);};
  cache_services.vertex_encoding=[&](auto,auto method,auto&){++encodings;check(method.identity==0x1cb030u,"Native query/cache requires actual shape encoding owner");return native_tu15_contact_shape_vertex_encoding();};
#else
  cache_services.shape_center=[&](auto shape,auto method,auto output,auto&state){const auto after=owner==0x309150u?0x30921du:centers==0?0x30907du:0x30908cu;++centers;dependencies.call(method.identity,shape.identity,{output},after,state);};
  cache_services.support_vertex=[&](auto shape,auto method,auto direction,auto output,auto&state){const auto after=owner==0x309150u?0x30926du:supports==0?0x3090c1u:0x309112u;++supports;dependencies.call(method.identity,shape.identity,{direction,output},after,state);};
  cache_services.vertex_encoding=[&](auto shape,auto method,auto&state){const auto after=owner==0x309150u?0x309284u:encodings==0?0x309137u:0x309141u;++encodings;return dependencies.call(method.identity,shape.identity,{},after,state);};
#endif
  if(owner==0x309060u)native_tu15_initialize_convex_contact_cache(memory,cache,first,second,transform,{cache_data},fp,cache_services,cache_observer);else native_tu15_initialize_triangle_contact_cache(memory,cache,first,second,transform,{cache_data},fp,cache_services,cache_observer);
  ++native_cache_parents;for(const auto&call:dependencies.calls)cache_calls.push_back(call);
 };
#else
 services.initialize_cache=[&](auto owner,auto cache,auto first,auto second,auto transform,auto&fp){bridge.call(owner,cache,{first.identity,second.identity,transform},owner==0x309150u?0x2e52b1u:0x2e52cau,fp);};
#endif
#if defined(HALO2_NATIVE_QUERY_CONVEX)
 J simplex_calls=J::array();unsigned native_convex_parents{};
 services.query_convex=[&](auto child,auto valid,auto first,auto second,auto context,auto output,auto&state){const auto data=scratch-0x200u;Bridge solver{native,data,before_fp,report};NativeTu15ConvexContactServices convex_services;convex_services.solve_simplex=[&](auto simplex,auto a,auto b,auto transform,auto cache,auto plane,auto&f){return solver.call(0x30e930u,simplex,{a.identity,b.identity,transform,cache,plane},0x2f876au,f);};const auto convex_observer=[&](const SourcePhysicsContactRecordStore&store){if(memory.retained(store.address,store.written.size()))native_stores.push_back(J{{"kind","write"},{"address",store.address},{"bytes",store.written.size()},{"before",query_hex(store.before)},{"written",query_hex(store.written)},{"instruction",store.instruction}});};native_tu15_query_convex_contact(memory,child,valid,first,second,context,output,{data,data-0x24u},state,0,convex_services,convex_observer);++native_convex_parents;for(const auto&call:solver.calls)simplex_calls.push_back(call);};
#else
 services.query_convex=[&](auto child,auto valid,auto first,auto second,auto context,auto output,auto&fp){bridge.call(0x2f8630u,child.identity,{valid,first.identity,second.identity,context.identity,output},0x2e52edu,fp);};
#endif
 NativeTu15PhysicsFrameFp fp{before_fp.cwd,before_fp.swd,before_fp.mxcsr};NativeTu15ContactQueryStats stats;
 const auto observer=[&](const SourcePhysicsContactRecordStore&store){if(memory.retained(store.address,store.written.size()))native_stores.push_back(J{{"kind","write"},{"address",store.address},{"bytes",store.written.size()},{"before",query_hex(store.before)},{"written",query_hex(store.written)},{"instruction",store.instruction}});};
 const auto result=native_tu15_query_contact(memory,{std::uint32_t(entry.rcx)},args[0],{args[1]},{args[2]},{args[3]},args[4],{scratch},fp,0,services,observer,&stats);
 std::size_t different{},compared{};J diffs=J::array();
 for(const auto&bank:banks){const auto actual=native.read(bank.address,bank.after.size());for(std::size_t n=0;n<actual.size();++n){const auto address=bank.address+n;if(address>=private_first&&address<private_end)continue;++compared;if(actual[n]!=bank.after[n]){++different;if(diffs.size()<32)diffs.push_back(J{{"address",address},{"original",std::to_integer<unsigned>(bank.after[n])},{"native",std::to_integer<unsigned>(actual[n])}});}}}
 const bool fp_equal=fp_words(expected_fp)==J::array({fp.control_word,fp.status,fp.mxcsr}),valid_equal=expected_valid==native.read(args[0],1),contact_equal=expected_contact==native.read(args[4],80),data_equal=expected_data==native.read(scratch+0x18u,0x58u);
 J data_diffs=J::array();const auto data=native.read(scratch+0x18u,0x58u);for(unsigned n=0;n<data.size();++n)if(data[n]!=expected_data[n])data_diffs.push_back(J{{"offset",n+0x18u},{"native",std::to_integer<unsigned>(data[n])},{"original",std::to_integer<unsigned>(expected_data[n])}});
 J ledger_diffs=J::array();for(std::size_t n=0;n<std::min(expected_ledger.size(),memory.ledger.size());++n)if(expected_ledger[n]!=memory.ledger[n]){ledger_diffs.push_back(J{{"index",n},{"original",expected_ledger[n]},{"native",memory.ledger[n]}});if(ledger_diffs.size()==8)break;}
 report.update(J{{"passed",different==0&&fp_equal&&valid_equal&&contact_equal&&data_equal&&result==args[0]&&expected_ledger==memory.ledger&&expected_stores==native_stores},{"ordered_query_operands_equal",expected_ledger==memory.ledger},{"ordered_query_stores_equal",expected_stores==native_stores},{"original_operand_events",expected_ledger.size()},{"native_operand_events",memory.ledger.size()},{"first_operand_differences",ledger_diffs},{"original_stores",expected_stores},{"native_stores",native_stores},{"original_parent_instructions",instructions},{"ordinal",ordinal},{"declared_variant",variation},{"declared_input_patches",patches},{"persistent_compared_bytes",compared},{"different_bytes",different},{"first_differences",diffs},{"fp_equal",fp_equal},{"valid_result_equal",valid_equal},{"contact_packet_equal",contact_equal},{"retained_query_data_equal",data_equal},{"query_data_differences",data_diffs},{"before_fp",fp_words(before_fp)},{"after_original_fp",fp_words(expected_fp)},{"after_native_fp",J::array({fp.control_word,fp.status,fp.mxcsr})},{"query_calls",stats.calls},{"motion_vectors",stats.motion_vectors},{"cache_refreshes",stats.cache_refreshes},{"convex_queries",stats.convex_queries},{"classifications",stats.classifications},{"actual_original_services",bridge.calls},{"excluded_private_control_data",J::array({scratch,scratch+0x18u})},{"source_entry",0x2e4fe0u}});
#if defined(HALO2_NATIVE_QUERY_CACHE)
 report["native_cache_parents"]=native_cache_parents;report["actual_cache_dependency_calls"]=cache_calls;
#endif
#if defined(HALO2_NATIVE_QUERY_CONVEX)
 report["native_convex_parents"]=native_convex_parents;report["actual_original_services"]=simplex_calls;
#endif
 }catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}
 if(argc>=7){std::ofstream output(argv[6]);output<<report.dump(2)<<'\n';}
 std::cout<<J{{"passed",report.at("passed")},{"different_bytes",report.value("different_bytes",0u)},{"fp_equal",report.value("fp_equal",false)},{"query_data_equal",report.value("retained_query_data_equal",false)},{"error",report.value("error",std::string{})}}.dump()<<'\n';return report.at("passed").get<bool>()?0:1;
}
