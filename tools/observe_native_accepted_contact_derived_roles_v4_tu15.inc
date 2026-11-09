 {
  J alias_results=J::array();constexpr std::uint32_t base=0x600e0000u,evt=base+0x5030u;
  const auto&event_fields=capture.at("event_fields");const auto first_payload=event_fields.at("first_payload").get<std::uint32_t>();
  const auto manager=event_fields.at("manager").get<std::uint32_t>();
  check(typed.at("event")==evt,"Actual moving resource identity differs");
  const std::array<const char*,10>names{"ancestor_event_alias","motion_weight_alias","property_list_weight_alias","shape_material_alias","component_weight_alias","returned_point_alias","returned_properties_alias","returned_allocation_crossing","row_destination_alias","lookup_list_request_alias"};
  for(unsigned mode=0;mode<names.size();++mode){
   Child native;start(native,argv[1],typed,0,directory);check(native.word(first_payload+0xcu)==0u,"Actual first payload ancestry changed");
   const auto body=native.word(first_payload+0x20u);check(body==event_fields.at("current_body"),"Actual first body differs");
   const auto count=native.word(body+0x34u),properties=native.word(body+0x30u);check(count&&count<64,"Actual body property count differs");
   std::uint32_t datum=0xffffffffu;for(unsigned i=0;i<count;++i)if(native.word(properties+i*8u)==0x2001u){datum=native.word(properties+i*8u+4u);break;}
   check(datum!=0xffffffffu,"Actual component property missing");const auto component_pool=native.word(0x52be5cu),rows=native.word(component_pool+0x44u),component=rows+(datum&0xffffu)*160u;
   const auto header=component+0x88u;check(native.word(header+4u)==0u&&native.word(header+8u)==0x80000000u,"Actual row allocator domain differs");
   const auto point_id=event_fields.at("id").get<unsigned>();const auto index=std::to_integer<unsigned>(native.read(native.word(manager+0x30u)+point_id,1u)[0]);
   J changed_inputs=J::array();const auto put=[&](std::uint32_t at,std::uint32_t value){check(at<base||at>=base+0x8000u,"Declared control cannot write moving private backing");const std::array<std::uint32_t,1>bits{value};changed_inputs.push_back(J{{"address",at},{"genuine",native.word(at)},{"declared",value}});native.write(at,std::as_bytes(std::span(bits)));};
   switch(mode){
    case 0:put(first_payload+0xcu,evt-8u);break;
    case 1:put(body+0x3cu,base+0x6100u);break;
    case 2:put(body+0x30u,base+0x6100u);break;
    case 3:put(first_payload,base+0x6110u);break;
    case 4:put(component_pool+0x44u,base+0x6100u-4u-(datum&0xffffu)*160u);break;
    case 5:put(manager+0x44u,base+0x6100u-(index<<5u));break;
    case 6:put(manager+0xa0u,base+0x6100u-index*0x1cu);break;
    case 7:{const auto allocator=native.word(0x48c938u);check(native.word(native.word(allocator)+0x10u)==0x149da0u,"Actual array router differs");
      const auto pool=native.word(native.word(0x486064u)==1u?0x486068u:0x48606cu);check(native.word(native.word(pool)+0x10u)==0x233250u,"Actual native pool owner differs");
      const auto size_class=std::to_integer<unsigned>(native.read(pool+0xacu+72u,1u)[0]);check(size_class<16u,"Actual row size class differs");
      put(pool+0x24u+size_class*4u,base-4u);put(base-4u,0u);break;}
    case 8:put(header,base+0x6100u);put(header+8u,0x80000001u);break;
    case 9:{const auto list_pool=native.word(0x52d690u);check(list_pool!=0u,"Actual model lookup list owner missing");
      put(component+0x20u,0u);put(list_pool+0x44u,base+0x6130u+8u-2u);break;}
   }
   const auto before_header=native.read(header,12u),before_arena=native.read(base,0x8000u);Memory storage(native);
   const std::array<NativeTu15ContactPrivateRange,1>owned{{{base,0x8000u}}};NativeTu15ContactCandidatePrivateData view(storage,owned,{});
   view.write(evt,native.read(source_event,2u));view.write(evt+4u,native.read(source_event+4u,32u));
   const auto fields=native.fp();NativeTu15PhysicsFrameFp fp{fields.cwd,fields.swd,fields.mxcsr};NativeTu15PairDeletionNativeScope scope;
   const NativeTu15AcceptedBodyContactBindingScratch scratch{{base+0x6100u,base+0x6104u,base+0x6108u,base+0x610cu},
    {base+0x6110u,base+0x6112u,base+0x6114u,base+0x6118u,base+0x611cu,base+0x6120u,base+0x6124u,base+0x6128u,{base+0x6130u},{base+0x6170u,base+0x6180u}}};
   NativeTu15AcceptedBodyContactBindingActivity out;std::string refusal;
   try{native_tu15_notify_bound_accepted_body_contact(view,typed.at("listener"),evt,scratch,fp,0,scope,{},&out);}catch(const std::runtime_error&e){refusal=e.what();}
   check(!refusal.empty()&&!scope.active()&&out.refusals==1u&&!out.completed&&out.role_refusals==1u,"Derived role alias did not stop at metadata boundary");
   const auto&s=out.publication;
   if(mode<5)check(!s.row_appends&&!s.row_initializations&&!out.body_update.attempts,"Early input alias reached a row publication");
   if(mode==5||mode==6)check(s.row_appends==1u&&s.row_initializations==1u&&out.body_update.attempts==1u&&out.body_update.refusals==1u&&out.body_update.update.point_queries==1u&&out.body_update.update.property_queries==unsigned(mode==6),"Returned point/properties extent checked at wrong phase");
   if(mode==7)check(s.row_appends==1u&&!s.row_initializations&&out.allocation.pool_allocations==1u&&native.read(header,12u)==before_header,"Allocator return extent escaped before rejection");
   if(mode==8)check(s.row_appends==1u&&!s.row_initializations&&!out.allocation.pool_allocations&&native.word(header+4u)==1u,"Row destination rejection did not preserve count-store prefix");
   if(mode==9)check(s.row_initializations==1u&&out.body_update.completed==1u&&s.lookups==1u&&!s.lookup_completions,"Lookup list alias bypassed typed request distinction");
   check(native.read(base,0x8000u)==before_arena,"Derived alias touched private backing bytes");
   alias_results.push_back(J{{"name",names[mode]},{"passed",true},{"declared_native_only_before_changes",changed_inputs},{"role_refusals",out.role_refusals},{"row_append_requests",s.row_appends},{"row_initializations",s.row_initializations},{"lookup_requests",s.lookups},{"lookup_completions",s.lookup_completions},{"error",refusal},{"backing32KiB_unchanged",true}});
  }
  report["derived_role_controls"]=alias_results;
 }
