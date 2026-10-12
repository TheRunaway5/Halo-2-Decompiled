#!/usr/bin/env python3
"""Export genuine original spatial CPU boundaries, without native expectations.

Mutable pages and caller backing are from the initialized original game. Its
unchanged execution supplies retained stores and the36-byte driver request,
confirmed at the same actual game boundary. The SDK/DSP is outside this fixture.
The source-private stack and consumed stack argument are excluded from the
semantic native ABI; the12-byte private position backing is compared explicitly.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe


def sha(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture',type=Path,action='append',required=True)
    parser.add_argument('--pair',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();args.output.mkdir(parents=True,exist_ok=False)
    pair=json.loads((args.pair/'report.json').read_text())
    image_path=ROOT/'assets/disc/default.xbe';image=Xbe(image_path)
    assert sha(image_path)=='03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d'
    pcs={};operands={}
    for row in pair['owners']:
        before=(args.pair/'instructions'/f'{row["base"]}-base.asm').read_text().splitlines()
        after=(args.pair/'instructions'/f'{row["base"]}-old.asm').read_text().splitlines()
        assert len(before)==len(after)==row['rows']
        pcs.update({int(a.split()[0],16):int(b.split()[0],16)for a,b in zip(before,after)})
        for operand in row['memory']:operands[int(operand['base_va'],16)]=operand
    constants=[]
    for address in (0x45dbc0,0x44a0b4,0x45dc08):
        operand=operands[address];raw=image.read(address,4)
        assert raw.hex()==operand['base_hex']==operand['old_hex']
        constants.append(int.from_bytes(raw,'little'))
    lines=['LIVE_SOUND_SPATIAL_ORIGINAL_V1','K\t'+'\t'.join(f'{v:08x}'for v in constants)]
    names=('voices','sources','system','driver','shared_voices','tag_table','current_matg','current_local_index_address')
    captures=[]
    for directory in args.capture:
        replay_path=directory/'replay.json';actual_path=directory/'actual-boundary.json'
        replay,actual=[json.loads(p.read_text())for p in (replay_path,actual_path)]
        assert replay['image_sha256']==sha(image_path)
        assert actual['pc_equal'] and actual['esp_equal'] and actual['driver_equal'] and all(r['equal']for r in actual['writes'])
        assert not any(replay[k]for k in ('instruction_patches','intercepted_callees','guest_memory_writes'))
        for line in (directory/'instructions.tsv').read_text().splitlines():
            address,hex_bytes=line.split();raw=bytes.fromhex(hex_bytes)
            assert image.read(int(address,16),len(raw))==raw
            assert int(address,16)in pcs
        roots=replay['state_roots'];pages={}
        lines.append('M\t'+'\t'.join(f'{roots[n]:08x}'for n in names))
        for row in replay['pages']:
            path=directory/f'page-{row["address"]:08x}.bin'
            assert sha(path)==row['sha256'] and path.stat().st_size==4096
            pages[row['address']]=bytearray(path.read_bytes())
            lines.append(f'R\t{row["address"]:08x}\t{pages[row["address"]].hex()}')
        def raw(address,size):return bytes(pages[(address+i)&~4095][(address+i)&4095]for i in range(size))
        for name,cell in replay['root_cells'].items():assert int.from_bytes(raw(cell,4),'little')==roots[name]
        sp=replay['registers']['esp'];position_address=sp-0x30
        initial_position=raw(position_address,12)
        stores=[]
        for row in replay['writes']:
            address=row['address'];before=bytes.fromhex(row['before']);after=bytes.fromhex(row['after'])
            assert raw(address,row['bytes'])==before
            for i,value in enumerate(after):pages[(address+i)&~4095][(address+i)&4095]=value
            if not sp-0x10000<=address<sp+8:
                assert row['instruction']in pcs and row['bytes']in(1,2,4)
                stores.append((pcs[row['instruction']],address,row['bytes'],int.from_bytes(before,'little'),int.from_bytes(after,'little')))
        final_position=raw(position_address,12)
        driver=replay['driver_call']
        assert driver==actual['driver_call']
        lines.append(f'F\t{replay["registers"]["fctrl"]:04x}\t{replay["registers"]["mxcsr"]:08x}')
        lines.append('C\t'+'\t'.join([f'{replay["voice_index"]&65535:04x}',f'{replay["attenuation_address"]:08x}',
            initial_position.hex(),final_position.hex(),f'{driver["voice_index"]&0xffffffff:08x}'if driver else 'ffffffff',
            driver['parameters_hex']if driver else '-']))
        for pc,address,size,before,after in stores:lines.append(f'S\t{pc:08x}\t{address:08x}\t{size}\t{before:08x}\t{after:08x}')
        lines.append('E')
        captures.append(dict(directory=str(directory),replay_sha256=sha(replay_path),actual_boundary_sha256=sha(actual_path),
            voice_index=replay['voice_index'],driver_call=driver is not None,pages=len(pages),stores=len(stores)))
    fixture=args.output/'fixtures.tsv';fixture.write_text('\n'.join(lines)+'\n')
    report=dict(boundary=__doc__,captures=captures,tool_sha256=sha(__file__),image_sha256=sha(image_path),
        pair_sha256=sha(args.pair/'report.json'),fixture_sha256=sha(fixture),expected_values_from_native=False,
        limits=['Actual game CPU calls only; no synthetic source readiness, listener, pool or spatial request.',
                'The driver method at this boundary is not replayed. SDK, DSP and final audible mixing remain separate.'])
    (args.output/'provenance.json').write_text(json.dumps(report,indent=2)+'\n')
    print(f'Exported {len(captures)} original CPU calls, {sum(r["driver_call"]for r in captures)} driver requests, {sum(r["stores"]for r in captures)} retained stores')


if __name__=='__main__':main()
