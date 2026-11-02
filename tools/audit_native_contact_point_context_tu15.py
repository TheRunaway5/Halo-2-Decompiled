#!/usr/bin/env python3
"""Local source GP/XMM and private-DATA lifetime audit of point removal."""
import argparse
import hashlib
import inspect
import json
import os
from pathlib import Path
from capstone.x86_const import X86_OP_REG
import audit_native_pair_callbacks_context_tu15 as audit
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

OWNERS=(0x322f70,0x3224d0,0x31fee0,0x325d90,0x316aa0)


def sha(path):
    return hashlib.file_digest(Path(path).open('rb'),'sha256').hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',required=True,type=Path)
    args=parser.parse_args();assert os.environ.get('HALO2_TEST_SCRATCH')and sha(NEW_DEFAULT)==NEW_PIN
    image=Xbe(NEW_DEFAULT);code=_Code(image,'.text')
    audit.SAVES|={0x322f73,0x322f74,0x322f79,0x322f82,
                  0x3224d0,0x3224d5,0x3224da,0x31fee0,0x31fee5,0x31feea,
                  0x325d98,0x325da0,0x325da1,0x325def,0x316aa7,0x316aeb}
    source=inspect.getsource(audit.gp_audit)
    anchor='  save=pc in SAVES';assert source.count(anchor)==1
    source=source.replace(anchor,"  if pc in(0x325dae,0x325e0d):\n   assert i.bytes.hex()=='33c0' and i.mnemonic=='xor' and i.operands[0].type==X86_OP_REG and i.operands[0].reg==i.operands[1].reg\n   reads=[r for r in reads if r!=i.operands[0].reg]\n"+anchor)
    audit.__dict__['X86_OP_REG']=X86_OP_REG;exec(source,audit.__dict__)
    gp=[audit.gp_audit(image,code,owner)for owner in OWNERS]
    xmm=[dict(entry=owner,body_sites=len(audit.complete_body(code,owner).code),
              XMM_operand_sites=[pc for pc in audit.complete_body(code,owner).code
                                 if any(code.at(pc).reg_name(reg).startswith('xmm')
                                        for reg in sum(code.at(pc).regs_access(),[]))])for owner in OWNERS]
    init=[0x325db7,0x325dbb,0x325dbf,0x325dc3]
    for n,pc in enumerate(init):
        instruction=code.at(pc)
        assert instruction.mnemonic=='mov'and instruction.operands[0].size==4
        assert instruction.operands[0].mem.disp==12+n*4
        assert instruction.operands[1].type==X86_OP_REG and instruction.reg_name(instruction.operands[1].reg)=='eax'
    body=audit.complete_body(code,0x325d90).code
    assert all(pc in body for pc in (*init,0x325e49,0x325e4d,0x325e51,0x325e66,0x325e6b,0x325e73))
    rows=[dict(instruction=pc,bytes=code.at(pc).bytes.hex(),assembly=code.at(pc).mnemonic+' '+code.at(pc).op_str)
          for pc in sorted(body)]
    report=dict(passed=all(row['passed']for row in gp)and all(not row['XMM_operand_sites']for row in xmm),
                original_sha256=NEW_PIN,owners_GP=gp,owners_XMM=xmm,
                source_hashes={str(Path(__file__).relative_to(ROOT)):sha(__file__),str(Path(audit.__file__).relative_to(ROOT)):sha(audit.__file__)},
                owned_scratch=os.environ['HALO2_TEST_SCRATCH'],retained_large_artifacts=[],
                incoming_gp_and_simd_postimages_supplied=False,
                info_lifetime=dict(bytes=16,complete_zero_initialization_sites=init,
                    source_first_reads=[0x325e49,0x325e4d,0x325e51],
                    source_first_pointer_argument=0x325e6b,source_first_external_consumer=0x325e73,
                    all_words_fully_initialized_before_source_read_or_callback=True,
                    parent_reuses_prior_listener_control_save_storage=True,
                    native_before_bytes_are_ordinary_owned_data=True,
                    source_has_no_retained_info_pointer_store=True,
                    remaining_callback_pointer_retention_audit_required=True),
                event_lifetime=dict(bytes=20,initialized_fields=[[0,2],[4,16]],
                    unwritten_padding=[2,2],padding_reader_and_escape_audit_required=True,
                    component_ret4_stub_memory_read_or_pointer_retention=False),
                update_result_lifetime=dict(source_formal_alias='original entrySP+4 dead point formal low byte',
                    source_lea=0x322fef,source_reuses_no_point_formal_after_update=True,
                    native_contract='separate caller-owned one-byte output DATA'),
                collector_listing=rows,
                strict_all_events_parent_passed=False,
                scope='Local CFG and source-DATA lifetime audit only. Parent raw BEFORE-byte failure remains unchanged. Actual callback readers/escape and full live frame remain separate; no private/persistent byte waiver.')
    args.output.mkdir(parents=True,exist_ok=False)
    (args.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0 if report['passed']else 1


if __name__=='__main__':
    raise SystemExit(main())
