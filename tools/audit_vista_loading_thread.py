#!/usr/bin/env python3
"""Pin original loading-thread timing and its bounded protected caller.

The protected decoder and literal401461 execute unchanged. This probe stops
before SLDLOpen and supplies no licensing result. It reuses the earlier explicit
map/GPU service boundaries, including66B85E as a recorded call; it does not
execute a Windows thread, D3D device or loading progress.
"""
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
AUDIT = ROOT / "analysis/frontend/vista-session-flow-audit"
sys.path.insert(0, str(AUDIT / "local-session-inputs-original"))
from audit import PE, EXE, EXE_SHA, DLL, DLL_SHA


def main():
    exe, dll = PE(EXE, EXE_SHA), PE(DLL, DLL_SHA)
    original_harness = AUDIT / "protected-load-original/execute.py"
    sys.path.insert(0, str(original_harness.parent))
    source = original_harness.read_text()
    old = "if pc==0x401461:event('engine_mode',0,0);return"
    assert source.count(old) == 1
    source = source.replace(old, "if pc==0x401461:pass # Execute actual literal2 owner")
    marker = "if pc==STOP:u.emu_stop();return"
    assert source.count(marker) == 1
    source = source.replace(marker, marker + "\n  if pc==0x42765a:\n   s=u.reg_read(UC_X86_REG_ESP)\n   events.append(dict(service='SLDLOpen_boundary',args=[u32(u,s+4)]))\n   u.reg_write(UC_X86_REG_EIP,STOP);u.emu_stop();return")
    namespace = {"__file__": str(original_harness), "__name__": "bounded_loading_thread_caller"}
    exec(compile(source, str(original_harness), "exec"), namespace)
    records = []
    for kind, reason in ((3, 7), (2, 4), (1, 4)):
        result = namespace["execute"](exe.data, kind=kind, reason=reason)
        assert result["events"][-1]["service"] == "SLDLOpen_boundary"
        init = [event for event in result["events"] if event["service"] == "load_mode"]
        assert len(init) == 1 and init[0]["args"] == [0]
        assert all(a == b for a, b in result["checksums"])
        records.append(dict(kind=kind, reason=reason, events=result["events"],
                            instruction_visits=result["instruction_visits"],
                            checksums=result["checksums"],
                            stopped_before="actual SLDLOpen import thunk42765A"))
    imports = [entry for entry in exe.imports() if int(entry["iat_va"], 16) in (
        0x79B22C, 0x79B260, 0x79B2CC, 0x79B2D0, 0x79B2DC, 0x79B2B0,
        0x79B56C, 0x79B570, 0x79B1F4)]
    tls = {}
    for name, pe in (("exe", exe), ("dll", dll)):
        optional = struct.unpack_from("<I", pe.data, 0x3C)[0] + 24
        rva, size = struct.unpack_from("<II", pe.data, optional + 96 + 9 * 8)
        item = dict(directory_rva=rva, directory_size=size)
        if rva:
            raw = pe.data[pe.file(rva):][:size]
            callbacks = struct.unpack_from("<I", raw, 12)[0]
            first = struct.unpack_from("<I", pe.data, pe.file(callbacks-pe.base))[0]
            item.update(directory_bytes=raw.hex(), callbacks_address=callbacks,
                        first_callback=first)
        tls[name] = item
    spans = {name: exe.span(begin, end) for name, begin, end in (
        ("mode_literal", 0x401461, 0x401467),
        ("thread", 0x66B808, 0x66B85E),
        ("initializer", 0x66B85E, 0x66BB08),
        ("throttle_and_first_conversion", 0x66A512, 0x66A5C1),
        ("loading_end", 0x66A060, 0x66A0B0),
        ("fade_begin", 0x66A0B0, 0x66A0E2),
        ("reset_join", 0x66370F, 0x66373A),
        ("temporary_truncation", 0x66ACF8, 0x66AD46))}
    decoded = {}
    for name in ("004091f0.asm", "00409453.asm"):
        file = AUDIT / "protected-load-original/load-consumer" / name
        decoded[name] = dict(sha256=hashlib.sha256(file.read_bytes()).hexdigest(), text=file.read_text())
    report = dict(exe_sha256=EXE_SHA, dll_sha256=DLL_SHA, imports=imports,
                  spans=spans, tls=tls, protected_caller=records,
                  protected_decoded_calls=decoded,
                  reused_harness_sha256=hashlib.sha256(original_harness.read_bytes()).hexdigest(),
                  boundaries=__doc__, ordinary_loading_thread_fp="unresolved Windows thread/DllMain/D3D boundary")
    output = AUDIT / "native-loading-thread-original"
    output.mkdir(parents=True, exist_ok=True)
    (output / "original.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(dict(caller_cases=len(records), visits=sum(row["instruction_visits"] for row in records),
                          clock=next(row for row in imports if row["iat_va"] == "0079B22C"),
                          ordinary_loading_thread_fp=report["ordinary_loading_thread_fp"])))


if __name__ == "__main__":
    main()
