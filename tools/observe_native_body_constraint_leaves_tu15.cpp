// Appended to the physical bridge; actual BEFORE operands only.
#include <algorithm>
int main(int argc,char**argv){J report{{"passed",false},{"captured_after_consumed",false},{"instruction_patches",J::array()},{"native_parent_receives_guest_register_state",false},{"manufactured_constraints",false},{"query_data_projected_from_existing_instance",CONSTRAINT_LEAF==3}};try{
 check(argc==7,"Supply ELF,capture,island,FP mode,metadata,output");std::ifstream input(argv[2]),meta_input(argv[5]);const auto capture=J::parse(input),metadata=J::parse(meta_input);const auto selection=std::uint32_t(std::stoul(argv[3])),mode=std::uint32_t(std::stoul(argv[4]));check(mode<4,"Unqualified FP mode");std::vector<std::uint32_t>components;
 {Child source;source.start(argv[1],0);const auto world=source.word(0x52be48u),ordinal=selection&255u,bank=source.word(world+(selection&256u?0x14u:8u)),count=source.word(world+(selection&256u?0x18u:12u));check(ordinal<count,"Actual island unavailable");const auto island=source.word(bank+ordinal*4u),bodies=source.word(island+0x3cu),body_count=source.word(island+0x40u);check(body_count>0&&body_count<=64,"Actual body cohort unavailable");for(unsigned n=0;n<body_count;++n){const auto body=source.word(bodies+n*4u),properties=source.word(body+0x30u),property_count=source.word(body+0x34u);for(unsigned k=0;k<property_count;++k)if(source.word(properties+k*8u)==0x2001u){const auto datum=source.word(properties+k*8u+4u);if(datum!=0xffffffffu){const auto manager=source.word(0x52be5cu),component=source.word(manager+0x44u)+(datum&0xffffu)*160u;if(std::find(components.begin(),components.end(),component)==components.end())components.push_back(component);}break;}}}
 check(!components.empty(),"Actual components unavailable");const auto actual_sp=capture.at("before").at("gp").at(4).get<std::uint32_t>(),private_first=capture.contains("private_returned_stack")?capture.at("private_returned_stack").at("first").get<std::uint32_t>():((actual_sp-1u)&~0xffffu),private_end=actual_sp,sp=actual_sp-0x1000u;check(sp>=private_first+0x2000u,"Owned service storage unavailable");J cases=J::array();
 for(const auto component:components){unsigned slots{};{Child source;source.start(argv[1],0);
#if CONSTRAINT_LEAF == 3 || CONSTRAINT_LEAF == 5 || CONSTRAINT_LEAF == 6 || CONSTRAINT_LEAF == 7 || CONSTRAINT_LEAF == 8
 const auto list=source.word(component+0x20u);if(list==0xffffffffu)slots=CONSTRAINT_LEAF>=5?0u:1u;else{const auto pool=source.word(0x52d690u),entries=source.word(pool+0x44u);const auto count=std::bit_cast<std::int16_t>(std::uint16_t(source.word(entries+(list&0xffffu)*64u+2u)));check(count>=0,"Actual list has negative count");slots=CONSTRAINT_LEAF>=5?unsigned(count):std::max(1u,unsigned(count));}
#elif CONSTRAINT_LEAF == 4
 slots=1;
#else
 slots=source.word(component+0x74u);
#endif
}if(CONSTRAINT_LEAF>=5&&!slots)continue;check(slots>0&&slots<=64,"Unbounded component slots");for(unsigned slot=0;slot<slots;++slot){
  Child original,native;original.start(argv[1],0);native.start(argv[1],0);auto before=original.fp();check(before.ftw==0,"Actual input requires empty x87");if(mode){before.cwd=std::uint16_t((before.cwd&~0xc00u)|(mode<<10u));before.mxcsr=(before.mxcsr&~0x6000u)|(mode<<13u);}request(PTRACE_SETFPREGS,original.pid,nullptr,&before);const auto put=[&](auto p,auto value){original.write(p,{reinterpret_cast<const std::byte*>(&value),4});};put(sp,0x1ccacau);auto entry=original.regs();entry.rsp=sp;std::uint32_t position{};
#if CONSTRAINT_LEAF == 1
  entry.rip=0x1d8b90u;entry.rax=slot;entry.rcx=component;entry.rsi=sp-0x400u;
#elif CONSTRAINT_LEAF == 2
  position=original.word(original.word(original.word(component+0x70u)+slot*0x60u+0x40u)+0x3cu)+0xb0u;entry.rip=0x230f60u;entry.rbx=position;put(sp+4u,std::uint32_t(1));
#elif CONSTRAINT_LEAF == 3
  const auto list=original.word(component+0x20u);if(list!=0xffffffffu){const auto pool=original.word(0x52d690u),entries=original.word(pool+0x44u),list_row=entries+(list&0xffffu)*64u;const auto count=std::bit_cast<std::int16_t>(std::uint16_t(original.word(list_row+2u)));if(count>0){
   const auto candidate=original.word(list_row+slot*4u+4u),instances=original.word(original.word(0x52d68cu)+0x44u),instance=instances+(candidate&0xffffu)*160u,query=sp-0x400u;
   // A query projected from a genuine existing descriptor is ordinary DATA.
   // No instance/contact row is changed and no reference result is supplied.
   const auto project=[&](std::uint32_t offset,std::uint32_t from,unsigned width){const auto bytes=original.read(instance+from,width);original.write(query+offset,bytes);native.write(query+offset,bytes);};
   project(4u,0x24u,4);project(0x10u,0x28u,4);project(0xcu,0x2cu,2);project(0x18u,0x2eu,2);project(0x38u,0xeu,1);project(0x3au,0x9cu,4);
   const auto kind=std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(original.word(instance+0xdu)))));original.write(query+0x34u,{reinterpret_cast<const std::byte*>(&kind),4});native.write(query+0x34u,{reinterpret_cast<const std::byte*>(&kind),4});
   project(0x1cu,0x68u,12);project(0x28u,0x5cu,12);
  }}
  entry.rip=0x1dd710u;entry.rax=component;entry.rcx=sp-0x400u;put(sp+4u,mode&1u);
#elif CONSTRAINT_LEAF == 4
  const auto list=original.word(component+0x20u);position=original.word(0x455dc4u);
  if(list!=0xffffffffu){const auto entries=original.word(original.word(0x52d690u)+0x44u),row=entries+(list&0xffffu)*64u;if(std::bit_cast<std::int16_t>(std::uint16_t(original.word(row+2u)))>0){const auto datum=original.word(row+4u),instances=original.word(original.word(0x52d68cu)+0x44u);position=original.word(instances+(datum&0xffffu)*160u+4u);}}
  before.xmm_space[0]=position;request(PTRACE_SETFPREGS,original.pid,nullptr,&before);entry.rip=0x1dd7b0u;entry.rcx=(mode&1u)?0u:0xffffffffu;put(sp+4u,component);
#elif CONSTRAINT_LEAF == 5
  const auto list=original.word(component+0x20u);check(list!=0xffffffffu,"Actual body-slot input needs an existing instance list");const auto entries=original.word(original.word(0x52d690u)+0x44u),row=entries+(list&0xffffu)*64u;check(std::bit_cast<std::int16_t>(std::uint16_t(original.word(row+2u)))>0,"Actual body-slot input needs an existing instance");
  const auto datum=original.word(row+slot*4u+4u),instance=original.word(original.word(0x52d68cu)+0x44u)+(datum&0xffffu)*160u;position=instance;entry.rip=0x22e390u;put(sp+4u,instance);put(sp+8u,datum);put(sp+12u,sp-0x400u);put(sp+16u,sp-0x3fcu);
#elif CONSTRAINT_LEAF == 6
  const auto list=original.word(component+0x20u),entries=original.word(original.word(0x52d690u)+0x44u),datum=original.word(entries+(list&0xffffu)*64u+slot*4u+4u),instance=original.word(original.word(0x52d68cu)+0x44u)+(datum&0xffffu)*160u;position=instance;entry.rip=0x22ede0u;entry.rax=original.word(instance+0x24u);entry.rcx=original.word(instance+0x28u);
#elif CONSTRAINT_LEAF == 7
  const auto list=original.word(component+0x20u),entries=original.word(original.word(0x52d690u)+0x44u),datum=original.word(entries+(list&0xffffu)*64u+slot*4u+4u),instance=original.word(original.word(0x52d68cu)+0x44u)+(datum&0xffffu)*160u;position=instance;entry.rip=0x22fe30u;entry.rcx=instance;before.xmm_space[0]=original.word(instance+0x14u);request(PTRACE_SETFPREGS,original.pid,nullptr,&before);
#else
  const auto list=original.word(component+0x20u),entries=original.word(original.word(0x52d690u)+0x44u),datum=original.word(entries+(list&0xffffu)*64u+slot*4u+4u),instance=original.word(original.word(0x52d68cu)+0x44u)+(datum&0xffffu)*160u;position=instance;entry.rip=0x22e1b0u;entry.rax=instance;entry.rsi=datum;
#endif
  request(PTRACE_SETREGS,original.pid,nullptr,&entry);ActivationMemory memory(native,private_first,private_end);J calls=J::array();std::size_t steps{};auto ledger=activation_trace(original,metadata,memory,calls,steps);NativeTu15PhysicsFrameFp fp{before.cwd,before.swd,before.mxcsr};NativeTu15ModelConstraintUpdateStats stats;NativeTu15ModelConstraintLookupStats lookup_stats;NativeTu15ModelConstraintCapacityStats capacity_stats;NativeTu15ConstraintBodySlotStats slot_stats;NativeTu15ConstraintMotionCompareStats motion_stats;NativeTu15ConstraintScalarCacheStats cache_stats;NativeTu15ConstraintReferenceReleaseStats release_stats;bool refused{},values{};
#if CONSTRAINT_LEAF == 1
  const auto frame=native_tu15_model_constraint_body_frame(memory,{component},slot,fp,&stats);const auto bytes=original.read(sp-0x400u,sizeof(frame));values=std::memcmp(bytes.data(),&frame,sizeof(frame))==0;
#elif CONSTRAINT_LEAF == 2
  const auto result=native_tu15_model_constraint_reference_distance(memory,position,1u,fp,&stats);values=result==original.fp().xmm_space[0];
#elif CONSTRAINT_LEAF == 3
  const auto result=native_tu15_find_model_constraint(memory,{component},{sp-0x400u},mode&1u,fp,&lookup_stats);values=result==std::uint32_t(original.regs().rax);
#elif CONSTRAINT_LEAF == 4
  const auto result=native_tu15_admit_model_constraint(memory,{component},(mode&1u)?0u:0xffffffffu,position,fp,{}, {},&capacity_stats);values=result==bool(original.regs().rax&255u);
#elif CONSTRAINT_LEAF == 5
  const auto result=native_tu15_constraint_body_slots(memory,position,original.word(sp+8u),&slot_stats);values=result.first==original.word(sp-0x400u)&&result.second==original.word(sp-0x3fcu);
#elif CONSTRAINT_LEAF == 6
  const auto result=native_tu15_second_constraint_body_moves_faster(memory,std::uint32_t(entry.rax),std::uint32_t(entry.rcx),fp,&motion_stats);values=result==bool(original.regs().rax&255u);
#elif CONSTRAINT_LEAF == 7
  native_tu15_update_constraint_cached_scalar(memory,position,before.xmm_space[0],fp,0u,{},&cache_stats);values=true;
#else
  try{native_tu15_release_constraint_reference(memory,position,std::uint32_t(entry.rsi),fp,0u,{}, {},&release_stats);}catch(const std::runtime_error&){if(!release_stats.unbound_runtime_removals)throw;refused=true;}values=refused==(std::uint32_t(original.regs().rip)==0x22e1fcu);
#endif
#if CONSTRAINT_LEAF == 7
  // Repeating the same genuine source operation exercises its enabled-cache
  // comparison using only the first operation's own publications.
  auto repeated=original.regs();repeated.rip=0x22fe30u;repeated.rsp=sp;repeated.rcx=position;request(PTRACE_SETREGS,original.pid,nullptr,&repeated);const auto second_ledger=activation_trace(original,metadata,memory,calls,steps);for(const auto&event:second_ledger)ledger.push_back(event);
  native_tu15_update_constraint_cached_scalar(memory,position,before.xmm_space[0],fp,0u,{},&cache_stats);
#endif
  std::size_t compared{},different{};for(const auto&bank:capture.at("regions")){const auto address=bank.at("address").get<std::uint32_t>(),size=bank.at("size").get<std::uint32_t>();const auto a=original.read(address,size),b=native.read(address,size);for(unsigned n=0;n<size;++n){const auto p=address+n;if(p>=private_first&&p<private_end)continue;++compared;if(a[n]!=b[n])++different;}}
  const bool ordered=ledger==memory.ledger,fp_equal=fp_words(original.fp())==J::array({fp.control_word,fp.status,fp.mxcsr}),ret_equal=refused||std::uint32_t(original.regs().rsp)==sp+((CONSTRAINT_LEAF==1||CONSTRAINT_LEAF==6||CONSTRAINT_LEAF==7||CONSTRAINT_LEAF==8)?4u:CONSTRAINT_LEAF==5?20u:8u);J differences=J::array();for(std::size_t n=0;n<std::min(ledger.size(),memory.ledger.size());++n)if(ledger[n]!=memory.ledger[n]&&differences.size()<8)differences.push_back(J{{"index",n},{"original",ledger[n]},{"native",memory.ledger[n]}});
  cases.push_back(J{{"passed",ordered&&fp_equal&&ret_equal&&different==0&&values},{"component",component},{"slot",slot},{"ordered_operands_equal",ordered},{"first_operand_differences",differences},{"values_equal",values},{"fp_equal",fp_equal},{"source_return_equal",ret_equal},{"persistent_compared_bytes",compared},{"different_bytes",different},{"source_operand_events",ledger.size()},{"native_operand_events",memory.ledger.size()},{"source_instructions",steps},{"frame_calls",stats.body_frame_calls},{"frame_compositions",stats.frame_compositions},{"distance_calls",stats.reference_distance_calls},{"distance_points",stats.reference_distance_points},{"lookup_calls",lookup_stats.calls},{"lookup_candidates",lookup_stats.candidates},{"lookup_key_matches",lookup_stats.key_matches},{"lookup_pose_queries",lookup_stats.pose_queries},{"lookup_body_visibility_queries",lookup_stats.body_visibility_queries},{"lookup_found",lookup_stats.found},{"capacity_calls",capacity_stats.calls},{"capacity_slot_refusals",capacity_stats.slot_refusals},{"capacity_local_candidates",capacity_stats.local_candidates},{"capacity_stack_queries",capacity_stats.stack_queries},{"capacity_local_removals",capacity_stats.local_removals},{"capacity_global_removals",capacity_stats.global_removals},{"capacity_refusals",capacity_stats.refusals},{"capacity_unbound_removals",capacity_stats.unbound_removals},{"slot_queries",slot_stats.calls},{"slot_model_queries",slot_stats.model_queries},{"slot_primary_queries",slot_stats.primary_queries},{"slot_child_rows",slot_stats.child_rows},{"slot_matching_children",slot_stats.matching_children},{"motion_compare_calls",motion_stats.calls},{"motion_compare_missing_second",motion_stats.missing_second},{"motion_compare_comparisons",motion_stats.comparisons},{"cache_calls",cache_stats.calls},{"cache_published",cache_stats.published},{"cache_already_retained",cache_stats.already_retained},{"cache_cooldown_checks",cache_stats.cooldown_checks},{"cache_cooldown_retained",cache_stats.cooldown_retained},{"release_calls",release_stats.calls},{"release_decrements",release_stats.decrements},{"release_unbound_runtime_removals",release_stats.unbound_runtime_removals},{"runtime_refusal_prefix",refused}});
 }}check(!cases.empty(),"No genuine eligible instance in selected cohort");bool passed=true;for(const auto&row:cases)passed&=row.at("passed").get<bool>();report.update(J{{"passed",passed},{"selection",selection},{"declared_FP_mode",mode},{"leaf",CONSTRAINT_LEAF},{"cases",cases},{"private_returned_storage",J::array({private_first,private_end})}});
 }catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}if(argc>=7){std::ofstream out(argv[6]);out<<report.dump(2)<<'\n';}std::cout<<J{{"passed",report["passed"]},{"error",report.value("error",std::string{})}}.dump()<<'\n';return report.at("passed").get<bool>()?0:1;}
