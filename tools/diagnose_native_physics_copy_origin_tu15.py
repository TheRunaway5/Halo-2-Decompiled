#!/usr/bin/env python3
"""Read-only original data-write watch, with bounded stack/source diagnostics.

Run through run_test.py. A private tracer adds a hardware write watch without
patching original instructions or supplying any original/native physics result.
All raw banks, compiler outputs and the private tracer die with the owner.
"""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    value = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1 << 20), b''):
            value.update(block)
    return value.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive', type=Path, required=True)
    parser.add_argument('--archive-sha256', required=True)
    parser.add_argument('--address', type=lambda text: int(text, 0), required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    report = dict(owned_scratch=str(scratch), read_only_original_watch=True,
                  original_code_or_operands_modified=False, retained_large_artifacts=[])
    try:
        assert sha(args.archive) == args.archive_sha256
        names = subprocess.check_output(['tar', '--zstd', '-tf', str(args.archive)], text=True).splitlines()
        assert names and all(not Path(name).is_absolute() and '..' not in Path(name).parts for name in names)
        inputs = scratch / 'inputs'
        inputs.mkdir()
        subprocess.run(['tar', '--zstd', '-xf', str(args.archive.resolve()), '--no-same-owner', '-C', str(inputs)], check=True)
        captures = list(inputs.rglob('capture.json'))
        assert len(captures) == 1
        source = ROOT / 'tools/live_physics_hardware_return_tu15.cpp'
        observer = source.read_text()
        anchor = ' const auto sound_views=bindings.value("sound_timing_views",J::array());'
        assert observer.count(anchor) == 1
        observer = observer.replace(anchor, f'''
 require(index<3,"Original write watch requires a free DR3");
 constexpr unsigned long watched=0x{args.address:x}ul;
 debug(3,watched);enabled|=(1ul<<6)|(1ul<<28)|(3ul<<30);debug(7,enabled);
 J watched_writes=J::array();
 auto snapshot=[&](unsigned long address,unsigned words){{
   J values=J::array();for(unsigned k=0;k<words;++k){{
     try{{values.push_back(static_cast<unsigned long>(request(PTRACE_PEEKDATA,child,reinterpret_cast<void *>(address+4*k)))&0xfffffffful);}}
     catch(const std::exception&){{values.push_back(nullptr);}}
   }}return J{{{{"address",address}},{{"words",values}}}};
 }};
''' + anchor)
        anchor = '  if(step_trace&&WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP&&!returns.contains(regs.rip))continue;'
        assert observer.count(anchor) == 1
        observer = observer.replace(anchor, '''
  if(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP&&
     (static_cast<unsigned long>(request(PTRACE_PEEKUSER,child,reinterpret_cast<void *>(offsetof(user,u_debugreg)+6*sizeof(unsigned long))))&8ul)) {
    require(watched_writes.size()<32,"Original write watch exceeded diagnostic bound");
    watched_writes.push_back(J{{"after_pc",regs.rip},{"word",snapshot(watched,1)},
      {"gp",J::array({regs.rax,regs.rcx,regs.rdx,regs.rbx,regs.rsp,regs.rbp,regs.rsi,regs.rdi})},
      {"stack",snapshot(regs.rsp,128)},{"source_window",snapshot(regs.rsi-64,48)}});
    debug(6,0);continue;
  }
''' + anchor)
        anchor = 'J{{"original_instructions",nullptr}'
        assert observer.count(anchor) == 1
        observer = observer.replace(anchor, 'J{{"watched_writes",watched_writes},{"original_instructions",nullptr}')
        private = scratch / 'private-root/tools'
        private.mkdir(parents=True)
        private_source = private / source.name
        private_source.write_text(observer)
        spec = importlib.util.spec_from_file_location('original_copy_origin_replay', ROOT / 'tools/replay_live_phantom_hardware_tu15.py')
        replay = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(replay)
        replay.ROOT = private.parent
        sys.argv = ['copy-origin-reference', '--native-frame-semantics', '--return-breakpoints',
                    '--capture', str(captures[0].parent), '--output', str(scratch / 'replay')]
        result = replay.main()
        trace = json.loads((scratch / 'replay/trace.json').read_text())
        original = json.loads((scratch / 'replay/report.json').read_text())
        report.update(watch_address=hex(args.address), watched_writes=trace['watched_writes'],
                      original_differences=original['differences'], reference_replay_exit=result,
                      original_sha256=original['original_sha256'], archive_sha256=args.archive_sha256,
                      private_observer_sha256=sha(private_source), diagnostic_completed=True,
                      source_hashes={str(p.relative_to(ROOT)):sha(p) for p in (Path(__file__), source, ROOT/'tools/replay_live_phantom_hardware_tu15.py')},
                      classification='Producer/copy origin diagnostics only. No compared byte is waived.')
        return 0
    finally:
        report['temporary_allocated_bytes_before_cleanup'] = sum(p.stat().st_blocks * 512 for p in scratch.rglob('*') if p.is_file() and not p.is_symlink())
        (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    raise SystemExit(main())
