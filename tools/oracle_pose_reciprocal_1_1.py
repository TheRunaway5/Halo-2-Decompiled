#!/usr/bin/env python3
"""Record original 143960 reciprocal stages and a separate host SSE boundary.

This does not identify the Xbox CPU's RCPSS lookup implementation.  The original
instruction oracle and this machine's physical SSE outputs are labeled separately.
No frozen suite or production source is changed.
"""
import argparse
import hashlib
import json
import platform
import random
import struct
import subprocess
import tempfile
from pathlib import Path

from oracle_spartan_default_pose_1_1 import PoseTrace, ROOT, PIN, CACHE_PIN
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_XMM0, UC_X86_REG_XMM1

INPUT, OUTPUT = 0x10F8000, 0x10F8100
HOST_SOURCE = r'''
#include <immintrin.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
static uint32_t bits(__m128 value) {
  float f = _mm_cvtss_f32(value); uint32_t result;
  std::memcpy(&result, &f, sizeof result); return result;
}
int main() {
  _mm_setcsr(0x1f80);
  uint32_t input;
  while (std::scanf("%x", &input)==1) {
    float f; std::memcpy(&f,&input,sizeof f);
    __m128 n = _mm_set_ss(f), r = _mm_rcp_ss(n);
    uint32_t approximate = bits(r);
    __m128 p = _mm_mul_ss(n,r); p = _mm_mul_ss(p,r);
    r = _mm_sub_ss(_mm_add_ss(r,r),p);
    uint32_t refined = bits(r);
    r = _mm_add_ss(r,r);
    __m128 d = _mm_div_ss(_mm_set_ss(1.f),n);
    uint32_t divided = bits(d);
    __m128 dp = _mm_mul_ss(n,d); dp = _mm_mul_ss(dp,d);
    d = _mm_sub_ss(_mm_add_ss(d,d),dp);
    std::printf("%08x %08x %08x %08x %08x %08x\n",input,approximate,
                refined,bits(r),divided,bits(d));
  }
}
'''


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output', type=Path, default=ROOT/'analysis/multiplayer/map-pack/pose-reciprocal-1.1-boundary.json')
    args = p.parse_args()
    t = PoseTrace()
    stages = []
    def observe(g,a,size,user):
        if a in (0x1439DF,0x1439E3,0x1439EF,0x1439F3,0x1439F7):
            stages.append({'instruction':f'{a:08x}',
                           'xmm0_low_bits':f'{g.reg_read(UC_X86_REG_XMM0)&0xFFFFFFFF:08x}',
                           'xmm1_low_bits':f'{g.reg_read(UC_X86_REG_XMM1)&0xFFFFFFFF:08x}'})
    t.g.hook_add(UC_HOOK_CODE,observe)
    fixtures = [(f'authored-node-{n["index"]}',bytes.fromhex(n['default_local_quaternion_hex']),'authored_mode_node')
                for n in t.pose_metadata['nodes']]
    rng = random.Random(0x143960)
    fixtures += [(f'explicit-quaternion-{i}',struct.pack('<4f',*(rng.uniform(-2.,2.) for _ in range(4))),
                  'explicit_varied_quaternion_fixture') for i in range(1000)]
    rows = []
    for name,q,authority in fixtures:
        stages.clear()
        t.g.mem_write(INPUT,q+struct.pack('<4f',0.,0.,0.,1.))
        t.g.mem_write(OUTPUT,bytes(52))
        t.run(0x143960,(OUTPUT,INPUT))
        assert len(stages)==5
        rows.append({'name':name,'input_authority':authority,'quaternion_hex':q.hex(),
                     'original_instruction_oracle_stages':stages.copy(),
                     'original_matrix_hex':bytes(t.g.mem_read(OUTPUT,52)).hex()})
    with tempfile.TemporaryDirectory(prefix='halo2-reciprocal-research-') as d:
        source=Path(d)/'host.cpp'; exe=Path(d)/'host'
        source.write_text(HOST_SOURCE)
        command=['c++','-std=c++17','-O0','-msse2','-ffp-contract=off',str(source),'-o',str(exe)]
        subprocess.run(command,check=True,capture_output=True,text=True)
        host=subprocess.run([str(exe)],input=''.join(r['original_instruction_oracle_stages'][0]['xmm0_low_bits']+'\n' for r in rows),
                            capture_output=True,text=True,check=True)
        output=[line.split() for line in host.stdout.splitlines()]
    assert len(output)==len(rows)
    for row,values in zip(rows,output):
        row['host_sse']={name:value for name,value in zip(
            ('norm_bits','rcpss_bits','refined_bits','doubled_refined_bits','divss_bits','divss_refined_bits'),values)}
        original=row['original_instruction_oracle_stages']
        assert values[0]==original[0]['xmm0_low_bits']
        row['host_rcpss_matches_oracle']=values[1]==original[1]['xmm1_low_bits']
        row['host_refined_matches_oracle']=values[2]==original[3]['xmm1_low_bits']
        row['host_divss_matches_oracle_rcpss']=values[4]==original[1]['xmm1_low_bits']
        row['host_divss_refined_matches_oracle']=values[5]==original[3]['xmm1_low_bits']
    cpu=Path('/proc/cpuinfo').read_text()
    identity={line.split(':',1)[0].strip():line.split(':',1)[1].strip() for line in cpu.split('\n\n')[0].splitlines() if ':' in line}
    summary={key:sum(row[key] for row in rows) for key in ('host_rcpss_matches_oracle','host_refined_matches_oracle',
                  'host_divss_matches_oracle_rcpss','host_divss_refined_matches_oracle')}
    authored={key:sum(row[key] for row in rows[:33]) for key in summary}
    report={'xbe_sha256':PIN,'cache_sha256':CACHE_PIN,'oracle_tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
            'case_count':len(rows),'authored_case_count':33,'summary':summary,'authored_summary':authored,
            'host':{'machine':platform.machine(),'cpu_identity':identity,'mxcsr':'00001f80',
                    'cpp_source_sha256':hashlib.sha256(HOST_SOURCE.encode()).hexdigest(),
                    'compiler_arguments':['-std=c++17','-O0','-msse2','-ffp-contract=off']},
            'scope':'Original unchanged143960 instruction stages under Unicorn versus separate physical host SSE arithmetic. Xbox RCPSS hardware lookup is not captured; CPU approximation and emulator output differences remain explicit.',
            'cases':rows}
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'cases':len(rows),'summary':summary,'authored_summary':authored}))


if __name__=='__main__':main()
