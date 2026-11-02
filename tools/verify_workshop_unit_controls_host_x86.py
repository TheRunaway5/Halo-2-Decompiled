"""Cross-check the unit-control reference on unchanged physical i386 code.

Reuse the checked XBE mapping/register boundary harness. All generated host
source/binaries and full outputs remain under run_test.py's scratch owner.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import zlib


def digest(path):
    with Path(path).open('rb')as stream:return hashlib.file_digest(stream,'sha256').hexdigest()


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--fixture',type=Path);a=p.parse_args()
    root=Path(__file__).resolve().parents[1];scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not a.output.resolve().is_relative_to(scratch)
    fixture=a.fixture or root/'tests/fixtures/workshop_unit_controls_tu15.json.zlib'
    descriptor=json.loads(fixture.with_suffix('').read_text());packed=fixture.read_bytes()
    assert digest(fixture)==descriptor['packed_sha256'] and len(packed)==descriptor['packed_bytes']
    decoder=zlib.decompressobj();raw=decoder.decompress(packed,4*1024*1024+1)
    assert decoder.eof and not decoder.unused_data and not decoder.unconsumed_tail
    assert len(raw)==descriptor['decoded_bytes'] and hashlib.sha256(raw).hexdigest()==descriptor['decoded_sha256']
    reference=json.loads(raw)
    # Use the same TU1.5 owner as the original oracle.
    import sys
    sys.path.insert(0,str(root/'tools'))
    from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
    image=NEW_DEFAULT;assert digest(image)==NEW_PIN==reference['xbe_sha256']
    template=root/'tools/original_workshop_titles_host_x86.cpp'
    source=template.read_text().split('    std::string line;\n')[0]
    first=source.index('static void boundary(');last=source.index('__attribute__((noinline,noclone))',first)
    source=source[:first]+'''static void boundary(int,siginfo_t*,void* state) {
    auto* context=static_cast<ucontext_t*>(state);auto& r=context->uc_mcontext.gregs;
    const auto pc=std::uint32_t(r[REG_EIP]),sp=std::uint32_t(r[REG_ESP]);
    const auto* stack=reinterpret_cast<const std::uint32_t*>(sp);
    if(pc==0x212010) {
        if(stack[1]!=0x12340001 || stack[2]!=2 || stack[3]!=0x457b66 || stack[4]!=0x23450002)_exit(121);
        title_parser_calls=title_parser_calls+1;r[REG_EAX]=title_parsed?0x2103000:0;
        r[REG_EIP]=stack[0];r[REG_ESP]=sp+20;return;
    }
    if(pc==0x211da0) {
        if(std::uint32_t(r[REG_ECX])!=0x12340001)_exit(122);
        title_completion_calls=title_completion_calls+1;r[REG_EIP]=stack[0];r[REG_ESP]=sp+4;return;
    }
    _exit(123);
}
'''+source[last:]
    assert source.count('std::uint16_t title_fp_status;')==1 and source.count('pushl $0x202')==1
    source=source.replace('std::uint16_t title_fp_status;','std::uint16_t title_fp_status;std::uint32_t title_initial_flags;')
    source=source.replace('pushl $0x202','pushl title_initial_flags')
    source+='''    std::string line;
    while(std::getline(std::cin,line)) {
        std::istringstream row(line);unsigned entry,parsed,object,enabled,flags,seed,armed;
        if(!(row>>entry>>parsed>>object>>enabled>>flags>>seed>>armed) ||
            (entry!=0x2ad5b0 && entry!=0x2b2da0 && entry!=0x2b2d80) || parsed>1 || seed>255)
            throw std::runtime_error("Malformed declared unit-control input");
        auto* bank=reinterpret_cast<unsigned char*>(0x2106000);
        for(unsigned n=0;n<0x800;++n)bank[n]=(seed+n*13)&255;
        auto* stack_bytes=reinterpret_cast<unsigned char*>(0x21ff800);
        for(unsigned n=0;n<2048;++n)stack_bytes[n]=(seed+n*7)&255;
        *reinterpret_cast<std::uint32_t*>(0x4ecbd0)=0x2104000;
        *reinterpret_cast<std::uint32_t*>(0x2104044)=0x2105000;
        *reinterpret_cast<std::uint32_t*>(0x2105020)=0x2106000;
        *reinterpret_cast<std::uint32_t*>(0x4f9b54)=0x2107000;
        auto* actions=reinterpret_cast<unsigned char*>(0x2107000);
        if(entry==0x2b2d80)for(unsigned n=0;n<64;++n)actions[n]=(seed+n*17)&255;
        *reinterpret_cast<std::uint32_t*>(0x2107004)=flags;
        if(entry==0x2b2d80)*reinterpret_cast<std::uint32_t*>(0x2107008)=armed;
        *reinterpret_cast<std::uint32_t*>(0x2106134)=flags;
        *reinterpret_cast<std::uint32_t*>(0x2103000)=object;
        *reinterpret_cast<std::uint32_t*>(0x2103004)=enabled;
        auto* stack=reinterpret_cast<std::uint32_t*>(0x21fff00);
        stack[1]=entry==0x2ad5b0?210:entry==0x2b2d80?479:480;stack[2]=0x12340001;stack[3]=0x23450002;
        for(unsigned n=0;n<8;++n)title_initial[n]=0x11223344u+n*0x112233u+seed*0x10203u;
        title_initial[4]=0x21fff00;title_initial_flags=seed&1?0x247:0x202;
        auto* xmm=reinterpret_cast<unsigned char*>(title_xmm);
        for(unsigned n=0;n<8;++n)for(unsigned j=0;j<16;++j)xmm[n*16+j]=(seed+n+j)&255;
        title_entry=entry;title_parsed=parsed;title_parser_calls=0;title_completion_calls=0;
        execute();
        std::cout<<std::hex;
        for(auto value:title_result)std::cout<<value<<' ';
        std::cout<<title_flags<<' '<<title_fp_status<<' '<<title_parser_calls<<' '<<title_completion_calls<<' ';
        constexpr char hex[]="0123456789abcdef";
        for(unsigned n=0;n<0x800;++n)std::cout<<hex[bank[n]>>4]<<hex[bank[n]&15];
        std::cout<<' ';
        for(unsigned n=0;n<128;++n)std::cout<<hex[xmm[n]>>4]<<hex[xmm[n]&15];
        if(entry==0x2b2d80){std::cout<<' ';for(unsigned n=0;n<64;++n)std::cout<<hex[actions[n]>>4]<<hex[actions[n]&15];}
        std::cout<<'\\n';
    }
}catch(const std::exception& error){std::cerr<<error.what()<<'\\n';return 1;}
'''
    path=scratch/'unit-controls-host.cpp';path.write_text(source);binary=scratch/'unit-controls-host'
    command=['g++','-m32','-msse2','-O2','-fomit-frame-pointer','-fno-pie','-no-pie',str(path),'-o',str(binary)]
    subprocess.run(command,check=True)
    inputs=''.join(' '.join(str(row[k])if k!='parsed'else str(int(row[k]))for k in
        ('entry','parsed','object','enabled','flags','seed'))+' '+str(row.get('armed',0))+'\n'for row in reference['cases'])
    result=subprocess.run([str(binary),str(image)],input=inputs,capture_output=True,text=True,timeout=30,check=True)
    lines=result.stdout.splitlines();assert len(lines)==len(reference['cases'])==descriptor['cases']
    for index,(line,row)in enumerate(zip(lines,reference['cases'])):
        fields=line.split();assert len(fields)==(15 if 'actions_after'in row else 14),(index,fields)
        assert [int(v,16)for v in fields[:8]]==row['registers'],('gp',index)
        assert int(fields[8],16)==row['eflags'] and int(fields[9],16)==0,('flags/fp',index)
        assert int(fields[10],16)==sum(c['target']==0x212010 for c in row['calls'])
        assert int(fields[11],16)==sum(c['target']==0x211da0 for c in row['calls'])
        assert fields[12]==row['bank_after'],('bank',index)
        expected=b''.join(bytes((row['seed']+n+j)&255 for j in range(16))for n in range(8)).hex()
        assert fields[13]==expected,('simd',index)
        if 'actions_after'in row:assert fields[14]==row['actions_after'],('action reset bank',index)
    report=dict(passed=True,cases=len(lines),scope='Unchanged physical i386 execution matches emulator reference GP/flags/SIMD/x87 status, full ordinary unit bank, action reset bank when present and HS boundary counts. Host return-address stack bytes are not compared.',
        xbe_sha256=NEW_PIN,reference=descriptor,template_sha256=digest(template),verifier_sha256=digest(__file__),
        host_source_sha256=digest(path),host_command=command,host_output_sha256=hashlib.sha256(result.stdout.encode()).hexdigest(),
        original_image_unchanged=digest(image)==NEW_PIN,instruction_patches=[],effect_interceptions=[],retained_large_artifacts=[],scratch=str(scratch))
    assert report['original_image_unchanged'];a.output.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(dict(passed=True,cases=len(lines))))


if __name__=='__main__':main()
