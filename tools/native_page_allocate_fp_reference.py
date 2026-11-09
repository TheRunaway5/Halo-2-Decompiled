"""Owned13FE50 oracle extension: declared FP inputs and 80-bit observation.

The shared original observer and executable remain unchanged on disk. This
private copy installs only declared caller inputs before the original entry;
it samples real FP state without changing an instruction or supplying an
original outcome. Exact original/helper copies are authenticated and receipted.
"""
import hashlib
import inspect
import re

ORIGINAL_PIN='9b1cd062da1b8c1bba41a0f781c21f4b3bc3cf7353a8ec7ca856985afd0f0041'
OBSERVER_PIN='1e8c7bd6ad5fbaab8b7ce3f567910edb1283f8ddce5413878a56ae5d5a1ab555'


def original_with_fp(reference,image,banks,rows,scratch):
    original=inspect.getsource(reference.original)
    observer=(reference.ROOT/'tools/observe_native_array_growth_tu15.cpp').read_text()
    assert hashlib.sha256(original.encode()).hexdigest()==ORIGINAL_PIN
    assert hashlib.sha256(observer.encode()).hexdigest()==OBSERVER_PIN
    abi=re.search(r'J abi\(pid_t child,const user_regs_struct&r\)\{.*?\}\n',observer)[0]
    updated=abi.replace('return J{{','J result{{',1)
    assert updated.endswith(';}\n')
    updated=updated[:-2]+'''J live=J::array();const auto top=(f.swd>>11u)&7u;
    for(unsigned n=0;n<8;++n){if(!(f.ftw&(1u<<((top+n)&7u))))break;
        std::vector<std::byte>value(10);std::memcpy(value.data(),reinterpret_cast<const std::byte*>(f.st_space)+n*16u,10);live.push_back(hex(value));}
    result["x87"]=live;return result;}\n'''
    observer=observer.replace(abi,updated,1)
    marker='  request(PTRACE_SINGLESTEP,child.pid);wait();\n  J writes='
    assert observer.count(marker)==1
    setup='''  request(PTRACE_SINGLESTEP,child.pid);wait();
  if(m.contains("declared_fp_inputs")){
    const auto &declared=m.at("declared_fp_inputs");user_fpregs_struct input_fp{};request(PTRACE_GETFPREGS,child.pid,nullptr,&input_fp);
    input_fp.cwd=declared.at("cw").get<unsigned>();input_fp.mxcsr|=declared.value("mxcsr_extra",0u);
    input_fp.ftw=0;std::memset(input_fp.st_space,0,sizeof(input_fp.st_space));const auto top=(input_fp.swd>>11u)&7u;
    const auto &live=declared.at("x87");check(live.size()<=8,"Too many declared extended inputs");
    for(unsigned n=0;n<live.size();++n){const auto value=live[n].get<std::string>();check(value.size()==20,"Invalid declared80-bit input");
      for(unsigned lane=0;lane<10;++lane)reinterpret_cast<unsigned char*>(input_fp.st_space)[n*16u+lane]=std::stoul(value.substr(lane*2u,2),nullptr,16);
      input_fp.ftw|=1u<<((top+n)&7u);}
    check(declared.at("xmm").size()==8,"Invalid declared XMM bank");
    for(unsigned reg=0;reg<8;++reg){check(declared.at("xmm")[reg].size()==4,"Invalid declared XMM extent");
      for(unsigned lane=0;lane<4;++lane)input_fp.xmm_space[reg*4u+lane]=declared.at("xmm")[reg][lane].get<unsigned>();}
    request(PTRACE_SETFPREGS,child.pid,nullptr,&input_fp);user_fpregs_struct installed{};request(PTRACE_GETFPREGS,child.pid,nullptr,&installed);
    check(installed.cwd==input_fp.cwd&&installed.swd==input_fp.swd&&installed.mxcsr==input_fp.mxcsr&&installed.ftw==input_fp.ftw,"Declared original FP controls/topology changed");
    check(!std::memcmp(installed.st_space,input_fp.st_space,sizeof(input_fp.st_space))&&!std::memcmp(installed.xmm_space,input_fp.xmm_space,128),"Declared original FP inputs changed");
  }
  J writes='''
    observer=observer.replace(marker,setup,1)
    (scratch/'page_fp_original_observer.cpp').write_text(observer)
    modified=original
    old="str(ROOT/'tools/observe_native_array_growth_tu15.cpp')"
    assert modified.count(old)==1
    modified=modified.replace(old,"str(scratch/'page_fp_original_observer.cpp')",1)
    old="        if case.get('abi_entries'):active['abi_entries']=case['abi_entries']"
    assert modified.count(old)==1
    modified=modified.replace(old,old+"\n        active['declared_fp_inputs']=case['declared_fp_inputs']",1)
    scope=dict(reference.__dict__);exec(modified,scope)
    directory,authority=scope['original'](image,banks,rows,scratch)
    receipt=dict(original_function_sha256=ORIGINAL_PIN,original_observer_sha256=OBSERVER_PIN,
        extended_function_sha256=hashlib.sha256(modified.encode()).hexdigest(),extended_observer_sha256=hashlib.sha256(observer.encode()).hexdigest(),
        declared_inputs_installed_only_before_original_entry=True,original_instruction_patches=0,supplied_original_outcomes=False,
        extended_values_sampled_from_real_getfpregs=True)
    return directory,authority,receipt
