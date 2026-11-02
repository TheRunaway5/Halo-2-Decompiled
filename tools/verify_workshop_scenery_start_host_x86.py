"""Cross-check scenery wrapper references using unchanged physical i386 code.

The parser, scenery effect and completion remain explicit intercepted ABI
boundaries, never animation acceptance. Run through run_test.py; all generated
host code, binaries and complete outputs stay in owned scratch.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import zlib


def digest(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    root=Path(__file__).resolve().parents[1];scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not a.output.resolve().is_relative_to(scratch)
    fixture=root/'tests/fixtures/workshop_scenery_start_tu15.json.zlib'
    descriptor=json.loads(fixture.with_suffix('').read_text());packed=fixture.read_bytes()
    assert len(packed)==descriptor['packed_bytes']and digest(fixture)==descriptor['packed_sha256']
    decoder=zlib.decompressobj();raw=decoder.decompress(packed,1024*1024+1)
    assert decoder.eof and not decoder.unused_data and not decoder.unconsumed_tail
    assert len(raw)==descriptor['decoded_bytes']and hashlib.sha256(raw).hexdigest()==descriptor['decoded_sha256']
    reference=json.loads(raw)
    from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
    assert digest(NEW_DEFAULT)==NEW_PIN==reference['xbe_sha256']
    template=root/'tools/original_workshop_titles_host_x86.cpp'
    source=template.read_text().split('    std::string line;\n')[0]
    first=source.index('static void boundary(');last=source.index('__attribute__((noinline,noclone))',first)
    source=source[:first]+'''static unsigned scenery_seed,scenery_effect_return,scenery_calls;
static unsigned scenery_targets[3],scenery_registers[3][8],scenery_stack[3][7];
static void boundary(int,siginfo_t*,void* state) {
    auto* context=static_cast<ucontext_t*>(state);auto& r=context->uc_mcontext.gregs;
    const auto pc=std::uint32_t(r[REG_EIP]),sp=std::uint32_t(r[REG_ESP]);
    const auto* stack=reinterpret_cast<const std::uint32_t*>(sp);
    if(scenery_calls>=3 || (pc!=0x212010 && pc!=0x10f710 && pc!=0x211da0))_exit(121);
    const unsigned regs[]{REG_EAX,REG_ECX,REG_EDX,REG_EBX,REG_ESP,REG_EBP,REG_ESI,REG_EDI};
    const auto index=scenery_calls++;scenery_targets[index]=pc;
    for(unsigned n=0;n<8;++n)scenery_registers[index][n]=r[regs[n]];
    for(unsigned n=0;n<7;++n)scenery_stack[index][n]=stack[n];
    r[REG_EAX]=pc==0x212010?(title_parsed?0x2103000:0):pc==0x10f710?scenery_effect_return:(0xabc00000|scenery_seed);
    r[REG_ECX]=0xdef00000|scenery_seed;r[REG_EDX]=0x98700000|scenery_seed;
    r[REG_EIP]=stack[0];r[REG_ESP]=sp+(pc==0x212010?20:pc==0x10f710?28:4);
}
'''+source[last:]
    assert source.count('std::uint16_t title_fp_status;')==1 and source.count('pushl $0x202')==1
    source=source.replace('std::uint16_t title_fp_status;','std::uint16_t title_fp_status;std::uint32_t scenery_initial_flags;')
    source=source.replace('pushl $0x202','pushl scenery_initial_flags')
    # The source retains DF. Capture it before restoring the host C++ ABI.
    source=source.replace('"movl title_host_stack,%%esp"','"movl title_host_stack,%%esp\\n\\tcld"')
    needle='if(sigaction(SIGSEGV,&action,nullptr) || mprotect(reinterpret_cast<void*>(0x211000),0x2000,PROT_READ|PROT_WRITE))'
    assert source.count(needle)==1
    source=source.replace(needle,needle[:-1]+' || mprotect(reinterpret_cast<void*>(0x10f000),0x1000,PROT_READ|PROT_WRITE))')
    source+='''    std::string line;
    while(std::getline(std::cin,line)) {
        std::istringstream row(line);unsigned parsed,object,graph,name,effect_return,seed,flags;
        if(!(row>>parsed>>object>>graph>>name>>effect_return>>seed>>flags)||parsed>1||seed>31)
            throw std::runtime_error("Malformed scenery wrapper ABI input");
        auto* stack_bytes=reinterpret_cast<unsigned char*>(0x21ff800);
        for(unsigned n=0;n<2048;++n)stack_bytes[n]=(seed+n*7)&255;
        *reinterpret_cast<std::uint32_t*>(0x2103000)=object;
        *reinterpret_cast<std::uint32_t*>(0x2103004)=graph;
        *reinterpret_cast<std::uint32_t*>(0x2103008)=name;
        auto* stack=reinterpret_cast<std::uint32_t*>(0x21fff00);
        stack[1]=184|(seed<<16);stack[2]=0x12340001;stack[3]=0x23450002;
        for(unsigned n=0;n<8;++n)title_initial[n]=0x11223344u+n*0x112233u+seed*0x10203u;
        title_initial[4]=0x21fff00;scenery_initial_flags=flags;
        auto* xmm=reinterpret_cast<unsigned char*>(title_xmm);
        for(unsigned n=0;n<8;++n)for(unsigned j=0;j<16;++j)xmm[n*16+j]=(seed+n+j)&255;
        title_entry=0x2accf0;title_parsed=parsed;scenery_calls=0;scenery_seed=seed;scenery_effect_return=effect_return;
        execute();std::cout<<std::hex;
        for(auto value:title_result)std::cout<<value<<' ';
        std::cout<<title_flags<<' '<<title_fp_status<<' '<<scenery_calls<<' ';
        constexpr char hex[]="0123456789abcdef";
        for(unsigned n=0;n<128;++n)std::cout<<hex[xmm[n]>>4]<<hex[xmm[n]&15];
        for(unsigned call=0;call<scenery_calls;++call){
            std::cout<<' '<<scenery_targets[call];
            for(auto value:scenery_registers[call])std::cout<<' '<<value;
            for(auto value:scenery_stack[call])std::cout<<' '<<value;
        }
        std::cout<<'\\n';
    }
    for(unsigned i=0;i<count;++i){
        const auto row=table+i*56,address=word(raw,row+4),offset=word(raw,row+12),size=word(raw,row+16);
        if(address<=0x2accf0&&0x2acd3d<=std::uint64_t(address)+size&&
            std::memcmp(reinterpret_cast<const void*>(address),raw.data()+offset,size))
            throw std::runtime_error("Original text changed during scenery wrapper execution");
    }
}catch(const std::exception& error){std::cerr<<error.what()<<'\\n';return 1;}
'''
    path=scratch/'scenery-host.cpp';path.write_text(source);binary=scratch/'scenery-host'
    command=['g++','-m32','-msse2','-O2','-fomit-frame-pointer','-fno-pie','-no-pie',str(path),'-o',str(binary)]
    subprocess.run(command,check=True)
    inputs=''.join(' '.join(str(int(row[k]))for k in('parsed','object','graph','name','effect_return','seed','initial_flags'))+'\n'for row in reference['cases'])
    result=subprocess.run([str(binary),str(NEW_DEFAULT)],input=inputs,capture_output=True,text=True,timeout=30,check=True)
    lines=result.stdout.splitlines();assert len(lines)==len(reference['cases'])==32
    for index,(line,row)in enumerate(zip(lines,reference['cases'])):
        fields=line.split();assert len(fields)==12+16*len(row['calls'])
        assert [int(v,16)for v in fields[:8]]==row['registers'],('gp',index)
        assert int(fields[8],16)==row['eflags']and int(fields[9],16)==0,('flags/fp',index)
        assert int(fields[10],16)==len(row['calls']),('calls',index)
        expected=b''.join(bytes((row['seed']+n+j)&255 for j in range(16))for n in range(8)).hex()
        assert fields[11]==expected,('simd',index)
        for n,call in enumerate(row['calls']):
            values=[int(v,16)for v in fields[12+n*16:28+n*16]]
            assert values[0]==call['target']and values[1:9]==call['registers'],('call GP',index,n)
            expected_stack=[int.from_bytes(bytes.fromhex(call['stack'])[j:j+4],'little')for j in range(0,28,4)]
            # The outer harness return address belongs to the physical host.
            skip=6 if call['target']==0x212010 else 2 if call['target']==0x211da0 else -1
            assert all(values[9+j]==value for j,value in enumerate(expected_stack)if j!=skip),('call stack',index,n)
    report=dict(scope=__doc__,passed=True,cases=len(lines),xbe_sha256=NEW_PIN,reference=descriptor,
        template_sha256=digest(template),verifier_sha256=digest(__file__),host_source_sha256=digest(path),host_command=command,
        host_output_sha256=hashlib.sha256(result.stdout.encode()).hexdigest(),original_image_unchanged=digest(NEW_DEFAULT)==NEW_PIN,
        instruction_patches=[],effect_interceptions=[0x10f710],engine_effects_qualified=False,
        scratch=str(scratch),retained_large_artifacts=[])
    assert report['original_image_unchanged'];a.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=True,cases=len(lines))))


if __name__=='__main__':main()
