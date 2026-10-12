#!/usr/bin/env python3
"""Original CPU sound-submit/voice/format bodies, not device admission."""
import argparse,json,struct
from pathlib import Path
import research_animation_selection_u5 as authority

OWNERS=(0x21f220,0x128380,0x128320,0x126760,0x129450,0x12c370,
 0x126930,0x21b900,0x128610,0x127750,0x1275b0,0x128140,0x127290,
 0x129a20,0x12ad30,0x12b900,0x12b670,0x12bba0,0x12bd20,0x220520,
 0x2215f0,0x221760,0x2219b0,0x221b90,0x220d10,0x220f90,0x2b4970,
 0x2b4810,0x2b48a0,0x2b4b30,0x2b4bf0,0x2b47c0,0x12c760,
 0x21fe40,0x18eac0,0x18eba0,0x12a3f0,0x2b4ce0,0x2b4d60,0x2b4d90,
 0x2b4dd0,0x2b4e10,0x222840)

CONSTANTS=(
 (0x221b90,0x4650bc,'source_pitch_to_driver_pitch_bits'),
 (0x221b90,0x4650b8,'driver_pitch_min_bits'),
 (0x221b90,0x4650b4,'driver_pitch_max_bits'),
 (0x221b90,0x46549c,'gain_normalized_to_driver_volume_bits'),
 (0x221b90,0x45120c,'minimum_source_gain_bits'),
 (0x221b90,0x4650d8,'parameter_change_threshold_bits'),
 (0x221b90,0x446548,'driver_pitch_change_threshold_bits'),
 (0x221760,0x4652ec,'driver_pitch_octave_scale_bits'),
 (0x221760,0x4511a4,'relative_rate_ln2_bits'),
 (0x220520,0x465030,'mono_channel_count_bits'),
 (0x220520,0x46507c,'stereo_channel_count_bits'),
 (0x221b90,0x451214,'routing_db_ln10_bits'),
 (0x221b90,0x46508c,'routing_db_scale_bits'),
 (0x221b90,0x465078,'routing_linear_db_scale_bits'),
 (0x12c760,0x46504c,'normalized_parameter_range_epsilon_bits'))

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 authority.OWNERS=OWNERS;authority.BOUNDARIES=();authority.run(a.output)
 report=json.loads((a.output/'report.json').read_text());rows={r['old']:r for r in report['owners']}
 images=[authority.Xbe(p)for p in(authority.OLD,authority.NEW)];constants=[]
 for owner,address,name in CONSTANTS:
  matches=[r for r in rows[f'{owner:08x}']['memory']if int(r['old_va'],16)==address]
  assert matches and all(r['old_hex']==r['new_hex']for r in matches),(name,matches)
  r=matches[0];constants.append(dict(name=name,old_va=r['old_va'],u5_va=r['new_va'],raw4_hex=r['old_hex'],uses=matches))
 # An immediate pointer is not included in the memory-operand list. Preserve
 # its actual loaded immutable word independently, paired through instruction
 # position rather than a presumed image-address delta.
 owner=rows['00221b90'];files=[a.output/'instructions/00221b90-old.asm',a.output/'instructions/00221b90-u5.asm']
 lines=[p.read_text().splitlines()for p in files];pos=next(n for n,l in enumerate(lines[0])if l.startswith('00221bd0 '))
 addresses=[int(lines[i][pos].split()[-1],16)for i in range(2)]
 values=[im.read(v,4).hex()for im,v in zip(images,addresses)];assert values[0]==values[1]
 constants.append(dict(name='silent_source_gain_bits',old_va=f'{addresses[0]:08x}',u5_va=f'{addresses[1]:08x}',raw4_hex=values[0],uses=[dict(old_site='00221bd0',u5_site=lines[1][pos].split()[0],kind='immediate_pointer_dereference')]))
 out=dict(schema='source-sound-platform-voice-immutable-providers-v1',constants=constants,
  source_pitch_bases=[-4597,-501,-2396],driver_parameter_fields_prefix_bytes=0x50,
  whole_effect_parameter_packet_extent_not_promoted=True,driver_stream_record_bytes=0x34,
  scope='Only original immutable words and encoded immediate bounds. Actual sound-system availability, driver records, source/voice ownership, listeners, resource/RNG/clock inputs and actual platform handles remain separately required.',image_sha256=report['image_sha256'])
 authority.save(a.output/'providers.json',authority.encoded(out))
 authority.save(a.output/'invocation.json',authority.encoded(dict(tool_sha256=authority.sha(__file__),authority_tool_sha256=authority.sha(authority.__file__),selected_owners=[f'{e:08x}'for e in OWNERS],new_gameplay_inputs=[],new_successful_platform_returns=[],instruction_patches=[])))
 print(json.dumps(dict(providers_sha256=authority.sha(a.output/'providers.json'))))

if __name__=='__main__':main()
