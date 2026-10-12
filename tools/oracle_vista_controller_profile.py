#!/usr/bin/env python3
"""Complete unchanged PC controller/profile chain, explicit external owners.

6080DC and461AD0/461B62/461BF4/461C17 execute their original instructions.
45EE72 binding-layout mutation and45E717 dependency refresh are explicit
service boundaries; this does not assert those lower-owner implementations.
"""
from pathlib import Path
import argparse
import hashlib
import importlib.util
import json
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from unicorn.x86_const import UC_X86_REG_ESP

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("controller_owner_audit", ROOT /
    "analysis/frontend/vista-session-flow-audit/controller-user-owner/audit.py")
prior = importlib.util.module_from_spec(spec)
spec.loader.exec_module(prior)
BASE, INPUT, CONTROLLERS, USERS = prior.BASE, prior.INPUT, prior.CONTROLLERS, prior.USERS
CONTROL_BANK = 0x8A89B0
CONTROL_STRIDE = 0x1680


def sha(data):
    return hashlib.sha256(data).hexdigest()


class Oracle(prior.Oracle):
    def __init__(self, image, case):
        super().__init__(image, xuid=bytes.fromhex(case["xuid_hex"]),
            hresult=case["hresult_word"], input_result=case["layout_result_al"])
        self.case = case

    def hook(self, m, address, size, unused):
        esp = m.reg_read(UC_X86_REG_ESP)
        if address == 0x6080DC:
            return  # Deliberately execute the complete original helper.
        if address == 0x45EE72:
            pointer = prior.u32(m, esp + 4)
            self.calls.append(dict(service="binding_layout", argument_words=[
                prior.u32(m, esp + i) for i in (8, 12, 16, 20)],
                control_hex=prior.raw(m, pointer, CONTROL_STRIDE).hex()))
            for offset, value in self.case["layout_patches"]:
                m.mem_write(pointer + offset, bytes([value]))
            prior.return_service(m, self.case["layout_result_al"])
            return
        if address == 0x45E717:
            self.calls.append(dict(service="input_dependencies",
                controller=prior.u32(m, esp + 8),
                control_hex=prior.raw(m, prior.u32(m, esp + 4), CONTROL_STRIDE).hex()))
            prior.return_service(m)
            return
        if address == 0x609674:
            self.calls.append(dict(service="presentation_context"))
            return  # The actual constant-pointer getter executes unchanged.
        if address == 0x44BD59:
            destination, count, fmt, source = [prior.u32(m, esp + i) for i in (4, 8, 12, 16)]
            assert count == 0x20 and fmt == 0x79BE1C and source == 0xD712C8 + 0x4E
            self.calls.append(dict(service="network_display_format",
                before_hex=prior.raw(m, destination, 0x40).hex(),
                context_hex=self.case["context_hex"]))
            # Declared real formatter result; do not claim original CRT/locale
            # execution. Untouched destination tail remains unchanged.
            output = bytes.fromhex(self.case["formatted_write_hex"])
            m.mem_write(destination, output)
            prior.return_service(m)
            return
        super().hook(m, address, size, unused)


def put(bank, offset, value):
    struct.pack_into("<I", bank, offset, value & 0xFFFFFFFF)


def name(bank, offset, value, terminate=True):
    raw = value.encode("utf-16le") + (b"\x00\x00" if terminate else b"")
    bank[offset:offset + len(raw)] = raw


