#!/usr/bin/env python3
"""Measure a genuine paused SDL device; never supplies a DirectSound return."""
import argparse,ctypes,ctypes.util,hashlib,json,os
from pathlib import Path
class Spec(ctypes.Structure):
 _fields_=[('freq',ctypes.c_int),('format',ctypes.c_ushort),('channels',ctypes.c_ubyte),('silence',ctypes.c_ubyte),
  ('samples',ctypes.c_ushort),('padding',ctypes.c_ushort),('size',ctypes.c_uint),('callback',ctypes.c_void_p),('userdata',ctypes.c_void_p)]
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--output',type=Path,required=True);a=ap.parse_args()
 library=ctypes.util.find_library('SDL2');assert library
 lib=ctypes.CDLL(library);lib.SDL_InitSubSystem.argtypes=[ctypes.c_uint];lib.SDL_InitSubSystem.restype=ctypes.c_int
 lib.SDL_GetError.restype=ctypes.c_char_p;lib.SDL_GetCurrentAudioDriver.restype=ctypes.c_char_p
 lib.SDL_OpenAudioDevice.argtypes=[ctypes.c_char_p,ctypes.c_int,ctypes.POINTER(Spec),ctypes.POINTER(Spec),ctypes.c_int]
 lib.SDL_OpenAudioDevice.restype=ctypes.c_uint;lib.SDL_GetAudioDeviceStatus.argtypes=[ctypes.c_uint];lib.SDL_GetAudioDeviceStatus.restype=ctypes.c_int
 lib.SDL_CloseAudioDevice.argtypes=[ctypes.c_uint];lib.SDL_QuitSubSystem.argtypes=[ctypes.c_uint]
 result={'tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'library':library,
  'environment_audio_driver':os.environ.get('SDL_AUDIODRIVER'),'SDL_driver_forced':False,
  'original_source_constructor_or_DirectSound_result_supplied':False,'original_caps_or_stream_consumption_claimed':False,
  'audible_playback_tested':False,'native_gameplay_update_applied':False}
 init=lib.SDL_InitSubSystem(0x10);result['actual_init_return']=init
 device=0
 try:
  if init:
   result['actual_error']=lib.SDL_GetError().decode()
  else:
   driver=lib.SDL_GetCurrentAudioDriver();result['actual_audio_driver']=None if driver is None else driver.decode()
   if driver in(b'dummy',b'disk'):result['unsupported_test_only_audio_driver']=True
   else:
    want=Spec();want.freq=48000;want.format=0x8120;want.channels=2;want.samples=512
    got=Spec();device=lib.SDL_OpenAudioDevice(None,0,ctypes.byref(want),ctypes.byref(got),0)
    result['request']={'default_playback_device':True,'frequency':48000,'format':'AUDIO_F32LSB','channels':2,'samples':512,
     'allowed_changes':0,'null_callback_queue_mode':True,'source_native_renderer_requested_format_only':True}
    result['actual_SDL_device_id']=device
    if device:
     result['obtained']={k:getattr(got,k)for k in('freq','format','channels','samples','size','silence')}
     result['actual_device_status']=lib.SDL_GetAudioDeviceStatus(device);assert result['actual_device_status']==2
     result['device_remained_paused']=True
    else:result['actual_error']=lib.SDL_GetError().decode()
 finally:
  if device:lib.SDL_CloseAudioDevice(device);result['actual_device_closed']=True
  if init==0:lib.SDL_QuitSubSystem(0x10)
 a.output.parent.mkdir(parents=True,exist_ok=True);a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
if __name__=='__main__':main()
