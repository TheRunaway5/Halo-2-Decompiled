#!/usr/bin/env python3
"""Semantic1D7660 composition for its1E0A60 zero-contact caller.

The removed-value slot is dead caller-private DATA after its initialized child
result. SourcePOP ECX exposes it in a volatile register which the authenticated
1E0B58 continuation never reads. This is an ordinary persistent-DATA/FP proof,
not a source-private all-events/control-afterimage qualification.
"""
import hashlib,inspect,json,os,struct
from pathlib import Path
import validate_native_body_properties_tu15 as prop
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

def cases(quick=False):
    u,d=prop.base.UI,prop.base.DATA;rows=[]
    for count in (0,1,4,8):
        for selected in sorted({-1,0,count//2,count-1}):
            if selected>=count:continue
            for poison,seed in ((0,0),(0xa5a5a5a5,0xa5),(0x5a5a5a5a,0x5a)):
                words=[(0x130,d),(0x134,count),(0x138,0x80000000|count)]
                for n in range(16):
                    words.extend([(0x1000+n*8,0x2003 if n==selected else 0x3000+n),(0x1004+n*8,0xcafe0000+n)])
                rows.append(dict(name=f'part_property_{count}_{selected}_{seed}',
                    entry=0x1d7660,receiver=u+0x100,arguments=[0x12345678],header=u+0x130,
                    user_words=words,poison_ecx=poison,private_seed=seed))
    return rows

def main():
    assert os.environ.get('HALO2_TEST_SCRATCH')
    original=inspect.getsource(prop.base.original)
    needle="mov ${case['receiver']},%ecx"
    assert original.count(needle)==1
    original=original.replace(needle,"mov ${case['receiver']},%esi\n mov ${case['poison_ecx']},%ecx")
    original=original.replace("case['before_fp']=fp","fp=[0x27f,0x20,0x1fa0];case['before_fp']=fp")
    namespace=dict(prop.base.__dict__);exec(original,namespace);prop.base.original=namespace['original']
    # Audit the exact selected caller continuation: after the call, no ECX/EAX
    # or removed-result pointer is read before the sourceRET12.
    image=Xbe(NEW_DEFAULT);assert hashlib.sha256(Path(NEW_DEFAULT).read_bytes()).hexdigest()==NEW_PIN
    code=_Code(image,'.text');continuation=[]
    pc=0x1e0b5d
    while True:
        i=code.at(pc);reads,writes=i.regs_access()
        assert not any(i.reg_name(r)in('ecx','cx','cl','ch','eax','ax','al','ah')for r in reads)
        continuation.append(f'{pc:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n')
        if i.mnemonic=='ret':break
        pc+=i.size
    assert pc==0x1e0b6e
    source=inspect.getsource(prop.main)
    source=source.replace('base.OWNERS=(0x31c450,','base.OWNERS=(0x1d7660,0x31c450,')
    needle="base.cases=cases";assert source.count(needle)==1
    source=source.replace(needle,needle+"\n    base.FILES=(*base.FILES,'tools/validate_native_part_property_parent_tu15.py')")
    # Driver additions apply after the direct property driver has been formed.
    needle="        driver.write_text(text)"
    assert source.count(needle)==1
    transform='''        text=text.replace('struct Fault{};struct Required{};', 'constexpr std::uint32_t PRIVATE=0x06000000u;bool private_data(std::uint32_t p,std::size_t n){return p>=PRIVATE&&std::uint64_t(p-PRIVATE)+n<=4096;}struct Fault{};struct Required{};')
        text=text.replace('if(log_reads)reads.push_back','if(log_reads&&!private_data(p,out.size()))reads.push_back')
        text=text.replace('      J actual_stores=J::array(),calls=J::array();','      memory.banks.emplace(PRIVATE,std::vector<std::byte>(4096,std::byte(c.at("private_seed").get<unsigned>())));J actual_stores=J::array(),calls=J::array();')
        text=text.replace('observe=[&](const auto&event){actual_stores.push_back','observe=[&](const auto&event){if(private_data(event.address,event.written.size()))return;actual_stores.push_back')
        text=text.replace('if(entry==0x31c450u)','if(entry==0x1d7660u)native_tu15_replace_part_contact_property(memory,receiver,args[0],PRIVATE,fp,growth,observe);else if(entry==0x31c450u)')
'''
    # This string is inside the outer generator's triple-quoted addition.
    source=source.replace(needle,transform+needle)
    source=source.replace("('removed_value','src/physics/native_tu15_body_properties.cpp','write(0x31c4ceu,output,value)','write(0x31c4ceu,output,0)')", "('replace_missing_remove','src/physics/native_tu15_body_properties.cpp','remove(body,0x2003u,output);',';')")
    environment=dict(prop.__dict__,cases=cases);exec(source,environment)
    try:return environment['main']()
    finally:
        # The enclosing validator owns this bounded report directory.
        import sys
        args=sys.argv;output=Path(args[args.index('--output')+1]);report=output/'report.json'
        if report.exists():
            data=json.loads(report.read_text());data.update(source_private_raw_ledger_qualified=False,
                no_private_control_afterimage_supplied=True,selected_caller=0x1e0b58,
                selected_caller_continuation_sha256=hashlib.sha256(''.join(continuation).encode()).hexdigest(),
                private_result_lifetime='31C450 fully writes one word;1D7660 does not read it as DATA, only POP ECX; authenticated1E0B58 continuation never reads that volatile output beforeRET12.',
                scope='Declared valid body properties: native1D7660→31C450/31C5E0→native growth/allocator, complete persistent DATA+ordered persistent operands/preimages+FP, independent incomingECX/nativeprivate seeds. No raw private ledger or unrelated caller ABI claim.',
                temporary_allocated_bytes_before_cleanup=sum(p.stat().st_blocks*512 for p in Path(os.environ['HALO2_TEST_SCRATCH']).rglob('*')if p.is_file()))
            report.write_text(json.dumps(data,indent=2,sort_keys=True)+'\n')
            (output/'selected-caller-continuation.txt').write_text(''.join(continuation))

if __name__=='__main__':main()
