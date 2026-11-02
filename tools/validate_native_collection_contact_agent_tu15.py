#!/usr/bin/env python3
"""Complete collection factory2F81F0/2F7D20 with genuine original services.

Declare a list-shape input graph around the protected BEFORE payloads. Execute
the original count method, allocator, filter and child factories independently
on both current banks. Source reserve2E31E0 is compared through its complete
native composition; no result, class, pointer or mutable postimage is supplied.
"""
from pathlib import Path
from validate_native_wrapped_contact_agent_tu15 import main as validate


DRIVER = r'''
int main(int argc,char**argv){J report{{"passed",false},{"instruction_patches",J::array()},{"supplied_original_outcomes",false}};try{
 check(argc==6,"Supply ELF, capture, ordinal, declared count/reversal and output");std::ifstream in(argv[2]);const auto capture=J::parse(in);const unsigned ordinal=std::stoul(argv[3]),option=std::stoul(argv[4]);const bool reverse=option&1u,shared_group=option&0x1000u;const auto encoded_count=(option&0xfffu)>>1u;const std::uint32_t count=encoded_count==255u?0xffffffffu:encoded_count;
 Child original;original.start(argv[1],ordinal);const auto entry=original.regs();const auto before=original.fp();check(before.ftw==0,"Collection factory requires declared empty incoming x87");const auto source_sp=std::uint32_t(entry.rsp),packet=((source_sp-4u)&~15u)-0x1000u;
 auto a=original.word(source_sp+4),b=original.word(source_sp+8);if(reverse)std::swap(a,b);const auto context=original.word(source_sp+12),user_data=original.word(std::uint32_t(entry.rcx)+8u);
 const auto shape_address=packet+0x100u,payload_address=packet+0x80u,list_address=packet+0x200u,native_payload=packet+0x300u,native_filter=packet+0x320u;
 const std::array<std::uint32_t,8> shape{0x461de0u,0,0,list_address,count,0,0,0};
 // Preserve the complete root payload/header fields, including its real
 // parent chain, group and owning body. Only the declared shape is replaced.
 auto payload=original.read(b,0x24);for(unsigned n=0;n<4;++n)payload[n]=std::byte(shape_address>>(n*8u));
 std::array<std::uint32_t,18> children{};for(unsigned n=0;n<9;++n){children[n*2]=original.word(n&1u?b:a);children[n*2+1]=n&1u?0xffffffffu:0;}
 original.write(shape_address,{reinterpret_cast<const std::byte*>(shape.data()),sizeof(shape)});original.write(payload_address,payload);original.write(list_address,{reinterpret_cast<const std::byte*>(children.data()),sizeof(children)});b=payload_address;
 if(shared_group){auto root=[&](std::uint32_t p){for(unsigned n=0;n<32;++n){const auto next=original.word(p+12u);if(!next)return p;p=next;}throw std::runtime_error("Declared parent chain did not terminate");};const std::uint32_t group=0x12340001u;const auto data=std::span(reinterpret_cast<const std::byte*>(&group),4);original.write(root(a)+0x1cu,data);original.write(root(b)+0x1cu,data);}
 struct Bank{std::uint32_t address;std::vector<std::byte>before,after;};std::vector<Bank>banks;for(const auto&r:capture.at("regions")){const auto address=r.at("address").get<std::uint32_t>();banks.push_back({address,original.read(address,r.at("size")),{}});}
 const std::array<std::uint32_t,5> call_words{0x1ccaca,a,b,context,user_data};original.write(packet-20u,{reinterpret_cast<const std::byte*>(call_words.data()),sizeof(call_words)});auto full_entry=entry;full_entry.rip=0x2f81f0;full_entry.rcx=0;full_entry.rsp=packet-20u;request(PTRACE_SETREGS,original.pid,nullptr,&full_entry);
 J actual_reads=J::array(),actual_stores=J::array(),actual_calls=J::array();std::size_t original_instructions{};std::uint32_t original_payload{},original_filter{};unsigned accepted{},rejected{};
 auto bytes=[](const auto&input){J result=J::array();for(auto b:input)result.push_back(std::to_integer<unsigned>(b));return result;};
 auto normalized=[](std::uint32_t p,std::uint32_t payload,std::uint32_t result){if(payload&&p>=payload&&p<payload+16u)return 0xffff0000u+p-payload;if(result&&p==result)return 0xffff2000u;return p;};
 auto normalize_args=[&](J args,std::uint32_t payload,std::uint32_t result){if(args.size()==4||args.size()==6)args[0]=normalized(args[0].get<std::uint32_t>(),payload,result);return args;};
 for(;;){const auto r=original.regs();const auto pc=std::uint32_t(r.rip);if(pc==0x1ccaca)break;check(++original_instructions<4096,"Collection original instruction budget exhausted");std::uint32_t address{};unsigned width=4;bool read=false,write=false,call=false;std::uint32_t target{},receiver{},return_pc{};J args=J::array();
  switch(pc){
   case 0x2f8206:case 0x2e31e0:case 0x2e321f:address=0x48c938;read=true;break;
   case 0x2f820c:case 0x2e31e6:case 0x2e3225:address=r.rcx;read=true;break;
   case 0x2f8213:case 0x2e31fc:address=r.rax+16u;read=true;call=true;target=original.word(address);receiver=r.rcx;return_pc=pc==0x2f8213?0x2f8216:0x2e31ff;args=J::array({original.word(r.rsp),original.word(r.rsp+4u)});break;
   case 0x2f8218:address=r.rsi+4u;width=2;write=true;break;
   case 0x2f7d46:address=r.rbp+8u;write=true;break;case 0x2f7d4e:address=r.rbp;write=true;break;
   case 0x2f7d5c:address=r.rbx;write=true;break;case 0x2f7d5e:address=r.rbx+4u;write=true;break;case 0x2f7d61:address=r.rbx+8u;write=true;break;
   case 0x2f7d6c:address=r.rax+8u;read=true;break;case 0x2f7d6f:address=r.rax;read=true;break;
   case 0x2f7d71:original_payload=r.rsp+0x1cu;address=original_payload+12u;write=true;break;case 0x2f7d75:address=original_payload+8u;write=true;break;
   case 0x2f7d79:address=r.rsi;read=true;break;
   case 0x2f7d86:address=r.rax+40u;read=true;call=true;target=original.word(address);receiver=r.rcx;return_pc=0x2f7d89;break;
   case 0x2f7d8b:address=r.rbx+8u;read=true;break;
   case 0x2e3203:case 0x2e3232:address=r.rsi;read=true;break;case 0x2e3207:address=r.rsi+4u;read=true;break;case 0x2e3215:address=r.rsi+8u;read=true;break;
   case 0x2e3210:check(original.word(r.rsp+8u)==0,"Declared construction reserve must copy its genuine initial zero count");original.step();original.go(0x2e3215);continue;
   case 0x2e3235:address=r.rdx+20u;read=true;call=true;target=original.word(address);receiver=r.rcx;return_pc=0x2e3238;args=J::array({original.word(r.rsp),original.word(r.rsp+4u),original.word(r.rsp+8u)});break;
   case 0x2e3238:address=r.rsi;write=true;break;case 0x2e323b:address=r.rsi+8u;write=true;break;
   case 0x2f7dab:address=r.rbx+4u;write=true;break;case 0x2f7dae:address=r.rbp+16u;read=true;break;
   case 0x2f7dc0:address=r.rsi+12u;read=true;break;case 0x2f7dc3:address=std::uint32_t(r.rcx+r.rdi*8u);read=true;break;case 0x2f7dc6:address=r.rbp+4u;read=true;break;
   case 0x2f7dca:address=r.rsp+0x20u;write=true;break;case 0x2f7dde:address=r.rsp+0x34u;write=true;break;
   case 0x2f7de2:address=r.rcx;read=true;break;
   case 0x2f7de5:address=r.rax;read=true;call=true;target=original.word(address);receiver=r.rcx;return_pc=0x2f7de7;for(unsigned n=0;n<6;++n)args.push_back(original.word(r.rsp+n*4u));original_filter=args[0];break;
   case 0x2f7de7:address=r.rax;width=1;read=true;if(original.read(address,1)[0]!=std::byte{})++accepted;else ++rejected;break;
   case 0x2f7dec:address=r.rsp+0x1cu;read=true;break;case 0x2f7df0:address=r.rbp;read=true;break;case 0x2f7df3:address=r.rcx;read=true;break;
   case 0x2f7df9:address=r.rdx+20u;read=true;call=true;target=original.word(address);receiver=r.rcx;return_pc=0x2f7dfc;break;
   case 0x2f7e02:address=r.rax;read=true;break;case 0x2f7e04:address=r.rcx;read=true;break;
   case 0x2f7e06:address=r.rdx+20u;read=true;call=true;target=original.word(address);receiver=r.rcx;return_pc=0x2f7e09;break;
   case 0x2f7e09:case 0x2f7e3b:address=r.rbx;read=true;break;
   case 0x2f7e18:address=std::uint32_t(r.rax+r.rsi*4u+0x18cu);read=true;break;
   case 0x2f7e2e:call=true;target=r.rax;return_pc=0x2f7e30;for(unsigned n=0;n<4;++n)args.push_back(original.word(r.rsp+n*4u));break;
   case 0x2f7e30:address=r.rsi;write=true;break;
   case 0x2f7e44:original.step();original.go(0x2f7e49);continue;
   case 0x2f7e4d:address=r.rdx;write=true;break;case 0x2f7e53:address=r.rax+16u;read=true;break;
   case 0x2f8249:address=r.rsi;write=true;break;
  }
  if(read)actual_reads.push_back(J::array({normalized(address,original_payload,original_filter),width}));
  if(call){const auto f=original.fp();original.step();original.go(return_pc);actual_calls.push_back(J{{"entry",target},{"this",receiver},{"arguments",normalize_args(args,original_payload,original_filter)},{"returned",normalized(original.regs().rax,original_payload,original_filter)},{"before_fp",fp_words(f)},{"after_fp",fp_words(original.fp())}});}else original.step();
  if(write)actual_stores.push_back(J::array({normalized(address,original_payload,original_filter),bytes(original.read(address,width))}));
 }
 const auto returned=std::uint32_t(original.regs().rax);const auto completed_fp=original.fp();for(auto&bank:banks)bank.after=original.read(bank.address,bank.before.size());report["original_completed"]=true;
 Child native;native.start(argv[1],ordinal);for(const auto&bank:banks)native.write(bank.address,bank.before);auto installed=entry;request(PTRACE_SETREGS,native.pid,nullptr,&installed);auto initial_fp=before;request(PTRACE_SETFPREGS,native.pid,nullptr,&initial_fp);Memory memory(native);Bridge dependencies{native,packet-0x2000u,before,report};NativeTu15CollectionContactServices services;unsigned allocations{},classifications{};
 services.allocation.allocate=[&](const auto&request,auto&fp){return dependencies.call(request.owner,request.receiver,{request.bytes,request.memory_class},allocations++==0?0x2f8216u:0x2e31ffu,fp);};
 services.allocation.release=[&](const auto&request,auto&fp){dependencies.call(request.owner,request.receiver,{request.data,request.bytes,request.memory_class},0x2e3238,fp);};
 services.count_children=[&](auto shape,auto owner,auto&fp){return dependencies.call(owner.identity,shape.identity,{},0x2f7d89,fp);};
 services.filter_child=[&](auto receiver,auto owner,auto output,auto context,auto second,auto first,auto collection,auto index,auto&fp){return dependencies.call(owner.identity,receiver,{output,context.identity,second.identity,first.identity,collection.identity,index},0x2f7de7,fp);};
 services.dispatch.classify_shape=[&](auto shape,auto owner,auto&fp){return dependencies.call(owner.identity,shape.identity,{},classifications++%2==0?0x2f7dfcu:0x2f7e09u,fp);};
 services.dispatch.create_agent=[&](auto owner,auto first,auto second,auto context,std::uint32_t data,auto&fp){return NativeTu15ContactAgent{dependencies.call(owner.identity,0,{first.identity,second.identity,context.identity,data},0x2f7e30,fp)};};
 NativeTu15PhysicsFrameFp fp{before.cwd,before.swd,before.mxcsr};const NativeTu15CollectionContactArguments arguments{[&]{return a;},[&]{return b;},[&]{return context;},[&]{return user_data;}};const auto result=native_tu15_create_collection_contact_agent(memory,arguments,{native_payload,native_filter},fp,services);
 J native_reads=J::array(),native_stores=J::array(),native_calls=J::array();for(const auto&read:memory.ledger)native_reads.push_back(J::array({normalized(read[0].get<std::uint32_t>(),native_payload,native_filter),read[1]}));for(const auto&store:memory.stores)native_stores.push_back(J::array({normalized(store[0].get<std::uint32_t>(),native_payload,native_filter),store[1]}));for(const auto&call:dependencies.calls)native_calls.push_back(J{{"entry",call.at("entry")},{"this",call.at("this")},{"arguments",normalize_args(call.at("arguments"),native_payload,native_filter)},{"returned",normalized(call.at("returned_eax").get<std::uint32_t>(),native_payload,native_filter)},{"before_fp",call.at("before_fp")},{"after_fp",call.at("after_fp")}});
 const auto private_first=capture.at("private_returned_stack").at("first").get<std::uint32_t>(),private_end=capture.at("private_returned_stack").at("end").get<std::uint32_t>();check(packet-0x3000u>=private_first&&native_filter+1u<private_end,"Declared DATA must fit protected private lifetime");std::size_t compared{},different{},private_different{};J diffs=J::array();for(const auto&bank:banks){const auto actual=native.read(bank.address,bank.after.size());for(std::size_t n=0;n<actual.size();++n){const auto address=bank.address+n;const bool is_private=address>=private_first&&address<private_end;if(!is_private)++compared;if(actual[n]==bank.after[n])continue;if(is_private){++private_different;continue;}++different;if(diffs.size()<32)diffs.push_back(J{{"address",address},{"native",std::to_integer<unsigned>(actual[n])},{"original",std::to_integer<unsigned>(bank.after[n])}});}}
 const bool reads_equal=native_reads==actual_reads,stores_equal=native_stores==actual_stores,calls_equal=native_calls==actual_calls,fp_equal=J::array({fp.control_word,fp.status,fp.mxcsr})==fp_words(completed_fp);
 report.update(J{{"passed",different==0&&fp_equal&&result.identity==returned&&reads_equal&&stores_equal&&calls_equal},{"source_entries",J::array({0x2f81f0,0x2f7d20,0x2e31e0})},{"producer_ordinal",ordinal},{"reversed_payload_inputs",reverse},{"declared_count_word",count},{"declared_shared_group_collision",shared_group},{"accepted_children",accepted},{"rejected_children",rejected},{"returned_original_agent",returned},{"returned_native_agent",result.identity},{"fp_equal",fp_equal},{"ordered_reads_equal",reads_equal},{"ordered_stores_equal",stores_equal},{"service_calls_equal",calls_equal},{"persistent_compared_bytes",compared},{"different_bytes",different},{"private_control_stack_different_bytes",private_different},{"first_differences",diffs},{"ordered_original_operand_reads",actual_reads},{"ordered_native_operand_reads",native_reads},{"ordered_original_stores",actual_stores},{"ordered_native_stores",native_stores},{"original_calls",actual_calls},{"native_calls",native_calls},{"original_owner_instructions",original_instructions},{"remaining_services_execute_original_cpu",true},{"production_native_only_closure",false}});
 }catch(const std::exception&e){report["error"]=e.what();std::cerr<<e.what()<<'\n';}if(argc>=6){std::ofstream out(argv[5]);out<<report.dump(2)<<'\n';}return report.at("passed").get<bool>()?0:1;}
'''


if __name__ == '__main__':
    raise SystemExit(validate(DRIVER, 'collection', (
        ('wrong_child_stride', 'children+index*8u', 'children+index*4u'),
        ('wrong_factory_stride', '(first_class<<5u)+second_class', '(first_class<<4u)+second_class'),
        ('wrong_count', 'array+4u,count)', 'array+4u,count+1u)'),
        ('wrong_vtable', '0x41cce8u', '0x41ccc8u'),
        ('missing_reversal', 'current{[&]{return second;},[&]{return first;}', 'current{[&]{return first;},[&]{return second;}'),
    ), (Path(__file__),),
        'src/physics/native_tu15_collection_contact_agent.cpp',
        'include/halo2/native_tu15_collection_contact_agent.hpp',
        ('src/physics/native_tu15_array_reserve.cpp','src/physics/native_tu15_array_growth.cpp'),
        tuple((count<<1)|reverse for count in (0,1,4,5,9,255) for reverse in (0,1))+
        tuple(0x1000|(count<<1)|reverse for count in (1,5) for reverse in (0,1)), 6))