def case_spec(controller, number, operation="assign", style="first"):
    record = bytearray(prior.patterned(prior.CONTROLLER_STRIDE, 0x29 + number))
    profile = bytearray(prior.patterned(0x1208, 0x53 + number))
    users = bytearray(prior.patterned(4 * prior.USER_STRIDE, 0x18 + number))
    controls = prior.patterned(CONTROL_STRIDE, 0x71 + number)
    slot = (controller + 2) % 4
    put(record, 0, (0x81240010 if number & 1 else 0x02000400) | (0 if style == "first" else 8))
    put(record, 4, 0xFFFFFFFF if style in ("first", "unmapped") else slot)
    put(record, 0x121C, 0xFFFFFFFF if number % 3 == 0 else 0x7100 + number)
    for index in range(4):
        users[index * prior.USER_STRIDE] = (2 if index == slot else 1) if index != controller else 0
    if style == "inactive":
        users[slot * prior.USER_STRIDE] = 0
    if number % 4 == 0:
        name(profile, 8, "P" * 32, terminate=False)
        name(record, 0x1C, "R" * 32, terminate=False)
    else:
        name(profile, 8, f"Actual PC profile {number}")
        name(record, 0x1C, f"Retained profile {number}")
    # Complete supplied PC profile. Controls begin at controller+300, hence
    # profile+2EC. Selectors, sensitivities and flags are actual source fields.
    selectors = [(0, 1, 2), (4, 1, 2), (0, 4, 2), (0, 1, 4), (255, 255, 255), (4, 4, 4)]
    s4, s5, s8 = selectors[number % len(selectors)]
    sensitivity = [0, 1, 9, 10, 11, 255]
    for bank, control_offset in ((profile, 0x2EC), (record, 0x300)):
        put(bank, control_offset, 0xDEADBEA5 ^ number)
        bank[control_offset + 4] = s4
        bank[control_offset + 5] = s5
        bank[control_offset + 6] = sensitivity[number % 6]
        bank[control_offset + 7] = sensitivity[(number + 3) % 6]
        bank[control_offset + 8] = s8
    teams = [-1, -2, 0, 15, 16, 17, -0x80000000, 0x7FFFFFFF]
    put(record, 0x1220, teams[number % len(teams)])
    index = [0xFFFFFFFF, 0, 0x82000000 | number][number % 3]
    if style == "same":
        index = struct.unpack_from("<I", record, 0x121C)[0]
    return dict(id=f"{operation}-{controller}-{number}-{style}", operation=operation,
        controller=controller, style=style, before_controller_hex=record.hex(),
        before_users_hex=users.hex(), before_input_hex=controls.hex(),
        profile_hex=profile.hex(), storage_index=index, argument_word=0x98760000 | number,
        xuid_hex=prior.patterned(8, 0x35 + number).hex(), hresult_word=0,
        layout_result_al=[0, 1, 255][number % 3],
        layout_patches=[[0x30, (number + 1) & 255], [0x1664, (number + 3) & 255],
                        [0x1678, (number + 7) & 255]],
        context_type=[0, 2, 3][number % 3], context_hex=b"Actual network owner\x00".hex(),
        formatted_write_hex=("Actual network owner".encode("utf-16le") + b"\x00\x00").hex(),
        supplied_identity_hex=prior.patterned(8, 0x87 + number).hex())


