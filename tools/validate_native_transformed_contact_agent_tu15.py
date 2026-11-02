#!/usr/bin/env python3
"""Complete native2F7190 against unchanged physical TU1.5 BEFORE inputs.

The source input graph and aligned local shape transform are declared inputs.
Each side executes genuine allocation, classifications and nested construction;
native affine composition uses its independently qualified2E44D0 owner. No
expected output, pointer, classification or mutable postimage is supplied.
"""
from pathlib import Path
from validate_native_wrapped_contact_agent_tu15 import main as validate


DRIVER = r'''
int main(int argc,char**argv){J report{{"passed",false},{"instruction_patches",J::array()},{"supplied_original_outcomes",false}};try{
 check(argc==6,"Supply ELF, capture, producer ordinal, reversal and output");std::ifstream in(argv[2]);const auto capture=J::parse(in);const unsigned ordinal=std::stoul(argv[3]);const bool reverse=std::stoul(argv[4]);
 Child original;original.start(argv[1],ordinal);const auto entry=original.regs();const auto before=original.fp();check(before.ftw==0,"Transformed factory requires declared empty incoming x87");const auto source_sp=std::uint32_t(entry.rsp),packet=((source_sp-4u)&~15u)-0x600u;
 auto a=original.word(source_sp+4),b=original.word(source_sp+8);if(reverse)std::swap(a,b);const auto context=original.word(source_sp+12),user_data=original.word(std::uint32_t(entry.rcx)+8u);
 // Source inputs and retained native DATA are ABOVE the original/service
 // stacks, not in any callee's downward-growing active control frame.
 const auto wrapper_shape=packet+0x100u,wrapper_payload=packet+0x80u,native_payload=packet+0x200u,native_geometry=packet+0x240u;
 const std::array<std::uint32_t,20> shape{0xabcde123u,0x88776655u,0x11223344u,original.word(a),0x3f800000u,0,0,0,0,0x3f800000u,0,0,0,0,0x3f800000u,0,0x40000000u,0x40400000u,0x40800000u,0};
 const std::array<std::uint32_t,4> payload{wrapper_shape,original.word(a+4u),original.word(a+8u),0xaabbccddu};
 original.write(wrapper_shape,{reinterpret_cast<const std::byte*>(shape.data()),sizeof(shape)});original.write(wrapper_payload,{reinterpret_cast<const std::byte*>(payload.data()),sizeof(payload)});a=wrapper_payload;
 struct Bank{std::uint32_t address;std::vector<std::byte>before,after;};std::vector<Bank>banks;for(const auto&r:capture.at("regions")){const auto address=r.at("address").get<std::uint32_t>();banks.push_back({address,original.read(address,r.at("size")),{}});}
 const std::array<std::uint32_t,5> call_words{0x1ccaca,a,b,context,user_data};original.write(packet-20u,{reinterpret_cast<const std::byte*>(call_words.data()),sizeof(call_words)});auto full_entry=entry;full_entry.rip=0x2f7190;full_entry.rcx=0;full_entry.rsp=packet-20u;request(PTRACE_SETREGS,original.pid,nullptr,&full_entry);
 J actual_reads=J::array(),actual_stores=J::array(),actual_calls=J::array();std::size_t original_instructions{};std::uint32_t original_payload{},original_geometry{};
 auto bytes=[](const auto& input){J result=J::array();for(auto b:input)result.push_back(std::to_integer<unsigned>(b));return result;};
 auto normalized=[](std::uint32_t p,std::uint32_t payload,std::uint32_t geometry){if(payload&&p>=payload&&p<payload+16u)return 0xffff0000u+p-payload;if(geometry&&p>=geometry&&p<geometry+96u)return 0xffff1000u+p-geometry;return p;};
 auto payload_bytes=[&](Child&child,std::uint32_t payload,std::uint32_t geometry){auto data=child.read(payload,16);const auto value=normalized(child.word(payload+8u),payload,geometry);for(unsigned n=0;n<4;++n)data[8+n]=std::byte(value>>(n*8u));return bytes(data);};
 auto store_bytes=[&](Child&child,std::uint32_t address,unsigned width,std::uint32_t payload,std::uint32_t geometry){auto data=child.read(address,width);if(address==payload+8u&&width==4){const auto value=normalized(child.word(address),payload,geometry);for(unsigned n=0;n<4;++n)data[n]=std::byte(value>>(n*8u));}return bytes(data);};
 for(;;){const auto r=original.regs();const auto pc=std::uint32_t(r.rip);if(pc==0x1ccaca)break;check(++original_instructions<256,"Transformed factory left complete authenticated owners");std::uint32_t address{};unsigned width=4;bool read=false,write=false,call=false;std::uint32_t target{},receiver{},return_pc{};J args=J::array();
  switch(pc){
   case 0x2f71b1:address=0x48c938;read=true;break;case 0x2f71b7:address=r.rcx;read=true;break;
   case 0x2f71c0:address=r.rax+16u;read=true;call=true;target=original.word(address);receiver=r.rcx;return_pc=0x2f71c3;args=J::array({original.word(r.rsp),original.word(r.rsp+4u)});break;
   case 0x2f71c5:address=r.rsi+4u;width=2;write=true;break;
   case 0x2f71dd:address=r.rsi+8u;write=true;break;case 0x2f71e3:address=r.rsi;write=true;break;
   case 0x2f71e9:address=r.rdi+8u;read=true;break;case 0x2f71ec:address=r.rax;width=16;read=true;break;
   case 0x2f71ef:address=r.rdi;read=true;break;case 0x2f71f1:address=r.rcx+12u;read=true;break;
   case 0x2f71f4:original_geometry=r.rsp+0x30u;address=original_geometry;width=16;write=true;break;
   case 0x2f71f9:address=r.rax+16u;width=16;read=true;break;case 0x2f71fd:address=r.rsp+0x40u;width=16;write=true;break;
   case 0x2f7202:address=r.rax+32u;width=16;read=true;break;case 0x2f7206:address=r.rsp+0x50u;width=16;write=true;break;
   case 0x2f720b:address=r.rax+48u;width=16;read=true;break;case 0x2f720f:address=r.rsp+0x60u;width=16;write=true;break;
   case 0x2f7214:address=r.rax+64u;width=16;read=true;break;case 0x2f721b:address=r.rsp+0x70u;width=16;write=true;break;
   case 0x2f7220:address=r.rax+80u;width=16;read=true;break;case 0x2f7235:address=r.rsp+0x88u;width=16;write=true;break;
   // Follow the genuine complete2E44D0 body, including every SIMD operand
   // reread and in-place publication. It is never intercepted as an outcome.
   case 0x2e44f0:address=r.rax;width=16;read=true;break;case 0x2e44f3:address=r.rdx+32u;width=16;read=true;break;case 0x2e44f7:address=r.rdx+16u;width=16;read=true;break;case 0x2e4516:address=r.rdx;width=16;read=true;break;
   case 0x2e4522:address=std::uint32_t(r.rsi+r.rax);width=16;write=true;break;case 0x2e452c:address=r.rdx+48u;width=16;read=true;break;case 0x2e4530:address=r.rcx+48u;width=16;read=true;break;case 0x2e4538:address=r.rcx+48u;width=16;write=true;break;
   case 0x2f7249:original_payload=r.rsp+0x20u;address=original_payload+8u;write=true;break;
   case 0x2f724d:address=r.rcx;read=true;break;case 0x2f724f:address=original_payload+12u;write=true;break;case 0x2f7253:address=r.rdi+4u;read=true;break;
   case 0x2f7256:address=original_payload+4u;write=true;break;case 0x2f725a:address=original_payload;write=true;break;case 0x2f725e:address=r.rbx;read=true;break;
   case 0x2f7266:address=r.rax+20u;read=true;call=true;target=original.word(address);receiver=r.rcx;return_pc=0x2f7269;break;
   case 0x2f726c:address=r.rbx;read=true;break;case 0x2f726e:address=r.rcx;read=true;break;
   case 0x2f7272:address=r.rdx+20u;read=true;call=true;target=original.word(address);receiver=r.rcx;return_pc=0x2f7275;break;
   case 0x2f7284:address=std::uint32_t(r.rax+r.rdi*4u+0x18cu);read=true;break;
   case 0x2f7293:call=true;target=r.rdi;return_pc=0x2f7295;args=J::array({original.word(r.rsp),original.word(r.rsp+4u),original.word(r.rsp+8u),original.word(r.rsp+12u)});report["original_child_payload_bytes"]=payload_bytes(original,original_payload,original_geometry);report["original_composed_geometry_bytes"]=bytes(original.read(original_geometry,96));break;
   case 0x2f729f:address=r.rsi+12u;write=true;break;
  }
  if(read)actual_reads.push_back(J::array({normalized(address,original_payload,original_geometry),width}));
  if(call){const auto f=original.fp();original.step();original.go(return_pc);auto values=args;if(pc==0x2f7293)values[0]=normalized(values[0].get<std::uint32_t>(),original_payload,original_geometry);actual_calls.push_back(J{{"entry",target},{"this",receiver},{"arguments",values},{"returned",std::uint32_t(original.regs().rax)},{"before_fp",fp_words(f)},{"after_fp",fp_words(original.fp())}});}else original.step();
  if(write)actual_stores.push_back(J::array({normalized(address,original_payload,original_geometry),store_bytes(original,address,width,original_payload,original_geometry)}));
 }
 const auto returned=std::uint32_t(original.regs().rax);const auto completed_fp=original.fp();for(auto&bank:banks)bank.after=original.read(bank.address,bank.before.size());report["original_completed"]=true;
 Child native;native.start(argv[1],ordinal);for(const auto&bank:banks)native.write(bank.address,bank.before);auto installed=entry;request(PTRACE_SETREGS,native.pid,nullptr,&installed);auto initial_fp=before;request(PTRACE_SETFPREGS,native.pid,nullptr,&initial_fp);Memory memory(native);Bridge dependencies{native,packet-0x1000u,before,report};NativeTu15WrappedContactServices services;unsigned classifications{};
 services.allocation.allocate=[&](const auto& request,auto&fp){return dependencies.call(request.owner,request.receiver,{request.bytes,request.memory_class},0x2f71c3,fp);};
 services.allocation.release=[&](const auto& request,auto&fp){dependencies.call(request.owner,request.receiver,{request.data,request.bytes,request.memory_class},0x1ccaca,fp);};
 services.dispatch.classify_shape=[&](auto shape,auto owner,auto&fp){return dependencies.call(owner.identity,shape.identity,{},classifications++==0?0x2f7269u:0x2f7275u,fp);};
 services.dispatch.create_agent=[&](auto owner,auto first,auto second,auto settings,std::uint32_t data,auto&fp){report["native_child_payload_bytes"]=payload_bytes(native,first.identity,native_geometry);report["native_composed_geometry_bytes"]=bytes(native.read(native_geometry,96));return NativeTu15ContactAgent{dependencies.call(owner.identity,0,{first.identity,second.identity,settings.identity,data},0x2f7295,fp)};};
 NativeTu15PhysicsFrameFp fp{before.cwd,before.swd,before.mxcsr};const NativeTu15WrappedContactArguments arguments{[&]{return a;},[&]{return b;},[&]{return context;},[&]{return user_data;}};const auto result=native_tu15_create_transformed_contact_agent(memory,arguments,{native_payload,native_geometry},fp,services);
 J native_reads=J::array(),native_stores=J::array(),native_calls=J::array();for(const auto&read:memory.ledger)native_reads.push_back(J::array({normalized(read[0].get<std::uint32_t>(),native_payload,native_geometry),read[1]}));
 for(const auto&store:memory.stores){const auto address=store[0].get<std::uint32_t>();auto data=store[1];if(address==native_payload+8u&&data.size()==4){const auto value=normalized(native_geometry,native_payload,native_geometry);for(unsigned n=0;n<4;++n)data[n]=(value>>(n*8u))&255u;}native_stores.push_back(J::array({normalized(address,native_payload,native_geometry),data}));}
 for(const auto&call:dependencies.calls){auto args=call.at("arguments");if(args.size()==4)args[0]=normalized(args[0].get<std::uint32_t>(),native_payload,native_geometry);native_calls.push_back(J{{"entry",call.at("entry")},{"this",call.at("this")},{"arguments",args},{"returned",call.at("returned_eax")},{"before_fp",call.at("before_fp")},{"after_fp",call.at("after_fp")}});}
 const auto private_first=capture.at("private_returned_stack").at("first").get<std::uint32_t>(),private_end=capture.at("private_returned_stack").at("end").get<std::uint32_t>();check(packet-0x2000u>=private_first&&native_geometry+96u<private_end,"Declared scratch must fit existing private DATA lifetime");std::size_t compared{},different{},private_different{};J diffs=J::array();for(const auto&bank:banks){const auto actual=native.read(bank.address,bank.after.size());for(std::size_t n=0;n<actual.size();++n){const auto address=bank.address+n;const bool is_private=address>=private_first&&address<private_end;if(!is_private)++compared;if(actual[n]==bank.after[n])continue;if(is_private){++private_different;continue;}++different;if(diffs.size()<32)diffs.push_back(J{{"address",address},{"native",std::to_integer<unsigned>(actual[n])},{"original",std::to_integer<unsigned>(bank.after[n])}});}}
 const bool reads_equal=native_reads==actual_reads,stores_equal=native_stores==actual_stores,calls_equal=native_calls==actual_calls,fp_equal=J::array({fp.control_word,fp.status,fp.mxcsr})==fp_words(completed_fp),payload_equal=report["native_child_payload_bytes"]==report["original_child_payload_bytes"],geometry_equal=report["native_composed_geometry_bytes"]==report["original_composed_geometry_bytes"],input_unchanged=original.read(wrapper_shape,sizeof(shape))==native.read(wrapper_shape,sizeof(shape))&&original.read(wrapper_payload,sizeof(payload))==native.read(wrapper_payload,sizeof(payload));
 report.update(J{{"passed",different==0&&fp_equal&&result.identity==returned&&reads_equal&&stores_equal&&calls_equal&&payload_equal&&geometry_equal&&input_unchanged},{"source_entries",J::array({0x2f7190,0x2e44d0})},{"producer_ordinal",ordinal},{"reversed_payload_inputs",reverse},{"returned_original_agent",returned},{"returned_native_agent",result.identity},{"fp_equal",fp_equal},{"ordered_reads_equal",reads_equal},{"ordered_stores_equal",stores_equal},{"service_calls_equal",calls_equal},{"child_payload_equal",payload_equal},{"composed_geometry_equal",geometry_equal},{"declared_input_graph_equal",input_unchanged},{"persistent_compared_bytes",compared},{"different_bytes",different},{"private_control_stack_different_bytes",private_different},{"first_differences",diffs},{"ordered_original_operand_reads",actual_reads},{"ordered_native_operand_reads",native_reads},{"ordered_original_stores",actual_stores},{"ordered_native_stores",native_stores},{"original_calls",actual_calls},{"native_calls",native_calls},{"original_owner_instructions",original_instructions},{"remaining_services_execute_original_cpu",true},{"production_native_only_closure",false}});
 }catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}if(argc>=6){std::ofstream out(argv[5]);out<<report.dump(2)<<'\n';}return report.at("passed").get<bool>()?0:1;}
'''


if __name__ == '__main__':
    raise SystemExit(validate(DRIVER, 'transformed', (
        ('wrong_child', 'm.word(shape+12u)', 'm.word(shape+8u)'),
        ('wrong_stride', '(first_class<<5u)+second_class', '(first_class<<4u)+second_class'),
        ('wrong_size', 'agent.identity+4u,16u,2', 'agent.identity+4u,20u,2'),
        ('missing_composition', 'native_tu15_compose_affine_geometry(geometry,fp);', '(void)geometry;'),
        ('wrong_extra', 'scratch.child_payload+8u,scratch.geometry', 'scratch.child_payload+8u,extra'),
    ), (Path(__file__),)))
