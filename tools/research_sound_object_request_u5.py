#!/usr/bin/env python3
"""Complete source sound-object owner/helper correspondence; read-only inputs."""
import argparse, json, struct
from pathlib import Path
import research_animation_selection_u5 as authority

OWNERS=(0x18dde0,0xbbcb0,0xbabd0,0xbb1f0,0xb9ac0,0xbb0c0,
        0x1d43c0,0x1d44c0,0x143ce0,0x18a800,0x11ceb0,0x11d4a0,
        0x126ab0,0x24bef0,0x24bf30,0x18eee0,0x18ef20,0xbbea0,0x156eb0)

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    authority.OWNERS=OWNERS;authority.BOUNDARIES=()
    authority.run(a.output)
    report=json.loads((a.output/'report.json').read_text())
    rows={r['old']:r for r in report['owners']}
    images=[authority.Xbe(path)for path in(authority.OLD,authority.NEW)]
    constants=[]
    for owner,address,name in ((0x143ce0,0x465030,'one_bits'),
            (0x18a800,0x46503c,'minus_one_bits'),(0x18a800,0x465298,'z_scale_bits'),
            (0x18a800,0x46529c,'xy_scale_bits')):
        references=[r for r in rows[f'{owner:08x}']['memory']if int(r['old_va'],16)==address]
        assert references and all(r['old_hex']==r['new_hex']for r in references)
        r=references[0]
        constants.append(dict(name=name,old_va=r['old_va'],u5_va=r['new_va'],raw4_hex=r['old_hex'],
                              original_uses=references))
    # BB0C0/1D43C0/1D44C0 dereference the immutable vector pointer. Retain
    # the pointer and its actual twelve bytes, never a substituted zero value.
    uses=[r for r in rows['000bb0c0']['memory']if r['old_va']=='0046fc44']
    assert len(uses)==1
    pointers=[struct.unpack('<I',im.read(int(uses[0][key],16),4))[0]
              for im,key in zip(images,('old_va','new_va'))]
    vectors=[im.read(address,12).hex()for im,address in zip(images,pointers)]
    assert vectors[0]==vectors[1]
    provider=dict(schema='source-sound-object-immutable-providers-v1',constants=constants,
        zero_vector=dict(old_global=uses[0]['old_va'],u5_global=uses[0]['new_va'],
                         old_target=f'{pointers[0]:08x}',u5_target=f'{pointers[1]:08x}',raw12_hex=vectors[0]),
        scope='Only immutable original scalar words and the dereferenced vector. Object/physics pools, current node banks, PVS bank, listeners, local bindings and current BSP remain caller-owned runtime inputs.',
        image_sha256=report['image_sha256'])
    authority.save(a.output/'providers.json',authority.encoded(provider))
    authority.save(a.output/'invocation.json',authority.encoded(dict(
        tool_sha256=authority.sha(__file__),authority_tool_sha256=authority.sha(authority.__file__),
        selected_owners=[f'{e:08x}'for e in OWNERS],new_inputs=[],instruction_patches=[])))

if __name__=='__main__':main()