def execute(image, case):
    oracle = Oracle(image, case)
    m = oracle.machine
    # Keep unrelated records nonzero so complete bank comparisons detect writes
    # to any controller/control record outside the chosen owner.
    controllers_before = prior.patterned(4 * prior.CONTROLLER_STRIDE, 0xA6)
    controls_before = prior.patterned(4 * CONTROL_STRIDE, 0xB8)
    m.mem_write(CONTROLLERS, controllers_before)
    m.mem_write(CONTROL_BANK, controls_before)
    r = CONTROLLERS + case["controller"] * prior.CONTROLLER_STRIDE
    ic = CONTROL_BANK + case["controller"] * CONTROL_STRIDE
    m.mem_write(r, bytes.fromhex(case["before_controller_hex"]))
    m.mem_write(ic, bytes.fromhex(case["before_input_hex"]))
    m.mem_write(USERS, bytes.fromhex(case["before_users_hex"]))
    prior.set32(m, 0xD712C8 + 8, case["context_type"])
    m.mem_write(0xD712C8 + 0x4E, bytes.fromhex(case["context_hex"]))
    m.mem_write(INPUT, bytes.fromhex(case["profile_hex"]))
    if case["operation"] == "assign":
        actual = oracle.call(0x6087BF, case["controller"], INPUT, case["storage_index"], case["argument_word"])
    elif case["operation"] == "publish":
        actual = oracle.call(0x608312, case["controller"])
    elif case["operation"] == "remove":
        actual = oracle.call(0x608257, case["controller"])
    elif case["operation"] == "identity":
        m.mem_write(INPUT, bytes.fromhex(case["supplied_identity_hex"]))
        actual = oracle.call(0x608986, case["controller"], INPUT)
    elif case["operation"] == "team":
        actual = oracle.call(0x6068F2, case["controller"], case["raw_team_word"])
    else:
        raise AssertionError(case["operation"])
    assert actual["returned"], actual
    case["after_controller_hex"] = prior.raw(m, r, prior.CONTROLLER_STRIDE).hex()
    case["after_users_hex"] = prior.raw(m, USERS, 4 * prior.USER_STRIDE).hex()
    case["after_input_hex"] = prior.raw(m, ic, CONTROL_STRIDE).hex()
    for i in range(4):
        if i == case["controller"]:
            continue
        assert prior.raw(m, CONTROLLERS + i * prior.CONTROLLER_STRIDE, prior.CONTROLLER_STRIDE) == \
            controllers_before[i * prior.CONTROLLER_STRIDE:(i + 1) * prior.CONTROLLER_STRIDE]
        assert prior.raw(m, CONTROL_BANK + i * CONTROL_STRIDE, CONTROL_STRIDE) == \
            controls_before[i * CONTROL_STRIDE:(i + 1) * CONTROL_STRIDE]
    case["al"] = actual["al"]
    case["calls"] = [call for call in actual["calls"] if "service" in call]
    return case


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--fixture", type=Path, default=ROOT / "tests/fixtures/vista_controller_profile_original.json")
    parser.add_argument("--evidence", type=Path, default=ROOT / "analysis/frontend/vista-session-flow-audit/controller-profile-native")
    args = parser.parse_args()
    image = prior.EXE.read_bytes()
    assert sha(image) == prior.SOURCE_SHA
    cases = []
    number = 0
    for controller in range(4):
        for style in ("first", "replace", "same", "inactive", "unmapped"):
            for variant in range(6):
                case = case_spec(controller, number, style=style)
                if style == "first":
                    case["hresult_word"] = [0, 1, 0x80004005][variant % 3]
                cases.append(execute(image, case))
                number += 1
        for operation in ("publish", "remove", "identity"):
            for style in ("first", "replace", "inactive", "unmapped"):
                for variant in range(3):
                    cases.append(execute(image, case_spec(controller, number, operation, style)))
                    number += 1
    windows = prior.pin_windows(image)
    for controller in range(4):
        for word in (0, 1, 15, 16, 17, 0xFFFFFFFF, 0x80000000, 0x7FFFFFFF):
            case = case_spec(controller, number, "team", "unmapped")
            # This leaf does not inspect mapping/profile state at all.
            before = bytearray.fromhex(case["before_controller_hex"])
            put(before, 4, 0xBADF00D)
            case["before_controller_hex"] = before.hex()
            case["raw_team_word"] = word
            cases.append(execute(image, case))
            number += 1
    for key, (begin, end) in dict(input_control_publication=(0x6080DC, 0x608257),
            team_write=(0x6068F2, 0x606907),
            binding_expand=(0x461AD0, 0x461B62), binding_contract=(0x461B62, 0x461BF4),
            input_control_read=(0x461BF4, 0x461C17), input_control_write=(0x461C17, 0x461C3B),
            sensitivity_tables_data=(0x7CB75C, 0x7CB7B4),
            display_format_data=(0x79BE1C, 0x79BE24)).items():
        raw = image[begin - BASE:end - BASE]
        window = dict(begin=f"{begin:08X}", end_exclusive=f"{end:08X}",
            bytes_hex=raw.hex(), sha256=sha(raw))
        if not key.endswith("_data"):
            window["instructions"] = [dict(address=f"{i.address:08X}", mnemonic=i.mnemonic,
                operands=i.op_str) for i in Cs(CS_ARCH_X86, CS_MODE_32).disasm(raw, begin)]
        windows[key] = window
    report = dict(version="H2V_CONTROLLER_PROFILE_COMPLETE_CHAIN_V1", source_sha256=prior.SOURCE_SHA,
        generator_sha256=sha(Path(__file__).read_bytes()), cases=cases,
        boundaries=dict(unchanged_source_instructions=True, native_runtime=False,
            platform="Actual XUserGetXUID HRESULT and eight output bytes are explicit service inputs.",
            input="6080DC plus all four 461AD0/461B62/461BF4/461C17 bank helpers execute. 45EE72 mapping mutations/raw AL and45E717 dependency refresh are declared external owners.",
            display="609674,5AC4C0 execute unchanged. Type2 44BD59 formatting bytes are explicitly supplied real service outputs; CRT/locale formatting is not asserted.",
            profiles="Complete1208-byte profiles and actual retained banks are supplied. Reference services are nonreentrant external events.",
            excluded=["live platform identity/backend", "SDL controller slot ownership", "actual profile storage lifetime", "45EE72 binding layout algorithm", "45E717 dependency bank updates", "44BD59 CRT/locale formatter", "session join/replication/admission", "arbitrary guest pointer aliasing"]))
    args.fixture.parent.mkdir(parents=True, exist_ok=True)
    args.fixture.write_text(json.dumps(report, separators=(",", ":")) + "\n")
    args.evidence.mkdir(parents=True, exist_ok=True)
    evidence = dict(report)
    evidence.pop("cases")
    evidence["case_count"] = len(cases)
    evidence["fixture_sha256"] = sha(args.fixture.read_bytes())
    evidence["source_windows"] = windows
    (args.evidence / "original.json").write_text(json.dumps(evidence, indent=2) + "\n")
    lines = []
    for name_, w in windows.items():
        lines.append(f"\n# {name_} {w['begin']}..{w['end_exclusive']} SHA256 {w['sha256']}")
        lines.extend(f"{i['address']} {i['mnemonic']:9s} {i['operands']}" for i in w.get("instructions", []))
    (args.evidence / "source.asm").write_text("\n".join(lines) + "\n")
    print(json.dumps(dict(cases=len(cases), fixture_sha256=evidence["fixture_sha256"])))


if __name__ == "__main__":
    main()
