#!/usr/bin/env python3
"""Locate original TU1.5 writes behind a retained strict native-frame failure.

Run with tools/run_test.py. Hardware data breakpoints observe the unchanged
original; the private tracer, extracted inputs and binaries die with its owner.
The report identifies writes, without exempting bytes or supplying results.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    with path.open("rb") as source:
        return hashlib.file_digest(source, "sha256").hexdigest()


def replace_once(text, before, after):
    assert text.count(before) == 1, "Qualified observer source changed"
    return text.replace(before, after)


def watched_tracer(source, address):
    source = replace_once(source,
        ' std::size_t sound_index=0,operand_reads=0;',
        ''' require(sound_views.empty(),"Writer observation cannot share sound operand observation");
 const unsigned watch_index=index;require(watch_index<4,"No hardware data breakpoint slot");
 const unsigned long watch_address=WATCH_ADDRESS;
 debug(watch_index,watch_address);
 enabled|=(1ul<<(2*watch_index))|(1ul<<(16+4*watch_index))|(3ul<<(18+4*watch_index));
 debug(6,0);debug(7,enabled);
 const unsigned copy_index=watch_index+1;require(copy_index<4,"No hardware copy-entry slot");
 debug(copy_index,0x2e2e30ul);enabled|=1ul<<(2*copy_index);debug(7,enabled);
 J watched_writes=J::array(),watched_copies=J::array();std::size_t watched_write_count=0,copy_entry_count=0;
 std::size_t sound_index=0,operand_reads=0;'''.replace("WATCH_ADDRESS", hex(address) + "ul"))
    source = replace_once(source, '(step_trace?5000000u:1024u)',
                          '(step_trace?5000000u:100000u)')
    source = replace_once(source,
        '  if(step_trace&&WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP&&!returns.contains(regs.rip))continue;',
        '''  if(WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP) {
   const auto dr6=static_cast<unsigned long>(request(PTRACE_PEEKUSER,child,reinterpret_cast<void *>(offsetof(user,u_debugreg)+6*sizeof(unsigned long))));
   if(dr6&(1ul<<copy_index)) {
    ++copy_entry_count;std::array<unsigned long,4>arguments{};
    for(unsigned k=0;k<4;++k)arguments[k]=static_cast<unsigned long>(request(PTRACE_PEEKDATA,child,reinterpret_cast<void *>(regs.rsp+k*4)))&0xfffffffful;
    if(arguments[1]<=watch_address&&watch_address+4<=arguments[1]+arguments[3]&&watched_copies.size()<32) {
     J stack=J::array();for(unsigned k=0;k<48;++k)stack.push_back(static_cast<unsigned long>(request(PTRACE_PEEKDATA,child,reinterpret_cast<void *>(regs.rsp+k*4)))&0xfffffffful);
     const auto source_word=arguments[2]+watch_address-arguments[1];
     watched_copies.push_back(J{{"entry",regs.rip},{"caller_return",arguments[0]},
      {"destination",arguments[1]},{"source",arguments[2]},{"bytes",arguments[3]},
      {"watched_source_address",source_word},
      {"watched_source_word",static_cast<unsigned long>(request(PTRACE_PEEKDATA,child,reinterpret_cast<void *>(source_word)))&0xfffffffful},
      {"stack_first",regs.rsp},{"stack_words",stack}});
    }
    debug(6,0);continue;
   }
   if(dr6&(1ul<<watch_index)) {
    ++watched_write_count;
    if(watched_writes.size()<256)watched_writes.push_back(J{
     {"after_instruction",regs.rip},{"address",watch_address},
     {"word_after",static_cast<unsigned long>(request(PTRACE_PEEKDATA,child,reinterpret_cast<void *>(watch_address)))&0xfffffffful},
     {"gp",J::array({regs.rax,regs.rcx,regs.rdx,regs.rbx,regs.rsp,regs.rbp,regs.rsi,regs.rdi})}});
    debug(6,0);
    if(!returns.contains(regs.rip))continue;
   }
  }
  if(step_trace&&WIFSTOPPED(status)&&WSTOPSIG(status)==SIGTRAP&&!returns.contains(regs.rip))continue;''')
    source = replace_once(source,
        '   fault["instructions_tail"]=tail;fault["stepped_instructions"]=instructions;',
        '   fault["instructions_tail"]=tail;fault["stepped_instructions"]=instructions;fault["watched_writes"]=watched_writes;fault["watched_write_count"]=watched_write_count;')
    source = replace_once(source,
        '{"original_instructions",nullptr},{"original_return",target}',
        '{"watched_address",watch_address},{"watched_write_count",watched_write_count},{"watched_writes",watched_writes},{"copy_entry_count",copy_entry_count},{"watched_copies",watched_copies},{"original_instructions",nullptr},{"original_return",target}')
    return source


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--archive", type=Path, required=True)
    parser.add_argument("--archive-sha256", required=True)
    parser.add_argument("--address", type=lambda value: int(value, 0), required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    assert 0 <= args.address <= 0xfffffffc and args.address % 4 == 0
    scratch = Path(os.environ["HALO2_TEST_SCRATCH"]).resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    report = dict(diagnostic_completed=False, owned_scratch=str(scratch),
                  retained_large_artifacts=[], original_or_operands_modified=False)
    replay_source = ROOT / "tools/replay_live_phantom_hardware_tu15.py"
    tracer_source = ROOT / "tools/live_physics_hardware_return_tu15.cpp"
    sources = [Path(__file__), replay_source, tracer_source]
    pins = {str(path.relative_to(ROOT)): sha(path) for path in sources}
    try:
        assert sha(args.archive) == args.archive_sha256, "Reference archive changed"
        names = subprocess.run(["tar", "--zstd", "-tf", str(args.archive.resolve())],
                               check=True, capture_output=True, text=True).stdout.splitlines()
        assert names and all(not Path(n).is_absolute() and ".." not in Path(n).parts for n in names)
        inputs = scratch / "inputs"
        inputs.mkdir()
        subprocess.run(["tar", "--zstd", "-xf", str(args.archive.resolve()),
                        "--no-same-owner", "-C", str(inputs)], check=True)
        captures = list(inputs.rglob("capture.json"))
        assert len(captures) == 1
        capture = captures[0].parent
        data = json.loads(captures[0].read_text())
        assert data["entry"] == 0x1cc8d0 and data["reference"] == "English TU1.5 only"
        assert any(row["address"] <= args.address and args.address + 4 <= row["address"] + row["size"]
                   for row in data["regions"])
        tracer = scratch / "writer-watch.cpp"
        tracer.write_text(watched_tracer(tracer_source.read_text(), args.address))
        replay_text = replace_once(replay_source.read_text(),
            "tracer=ROOT/('tools/live_physics_hardware_return_tu15.cpp' if a.return_breakpoints else 'tools/live_phantom_hardware_trace_tu15.cpp')",
            "tracer=Path(" + repr(str(tracer)) + ")")
        wrapper = scratch / "writer-watch-replay.py"
        wrapper.write_text("import sys\nsys.path.insert(0," + repr(str(ROOT / "tools")) + ")\n"
                           + "exec(compile(" + repr(replay_text) + "," + repr(str(replay_source))
                           + ",'exec'),{'__name__':'__main__','__file__':" + repr(str(replay_source)) + "})\n")
        replay = scratch / "replay"
        command = [sys.executable, str(wrapper), "--native-frame-semantics",
                   "--return-breakpoints", "--capture", str(capture), "--output", str(replay)]
        result = subprocess.run(command, check=False, timeout=180, capture_output=True, text=True)
        (args.output / "replay.log").write_text((result.stdout + result.stderr)[-65536:])
        trace = json.loads((replay / "trace.json").read_text())
        (args.output / "writer-trace.json").write_text(json.dumps(trace, indent=2) + "\n")
        original = json.loads((replay / "report.json").read_text())
        (args.output / "original-report.json").write_text(json.dumps(original, indent=2) + "\n")
        # Decode authentic original bytes around the observed after-instruction
        # PCs. A hardware data trap occurs after a write; REP may retain its PC.
        sys.path.insert(0, str(ROOT / "tools"))
        from oracle_smg_firing_1_1 import Xbe, _Code
        from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
        assert sha(NEW_DEFAULT) == NEW_PIN
        code = _Code(Xbe(NEW_DEFAULT), ".text")
        contexts = []
        for pc in sorted({event["after_instruction"] for event in trace["watched_writes"]}):
            candidates = []
            for start in range(pc - 15, pc + 1):
                try:
                    instruction = code.at(start)
                    if start == pc or start + len(instruction.bytes) == pc:
                        candidates.append(dict(pc=hex(start), bytes=instruction.bytes.hex(),
                                               mnemonic=instruction.mnemonic, operands=instruction.op_str))
                except (AssertionError, KeyError):
                    pass
            contexts.append(dict(after_instruction=hex(pc), possible_preceding_or_repeat_instructions=candidates))
        assert all(sha(ROOT / path) == digest for path, digest in pins.items())
        report.update(diagnostic_completed=True, physics_parity_passed=original["passed"],
                      reference_replay_exit=result.returncode, original_sha256=NEW_PIN,
                      original_differences=original["differences"],
                      compared_memory_bytes=original["compared_memory_bytes"],
                      watched_address=hex(args.address), watched_write_count=trace["watched_write_count"],
                      copy_entry_count=trace["copy_entry_count"], watched_copies=trace["watched_copies"],
                      events_truncated=trace["watched_write_count"] > len(trace["watched_writes"]),
                      instruction_contexts=contexts, archive=str(args.archive.resolve()),
                      archive_sha256=args.archive_sha256, source_hashes=pins,
                      private_tracer_sha256=sha(tracer), private_wrapper_sha256=sha(wrapper), command=command,
                      scope="Read-only original writer census; no byte waiver, callee replacement or parity claim")
    finally:
        report["temporary_allocated_bytes_before_cleanup"] = sum(
            path.stat().st_blocks * 512 for path in scratch.rglob("*") if path.is_file())
        (args.output / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({key: report[key] for key in
                      ("diagnostic_completed", "physics_parity_passed", "watched_write_count")}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
