#!/usr/bin/env python3
"""Execute unchanged TU1.5 game startup with an observed native XNet API input.

No game bank snapshot, constructor field, allocation cursor, pool validity or
completion byte is imported. Actual anonymous host pages and real temporary
file open/seek/truncate outcomes cross named platform boundaries. Mutex and
semaphore objects are real host resources. The reference emulator uses an
explicit cooperative thread schedule: main yields run the complete original
worker until its next unavailable semaphore wait. This is a scheduler input,
not an Xbox thread timing/TLS/priority contract or live Match/world proof.
The priority boundary attempts the native operation on an owned host thread;
its real failure is allowed because the source caller ignores that BOOL.

The XNet API uses a captured actual host interface observation from the native
port, not a forced online/offline result. Its declared native cdecl ABI returns
the decoded flags and preserves other source CPU state. Original Xbox PHY/IRQL
code is not executed at this platform boundary. All game instructions after
the API call execute unchanged until the next unimplemented initializer.
With --advance-online-startup, XNet/Winsock are declared native platform API
boundaries too. Their results come from a measured native context receipt;
Xbox's Ethernet-driver/secure-network initialization is not emulated or claimed.
With --advance-online-services, the existing desktop online clients are also a
declared native library boundary. Authentication/discovery/matchmaking are not
started; the original game proceeds to its actual title-address query.
With --complete-network, the title-address queries use measured kernel/manager
observations too. The complete original network initializer and its two literal
RET callbacks execute, stopping before the next real RNG initializer.
With --complete-rng, the complete original heap constructor also executes,
followed by source time/rand/CRT allocation and RNG initialization. Native UTC,
monotonic tick and ordinary executor thread identity are measured inputs. The
next initializer remains pending; there is no supplied random result.
With --complete-session, native socket resources, logical-to-native endpoint
translation and private file absence are replayed measured platform inputs.
Original session/configuration/protocol/simulation constructors all execute.
The scalar SSE backend verifies exact arithmetic results on hardware
(including relocated read-only scalar operands) and supplies MXCSR sticky
status that the emulator omits; no game opcode is
patched and no source completion, random or gameplay result is supplied.
With --complete-input, the complete source controller producer1273C0 executes
before13A700. Controller changes/open/state/capability/feedback boundaries replay
measured native SDL resource receipts. The complete21E5D0 preset constructor
then returns; the next real1589D0 callback remains pending. Original callers,
dead zones, button hysteresis and presence publication execute unchanged.
With --complete-bink-memory, the complete1589D0 registration constructor and
its real BINK-section jump wrapper execute. Original377460 stores the two
allocator/release callback identities; no platform API return is substituted.
The next literal RET also executes before1A5410 remains pending. This does not
claim the later Bink buffer callbacks or movie decode/presentation are complete.
With --complete-level-handles, original1A5410 constructs both level-handle datum
pools through the already-owned allocator, initializes all64 table pairs and
returns. New raw pages are actual native resources; no pool header, validity
bit, table entry or gameplay result is supplied. The next134B00 remains pending.
With --complete-renderer, both original renderer descriptor constructors and
their ordinary process-heap allocations execute. Fresh native page commitment
is replayed through its measured resource frames. Descriptor/pool/header bytes
and allocation results remain products of original code, not supplied state.
With --complete-clock, original22C100 constructs its four fields,126640 executes
its already-initialized arena branch and149050 allocates/constructs clock
storage through the real source arena/CRC path. The next1E7710 remains pending.
With --complete-state-context, original1E7710 allocates228 bytes through the
same arena/CRC path, clears its complete destination and publishes5020C4.
No consumer meaning or initialized state is supplied;14DFE0 remains pending.
With --complete-players, original14DFE0 constructs its sixteen-slot player datum
pool and304-byte player-state allocation through the owned arena/CRC path.
The exact127210 allocator body executes; no pool validity/header or player
state is supplied. The next1888A0 remains pending.
With --complete-view-storage, original1888A0,1EECF0 and159F80 construct their
arena-backed view/action/storage banks. Only original writes initialize the
allocations and static destinations;69220 remains pending.
With --complete-static-registration, original69220 and85E70 clear both static
registration arrays, publish43 literal source definition identities and write
counts17/26. The actual next literal RET also executes;1F3830 stays pending.
These opaque source identities are not native host pointers.
With --complete-scalar-state, original1F3830 allocates28 arena bytes and copies
its exact three scalar and three vector operands, then clears the last field.
The following actual RET also executes;B87F0 remains pending.
With --complete-object-storage, originalB87F0 and its complete object-type,
widget/light/device/cluster constructors execute. Native formatting is an
explicit CRT boundary; source allocators construct all pool/header/chain and
object storage bytes. The next10E3B0 remains pending.
With --complete-aux-object-state, original10E3B0,10FA30 and1F4700 construct their
arena allocations and exact clear fields. Consumer meanings remain unresolved;
the actual12560 renderer/resource constructor stays pending.
With --renderer-device-request, original12560,169D0,16A10 and128A0 run through
their actual logical viewport, display fields and device request producer.
The real3FF320 SDK device-creation call stays pending; no device, GPU result
or renderer initialization completion is supplied.
With --bind-native-graphics-device, the real port Vulkan renderer is an
explicit native graphics adapter. The measured device binding, opaque handle
and defined native return cross3FF320; no Xbox driver state is imported.
Original128A0 copies its authored control defaults and publishes its own byte;
the next125C8 ->16AF0 remains pending, not complete renderer initialization.
With --complete-renderer-state, original16AF0 and1C190 construct their complete
cold renderer state and bit-mask table. The actual125D2 ->1C7F0 allocation
constructor remains pending; no GPU resource or completion state is supplied.
With --complete-renderer-pool, original1C7F0 and13FC50 construct their owned
raw-backed cache pool. Existing original allocation/array routines execute;
the next125E6 ->1D3D0 palette wrapper remains pending.
With --native-renderer-palette, the engine graphics palette wrapper and SDK
stage setter are explicit native graphics APIs. Real renderer-owned color
resources, four bindings and defined native return ABIs are measured inputs.
Original1D3D0/15AF0 Xbox object allocation internals are not executed or claimed.
Source12560 continues unchanged to12653 ->400EF0; no shader program, Xbox
driver bank, renderer completion or gameplay result is supplied.
With --native-renderer-constants,400EF0 selects the native renderer existing
application-owned logical192-register configuration. Live uniform storage and
descriptor capacity are verified; Xbox SDK allocation/state is not supplied.
The original caller next reaches1265A ->3FF5D0 with depth-state input2.
With --native-renderer-depth,3FF5D0 selects the existing live native D24S8 world
W-depth path. Actual attachment/LEQUAL probes are required in the host receipt;
Xbox SDK viewport/program-upload internals are not imported. Original12560
continues to12669 ->3FFDD0, ECX4035C, EDX1 (depth-write enable).
With --native-renderer-output, the complete next eight-call output block is
mapped to live native pipeline/uniform defaults. Original12560 writes its
scalar caches and reaches1276B ->3FFE00 (texture stage0), left pending.
No Xbox driver bank, shader program or completed renderer is supplied.
With --native-renderer-depth-state, native depth writes, LESS_EQUAL selection
and disabled depth bias consume the three reached SDK calls. Original12560
writes its two scalar caches unchanged, then stops at1269E ->3FFDD0,
ECX40338 and EDX0 (polygon-offset fill disabled). No Xbox FIFO is imported.
"""
import argparse,ctypes,hashlib,json,mmap,os,struct,tempfile,threading
from pathlib import Path
from unicorn import Uc,UC_ARCH_X86,UC_MODE_32,UC_HOOK_CODE,UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN as PIN

