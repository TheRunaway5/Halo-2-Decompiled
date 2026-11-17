#!/usr/bin/env python3
"""Exact old public object storage API versus the shared salted-lookup DATA view."""
import argparse,copy,hashlib,json,os,re,subprocess
from pathlib import Path
from validate_native_object_body_frame_tu15 import corpus
ROOT=Path(__file__).resolve().parents[1]
PUBLIC='src/gameplay/objects/object_creation_request.cpp'
PUBLIC_PIN='bc66a1665f61efa13e1b575af9914a20cebc314f0ec25f196b50380369db90d1'
PUBLIC_NEW_PIN='b7b4160c324e3c46ec9cd74826a42afdda2f7877f4bcd4d397950c4a6262a18c'
BODY_PIN='d2c55fcb1a9035b353fbcc61c3ce6955988a6ec508c050d9e3c028d23a5c6320'
INCLUDE='#include "halo2/native_tu15_object_body_value.hpp"\n'
ORIGINAL_BODY='''  std::uint32_t body(std::uint32_t pool,std::uint32_t datum,std::uint32_t mask) const {
    if(datum==0xffffffffu)return 0;
    const auto index=datum&0xffffu;
    if(std::int32_t(index)>=std::bit_cast<std::int32_t>(read(pool+0x38)))return 0;
    const auto stride=read(pool+0x24);
    const auto record=stride*index+read(pool+0x44);
    const auto salt=std::uint16_t(read(record,2));
    if(!salt || std::bit_cast<std::int16_t>(salt)!=
                std::bit_cast<std::int16_t>(std::uint16_t(datum>>16)))return 0;
    if(!(mask&(1u<<(read(record+3,1)&31))))return 0;
    return read(record+8);
  }
'''
REPLACEMENT_BODY='''  std::uint32_t body(std::uint32_t pool,std::uint32_t datum,std::uint32_t mask) const {
    return native_object_body_detail::value(pool,datum,mask,
        [this](std::uint32_t address,unsigned bytes){return read(address,bytes);});
  }
'''
def digest(value):return hashlib.sha256(value).hexdigest()
def predecessor(source):
    pin=digest(source.encode())
    if pin==PUBLIC_PIN:return source
    if pin!=PUBLIC_NEW_PIN or source.count(INCLUDE)!=1 or source.count(REPLACEMENT_BODY)!=1:
        raise ValueError('Complete installed public object storage owner drift')
    result=source.replace(REPLACEMENT_BODY,ORIGINAL_BODY,1).replace(INCLUDE,'',1)
    if digest(result.encode())!=PUBLIC_PIN:raise ValueError('Exact public predecessor reconstruction failed')
    return result
def candidate(source):
    source=predecessor(source)
    start=source.index('  std::uint32_t body(');end=source.index('  void initialize(',start)
    body=source[start:end]
    if digest(body.encode())!=BODY_PIN:raise ValueError('Complete public salted lookup body drift')
    result=INCLUDE+source[:start]+REPLACEMENT_BODY+source[end:]
    if digest(result.encode())!=PUBLIC_NEW_PIN:raise ValueError('Exact public lookup candidate drift')
    return result
