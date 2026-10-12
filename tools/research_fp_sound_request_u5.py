#!/usr/bin/env python3
"""Read-only complete-body FP sound request correspondence to authenticated u5."""
import argparse
import json
import struct
from pathlib import Path
import research_animation_selection_u5 as authority

OWNERS = (0x1687c2, 0x18aa90, 0x18aa40, 0x18acd0, 0x18ad70,
          0x18ef20, 0x18ba10, 0x18dde0, 0x18eee0, 0xbbea0, 0x156eb0)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    authority.OWNERS = OWNERS
    authority.BOUNDARIES = ()
    authority.run(args.output)
    report = json.loads((args.output/'report.json').read_text())
    rows = {row['old']: row for row in report['owners']}
    images = [authority.Xbe(path) for path in (authority.OLD, authority.NEW)]
    vectors = []
    for address in (0x46fc28, 0x46fc48):
        pair = next(item for item in rows['001687c2']['memory'] if int(item['old_va'],16)==address)
        pointers = [struct.unpack('<I',image.read(int(pair[key],16),4))[0]
                    for image,key in zip(images,('old_va','new_va'))]
        raw = [image.read(pointer,12).hex() for image,pointer in zip(images,pointers)]
        assert raw[0] == raw[1]
        vectors.append(dict(old_global=pair['old_va'],u5_global=pair['new_va'],
                            old_pointer=f'{pointers[0]:08x}',u5_pointer=f'{pointers[1]:08x}',raw12_hex=raw[0]))
    sequences = [(args.output/'instructions'/f'0018aa90-{label}.asm').read_text().splitlines() for label in ('old','u5')]
    callbacks=[]
    for left,right in zip(*sequences):
        old_tokens=left.split();new_tokens=right.split()
        if old_tokens[1].startswith('bf') and len(old_tokens[1])==10:
            addresses=[int.from_bytes(bytes.fromhex(tokens[1])[1:],'little') for tokens in (old_tokens,new_tokens)]
            tables=[image.read(address,32) for image,address in zip(images,addresses)]
            assert struct.unpack_from('<I',tables[0])[0] == 0x18dde0
            assert struct.unpack_from('<I',tables[1])[0] == int(rows['0018dde0']['u5'],16)
            callbacks.append(dict(old_address=f'{addresses[0]:08x}',u5_address=f'{addresses[1]:08x}',
                old_table32_hex=tables[0].hex(),u5_table32_hex=tables[1].hex(),
                create_owner_complete_body_proven=True,later_callbacks_not_ported_here=True))
    current_bsp=next(item for item in rows['0018ad70']['memory'] if item['old_va']=='0046fb64')
    assert current_bsp['old_hex']==current_bsp['new_hex']
    current_bsp['runtime_owner_required']=True
    current_bsp['fresh_image_bytes_are_not_live_default']=True
    current_bsp['source_semantics']='Current active BSP word; original 18AD70 copies it to request+2E. Not an immutable sentinel.'
    authority.save(args.output/'providers.json',authority.encoded(dict(
        schema='source-fp-sound-request-providers-v1',vectors=vectors,callbacks=callbacks,
        current_bsp_word=current_bsp,image_sha256=report['image_sha256'],
        scope='Dereferenced immutable basis values and callback identities; current-BSP word reference requires a runtime owner. No device, listener, source datum, scheduler success, RNG or live pose supplied.')))
    authority.save(args.output/'invocation.json', authority.encoded(dict(
        controlling_tool_sha256=authority.sha(__file__),
        reused_authority_tool_sha256=authority.sha(authority.__file__),
        selected_owners=[f'{entry:08x}' for entry in OWNERS],
        overrides='Read-only body comparator owner list only; no original executable, inputs, returns or request fields changed.')))


if __name__=='__main__':main()