def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True)
 ap.add_argument('--host-link-observation',type=Path,required=True)
 ap.add_argument('--advance-xnet-startup',action='store_true')
 ap.add_argument('--advance-online-startup',action='store_true')
 ap.add_argument('--advance-online-services',action='store_true')
 ap.add_argument('--complete-network',action='store_true');ap.add_argument('--complete-rng',action='store_true');ap.add_argument('--complete-session',action='store_true');ap.add_argument('--complete-cache',action='store_true');ap.add_argument('--complete-input',action='store_true');ap.add_argument('--complete-bink-memory',action='store_true');ap.add_argument('--complete-level-handles',action='store_true');ap.add_argument('--complete-renderer',action='store_true');ap.add_argument('--complete-clock',action='store_true');ap.add_argument('--complete-state-context',action='store_true');ap.add_argument('--complete-players',action='store_true');ap.add_argument('--complete-view-storage',action='store_true');ap.add_argument('--complete-static-registration',action='store_true');ap.add_argument('--complete-scalar-state',action='store_true');ap.add_argument('--complete-object-storage',action='store_true');ap.add_argument('--complete-aux-object-state',action='store_true');ap.add_argument('--renderer-device-request',action='store_true');ap.add_argument('--bind-native-graphics-device',action='store_true');ap.add_argument('--complete-renderer-state',action='store_true');ap.add_argument('--complete-renderer-pool',action='store_true');ap.add_argument('--native-renderer-palette',action='store_true');ap.add_argument('--native-renderer-constants',action='store_true');ap.add_argument('--native-renderer-depth',action='store_true');ap.add_argument('--native-renderer-depth-state',action='store_true');ap.add_argument('--native-renderer-output',action='store_true');a=ap.parse_args()
 if a.native_renderer_output:a.native_renderer_depth_state=True
 if a.native_renderer_depth_state:a.native_renderer_depth=True
 if a.native_renderer_depth:a.native_renderer_constants=True
 if a.native_renderer_constants:a.native_renderer_palette=True
 if a.native_renderer_palette:a.complete_renderer_pool=True
 if a.complete_renderer_pool:a.complete_renderer_state=True
 if a.complete_renderer_state:a.bind_native_graphics_device=True
 if a.bind_native_graphics_device:a.renderer_device_request=True
 if a.renderer_device_request:a.complete_aux_object_state=True
 if a.complete_aux_object_state:a.complete_object_storage=True
 if a.complete_object_storage:a.complete_scalar_state=True
 if a.complete_scalar_state:a.complete_static_registration=True
 if a.complete_static_registration:a.complete_view_storage=True
 if a.complete_view_storage:a.complete_players=True
 if a.complete_players:a.complete_state_context=True
 if a.complete_state_context:a.complete_clock=True
 if a.complete_clock:a.complete_renderer=True
 if a.complete_renderer:a.complete_level_handles=True
 if a.complete_level_handles:a.complete_bink_memory=True
 if a.complete_bink_memory:a.complete_input=True
 if a.complete_input:a.complete_cache=True
 if a.complete_cache:a.complete_session=True
 if a.complete_session:a.complete_rng=True
 if a.complete_rng:a.complete_network=True
 if a.complete_network:a.advance_online_services=True
 if a.advance_online_services:a.advance_online_startup=True
 if a.advance_online_startup:a.advance_xnet_startup=True
 native=json.loads(a.host_link_observation.read_text());session_calls=native.get('native_socket_calls',[]);session_index=[0];tick_inputs=native.get('native_millisecond_inputs',[]);tick_index=[0];raw_next=[0x10000000];host=native['host_link_observation']
 flags=0
 if host['administratively_up']:
  assert isinstance(host['carrier'],bool)
  if host['carrier']:
   flags=1
   if host['speed_mbps']==100:flags|=2
   elif host['speed_mbps']==10:flags|=4
   if host['duplex']=='full':flags|=8
   elif host['duplex']=='half':flags|=16
 assert native['native_link_flags']==flags
 if a.advance_xnet_startup:assert flags&1,'Actual active host carrier is required'
 a.output.mkdir(parents=True,exist_ok=False)
 source=NEW_DEFAULT;assert sha(source)==PIN
 image=Xbe(source);text_code=_Code(image,'.text');bink_code=_Code(image,'BINK')
 class Code:
  def at(self,p):return text_code.at(p) if text_code.inside(p) else bink_code.at(p) if bink_code.inside(p) else None
 code=Code()
 native_apis={};depth_state_calls=native.get('native_depth_state_calls',[]);depth_state_index=[0];output_calls=native.get('native_output_calls',[]);output_index=[0];xnet_parameter_address=[None];title_inputs=native.get('native_title_address_calls',[]);title_index=[0]
 title_flags_report=None
 if a.complete_network:
  assert len(title_inputs)==2 and native['source_network_startup_complete'] and native['source_qos_pool_activated']
  for row in title_inputs:
   observation=row['observation'];payload=bytearray(36)
   if observation['ipv4']is not None:payload[:4]=bytes(observation['ipv4'])
   payload[10:16]=bytes(observation['ethernet']);assert payload.hex()==row['bytes']
   encoded=0 if observation['acquisition_pending'] else (2 if observation['ipv4']is None else 8 if observation['assignment']=='dhcp' else 4)
   if not observation['acquisition_pending']:
    if observation['gateway']is not None and any(observation['gateway']):encoded|=0x20
    if observation['dns']:encoded|=0x40
   assert row['return_eax']==encoded
  flag_path=ROOT/'analysis/asset-setup/mcc-native-20261014/native-startup/original-title-address-flags.json'
  title_flags_report=json.loads(flag_path.read_text());assert title_flags_report['source_sha256']==PIN and title_flags_report['cases']==24
 if a.advance_online_startup:
  assert native['native_network_context_initialized'] and native['native_network_provider_probe_succeeded']
  assert native['xnet_references']==native['winsock_references']==1
  assert native['native_network_pool_address_bits']==64 and native['native_network_private_pool_bytes']==49152
  defaults=image.read(0x3ea2d0,76);minimum=image.read(0x3ea320,76);maximum=image.read(0x3ea370,76)
  effective=bytearray(defaults);params=bytes.fromhex(native['xnet_startup_parameter_bytes']);assert params==bytes.fromhex('0c000c0804081808080828ff')
  for i,value in enumerate(params):effective[i]=min(max(value,minimum[i]),maximum[i]) if value else defaults[i]
  assert list(effective)==native['native_network_configuration']
  native_apis={row['target']:row for row in native['native_api_calls']}
  assert set(native_apis)==({0x3d7656,0x3d766d,0x3aad19}if a.advance_online_services else {0x3d7656,0x3d766d})
  if a.advance_online_services:
   assert native['native_online_initialized'] and not native['native_online_authenticated'] and native['native_browser_requests']==0
   assert native['native_online_context_address_bits']==64 and native['native_online_references']==1
 paired_path=ROOT/'analysis/multiplayer/sandbox-source/physics-game-arena-tu15-20261012/correspondence-v1/complete-report.json'
 physical_path=ROOT/'analysis/multiplayer/sandbox-source/physics-physical-storage-tu15-20261012/correspondence-v1/complete-report.json'
 assert sha(paired_path)=='616bee89417916769418d9f2a70075ab036c2bba8a1decadd46698c5d57f1933'
 assert sha(physical_path)=='4e81e7a5fd05edae4d24a35152896df4191aed827da55ba1d87b559bcb8e540f'
 pair=json.loads(paired_path.read_text());physical_pair=json.loads(physical_path.read_text())
 world_path=ROOT/'analysis/multiplayer/sandbox-source/physics-game-frame-runtime-20261011/correspondence-v2/complete-report.json'
 assert sha(world_path)=='92379da9dc9d29baea9eaca194cebdab7d69f8be319aecf6cd0b759e26e744cc'
 world_pair=json.loads(world_path.read_text())
 pool_path=ROOT/'analysis/multiplayer/sandbox-source/physics-pool-startup-tu15-20261012/correspondence-v1/complete-report.json'
 assert sha(pool_path)=='6d10227d9838a4e6e2e2db443df0c699bbb203a7843a2b6f756a569fd755b527'
 pool_pair=json.loads(pool_path.read_text())
 entries={int(p,16):q for d in (pair,physical_pair)for p,q in d['entries'].items()}
 entries[0x13a700]=0x13a700;entries[0x1273c0]=0x1273c0
 imap={p:q for d in (pair,physical_pair)for p,q in d['instruction_map'].items()};allowed=set(imap.values())
 control_map={p:q for p,q in world_pair['instruction_map'].items() if 0x3276fc<=int(p,16)<=0x327897}
 allowed.update(control_map.values())
 allowed.update(pool_pair['instruction_map'].values())
 if a.complete_rng:allowed.update(world_pair['instruction_map'].values())
 if a.complete_session:
  session_body_path=ROOT/'analysis/asset-setup/mcc-native-20261014/native-startup/next-network-owner/network-session-bodies.json'
  session_bodies=json.loads(session_body_path.read_text());assert session_bodies['original_sha256']==PIN
  allowed.update(session_bodies['instruction_map'].values())
 if a.complete_cache:
  cache_body_path=ROOT/'analysis/asset-setup/mcc-native-20261014/native-startup/cache-startup/cache-startup-bodies.json'
  cache_bodies=json.loads(cache_body_path.read_text());assert cache_bodies['original_sha256']==PIN
  allowed.update(cache_bodies['instruction_map'].values())
 if a.complete_input:
  input_body_path=ROOT/'analysis/asset-setup/mcc-native-20261014/native-startup/input-startup/input-startup-bodies.json'
  input_bodies=json.loads(input_body_path.read_text());assert input_bodies['original_sha256']==PIN
  allowed.update(input_bodies['instruction_map'].values())
 input_calls=native.get('native_input_calls',[]);input_index=[0]
 if a.complete_bink_memory:
  bink_body_path=ROOT/'analysis/asset-setup/mcc-native-20261014/native-startup/bink-memory-startup/bink-memory-startup-bodies.json'
  bink_bodies=json.loads(bink_body_path.read_text());assert bink_bodies['original_sha256']==PIN
  allowed.update(bink_bodies['instruction_map'].values())
 if a.complete_level_handles:
  level_body_path=ROOT/'analysis/asset-setup/mcc-native-20261014/native-startup/level-handles-startup/level-handles-startup-bodies.json'
  level_bodies=json.loads(level_body_path.read_text());assert level_bodies['original_sha256']==PIN
  allowed.update(level_bodies['instruction_map'].values())
 heap_commit_calls=native.get('native_heap_commit_calls',[]);heap_commit_index=[0]
 if a.complete_renderer:
  renderer_path=ROOT/'analysis/multiplayer/sandbox-source/physics-renderer-storage-tu15-20261012/correspondence-v1/complete-report.json'
  assert sha(renderer_path)=='59dd620260f89a6ccc4900879f7c39281117feeec6eff042a6722f6c055f83d5'
  renderer=json.loads(renderer_path.read_text());allowed.update(renderer['instruction_map'].values())
  growth_path=ROOT/'analysis/asset-setup/mcc-native-20261014/native-startup/renderer-startup/heap-growth-bodies.json'
  growth=json.loads(growth_path.read_text());assert growth['original_sha256']==PIN;allowed.update(growth['instruction_map'].values())
 if a.complete_clock:
  state_body_path=ROOT/'analysis/asset-setup/mcc-native-20261014/native-startup/clock-startup/startup-state-bodies.json'
  state_bodies=json.loads(state_body_path.read_text());assert state_bodies['original_sha256']==PIN;allowed.update(state_bodies['instruction_map'].values())
 if a.complete_state_context:
  context_body_path=ROOT/'analysis/asset-setup/mcc-native-20261014/native-startup/state-context-startup/state-context-startup-bodies.json'
  context_bodies=json.loads(context_body_path.read_text());assert context_bodies['original_sha256']==PIN;allowed.update(context_bodies['instruction_map'].values())
 if a.complete_view_storage:
  view_body_path=ROOT/'analysis/asset-setup/mcc-native-20261014/native-startup/view-storage/bodies.json'
  view_bodies=json.loads(view_body_path.read_text());assert view_bodies['original_sha256']==PIN
  for sites in view_bodies['body_sites'].values():allowed.update(sites)
 if a.complete_static_registration:
  registration_body_path=ROOT/'analysis/asset-setup/mcc-native-20261014/native-startup/static-registration/static-registration-bodies.json'
  registration_bodies=json.loads(registration_body_path.read_text());assert registration_bodies['original_sha256']==PIN
  for sites in registration_bodies['body_sites'].values():allowed.update(sites)
 if a.complete_object_storage:
  object_body_path=ROOT/'analysis/asset-setup/mcc-native-20261014/native-startup/object-storage/data-and-bodies.json'
  object_bodies=json.loads(object_body_path.read_text());assert object_bodies['original_sha256']==PIN
  for sites in object_bodies['body_sites'].values():allowed.update(sites)
 if a.complete_aux_object_state:
  auxiliary_body_path=ROOT/'analysis/asset-setup/mcc-native-20261014/native-startup/aux-object-state/aux-object-state-bodies.json'
  auxiliary_bodies=json.loads(auxiliary_body_path.read_text());assert auxiliary_bodies['original_sha256']==PIN
  for sites in auxiliary_bodies['body_sites'].values():allowed.update(sites)
 if a.renderer_device_request:
  renderer_request_body_path=ROOT/'analysis/asset-setup/mcc-native-20261014/native-startup/renderer-resource/renderer-request-bodies.json'
  renderer_request_bodies=json.loads(renderer_request_body_path.read_text());assert renderer_request_bodies['original_sha256']==PIN
  for sites in renderer_request_bodies['body_sites'].values():allowed.update(sites)
 if a.complete_renderer_state:
  renderer_state_body_path=ROOT/'analysis/asset-setup/mcc-native-20261014/native-startup/renderer-state/renderer-state-bodies.json'
  renderer_state_bodies=json.loads(renderer_state_body_path.read_text());assert renderer_state_bodies['original_sha256']==PIN
  for sites in renderer_state_bodies['body_sites'].values():allowed.update(sites)
 if a.complete_renderer_pool:
  renderer_pool_body_path=ROOT/'analysis/asset-setup/mcc-native-20261014/native-startup/renderer-pool/renderer-pool-bodies.json'
  renderer_pool_bodies=json.loads(renderer_pool_body_path.read_text());assert renderer_pool_bodies['original_sha256']==PIN
  for sites in renderer_pool_bodies['body_sites'].values():allowed.update(sites)
 for start,size,pin in ((0x6de60,61,'56f605989b6fb19f929f71f9b534ac85415f824d4ff1588d0a1ef5f61734b463'),
                        (0x70660,87,'d8a807d350cdf84609c1bb3ba95f355dbf0cfea882d829b3473e411c139f67d9'),
                        (0x7ec90,76,'d658e9fed01a7fb3f002226e440421820570864d0d017d09d9618b96e1061811'),
                        (0x6e010,295,'250f77c6b998f2fcb3742c0f5304a2c4d3900eaa32b27dc6572ca26ac477b674'),
                        (0x70ce0,74,'9acaff9cb8b6feb4cf8adb25b99b882b69de7116cb15155db3171ce9a0c9c3e2'),
                        (0x707d0,234,'8807f11d2895268a08793c6c8d3061063f302c00677ef968caa03b854c820775')):
  assert hashlib.sha256(image.read(start,size)).hexdigest()==pin
  p=start
  while p<start+size:allowed.add(p);p+=code.at(p).size
 # The actual-ID pool constructor/activation/byte-copy bodies are independently
 # lowered from this authenticated image. Admit only those complete bodies.
 import re
 support=(ROOT/'src/physics_live_support_tu15_generated.cpp').read_text()
 for name in (('16e450','16e4b0','16e600','32a4e0','148d40','32bbae','32c075','33b590','330179','332383','2dc124','32ae00','33a4d3') if a.complete_rng else ('16e450','16e4b0','16e600','32a4e0')) + (('2e0c80',) if a.complete_renderer else ()) + (('149050',) if a.complete_clock else ()) + (('14dfe0',) if a.complete_players else ()) + (('1f3830',) if a.complete_scalar_state else ()) + (('10e3b0','1f4700') if a.complete_aux_object_state else ()) + (('13fc50',) if a.complete_renderer_pool else ()):
  m=re.search(r'static void run_'+name+r'\([^\n]*\)\s*\{',support);assert m
  body=support[m.start():support.index('\n}\n',m.start())+3]
  allowed.update(int(p,16)for p in re.findall(r'services.instruction\(0x([a-f0-9]+)u\)',body))
 assert image.read(0x31d400,1)==b'\xc3';allowed.add(0x31d400)
 assert hashlib.sha256(image.read(0x13a700,123)).hexdigest()=='1370232f655b7636c1e00e623b3d171d901f36088e978d15e12f398cb56b1405'
 p=0x13a700
 while p<0x13a77b:allowed.add(p);p+=code.at(p).size
 frames=json.loads((paired_path.parent.parent/'external-owners.json').read_text())
 frame_map={row['old']:row for routine in pair['routines']for row in routine['instructions']}
 expected_frames={body['later_target']:{frame_map[call['old_site']]['later']+len(bytes.fromhex(frame_map[call['old_site']]['later_bytes']))for call in body['calls']}for body in frames['external_direct_calls'].values()}
 expected_frames.setdefault(0x32a4e0,set()).add(0x16e4c9) # Complete original QoS pool name-copy caller.
 expected_frames[0x32a4e0].add(0x16e622) # Complete original pool activation's two-byte copy.
 if a.complete_renderer_pool:
  assert code.at(0x13fc97).mnemonic=='call' and code.at(0x13fc97).operands[0].imm==0x32a4e0
  expected_frames[0x32a4e0].add(0x13fc9c) # Authenticated13FC50 authored pool-name copy.
 if a.complete_session:
  for sites in [*session_bodies['body_sites'].values(),*(cache_bodies['body_sites'].values() if a.complete_cache else []),*(object_bodies['body_sites'].values() if a.complete_object_storage else []),*(renderer_pool_bodies['body_sites'].values() if a.complete_renderer_pool else [])]:
   for site in sites:
    ins=code.at(site)
    if ins.mnemonic=='call' and ins.operands[0].type==2 and ins.operands[0].imm in expected_frames:
     expected_frames[ins.operands[0].imm].add(site+ins.size)
 instruction_count=[0]
 g=Uc(UC_ARCH_X86,UC_MODE_32)
 g.mem_map(0,0x810000)
 for s in image.sections:g.mem_write(s.va,image.section_bytes(s))
 g.mem_map(0x80000000,4096)
 main_stack,worker_stack,stop=0x60000000,0x60100000,0x60200000
 g.mem_map(main_stack,0x100000 if a.complete_rng else 0x10000);g.mem_map(worker_stack,0x10000)
 g.mem_map(stop,0x2000 if a.complete_rng else 4096)
 heap_phase=[False];heap_resources=[];heap_mappings=[];rng_inputs=native.get('native_rng_inputs')
 if a.complete_rng:
  assert isinstance(rng_inputs,dict) and native['source_rng_initializer_complete'] and native['source_crt_heap_allocation_complete']
  from oracle_native_tu15_heap_data import segment
  gdt=0x60400000;g.mem_map(gdt,4096);g.mem_write(gdt+8,segment(0,0x9b)+segment(0,0x93)+segment(stop,0x93))
  g.reg_write(R.UC_X86_REG_GDTR,(0,gdt,0xfff,0))
  for register,value in ((R.UC_X86_REG_CS,8),(R.UC_X86_REG_DS,16),(R.UC_X86_REG_ES,16),(R.UC_X86_REG_SS,16),(R.UC_X86_REG_FS,24)):g.reg_write(register,value)
  assert g.reg_read(R.UC_X86_REG_FS_BASE)==stop
  g.mem_map(0x60212000,0x3000)
  g.mem_write(stop,struct.pack('<I',0xffffffff));g.mem_write(stop+0x28,struct.pack('<I',0x60213000));g.mem_write(stop+0x20,struct.pack('<I',0x60214000))
  g.mem_write(0x6021312c,struct.pack('<I',rng_inputs['thread_token']))
  # Rebind the imported data cell to retained native tick storage. No game
  # opcode or immediate is changed; the actual MOV/RET instructions execute.
  g.mem_write(0x41b774,struct.pack('<I',0x60212000))
  libc=ctypes.CDLL(None,use_errno=True)
  libc.mmap.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_int,ctypes.c_int,ctypes.c_int,ctypes.c_long];libc.mmap.restype=ctypes.c_void_p
  libc.mprotect.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_int];libc.munmap.argtypes=[ctypes.c_void_p,ctypes.c_size_t]
 regs=(R.UC_X86_REG_EAX,R.UC_X86_REG_ECX,R.UC_X86_REG_EDX,R.UC_X86_REG_EBX,R.UC_X86_REG_ESP,R.UC_X86_REG_EBP,R.UC_X86_REG_ESI,R.UC_X86_REG_EDI)
 def u(p,n=4):return int.from_bytes(g.mem_read(p,n),'little')
 def w(p,v,n=4):g.mem_write(p,v.to_bytes(n,'little'))
 def string(p):
  raw=bytearray()
  while True:
   b=u(p+len(raw),1)
   if not b:return raw.decode('ascii')
   raw.append(b);assert len(raw)<4096
 def ret(n,eax=None):
  sp=g.reg_read(R.UC_X86_REG_ESP);target=u(sp)
  if eax is not None:g.reg_write(R.UC_X86_REG_EAX,eax&0xffffffff)
  g.reg_write(R.UC_X86_REG_ESP,sp+4+n);g.reg_write(R.UC_X86_REG_EIP,target)
 frontiers=[];services=[];executed=set();site=[0];store_count=[0];role=['main'];paused=[False];yielded=[False];worker=[None]
 prefix=[False];prefix_stores=[];prefix_instructions=[];game_frontier=[None]
 sse_pending=[None];sse_rows=[]
 if a.complete_session:
  from oracle_native_sse_registers import NativeSseRegisters
  native_sse=NativeSseRegisters()
 palette_calls=native.get("native_palette_calls",[]);palette_index=[0]
 sync={};semaphores={};files={};physical=[];next_sync=[0xf3000001];next_file=[0xf4000001]
 priority_ready=threading.Event();priority_stop=threading.Event();priority_id=[None]
 def priority_task():priority_id[0]=threading.get_native_id();priority_ready.set();priority_stop.wait()
 native_priority_thread=threading.Thread(target=priority_task);native_priority_thread.start();priority_ready.wait()
 resource_directory=tempfile.TemporaryDirectory(prefix='halo2-source-death-startup-')
 output=(a.output/'stores.bin').open('wb')
 def record(p,n,value):
  if main_stack<=p<main_stack+(0x100000 if a.complete_rng else 0x10000) or worker_stack<=p<worker_stack+0x10000:return
  assert n in (1,2,4,8)
  output.write(struct.pack('<IIBQQ',site[0],p,n,u(p,n),value&((1<<(n*8))-1)));store_count[0]+=1
  if prefix[0]:prefix_stores.append(dict(instruction=site[0],address=p,before=bytes(g.mem_read(p,n)).hex(),written=(value&((1<<(n*8))-1)).to_bytes(n,'little').hex()))
 def write(uc,access,p,n,value,user):record(p,n,value)
 def hook(uc,p,n,user):
  if sse_pending[0]is not None:
   instruction,lanes,status=sse_pending[0]
   assert [g.reg_read(getattr(R,f'UC_X86_REG_XMM{i}'))for i in range(8)]==lanes,'emulated original SSE result differs from exact hardware instruction'
   g.reg_write(R.UC_X86_REG_MXCSR,status);sse_rows.append(dict(instruction=instruction,mxcsr=status));sse_pending[0]=None
  if p in ((0x3ffe00,) if a.native_renderer_output else (0x3ffdd0,) if a.native_renderer_depth else (0x3ff5d0,) if a.native_renderer_constants else (0x400ef0,) if a.native_renderer_palette else (0x1d3d0,) if a.complete_renderer_pool else (0x1c7f0,) if a.complete_renderer_state else (0x16af0,) if a.bind_native_graphics_device else (0x3ff320,) if a.renderer_device_request else (0x12560,) if a.complete_aux_object_state else (0x10e3b0,) if a.complete_object_storage else (0xb87f0,) if a.complete_scalar_state else (0x1f3830,) if a.complete_static_registration else (0x69220,) if a.complete_view_storage else (0x1888a0,) if a.complete_players else (0x14dfe0,) if a.complete_state_context else (0x1e7710,) if a.complete_clock else (0x22c100,) if a.complete_renderer else (0x134b00,) if a.complete_level_handles else (0x1a5410,) if a.complete_bink_memory else (0x1589d0,) if a.complete_input else (0x21e5d0,) if a.complete_cache else (0x21c0a0,) if a.complete_session else (0x6e830,) if a.complete_rng else (0x148d40,) if a.complete_network else (0x3d725d,0x31d400) if a.advance_online_services else (0x3aad19,0x31d400) if a.advance_online_startup else (0x3d7656,0x31d400) if a.advance_xnet_startup else (0x6e010,0x31d400)) and (a.native_renderer_output or not a.native_renderer_depth_state or u(g.reg_read(R.UC_X86_REG_ESP))==0x126a3):
   game_frontier[0]=abi();g.emu_stop();return
  output_sites=(0x1269e,0x126b0,0x126c5,0x126db,0x126ed,0x12702,0x12718,0x1272d)
  if a.native_renderer_output and p==0x3ffdd0 and u(g.reg_read(R.UC_X86_REG_ESP)) in tuple(q+5 for q in output_sites):
   index=output_index[0];assert index<8 and len(output_calls)==8
   measured=output_calls[index];sp=g.reg_read(R.UC_X86_REG_ESP)
   methods=(0x40338,0x40300,0x4033c,0x40340,0x40304,0x40344,0x40348,0x40350)
   values=(0,0,0x204,0,0,1,0,0x8006)
   assert measured['instruction']==output_sites[index] and measured['target']==p and measured['entry_esp']==sp and u(sp)==output_sites[index]+5
   assert [g.reg_read(r)for r in regs]==measured['entry_gp'] and g.reg_read(R.UC_X86_REG_EFLAGS)==measured['entry_eflags']
   assert g.reg_read(R.UC_X86_REG_ECX)==methods[index] and g.reg_read(R.UC_X86_REG_EDX)==values[index]
   assert measured['values']==list(values) and measured['pipeline_handle'] and measured['state_backing']>0xffffffff and measured['material_backing']>0xffffffff
   ret(0)
   assert [g.reg_read(r)for r in regs]==measured['returned_gp'] and g.reg_read(R.UC_X86_REG_EFLAGS)==measured['returned_eflags']
   output_index[0]+=1;services.append(dict(kind='measured_native_renderer_output_state',call=measured));return
  if a.native_renderer_depth_state and p in (0x3ffdd0,0x3ffa90):
   index=depth_state_index[0];assert index<3 and len(depth_state_calls)==3
   measured=depth_state_calls[index];sp=g.reg_read(R.UC_X86_REG_ESP)
   assert measured['instruction']==(0x12669,0x12682,0x12692)[index] and measured['target']==p==(0x3ffdd0,0x3ffdd0,0x3ffa90)[index]
   assert measured['entry_esp']==sp and u(sp)==(0x1266e,0x12687,0x12697)[index]
   assert [g.reg_read(r)for r in regs]==measured['entry_gp'] and g.reg_read(R.UC_X86_REG_EFLAGS)==measured['entry_eflags']
   if index<2:
    assert measured['arguments']==[] and g.reg_read(R.UC_X86_REG_ECX)==(0x4035c,0x40354)[index] and g.reg_read(R.UC_X86_REG_EDX)==(1,0x203)[index]
   else:assert measured['arguments']==[0] and u(sp+4)==0
   assert measured['write_enabled']==1 and measured['function']==0x203 and measured['bias']==0
   assert measured['state_backing']>0xffffffff and measured['attachment_handle']
   ret(4 if index==2 else 0)
   assert [g.reg_read(r)for r in regs]==measured['returned_gp'] and g.reg_read(R.UC_X86_REG_EFLAGS)==measured['returned_eflags']
   depth_state_index[0]+=1;services.append(dict(kind='measured_native_renderer_depth_state',call=measured));return
  if a.native_renderer_depth and p==0x3ff5d0:
   measured=native['native_depth_mode_call'];sp=g.reg_read(R.UC_X86_REG_ESP)
   assert measured['instruction']==0x1265a and measured['target']==p and measured['entry_esp']==sp and u(sp)==0x1265f
   assert [g.reg_read(r)for r in regs]==measured['entry_gp'] and g.reg_read(R.UC_X86_REG_EFLAGS)==measured['entry_eflags']
   assert u(sp+4)==measured['argument']==measured['mode']==2 and measured['format']==129
   assert measured['depth_bits']==24 and measured['stencil_bits']==8 and measured['attachment_handle'] and measured['state_backing']>0xffffffff
   assert native['native_depth_gpu_attachment_and_occlusion_verified'] and native['native_depth_control_retirement_verified']
   assert [r['raw_d24'] for r in native['native_depth_gpu_samples']]==[123456,65536]
   ret(4)
   assert [g.reg_read(r)for r in regs]==measured['returned_gp'] and g.reg_read(R.UC_X86_REG_EFLAGS)==measured['returned_eflags']
   services.append(dict(kind='measured_native_renderer_depth_configuration',call=measured));return
  if a.native_renderer_constants and p==0x400ef0:
   measured=native['native_constant_mode_call'];sp=g.reg_read(R.UC_X86_REG_ESP)
   assert measured['instruction']==0x12653 and measured['target']==p and measured['entry_esp']==sp and u(sp)==0x12658
   assert [g.reg_read(r)for r in regs]==measured['entry_gp'] and g.reg_read(R.UC_X86_REG_EFLAGS)==measured['entry_eflags']
   assert g.reg_read(R.UC_X86_REG_EAX)==measured['mode']==0x11 and measured['registers']==192 and measured['application_owned']
   assert measured['material_backing']>0xffffffff and measured['extended_backing']>0xffffffff and measured['extended_bytes']==2544
   ret(0)
   assert [g.reg_read(r)for r in regs]==measured['returned_gp'] and g.reg_read(R.UC_X86_REG_EFLAGS)==measured['returned_eflags']
   services.append(dict(kind='measured_native_renderer_constant_configuration',call=measured));return
  if a.native_renderer_palette and p in (0x1d3d0,0x4025b0):
   measured=palette_calls[palette_index[0]];palette_index[0]+=1;sp=g.reg_read(R.UC_X86_REG_ESP)
   assert measured['target']==p and measured['entry_esp']==sp and u(sp)==measured['instruction']+5
   assert [g.reg_read(r)for r in regs]==measured['entry_gp']
   assert g.reg_read(R.UC_X86_REG_EFLAGS)==measured['entry_eflags']
   assert [u(sp+4+i*4)for i in range(len(measured['arguments']))]==measured['arguments']
   assert measured['native_color_address']>0xffffffff and measured['native_device_address']>0xffffffff
   assert measured['handle']==0xf9000101
   assert hashlib.sha256(g.mem_read(0x474da8,1024)).hexdigest()==measured['color_sha256']
   if p==0x1d3d0:
    assert measured['instruction']==0x125e6 and g.reg_read(R.UC_X86_REG_EAX)==3 and measured['arguments']==[0x474da8,0x4915dc] and u(0x4915dc)==0
    g.mem_write(0x4915dc,struct.pack('<I',measured['handle']));ret(8,1)
   else:
    stage=palette_index[0]-2
    assert 0<=stage<4 and g.reg_read(R.UC_X86_REG_EAX)==stage and measured['arguments']==[measured['handle']]
    assert measured['instruction']==(0x125fd,0x1260e,0x1261f,0x1262f)[stage]
    ret(4)
   assert [g.reg_read(r)for r in regs]==measured['returned_gp'] and g.reg_read(R.UC_X86_REG_EFLAGS)==measured['returned_eflags']
   services.append(dict(kind='measured_native_renderer_palette_api',call=measured));return
  if p==0x3ff320 and a.bind_native_graphics_device:
   measured=native['native_graphics_device_call'];sp=g.reg_read(R.UC_X86_REG_ESP)
   assert measured['native_renderer_live'] and measured['native_device_address']>0xffffffff
   assert measured['instruction']==0x129ad and measured['target']==p and measured['entry_esp']==sp and u(sp)==0x129b2
   assert g.reg_read(R.UC_X86_REG_EAX)==measured['flags']==0x40 and g.reg_read(R.UC_X86_REG_ECX)==measured['output_token']==0x515e90
   parameters=u(sp+4);assert parameters==measured['parameters_token']==sp+0x14
   assert [u(parameters+4*i)for i in range(17)]==measured['presentation']
   assert measured['handle']==0xf9000001 and u(measured['output_token'])==0
   g.mem_write(measured['output_token'],struct.pack('<I',measured['handle']));ret(4,0)
   assert [g.reg_read(r)for r in regs]==measured['returned_gp']
   services.append(dict(kind='measured_native_port_renderer_binding',call=measured));return
  if p==0x37654d:
   assert u(g.reg_read(R.UC_X86_REG_ESP)) in (0x6de85,0x6e092)
   services.append(dict(kind='native_xnet_link_query',observation=host,result=flags))
   ret(0,flags);return
  sp=g.reg_read(R.UC_X86_REG_ESP)
  args=lambda count:[u(sp+4+4*i)for i in range(count)]
  if a.complete_input and p in (0x4139a0,0x4139a5,0x413a12,0x413a68,0x413a74,0x413c4c,0x413cbf):
   measured=input_calls[input_index[0]];input_index[0]+=1
   assert measured['target']==p and measured['instruction']==site[0] and measured['entry_esp']==sp and u(sp)==site[0]+5
   assert args(len(measured['arguments']))==measured['arguments']
   if p in (0x4139a0,0x413a68):assert measured['return_eax']==g.reg_read(R.UC_X86_REG_EAX)
   for value in measured['writes']:g.mem_write(value['address'],bytes.fromhex(value['bytes']))
   ret(len(measured['arguments'])*4,measured['return_eax']);assert g.reg_read(R.UC_X86_REG_ESP)==measured['return_esp']
   services.append(dict(kind='measured_native_input',call=measured));return
  if p==0x3d725d and a.complete_network:
   measured=title_inputs[title_index[0]];title_index[0]+=1
   assert sp==measured['entry_esp'] and u(sp)==measured['instruction']+5 and args(1)[0]==measured['output']
   g.mem_write(measured['output'],bytes.fromhex(measured['bytes']));ret(4,measured['return_eax'])
   assert g.reg_read(R.UC_X86_REG_ESP)==measured['return_esp']
   services.append(dict(kind='measured_native_title_address',observation=measured['observation'],result=measured['return_eax']));return
  if p in native_apis:
   measured=native_apis[p];assert measured['entry_esp']==sp and measured['return_eax']==0
   if p==0x3d7656:
    assert u(sp)==0x6e09c and bytes(g.mem_read(args(1)[0],12))==params
    xnet_parameter_address[0]=args(1)[0]
    ret(4,measured['return_eax'])
   elif p==0x3d766d:
    assert u(sp)==0x6e0b0 and args(2)[0]==2
    output=args(2)[1]
    for offset,size,value in ((0,2,2),(2,2,0x202),(0x18c,4,0),(0x186,2,0),(0x188,2,0),(4,1,0),(0x105,1,0)):
     g.mem_write(output+offset,value.to_bytes(size,'little'))
    ret(8,measured['return_eax'])
   else:
    assert p==0x3aad19 and u(sp)==0x6e0c2 and u(args(1)[0])==0
    ret(4,measured['return_eax'])
   assert g.reg_read(R.UC_X86_REG_ESP)==measured['return_esp']
   services.append(dict(kind='measured_native_network_api',target=p,return_eax=measured['return_eax']));return
  if a.complete_rng and p==0x33b595:
   value=tick_inputs[tick_index[0]] if a.complete_session else rng_inputs['monotonic_milliseconds'];tick_index[0]+=1
   w(0x60212000,value)
  if a.complete_session and p in (0x3d7268,0x3d72b2,0x3d72a3,0x3d7289,0x3d7294):
   measured=session_calls[session_index[0]];session_index[0]+=1
   assert p==measured['target'] and sp==measured['entry_esp'] and u(sp)==measured['instruction']+5 and args(len(measured['arguments']))==measured['arguments']
   for output in measured['outputs']:w(output['address'],output['value'])
   ret(len(measured['arguments'])*4,measured['result']);assert g.reg_read(R.UC_X86_REG_ESP)==measured['return_esp']
   services.append(dict(kind='measured_native_session_socket_api',**measured));return
  if a.complete_session and p==0x2e0b1e:
   logical=string(args(1)[0]);measured=native['native_cache_directory_probe'] if a.complete_cache and logical=='n:\\sgf' else native['native_profile_probe']
   assert sp==measured['entry_esp'] and u(sp)==measured['instruction']+5 and logical==measured['logical_path']
   assert measured['attributes']==0xffffffff and measured['error']==2
   ret(4,measured['attributes']);services.append(dict(kind='measured_native_private_path_absence',**measured));return
  if a.complete_cache and p==0x33bb15:
   measured=native['native_cache_directory_creation'];assert sp==measured['entry_esp'] and u(sp)==measured['instruction']+5 and string(args(2)[0])==measured['logical_path'] and args(2)[1]==0
   assert measured['result']==1 and measured['error']==0
   ret(8,measured['result']);services.append(dict(kind='measured_native_cache_directory_creation',**measured));return
  if a.complete_cache and p==0x80000018:
   bp=g.reg_read(R.UC_X86_REG_EBP);assert u(sp)==0x33a4be and args(5)==[7,bp-8,bp-4,4,0]
   w(bp-8,4);w(bp-4,native['native_language_preference']['platform_value'])
   ret(20,0);services.append(dict(kind='measured_native_language_preference',**native['native_language_preference']));return
  if a.complete_session and p==0x2dbc2e:
   assert u(sp)==0x139421 and native['native_profile_probe']['error']==2
   ret(0,2);return
  if a.complete_rng and p==0x80000080:
   assert u(sp)==0x32bbbc and args(1)[0]==g.reg_read(R.UC_X86_REG_EBP)-8
   g.mem_write(args(1)[0],struct.pack('<Q',rng_inputs['utc_filetime']))
   services.append(dict(kind='measured_native_utc',filetime=rng_inputs['utc_filetime']));ret(4);return
  if a.complete_rng and p in (0x80000123,0x80000115,0x80000126):
   section=args(1)[0]
   if p==0x80000123:
    assert u(sp)==0x2de66c
    w(section,1,1);w(section+2,4,1)
    for offset,value in ((4,0),(12,section+8),(8,section+8),(16,0xffffffff),(20,0),(24,0)):w(section+offset,value)
   elif p==0x80000115:
    assert u(sp)==0x2de763 and u(section+16)==0xffffffff and u(section+20)==u(section+24)==0
    w(section+16,0);w(section+24,rng_inputs['thread_token']);w(section+20,1)
   else:
    assert u(sp) in (0x2ded14,0x2dee88) and u(section+16)==0 and u(section+20)==1 and u(section+24)==rng_inputs['thread_token']
    w(section+20,0);w(section+16,0xffffffff);w(section+24,0)
   services.append(dict(kind='native_heap_critical_section',target=p,section=section));ret(4);return
  if p==0x800000b8 and heap_phase[0]:
   base_cell,zero_bits,size_cell,kind,protection=args(5)
   base,count=u(base_cell),u(size_cell);assert zero_bits==0 and protection==4 and count>0 and count%4096==0
   if kind==0x2000:
    assert u(sp)==0x2de52b and not heap_mappings and base==0
    host_pages=libc.mmap(None,count,0,0x22,-1,0)
    if host_pages==ctypes.c_void_p(-1).value:raise OSError(ctypes.get_errno(),'native heap reservation failed')
    base=0x08000000;heap_mappings.append((host_pages,count));g.mem_map(base,count,0)
   else:
    assert u(sp)==0x2de564 and kind==0x1000 and len(heap_mappings)==1
    host_pages,extent=heap_mappings[0];assert 0x08000000<=base<base+count<=0x08000000+extent
    if libc.mprotect(host_pages+base-0x08000000,count,3):raise OSError(ctypes.get_errno(),'native heap commit failed')
    g.mem_protect(base,count,3);g.mem_write(base,ctypes.string_at(host_pages+base-0x08000000,count))
   heap_resources.append(dict(kind=kind,base=base,bytes=count));w(base_cell,base);w(size_cell,count);ret(20,0);return
  if a.complete_renderer and p==0x800000b8 and u(sp)==0x2dd3de:
   measured=heap_commit_calls[heap_commit_index[0]];heap_commit_index[0]+=1
   base_cell,zero,size_cell,kind,protection=args(5);bp=g.reg_read(R.UC_X86_REG_EBP)
   assert measured['instruction']==0x2dd3d8 and measured['base_cell']==base_cell==bp+12 and measured['size_cell']==size_cell==g.reg_read(R.UC_X86_REG_EBX)==u(bp+16)
   assert [u(base_cell),zero,u(size_cell),kind,protection]==measured['request'] and zero==0 and kind==0x1000 and protection==4
   status,base,count=measured['result'];assert status==0 and base==u(base_cell) and count==u(size_cell) and count%4096==0
   host_pages,extent=heap_mappings[0];assert 0x08000000<=base<base+count<=0x08000000+extent
   if libc.mprotect(host_pages+base-0x08000000,count,3):raise OSError(ctypes.get_errno(),'native ordinary heap commitment failed')
   g.mem_protect(base,count,3);g.mem_write(base,ctypes.string_at(host_pages+base-0x08000000,count))
   w(base_cell,base);w(size_cell,count);ret(20,status);heap_resources.append(dict(kind=kind,base=base,bytes=count));return
  if p==0x800000b8:
   base_cell,zero_bits,size_cell,kind,protection=args(5)
   assert u(sp)==0x33b3c5 and u(base_cell)==zero_bits==0 and kind==0x101000 and protection==4
   assert a.complete_session or u(size_cell)==0x150
   requested=u(size_cell);size=(requested+4095)&~4095;base=raw_next[0];raw_next[0]+=size
   pages=mmap.mmap(-1,size);physical.append(pages);g.mem_map(base,size);g.mem_write(base,pages[:])
   w(base_cell,base);w(size_cell,size)
   services.append(dict(kind='shared_raw_allocate',arguments=[0,zero_bits,requested,kind,protection],result=[0,base,size]))
   ret(20,0);return
  if p in expected_frames:
   assert u(sp) in expected_frames[p],(hex(p),hex(u(sp)))
   frontiers.append(dict(target=p,role=role[0],abi=abi()))
  if p==0x800000a6:
   size,low,high,alignment,protection=args(5);assert (size,low,high,alignment,protection)==(0x30e4000,0x61000,0x3144fff,0,4)
   pages=mmap.mmap(-1,size);base=low|0x80000000;g.mem_map(base,size);g.mem_write(base,pages[:]);physical.append(pages)
   services.append(dict(kind='physical_allocate',arguments=args(5),result=base));ret(20,base);return
  if p==0x800000b6:
   base,size,protection=args(3);assert protection==4 and base>=0x80061000 and base+size<=0x83145000
   # Anonymous pages already have actual host read/write protection. The
   # native comparison additionally invokes mprotect on this retained range.
   services.append(dict(kind='physical_protect',arguments=args(3)));ret(12);return
  if p==0x2e02ea:
   assert args(3)==[0,0,0];token=next_sync[0];next_sync[0]+=1;sync[token]=threading.RLock()
   services.append(dict(kind='mutex_create',result=token));ret(12,token);return
  if p==0x2e0269:
   assert args(4)==[0,0,150,0];token=next_sync[0];next_sync[0]+=1;semaphores[token]=threading.BoundedSemaphore(150)
   for _ in range(150):assert semaphores[token].acquire(False)
   services.append(dict(kind='semaphore_create',result=token));ret(16,token);return
  if p==0x2dc0b6:
   assert args(6)==[0,0x4000,0x1235f0,0,0,0]
   context=g.context_save()
   for q in regs:g.reg_write(q,0)
   g.reg_write(R.UC_X86_REG_ESP,worker_stack+0xf000);g.reg_write(R.UC_X86_REG_EFLAGS,2)
   g.reg_write(R.UC_X86_REG_EIP,0x1235f0);worker[0]=g.context_save();g.context_restore(context)
   services.append(dict(kind='worker_create',entry=0x1235f0,result=0xf5000001));ret(24,0xf5000001);return
  if p==0x2dbea6:
   assert args(2)==[0xf5000001,1]
   try:os.setpriority(os.PRIO_PROCESS,priority_id[0],-1);result=1;error=0
   except OSError as e:result=0;error=e.errno
   services.append(dict(kind='native_priority_attempt',result=result,error=error));ret(8,result);return
  if p==0x2e0478:
   token,milliseconds=args(2);assert milliseconds==0xffffffff
   if token in sync:assert sync[token].acquire(False)
   else:
    assert token in semaphores and role[0]=='worker'
    if not semaphores[token].acquire(False):paused[0]=True;g.emu_stop();return
   ret(8,0);return
  if p==0x2e0342:
   token,=args(1);sync[token].release();ret(4,1);return
  if p==0x2e02c4:
   token,count,previous=args(3);assert count==1 and token in semaphores
   semaphores[token].release();w(previous,0);ret(12,1);return
  if p==0x2dbf74:
   assert role[0]=='main';ret(0);yielded[0]=True;g.emu_stop();return
  if p==0x32ba80:
   destination,capacity,fmt,arguments=args(4);pattern=string(fmt)
   assert pattern in ('n:\\savegame%d.bin','cluster %s','%s cluster','%s reference'), (hex(fmt),repr(pattern))
   value=u(arguments);value=string(value)if '%s'in pattern else struct.unpack('<i',struct.pack('<I',value))[0]
   raw=(pattern%value).encode('ascii');assert len(raw)<capacity
   g.mem_write(destination,raw+b'\0');ret(0,len(raw));return
  if p==0x2dc640:
   name,access,share,security,creation,file_flags,template=args(7)
   assert (access,share,security,creation,file_flags,template)==(0xc0000000,0,0,4,0,0)
   name=string(name);assert name in ('n:\\savegame0.bin','n:\\savegame1.bin')
   path=Path(resource_directory.name)/name[3:]
   try:fd=os.open(path,os.O_RDWR|os.O_CREAT,0o600);token=next_file[0];next_file[0]+=1;files[token]=(fd,path);error=0
   except OSError as e:token=0xffffffff;error=e.errno
   services.append(dict(kind='file_open',source_name=name,result=token,error=error));ret(28,token);return
  if p==0x2dc378:
   token,offset,high,method=args(4);assert method==0 and u(high)==0
   result=os.lseek(files[token][0],offset,os.SEEK_SET);assert result<=0xffffffff
   services.append(dict(kind='file_seek',file=token,offset=offset,result=result));ret(16,result);return
  if p==0x2dc2f4:
   token,=args(1);fd,path=files[token];length=os.lseek(fd,0,os.SEEK_CUR);os.ftruncate(fd,length)
   services.append(dict(kind='file_truncate',file=token,bytes=os.stat(path).st_size));ret(4,1);return
  assert (heap_phase[0] or p in allowed),hex(p)
  if a.complete_session and code.at(p).mnemonic in ('cvtsi2ss','mulss','divss','addss','subss'):
   ins=code.at(p);lanes,status=native_sse.evaluate(ins,[g.reg_read(r)for r in regs],
    [g.reg_read(getattr(R,f'UC_X86_REG_XMM{i}'))for i in range(8)],g.reg_read(R.UC_X86_REG_MXCSR),g.mem_read)
   sse_pending[0]=(p,lanes,status)
  instruction_count[0]+=1
  assert bytes(g.mem_read(p,n))==image.read(p,n),(hex(p),n)
  executed.add(p);site[0]=p
  if prefix[0]:prefix_instructions.append(p)
 g.hook_add(UC_HOOK_CODE,hook);g.hook_add(UC_HOOK_MEM_WRITE,write)
 def abi():return dict(gp=[g.reg_read(r)for r in regs],eflags=g.reg_read(R.UC_X86_REG_EFLAGS),
  x87_control=g.reg_read(R.UC_X86_REG_FPCW),x87_status=g.reg_read(R.UC_X86_REG_FPSW),mxcsr=g.reg_read(R.UC_X86_REG_MXCSR),
  xmm=[g.reg_read(getattr(R,f'UC_X86_REG_XMM{i}')).to_bytes(16,'little').hex()for i in range(8)])
 def run(entry,arguments=(),call_stack=None):
  role[0]='main'
  for q in regs:g.reg_write(q,0)
  sp=call_stack or main_stack+0xf000;w(sp,stop);g.reg_write(R.UC_X86_REG_ESP,sp);g.reg_write(R.UC_X86_REG_EFLAGS,2)
  g.reg_write(R.UC_X86_REG_FPCW,0x27f);g.reg_write(R.UC_X86_REG_MXCSR,0x1f80)
  g.reg_write(R.UC_X86_REG_FPSW,0);g.reg_write(R.UC_X86_REG_FPTAG,0xffff)
  for i in range(8):g.reg_write(getattr(R,f'UC_X86_REG_XMM{i}'),0)
  for i,value in enumerate(arguments):w(sp+4+4*i,value)
  before=abi();pc=entry if entry==0x2de2ea else entries[entry]
  while True:
   yielded[0]=False;g.emu_start(pc,stop,count=12000000)
   if not yielded[0]:break
   main=g.context_save();role[0]='worker';g.context_restore(worker[0]);paused[0]=False
   g.emu_start(g.reg_read(R.UC_X86_REG_EIP),stop,count=12000000);assert paused[0]
   worker[0]=g.context_save();g.context_restore(main);role[0]='main';pc=g.reg_read(R.UC_X86_REG_EIP)
  assert g.reg_read(R.UC_X86_REG_EIP)==(((0x3ffe00 if a.native_renderer_output else 0x3ffdd0 if a.native_renderer_depth else 0x3ff5d0 if a.native_renderer_constants else 0x400ef0 if a.native_renderer_palette else 0x1d3d0 if a.complete_renderer_pool else 0x1c7f0 if a.complete_renderer_state else 0x16af0 if a.bind_native_graphics_device else 0x3ff320 if a.renderer_device_request else 0x12560 if a.complete_aux_object_state else 0x10e3b0 if a.complete_object_storage else 0xb87f0 if a.complete_scalar_state else 0x1f3830 if a.complete_static_registration else 0x69220 if a.complete_view_storage else 0x1888a0 if a.complete_players else 0x14dfe0 if a.complete_state_context else 0x1e7710 if a.complete_clock else 0x22c100 if a.complete_renderer else 0x134b00 if a.complete_level_handles else 0x1a5410 if a.complete_bink_memory else 0x1589d0 if a.complete_input else 0x21e5d0 if a.complete_cache else 0x21c0a0 if a.complete_session else 0x6e830 if a.complete_rng else 0x148d40 if a.complete_network else 0x3d725d if a.advance_online_services else 0x3aad19 if a.advance_online_startup else 0x3d7656 if a.advance_xnet_startup else 0x6e010) if flags&1 else 0x31d400) if entry==0x13a700 else stop)
  return dict(entry=f'{entry:08x}',arguments=arguments,entry_abi=before,exit_abi=abi(),eax=g.reg_read(R.UC_X86_REG_EAX),stores=store_count[0])
 steps=[]
 try:
  if a.complete_rng:
   heap_phase[0]=True;w(stop+0x1000,0x30)
   steps.append(run(0x2de2ea,(2,0,0x100000,0x1000,0,stop+0x1000),main_stack+0xf0000))
   assert g.reg_read(R.UC_X86_REG_EAX)==0x08000000 and u(stop)==0xffffffff
   w(0x56ee28,g.reg_read(R.UC_X86_REG_EAX));heap_phase[0]=False
  for entry in (0x12cb70,0x1221b0,0x125280):
   steps.append(run(entry));print(json.dumps(steps[-1]),flush=True)
  source_input_prelude=None
  if a.complete_input:
   prefix[0]=True;steps.append(run(0x1273c0))
   source_input_prelude=dict(**abi(),instructions=list(prefix_instructions),stores=list(prefix_stores))
   assert game_frontier[0] is None
   prefix_instructions.clear();prefix_stores.clear()
  prefix[0]=True;steps.append(run(0x13a700));assert game_frontier[0] is not None
  language_remap_cases=[]
  if a.complete_cache:
   saved=g.context_save();prefix[0]=False
   for value in (0,1,2,3,4,5,6,7,8,9,10,255,0xffffffff):
    initial=[value,0x11223344,0x22334455,0x33445566,main_stack+0xe800,0x44556677,0x55667788,0x66778899]
    for register,word in zip(regs,initial):g.reg_write(register,word)
    g.reg_write(R.UC_X86_REG_EFLAGS,2);w(initial[4],stop)
    g.emu_start(0x11f440,stop,count=100)
    assert g.reg_read(R.UC_X86_REG_EIP)==stop
    language_remap_cases.append(dict(input=value,initial_gp=initial,initial_eflags=2,gp=[g.reg_read(r)for r in regs],eflags=g.reg_read(R.UC_X86_REG_EFLAGS)))
   g.context_restore(saved)
  output.close()
  finals=[]
  for s in image.sections:
   if not s.flags&1:continue
   raw=bytes(g.mem_read(s.va,s.vsize));finals.append(dict(address=s.va,bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest()))
  raw=bytes(g.mem_read(0x80061000,0x30e4000));finals.append(dict(address=0x80061000,bytes=len(raw),sha256=hashlib.sha256(raw).hexdigest()))
  listing=a.output/'executed-original.asm';listing.write_text(''.join(f'{p:08x} {code.at(p).bytes.hex()} {code.at(p).mnemonic} {code.at(p).op_str}\n'for p in sorted(executed)))
  arena=u(0x4f2950)
  report=dict(source_sha256=PIN,oracle_sha256=sha(__file__),boundaries=__doc__,steps=steps,stores=store_count[0],store_file='stores.bin',store_sha256=sha(a.output/'stores.bin'),final_banks=finals,services=services,external_frontiers=frontiers,arena=arena,arena_offset=u(0x4f2954),async_free_head=u(0x4ef1b8),async_work_head=u(0x4ef1bc),arena_checksum=u(0x4f295c),cache_files=[dict(token=p,bytes=os.stat(path).st_size)for p,(fd,path)in files.items()],executed_original_instructions=instruction_count[0],executed_sites=len(executed),canonical_instruction_map=imap,correspondence_sha256=sha(paired_path),physical_correspondence_sha256=sha(physical_path),instruction_listing_sha256=sha(listing),instruction_patches=[],supplied_gameplay_results=[],live_match_proof=False)
  report.update(schema='H2_NATIVE_TU15_GRAPHICS_OUTPUT_ORIGINAL_V1' if a.native_renderer_output else 'H2_NATIVE_TU15_GRAPHICS_DEPTH_STATE_ORIGINAL_V1' if a.native_renderer_depth_state else 'H2_NATIVE_TU15_GRAPHICS_DEPTH_ORIGINAL_V1' if a.native_renderer_depth else 'H2_NATIVE_TU15_GRAPHICS_CONSTANTS_ORIGINAL_V1' if a.native_renderer_constants else 'H2_NATIVE_TU15_GRAPHICS_PALETTE_ORIGINAL_V1' if a.native_renderer_palette else 'H2_NATIVE_TU15_GRAPHICS_POOL_ORIGINAL_V1' if a.complete_renderer_pool else 'H2_NATIVE_TU15_GRAPHICS_STATE_ORIGINAL_V1' if a.complete_renderer_state else 'H2_NATIVE_TU15_GRAPHICS_DEVICE_ORIGINAL_V1' if a.bind_native_graphics_device else 'H2_NATIVE_TU15_RENDERER_REQUEST_ORIGINAL_V1' if a.renderer_device_request else 'H2_NATIVE_TU15_AUX_OBJECT_STATE_STARTUP_ORIGINAL_V1' if a.complete_aux_object_state else 'H2_NATIVE_TU15_OBJECT_STORAGE_STARTUP_ORIGINAL_V1' if a.complete_object_storage else 'H2_NATIVE_TU15_SCALAR_STATE_STARTUP_ORIGINAL_V1' if a.complete_scalar_state else 'H2_NATIVE_TU15_STATIC_REGISTRATION_STARTUP_ORIGINAL_V1' if a.complete_static_registration else 'H2_NATIVE_TU15_VIEW_STORAGE_STARTUP_ORIGINAL_V1' if a.complete_view_storage else 'H2_NATIVE_TU15_PLAYER_STARTUP_ORIGINAL_V1' if a.complete_players else 'H2_NATIVE_TU15_STATE_CONTEXT_STARTUP_ORIGINAL_V1' if a.complete_state_context else 'H2_NATIVE_TU15_CLOCK_STARTUP_ORIGINAL_V1' if a.complete_clock else 'H2_NATIVE_TU15_RENDERER_STARTUP_ORIGINAL_V1' if a.complete_renderer else 'H2_NATIVE_TU15_LEVEL_HANDLES_STARTUP_ORIGINAL_V1' if a.complete_level_handles else 'H2_NATIVE_TU15_BINK_MEMORY_STARTUP_ORIGINAL_V1' if a.complete_bink_memory else 'H2_NATIVE_TU15_INPUT_STARTUP_ORIGINAL_V1' if a.complete_input else 'H2_NATIVE_TU15_CACHE_STARTUP_ORIGINAL_V1' if a.complete_cache else 'H2_NATIVE_TU15_NETWORK_SESSION_ORIGINAL_V1' if a.complete_session else 'H2_NATIVE_TU15_RNG_STARTUP_ORIGINAL_V1' if a.complete_rng else 'H2_NATIVE_TU15_NETWORK_COMPLETE_ORIGINAL_V1' if a.complete_network else 'H2_NATIVE_TU15_ONLINE_SERVICES_ORIGINAL_V1' if a.advance_online_services else 'H2_NATIVE_TU15_NETWORK_PLATFORM_ORIGINAL_V1' if a.advance_online_startup else 'H2_NATIVE_TU15_NETWORK_CONNECTION_ORIGINAL_V1' if a.advance_xnet_startup else 'H2_NATIVE_TU15_NETWORK_LINK_ORIGINAL_V1',
                game_frontier=game_frontier[0],frontier_target=(0x3ffe00 if a.native_renderer_output else 0x3ffdd0 if a.native_renderer_depth else 0x3ff5d0 if a.native_renderer_constants else 0x400ef0 if a.native_renderer_palette else 0x1d3d0 if a.complete_renderer_pool else 0x1c7f0 if a.complete_renderer_state else 0x16af0 if a.bind_native_graphics_device else 0x3ff320 if a.renderer_device_request else 0x12560 if a.complete_aux_object_state else 0x10e3b0 if a.complete_object_storage else 0xb87f0 if a.complete_scalar_state else 0x1f3830 if a.complete_static_registration else 0x69220 if a.complete_view_storage else 0x1888a0 if a.complete_players else 0x14dfe0 if a.complete_state_context else 0x1e7710 if a.complete_clock else 0x22c100 if a.complete_renderer else 0x134b00 if a.complete_level_handles else 0x1a5410 if a.complete_bink_memory else 0x1589d0 if a.complete_input else 0x21e5d0 if a.complete_cache else 0x21c0a0 if a.complete_session else 0x6e830 if a.complete_rng else 0x148d40 if a.complete_network else 0x3d725d if a.advance_online_services else 0x3aad19 if a.advance_online_startup else 0x3d7656 if a.advance_xnet_startup else 0x6e010) if flags&1 else 0x31d400,
   prefix_instructions=prefix_instructions,prefix_stores=prefix_stores,control_instruction_map=control_map,
   world_correspondence_sha256=sha(world_path),game_options=u(0x4f3218),game_options_sha256=hashlib.sha256(g.mem_read(u(0x4f3218),0x1200)).hexdigest(),
   game_storage_complete=False,world_startup_complete=False,reference_executable_is_runtime_input=False)
  report.update(pool_instruction_map=pool_pair['instruction_map'],pool_correspondence_sha256=sha(pool_path),
   qos_pool=u(0x4dc538),qos_pool_sha256=hashlib.sha256(g.mem_read(u(0x4dc538),0x150)).hexdigest(),
   network_fields=[dict(address=p,bytes=n,sha256=hashlib.sha256(g.mem_read(p,n)).hexdigest())for p,n in
      ((0x4dc060,0x88),(0x4dc3f0,0x144),(0x4dc534,8),(0x56e1cc,4),(0x10000000,0x1000))])
  report.update(host_link_observation=host,native_link_flags=flags,host_observation_receipt_sha256=sha(a.host_link_observation),
                native_xnet_api_abi=True,xbox_phy_or_irql_emulation=False)
  if a.complete_rng:
   report.update(world_instruction_map=world_pair['instruction_map'],native_rng_inputs=rng_inputs,
                 heap_resources=heap_resources,source_rng_initializer_complete=True,source_crt_heap_allocation_complete=True,
                 rng_fields=[dict(address=p,bytes=n,sha256=hashlib.sha256(g.mem_read(p,n)).hexdigest()) for p,n in
                     ((0x4f3cd8,4),(0x573840,4),(u(0x4f3cd8),8),(0x08000000,0x1000))])
  if a.complete_session:
   assert session_index[0]==len(session_calls) and tick_index[0]==len(tick_inputs)
   report.update(native_socket_calls=session_calls,native_profile_probe=native['native_profile_probe'],native_millisecond_inputs=tick_inputs,
    session_body_correspondence_sha256=sha(session_body_path),source_network_session_initializer_complete=True,
    native_sse_register_status_backend_sha256=sha(Path(__file__).with_name('oracle_native_sse_registers.py')),native_sse_register_operations=sse_rows,
    session_raw_fields=[dict(address=row['address'],bytes=row['bytes'],sha256=hashlib.sha256(g.mem_read(row['address'],row['bytes'])).hexdigest())for row in native['session_raw_fields']],
    session_fields=[dict(address=row['address'],bytes=row['bytes'],sha256=hashlib.sha256(g.mem_read(row['address'],row['bytes'])).hexdigest())for row in native['session_fields']])
  if a.complete_cache:
   report.update(native_language_preference=native['native_language_preference'],native_cache_directory_probe=native['native_cache_directory_probe'],native_cache_directory_creation=native['native_cache_directory_creation'],
    source_game_language=u(0x48c750),language_remap_cases=language_remap_cases,source_cache_startup_complete=True,cache_body_correspondence_sha256=sha(cache_body_path),
    cache_fields=[dict(address=row['address'],bytes=row['bytes'],sha256=hashlib.sha256(g.mem_read(row['address'],row['bytes'])).hexdigest())for row in native['cache_fields']])
  if a.complete_input:
   assert input_index[0]==len(input_calls)
   report.update(source_input_prelude=source_input_prelude,native_input_calls=input_calls,source_input_initializer_complete=True,
    input_body_correspondence_sha256=sha(input_body_path),
    input_fields=[dict(address=row['address'],bytes=row['bytes'],sha256=hashlib.sha256(g.mem_read(row['address'],row['bytes'])).hexdigest())for row in native['input_fields']])
   if 'native_input_device_selection'in native:report['native_input_device_selection']=native['native_input_device_selection']
  if a.complete_bink_memory:
   report.update(source_bink_memory_registration_complete=True,bink_memory_body_correspondence_sha256=sha(bink_body_path),
    bink_memory_fields=[dict(address=row['address'],bytes=row['bytes'],sha256=hashlib.sha256(g.mem_read(row['address'],row['bytes'])).hexdigest())for row in native['bink_memory_fields']])
  if a.complete_level_handles:
   report.update(source_level_handles_initializer_complete=True,level_handles_body_correspondence_sha256=sha(level_body_path),
    level_handles_fields=[dict(address=row['address'],bytes=row['bytes'],sha256=hashlib.sha256(g.mem_read(row['address'],row['bytes'])).hexdigest())for row in native['level_handles_fields']])
  if a.complete_renderer:
   assert heap_commit_index[0]==len(heap_commit_calls)
   report.update(renderer_instruction_map=renderer['instruction_map'],renderer_correspondence_sha256=sha(renderer_path),heap_growth_correspondence_sha256=sha(growth_path),
    source_renderer_initializer_complete=True,native_heap_commit_calls=heap_commit_calls,
    renderer_fields=[dict(address=row['address'],bytes=row['bytes'],sha256=hashlib.sha256(g.mem_read(row['address'],row['bytes'])).hexdigest())for row in native['renderer_fields']],
    renderer_heap_fields=[dict(address=row['address'],bytes=row['bytes'],sha256=hashlib.sha256(g.mem_read(row['address'],row['bytes'])).hexdigest())for row in native['renderer_heap_fields']])
  if a.complete_clock:
   report.update(source_clock_initializer_complete=True,source_arena_reentry_callback_complete=True,startup_state_body_correspondence_sha256=sha(state_body_path),
    clock_fields=[dict(address=row['address'],bytes=row['bytes'],sha256=hashlib.sha256(g.mem_read(row['address'],row['bytes'])).hexdigest())for row in native['clock_fields']])
  if a.complete_state_context:
   report.update(source_state_context_initializer_complete=True,state_context_body_correspondence_sha256=sha(context_body_path),
    state_context_fields=[dict(address=row['address'],bytes=row['bytes'],sha256=hashlib.sha256(g.mem_read(row['address'],row['bytes'])).hexdigest())for row in native['state_context_fields']])
  if a.native_renderer_output:
   assert output_index[0]==8
   report.update(native_renderer_output_complete=True,native_output_calls=output_calls,
    renderer_inline_output_fields=[dict(address=row['address'],bytes=row['bytes'],sha256=hashlib.sha256(g.mem_read(row['address'],row['bytes'])).hexdigest())for row in native['renderer_inline_output_fields']])
  if a.native_renderer_depth_state:
   assert depth_state_index[0]==3
   report.update(native_renderer_depth_state_complete=True,native_depth_state_calls=depth_state_calls,
    renderer_inline_depth_state_fields=[dict(address=row['address'],bytes=row['bytes'],sha256=hashlib.sha256(g.mem_read(row['address'],row['bytes'])).hexdigest())for row in native['renderer_inline_depth_state_fields']])
  if a.native_renderer_depth:
   report.update(native_renderer_depth_mode_complete=True,native_depth_mode_call=native['native_depth_mode_call'])
  if a.native_renderer_constants:
   report.update(native_renderer_constant_mode_complete=True,native_constant_mode_call=native['native_constant_mode_call'])
  if a.native_renderer_palette:
   assert palette_index[0]==len(palette_calls)==5
   report.update(native_renderer_palette_complete=True,native_palette_calls=palette_calls,
    renderer_palette_fields=[dict(address=row['address'],bytes=row['bytes'],sha256=hashlib.sha256(g.mem_read(row['address'],row['bytes'])).hexdigest())for row in native['renderer_palette_fields']])
  if a.complete_renderer_pool:
   report.update(source_renderer_pool_constructor_complete=True,
    renderer_pool_body_correspondence_sha256=sha(renderer_pool_body_path),
    renderer_pool_fields=[dict(address=row['address'],bytes=row['bytes'],sha256=hashlib.sha256(g.mem_read(row['address'],row['bytes'])).hexdigest())for row in native['renderer_pool_fields']])
  if a.complete_renderer_state:
   report.update(source_renderer_state_and_table_constructors_complete=True,
    renderer_state_body_correspondence_sha256=sha(renderer_state_body_path),
    renderer_state_fields=[dict(address=row['address'],bytes=row['bytes'],sha256=hashlib.sha256(g.mem_read(row['address'],row['bytes'])).hexdigest())for row in native['renderer_state_fields']])
  if a.bind_native_graphics_device:
   report.update(native_port_renderer_bound=True,native_graphics_device_call=native['native_graphics_device_call'],
    graphics_device_fields=[dict(address=row['address'],bytes=row['bytes'],sha256=hashlib.sha256(g.mem_read(row['address'],row['bytes'])).hexdigest())for row in native['graphics_device_fields']])
  if a.renderer_device_request:
   report.update(source_renderer_device_request_producer_complete=True,source_renderer_resource_initializer_complete=False,native_gpu_device_creation_attempted=False,
    renderer_request_body_correspondence_sha256=sha(renderer_request_body_path),
    renderer_request_fields=[dict(address=row['address'],bytes=row['bytes'],sha256=hashlib.sha256(g.mem_read(row['address'],row['bytes'])).hexdigest())for row in native['renderer_request_fields']])
  if a.complete_aux_object_state:
   report.update(source_aux_object_state_initializers_complete=True,aux_object_state_body_correspondence_sha256=sha(auxiliary_body_path),
    aux_object_state_fields=[dict(address=row['address'],bytes=row['bytes'],sha256=hashlib.sha256(g.mem_read(row['address'],row['bytes'])).hexdigest())for row in native['aux_object_state_fields']])
  if a.complete_object_storage:
   report.update(source_object_storage_initializer_complete=True,object_storage_body_correspondence_sha256=sha(object_body_path),
    object_storage_fields=[dict(address=row['address'],bytes=row['bytes'],sha256=hashlib.sha256(g.mem_read(row['address'],row['bytes'])).hexdigest())for row in native['object_storage_fields']])
  if a.complete_scalar_state:
   report.update(source_scalar_state_initializer_complete=True,
    scalar_state_fields=[dict(address=row['address'],bytes=row['bytes'],sha256=hashlib.sha256(g.mem_read(row['address'],row['bytes'])).hexdigest())for row in native['scalar_state_fields']])
  if a.complete_static_registration:
   report.update(source_static_registration_initializer_complete=True,static_registration_body_correspondence_sha256=sha(registration_body_path),
    static_registration_fields=[dict(address=row['address'],bytes=row['bytes'],sha256=hashlib.sha256(g.mem_read(row['address'],row['bytes'])).hexdigest())for row in native['static_registration_fields']])
  if a.complete_view_storage:
   report.update(source_view_storage_initializers_complete=True,view_storage_body_correspondence_sha256=sha(view_body_path),
    view_storage_fields=[dict(address=row['address'],bytes=row['bytes'],sha256=hashlib.sha256(g.mem_read(row['address'],row['bytes'])).hexdigest())for row in native['view_storage_fields']])
  if a.complete_players:
   report.update(source_player_initializer_complete=True,
    player_fields=[dict(address=row['address'],bytes=row['bytes'],sha256=hashlib.sha256(g.mem_read(row['address'],row['bytes'])).hexdigest())for row in native['player_fields']])
  if a.complete_network:
   assert title_index[0]==2
   report.update(native_title_address_calls=title_inputs,title_address_flag_cases=title_flags_report['rows'],
                 source_network_cold_initializer_complete=True,source_noop_callbacks_completed=9 if a.complete_scalar_state else 8 if a.complete_static_registration else 7 if a.complete_bink_memory else 6 if a.complete_session else 2)
  if a.advance_xnet_startup:
   sp=game_frontier[0]['gp'][4];parameters=xnet_parameter_address[0] if a.advance_online_services else u(sp+4)-(12 if a.advance_online_startup else 0)
   report.update(xnet_startup_parameters=parameters,xnet_startup_parameter_bytes=params.hex() if a.advance_online_startup else bytes(g.mem_read(parameters,12)).hex(),
                 pending_stack_bytes=bytes(g.mem_read(sp,main_stack+0xf000-sp+4)).hex())
  (a.output/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:report[k]for k in ('stores','arena','arena_offset','cache_files')}),flush=True)
 finally:
  output.close()
  for fd,path in files.values():os.close(fd)
  for pages in physical:pages.close()
  for address,size in heap_mappings:libc.munmap(address,size)
  resource_directory.cleanup();priority_stop.set();native_priority_thread.join()
if __name__=='__main__':main()
