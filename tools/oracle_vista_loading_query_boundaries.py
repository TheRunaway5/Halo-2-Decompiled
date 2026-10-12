#!/usr/bin/env python3
"""Retain actual original post-spill query input without changing V1 outputs.

The V1 map query field is supplied before its declared x87 service thunk. The
native API accepts the binary32 word after original647F7D's spill. Observe that
word by executing the unchanged original method again. The V1 fixture/tool and
expected state words remain frozen. This does not establish physical Vista
signaling-NaN/subnormal behavior at the emulated external query boundary.
"""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import struct
from pathlib import Path

from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP

V1_FIXTURE_SHA = "c6187460d364ffc87034c181db3e2b1340911fd2063c8f877dc972a9e15915bf"
V1_TOOL_SHA = "5921247f8d1cf6fadd2407bb00735431904dac10e08a08235802b68fe09730f3"


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, default=Path("Halo 2 Vista installer/halo2.exe"))
    parser.add_argument("--v1-fixture", type=Path, default=Path("tests/fixtures/vista_loading_original.tsv"))
    parser.add_argument("--v1-report", type=Path, default=Path("analysis/frontend/vista-session-flow-audit/loading-original.json"))
    parser.add_argument("--fixture", type=Path, default=Path("tests/fixtures/vista_loading_v2_original.tsv"))
    parser.add_argument("--report", type=Path, default=Path("analysis/frontend/vista-session-flow-audit/loading-query-boundaries-original.json"))
    args = parser.parse_args()
    tool = Path(__file__).with_name("oracle_vista_loading.py")
    if sha(tool) != V1_TOOL_SHA or sha(args.v1_fixture) != V1_FIXTURE_SHA:
        raise SystemExit("Frozen V1 evidence identity changed")
    spec = importlib.util.spec_from_file_location("vista_loading_v1_original", tool)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    if sha(args.exe) != module.SOURCE_SHA:
        raise SystemExit("Original Vista executable identity changed")
    v1 = json.loads(args.v1_report.read_text())
    if v1["fixture_sha256"] != V1_FIXTURE_SHA or v1["tool_sha256"] != V1_TOOL_SHA:
        raise SystemExit("Frozen V1 report identity changed")
    original = module.Oracle(args.exe.read_bytes())
    observed = []

    def hook(uc, pc, size, opaque):
        if pc == 0x647F81:
            sp = uc.reg_read(UC_X86_REG_ESP)
            observed.append(struct.unpack("<I", bytes(uc.mem_read(sp+8, 4)))[0])

    original.uc.hook_add(UC_HOOK_CODE, hook)
    measured, differences = {}, []
    for row in v1["cases"]:
        if row["kind"] != "P":
            continue
        observed.clear()
        actual = original.progress(row["cw"], row["mxcsr"], tuple(row["before"]), tuple(row["supplied"]))
        if len(observed) != 1 or actual["after"] != row["after"] or actual["calls"] != row["calls"]:
            raise RuntimeError(f'Original V1 outputs changed: {row["label"]}')
        measured[row["label"]] = observed[0]
        if observed[0] != row["supplied"][0]:
            differences.append(dict(label=row["label"], x87_control_word=f'{row["cw"]:04x}',
                mxcsr=f'{row["mxcsr"]:08x}', declared_service_word=f'{row["supplied"][0]:08x}',
                observed_post_spill_word=f'{observed[0]:08x}'))
    lines = ["# H2V_LOADING_ORIGINAL_V2",
        "# P label cw mxcsr before_baseline progress previous_clock map_declared map_post_spill duration now secondary after_baseline progress clock call_count calls",
        "# B label cw mxcsr before_active start gain origin now after_active start gain call_count calls",
        "# F label cw mxcsr before_active start gain origin now after_active start gain elapsed complete packed_or_dash call_count calls"]
    for line in args.v1_fixture.read_text().splitlines():
        if not line or line[0] == "#":
            continue
        fields = line.split()
        if fields[0] == "P":
            fields.insert(8, f'{measured[fields[1]]:08x}')
        lines.append("\t".join(fields))
    args.fixture.parent.mkdir(parents=True, exist_ok=True)
    args.fixture.write_text("\n".join(lines)+"\n")
    report = dict(version="H2V_LOADING_ORIGINAL_V2", source_sha256=module.SOURCE_SHA,
        tool_sha256=sha(Path(__file__)), frozen_v1_tool_sha256=V1_TOOL_SHA,
        frozen_v1_fixture_sha256=V1_FIXTURE_SHA, frozen_v1_report_sha256=sha(args.v1_report),
        fixture_sha256=sha(args.fixture), progress_cases=len(measured),
        expected_state_and_service_outputs_unchanged=True,
        observer=dict(pc="00647f81", original_spill_pc="00647f7d", address="ESP+8 at observer",
            original_instructions_unchanged=True, observer_only=True),
        boundary="map_post_spill is the actual binary32 input consumed after original647F7D, not a native/emulated normalization rule. V1 map_declared remains intact. Exceptional service thunk conversion is emulator-boundary evidence; physical Vista NaN/subnormal equivalence is not claimed.",
        differences=differences,
        signaling_nan="V1 declared x87 service thunk/original spill retains7F812345 in Unicorn. A physical host x87 FLD/FSTP quiets that signaling NaN. The native API therefore receives the observed post-spill word explicitly and does not claim equivalence of that external exceptional query conversion.",
        measured_post_spill_words={name:f'{value:08x}' for name,value in measured.items()})
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2)+"\n")
    print(json.dumps({"progress_cases":len(measured),"differences":differences,
        "fixture":str(args.fixture),"fixture_sha256":report["fixture_sha256"]}))


if __name__ == "__main__":
    main()
