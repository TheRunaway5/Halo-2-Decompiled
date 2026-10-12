#!/usr/bin/env python3
"""Compare actual preview coverage and per-part metrics to the strict native kernels.

The capture stops first at2CBC0, then the original1337DF sphere reader,
2CE7B byte stores and41116 part metrics in one UI clock. Captured original
outputs are immutable expectations. The later UI-camera calculation is retained
only to expose the wrong native context, not as a fitted alternative.
"""
import hashlib,json,struct,subprocess
from audit_rendering import Cache
from oracle_multiplayer_hud_positions import ROOT
base=ROOT/'analysis/frontend/customization';path=base/'original-preview-collector-prior-spartan.json'
capture=json.loads(path.read_text());rows=capture['rows'];kernel=base/'preview-shader-lod-kernel'
c=Cache(ROOT/'assets/disc/maps/mainmenu.map')
def words(data):return list(struct.unpack('<'+'I'*(len(data)//4),data))
def probe(row):
 p=subprocess.run([str(kernel),'--probe'],input=' '.join(f'{v:x}'for v in [len(row),*row])+'\n',text=True,capture_output=True,check=True)
 return [int(v,16)for v in p.stdout.split()]
owner=next(r for r in rows if r['pc']=='001337df');sphere=words(bytes.fromhex(owner['cached_sphere_hex']))
prior_camera=[int(v,16)for v in owner['coverage_camera_words']]
coverage=probe([7,*prior_camera,*sphere,int(owner['object_multiplier_word'],16)])
byte_rows=[r for r in rows if r['pc']=='0002ce7b']
checks=[dict(collector_index=r['collector_index'],original_coverage_word=r['actual_input_coverage_word'],original_byte=r['collector_byte'],native_coverage_word=f'{coverage[0]:08x}',native_byte=coverage[1],different_words=int(coverage[0]!=int(r['actual_input_coverage_word'],16))+int(coverage[1]!=r['collector_byte']))for r in byte_rows]
metric_rows=[r for r in rows if r['pc']=='00041116'];metrics=[]
for r in metric_rows:
 part=bytes.fromhex(r['part_record_hex']);assert int(r['resolved_part_scale_word'],16)==struct.unpack_from('<I',part,0x2c)[0]
 descriptor=bytes.fromhex(r['collector_pool_record_hex']);render_datum=struct.unpack_from('<I',descriptor,4)[0]
 _,mode=c.tag(render_datum,'mode');material=struct.unpack_from('<H',part,4)[0];material_rows=c.block(mode+0x60,32);datum=c.u32(material_rows[material]+12)
 tag,shader=c.tag(datum,'shad');pp=c.block(shader+32,124);_,template=c.tag(c.u32(pp[0]),'stem');tp=c.block(template+88,40)
 thresholds=[c.u32(v+6) for v in c.block(tp[0],10)]
 bias=c.u16(shader+0x3c);modifier=bytes.fromhex(r['globals_4ba010'])[9]
 # Metric arithmetic is independent of the explicit later view's cinematic
 # flag. Do not infer the captured multiplier bank from its resulting value.
 result=probe([2,0,480,0,0,coverage[1],int(r['resolved_part_scale_word'],16),int(r['part_multiplier_word'],16),bias,modifier,len(thresholds),*thresholds])
 bank=words(bytes.fromhex(r['lod_multipliers_hex']))
 choice=probe([1,bias,modifier,result[7],*bank,len(thresholds),*thresholds])
 source_choices=json.loads((base/'preview-shader-lod-choice-audit.json').read_text())['rows']
 expected=next(v['original_choice']for v in source_choices if int(v['shader'],16)==datum and v['metric_bits']==r['metric_word'] and list(map(lambda x:int(x,16),v['view_bits']))==bank)
 metrics.append(dict(collector_index=r['collector_index'],part_index=r['part_index'],shader=f'{datum:08x}',shader_name=tag['name'],scale_word=r['resolved_part_scale_word'],original_metric_word=r['metric_word'],native_metric_word=f'{result[7]:08x}',captured_view_words=r['lod_multipliers_hex'],original_choice=expected,native_choice=choice,different_words=int(result[7]!=int(r['metric_word'],16))+sum(a!=b for a,b in zip(choice,expected))))
ui_camera=[int(v,16)for v in metric_rows[0]['coverage_camera_words']]
wrong=probe([7,*ui_camera,*sphere,int(owner['object_multiplier_word'],16)])
report=dict(capture_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),mainmenu_sha256=hashlib.sha256(c.data).hexdigest(),kernel_binary_sha256=hashlib.sha256(kernel.read_bytes()).hexdigest(),method=__doc__.splitlines()[2:],owner=dict(object=owner['object'],definition=owner['definition'],sphere_hex=owner['cached_sphere_hex'],node0_hex=owner['node0_world_hex']),prior_camera_words=owner['coverage_camera_words'],actual_coverage_byte_checks=checks,actual_part_metric_checks=metrics,different_words=sum(r['different_words']for r in checks+metrics),wrong_later_ui_camera=dict(camera_words=metric_rows[0]['coverage_camera_words'],coverage_word=f'{wrong[0]:08x}',coverage_byte=wrong[1]),boundary='Literal source1337DF coverage/2CE7B quantization and actual41116 part metric exact for supplied original inputs; enclosing camera producer has a separate unchanged2232A0 fixture and production helper test. Lighting/pass binding and framebuffer parity are separate proofs.')
(base/'preview-collector-audit.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
