#!/usr/bin/env python3
"""Lower whole authenticated TU1.5 decorator owners; foreign owners stay explicit.

Run through tools/run_test.py. Generated intermediates and source output must
stay in its owned scratch. The caller may then retain the generated C++ source
and bounded manifest as ordinary reviewable build inputs.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
from types import SimpleNamespace

from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from generate_physics_live_support_tu15 import generate, sha

ROOT=Path(__file__).resolve().parents[1]
OWNERS=(0x3f820,0x3f960,0x3fd10,0x3f210,0x41630,0x4df90,0x4ebc0)
INITIALIZERS=(0x3805f0,0x380650,0x3807b0,0x3807d0,0x3807f0,0x380810,
    0x380830,0x380850,0x3808b0,0x3808f0,0x380960)
ENTRIES=tuple(sorted((*OWNERS,*INITIALIZERS)))


def memory_access(instruction,index,operand):
    if operand.access:
        return ("read" if operand.access&1 else "")+("/write" if operand.access&2 else "")
    # Capstone omits access annotations for a few x87 forms. Keep the
    # inference and its reason visible in the receipt instead of hiding it.
    name=instruction.mnemonic
    if name.startswith(("fst","fist")):return "write-inferred-store"
    if name in ("mov","movss","movaps","movq","movd") and index==0:return "write-inferred-move"
    if name in ("cmp","test") or index or name.startswith(("f","ucom","com")):return "read-inferred-source"
    return "read/write-inferred-destination"


def input_inventory(image,instructions):
    constant_reads={};mutable_refs=[];dynamic_refs=[];literals=[]
    def section(address,size=1):
        return next((s for s in image.sections if s.va<=address and address+size<=s.va+s.vsize),None)
    for instruction in instructions:
        for index,operand in enumerate(instruction.operands):
            if operand.type==2:
                value=operand.imm&0xffffffff;s=section(value)
                if s and s.name==".rdata":literals.append({"pc":f"{instruction.address:08x}","address":f"{value:08x}",
                    "role":"authored RO address literal; not itself a read or an inferred table extent"})
            if operand.type!=3:continue
            access=memory_access(instruction,index,operand);address=operand.mem.disp&0xffffffff
            base=instruction.reg_name(operand.mem.base) if operand.mem.base else None
            register=instruction.reg_name(operand.mem.index) if operand.mem.index else None
            row={"pc":f"{instruction.address:08x}","mnemonic":instruction.mnemonic,"operand":index,
                "width":operand.size,"access":access,"displacement":f"{address:08x}",
                "base":base,"index":register,"scale":operand.mem.scale}
            owner=section(address,operand.size)
            if base or register:
                # Indexed absolute globals remain mutable caller-owned ranges;
                # a register-dependent extent cannot be inferred from one read.
                (mutable_refs if not base and owner else dynamic_refs).append(row)
            elif owner and not owner.flags&1 and access.startswith("read") and "/write" not in access:
                data=image.read(address,operand.size)
                constant_reads.setdefault((address,operand.size),{"address":f"{address:08x}","bytes":operand.size,
                    "section":owner.name,"raw_hex":data.hex(),"sha256":hashlib.sha256(data).hexdigest(),"read_pcs":[]})["read_pcs"].append(f"{instruction.address:08x}")
            else:
                row["section"]=owner.name if owner else None
                row["initial_bytes_supplied"]=False;mutable_refs.append(row)
    return {"authored_read_only_reads":list(constant_reads.values()),"authored_read_only_address_literals":literals,
        "mutable_absolute_and_indexed_inputs_outputs":mutable_refs,"caller_relative_memory_operands":dynamic_refs,
        "boundary":"RO bytes are only actual reads from authenticated nonwritable XBE sections. Mutable globals, metadata, stack, opaque service memory and register-dependent extents are not populated by this inventory."}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--directory",type=Path,required=True)
    parser.add_argument("--output",type=Path,required=True)
    args=parser.parse_args()
    scratch=Path(os.environ["HALO2_TEST_SCRATCH"]).resolve()
    assert args.directory.resolve().is_relative_to(scratch) and args.output.resolve().is_relative_to(scratch)
    assert sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=_Code(image,".text")
    sites,instruction_map,addresses,routines={},{},{},[]
    all_instructions={}
    for entry in ENTRIES:
        body=_trace(code,entry,set(ENTRIES))
        assert body.code and not body.tables,"decorator body acquired an unqualified table owner"
        sites[f"{entry:08x}"]=sorted(body.code)
        routines.append({"entry":entry,"tables":[],"byte_tables":[]})
        for pc in sorted(body.code):
            instruction=code.at(pc)
            assert image.read(pc,instruction.size)==bytes(instruction.bytes)
            all_instructions[pc]=instruction;instruction_map[f"{pc:08x}"]=pc
            addresses[pc]=pc;addresses[pc+instruction.size]=pc+instruction.size
            for operand in instruction.operands:
                if operand.type==2:addresses[operand.imm&0xffffffff]=operand.imm&0xffffffff
                elif operand.type==3:addresses[operand.mem.disp&0xffffffff]=operand.mem.disp&0xffffffff
    args.directory.mkdir(parents=True,exist_ok=True)
    paired={"image":"English TU1.5","original_sha256":NEW_PIN,"entries":{f"{p:08x}":p for p in ENTRIES},
        "body_sites":sites,"instruction_map":instruction_map,"address_map":{f"{p:08x}":q for p,q in addresses.items()},"routines":routines}
    correspondence=args.directory/"bodies.json";correspondence.write_text(json.dumps(paired,indent=2,sort_keys=True)+"\n")
    manifest=args.directory/"generation.json"
    generate(SimpleNamespace(output=args.output,manifest=manifest,correspondence=correspondence,
        additional_entries=Path(__file__)),paired)
    source=args.output.read_text().replace("tools/generate_physics_live_support_tu15.py","tools/generate_decorator_owners_tu15.py").replace(
        "halo2/physics_live_support_tu15.hpp","halo2/source_decorator_owners_tu15.hpp").replace(
        "source_run_physics_live_support_tu15","source_run_decorator_owners_tu15").replace(
        "source_physics_live_support_tu15_owns_entry","source_decorator_owners_tu15_owns_entry")
    args.output.write_text(source)
    report=json.loads(manifest.read_text())
    report.update(schema="H2_LITERAL_DECORATOR_OWNERS_TU15_V1",module_generator_sha256=sha(Path(__file__)),
        module_header_sha256=sha(ROOT/"include/halo2/source_decorator_owners_tu15.hpp"),
        generated_cpp_sha256=sha(args.output),explicit_entries=[f"{p:08x}"for p in OWNERS],
        authored_initializer_entries=[f"{p:08x}"for p in INITIALIZERS],instruction_patches=[],
        input_inventory=input_inventory(image,[all_instructions[p]for p in sorted(all_instructions)]),
        entry_instruction_counts={key:len(value)for key,value in sites.items()},
        instruction_operands={f"{p:08x}":f"{instruction.mnemonic} {instruction.op_str}" for p,instruction in all_instructions.items()},
        boundaries=["No old shadow module is edited or used to supply an unknown foreign result.",
            "Foreign cache, shader, portal and GPU calls preserve full required-owner ABI and fail without their actual service.",
            "This source lowering is not live camera/cell, GPU or Xbox framebuffer acceptance."])
    manifest.write_text(json.dumps(report,indent=2,sort_keys=True)+"\n")
    print(json.dumps({"entries":len(ENTRIES),"instructions":len(all_instructions),"external_call_sites":len(report["external_owners"]),
        "source_bytes":args.output.stat().st_size,"manifest_bytes":manifest.stat().st_size,
        "generated_cpp_sha256":report["generated_cpp_sha256"]}))


if __name__=="__main__":main()