def public_inputs():
    rows=[]
    for original in corpus():
        row=copy.deepcopy(original)
        bank=next(item for item in row['banks']if item['address']==0x4ec000)
        row.update(pool=int.from_bytes(bytes.fromhex(bank['before'])[0xbd0:0xbd4],'little'),
                   datum=row['gp'][0],mask=row['matrix'])
        rows.append(row)
    # The preexisting public API admits a backed record at address zero. The
    # physical controller uses EDI==0 as its failed-lookup sentinel instead.
    # Preserve that public behavior; never widen the CPU-facing frame with it.
    row=copy.deepcopy(rows[0]);row.update(id=128,datum=0x10000,pool=0x61000100,
        mask=1,zero_record=True)
    for bank in row['banks']:
        if bank['address']!=0x61000000:continue
        raw=bytearray.fromhex(bank['before'])
        for offset,value in ((0x138,1),(0x124,12),(0x144,0)):
            raw[offset:offset+4]=value.to_bytes(4,'little')
        bank['before']=raw.hex()
    raw=bytearray(16);raw[:2]=(1).to_bytes(2,'little');raw[8:12]=(0xcafebabe).to_bytes(4,'little')
    row['banks'].append(dict(address=0,before=raw.hex()));rows.append(row)
    return rows
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--compiler',action='append')
    parser.add_argument('--sanitizers',action='store_true')
    parser.add_argument('--require-binding',action='store_true')
    args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    names=[PUBLIC,'include/halo2/native_tu15_object_body_kernel.hpp',
        'include/halo2/native_tu15_object_body_value.hpp','tests/native_tu15_object_body_value_tests.cpp',
        'tools/validate_native_object_body_value_tu15.py','tools/validate_native_object_body_frame_tu15.py']
    pins={name:digest((ROOT/name).read_bytes())for name in names}
    report=dict(passed=False,production_bound=False,owned_scratch=str(scratch),
        source_before=pins,retained_large_artifacts=[],scope=__doc__)
    try:
        current=(ROOT/PUBLIC).read_text();source=predecessor(current);proposed=candidate(current)
        if args.require_binding and proposed!=current:raise ValueError('Public object lookup reuse is not installed')
        if candidate(proposed)!=proposed:raise ValueError('Public reuse is not idempotent')
        report.update(predecessor_sha256=digest(source.encode()),candidate_sha256=digest(proposed.encode()))
        if source.count('namespace halo2 {')!=1:raise ValueError('Public predecessor namespace seam drift')
        old=scratch/'predecessor.cpp';old.write_text(source.replace('namespace halo2 {','namespace halo2::object_body_predecessor {',1))
        new=scratch/'candidate.cpp';new.write_text(proposed)
        fixture=scratch/'inputs.json';fixture.write_text(json.dumps(public_inputs(),separators=(',',':')))
        report['input_sha256']=digest(fixture.read_bytes());results=[]
        for compiler in args.compiler or('g++','clang++'):
            binary=scratch/(Path(compiler).name+'-value')
            command=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off',
                '-I'+str(ROOT/'include'),str(ROOT/'tests/native_tu15_object_body_value_tests.cpp'),
                str(old),str(new),'-o',str(binary)]
            if args.sanitizers:command+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
            built=subprocess.run(command,capture_output=True,text=True)
            if built.returncode:raise ValueError('Public lookup compile failed: '+built.stderr[-7000:])
            done=subprocess.run([str(binary),str(fixture)],capture_output=True,text=True)
            if done.returncode:raise ValueError('Public lookup comparison failed: '+(done.stdout+done.stderr)[-7000:])
            # A genuine public record-zero case must reject a CPU-sentinel
            # shortcut compiled into this distinct public view.
            relative='include/halo2/native_tu15_object_body_value.hpp'
            header=(ROOT/relative).read_text();before='if(!admitted_)return 0u;'
            if header.count(before)!=1:raise ValueError('Public sentinel defect seam drift')
            overlay=scratch/(Path(compiler).name+'-zero-record');target=overlay/relative
            target.parent.mkdir(parents=True);target.write_text(header.replace(before,'if(!selected_)return 0u;',1))
            bad_command=list(command);bad_command.insert(1,'-I'+str(overlay/'include'))
            bad_command[bad_command.index('-o')+1]=str(overlay/'bad')
            built=subprocess.run(bad_command,capture_output=True,text=True)
            if built.returncode:raise ValueError('Public sentinel defect compile failed: '+built.stderr[-5000:])
            bad=subprocess.run([str(overlay/'bad'),str(fixture)],capture_output=True,text=True)
            diagnostic=(bad.stdout+bad.stderr)[-5000:]
            if not bad.returncode or 'Public object-body result/exception differs'not in diagnostic:
                raise ValueError('Public sentinel defect survived/unrecognized: '+diagnostic)
            results.append(dict(compiler=compiler,command=command,result=json.loads(done.stdout),
                compiled_defect=dict(name='CPU-zero-sentinel-in-public-view',rejected=True,diagnostic=diagnostic)))
        report.update(passed=True,compilers=results,public_record_zero_distinct_from_physical=True,
            public_reuse_installed=current==proposed)
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:digest((ROOT/name).read_bytes())for name in names}
        if report['source_after']!=pins:report.update(passed=False,source_drift=True)
        (args.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0
if __name__=='__main__':raise SystemExit(main())
