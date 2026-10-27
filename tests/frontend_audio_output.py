"""Owned actual SDL/Pulse output capture for an opt-in rendered ONLINE flow."""
import asyncio
import hashlib
import json
import os
from pathlib import Path
import uuid
import wave


class NativeFrontendAudioCapture:
    def __init__(self, output):
        self.output=Path(output).resolve(strict=True)
        assert self.output.is_relative_to(Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True))
        self.sink='halo2_ui_'+uuid.uuid4().hex
        self.module=None;self.process=None;self.file=None
        self.module_removed=False
        self.raw=self.output/'one-ui-device-output.f32'
        self.watch=None

    async def command(self,*argv):
        p=await asyncio.create_subprocess_exec(*argv,stdout=asyncio.subprocess.PIPE,stderr=asyncio.subprocess.PIPE)
        out,err=await asyncio.wait_for(p.communicate(),10)
        assert p.returncode==0,(argv[0],p.returncode,err.decode()[-1000:])
        return out.decode()

    async def start(self):
        try:
            result=await self.command('pactl','load-module','module-null-sink','sink_name='+self.sink,
                'rate=48000','channels=2','channel_map=front-left,front-right')
            assert result.strip().isdigit();self.module=int(result.strip())
            if destination:=os.environ.get('HALO2_ONLINE_AUDIO_EVIDENCE'):
                destination=Path(destination);assert destination.is_absolute()
                destination.mkdir(parents=True,exist_ok=True)
                (destination/'device-owner.json').write_text(json.dumps(dict(sink=self.sink,module=self.module,cleanup_pending=True),indent=2)+'\n')
            self.file=self.raw.open('wb')
            self.process=await asyncio.create_subprocess_exec('parec','--raw','--format=float32le','--rate=48000',
                '--channels=2','--latency-msec=10','--device='+self.sink+'.monitor',
                stdout=self.file,stderr=asyncio.subprocess.PIPE)
            await asyncio.sleep(.05)
            assert self.process.returncode is None,'Actual Pulse monitor could not start'
            return dict(SDL_AUDIODRIVER='pulseaudio',PULSE_SINK=self.sink,HALO2_ONLINE_MATCH_AUDIO='1')
        except BaseException:
            await self.close();raise

    def watch_completion(self,processes):
        async def observe():
            try:
                end=asyncio.get_running_loop().time()+120
                while not (self.output/'one-ui-audio-complete').is_file():
                    assert asyncio.get_running_loop().time()<end,'Actual rendered audio capture timed out'
                    assert self.raw.stat().st_size<=64*1024*1024,'Actual audio output exceeded its declared bound'
                    if processes and processes[0].returncode is not None:
                        raise AssertionError('Native owner exited before its actual rendered audio handoff')
                    await asyncio.sleep(.02)
            finally:
                # Bounds/cancellation stop the owned recorder immediately;
                # never let it run until the larger native-flow timeout.
                await self.stop_recording()
        self.watch=asyncio.create_task(observe())

    async def stop_recording(self):
        if self.process and self.process.returncode is None:
            self.process.terminate()
            try:await asyncio.wait_for(self.process.wait(),3)
            except asyncio.TimeoutError:
                self.process.kill();await asyncio.wait_for(self.process.wait(),3)
        if self.file:self.file.close();self.file=None

    async def close(self):
        if self.watch and not self.watch.done():
            # The owner can return immediately after publishing its marker.
            # Let the bounded watcher consume that real completion instead of
            # cancelling a successful capture during this scheduling window.
            if (self.output/'one-ui-audio-complete').is_file():
                await asyncio.gather(self.watch,return_exceptions=True)
            else:self.watch.cancel()
        if self.watch:await asyncio.gather(self.watch,return_exceptions=True)
        await self.stop_recording()
        if self.module is not None:
            # PipeWire's pactl JSON omits numeric module IDs. The stable short
            # table retains them, so verify the exact returned owner before
            # unloading it and never select by a shared module name.
            modules=[row for line in (await self.command('pactl','list','short','modules')).splitlines()
                     if (row:=line.split('\t',3)) and row[0].isdigit()]
            found=next((m for m in modules if int(m[0])==self.module),None)
            if found:
                assert len(found)>=3 and found[1]=='module-null-sink' and 'sink_name='+self.sink in found[2].split()
                await self.command('pactl','unload-module',str(self.module))
            self.module=None
            self.module_removed=True
            if destination:=os.environ.get('HALO2_ONLINE_AUDIO_EVIDENCE'):
                (Path(destination)/'device-owner.json').write_text(json.dumps(dict(sink=self.sink,removed=True,cleanup_pending=False),indent=2)+'\n')

    async def prove(self):
        # Resource cleanup does not turn a failed watcher into a successful
        # capture. Await the original task again to preserve its exact error.
        try:
            assert self.watch is not None;await self.watch
        finally:await self.close()
        import numpy as np
        assert self.module_removed,'Owned Pulse capture device was not removed before evidence publication'
        assert self.raw.stat().st_size%8==0 and self.raw.stat().st_size<=64*1024*1024
        captured=np.fromfile(self.raw,dtype='<f4').reshape(-1,2)
        assert np.isfinite(captured).all() and len(captured)>48000
        signal=captured.mean(axis=1,dtype=np.float64)
        cues=json.loads((self.output/'one-ui-frames.json').read_text())['sounds']
        sources=json.loads((self.output/'one-ui-audio-source.json').read_text())
        assert sources['source_cache']=='admitted MCC' and {s['slot']for s in sources['slots']}=={10,12}
        rows=[];clips=[]
        for slot in (10,12):
            with wave.open(str(self.output/f'one-ui-slot-{slot}.wav'),'rb')as f:
                assert f.getframerate()==48000 and f.getnchannels()==2 and f.getsampwidth()==2
                reference=np.frombuffer(f.readframes(f.getnframes()),dtype='<i2').reshape(-1,2).mean(axis=1)/32768.
            # Compare the waveform submitted to the real SDL device, including
            # native mixing and Pulse playback. No cue output is synthesized.
            size=1<<(len(signal)+len(reference)-2).bit_length()
            convolution=np.fft.irfft(np.fft.rfft(signal,size)*np.fft.rfft(reference[::-1],size),size)
            dot=convolution[len(reference)-1:len(signal)]
            energy=np.concatenate(([0.],np.cumsum(signal*signal)))
            energy=energy[len(reference):]-energy[:-len(reference)]
            score=dot/np.sqrt(np.maximum(energy*np.dot(reference,reference),1e-30))
            indices=np.flatnonzero((score[1:-1]>=.999) & (score[1:-1]>=score[:-2]) & (score[1:-1]>=score[2:]))+1
            matches=[]
            for index in sorted(indices,key=lambda i:float(score[i]),reverse=True):
                if all(abs(int(index)-v)>24000 for v in matches):matches.append(int(index))
            matches.sort()
            requested=[c for c in cues if c['cue']==slot]
            assert requested and len(matches)==len(requested),(slot,'Actual SDL waveform/cue count differs',len(matches),len(requested),float(score.max()))
            if slot==12:
                assert len(requested)==1 and requested[0]['screen']==210 and requested[0]['progress_pane']==3
            else:assert {3,2,1}<=set(c['ui_countdown']for c in requested)
            rows.append(dict(slot=slot,native_requests=requested,actual_waveform_starts=matches,
                correlations=[float(score[i])for i in matches],reference_frames=len(reference),
                reference_sha256=hashlib.sha256((self.output/f'one-ui-slot-{slot}.wav').read_bytes()).hexdigest()))
            start=max(0,matches[0]-2400);end=min(len(captured),matches[0]+len(reference)+2400)
            clips.append((slot,captured[start:end]))
            del convolution,dot,energy,score
        receipt=dict(schema='H2_ACTUAL_FRONTEND_AUDIO_OUTPUT_1',passed=True,device='SDL pulseaudio -> owned Pulse null sink -> real monitor',
            rate=48000,channels=2,captured_frames=len(captured),captured_bytes=self.raw.stat().st_size,
            captured_sha256=hashlib.sha256(self.raw.read_bytes()).hexdigest(),roles=rows,
            admitted_source=sources,original_source_trigger_policy=True,
            physical_speaker_output_claimed=False,original_xbox_device_output_claimed=False,
            temporary_pulse_sink_removed=True,
            raw_audio_retained=False,retained_clips=[])
        if destination:=os.environ.get('HALO2_ONLINE_AUDIO_EVIDENCE'):
            destination=Path(destination);assert destination.is_absolute()
            destination.mkdir(parents=True,exist_ok=True);assert not (destination/'evidence.json').exists()
            for slot,clip in clips:
                path=destination/f'actual-ui-slot-{slot}.wav'
                with wave.open(str(path),'wb')as f:
                    f.setnchannels(2);f.setsampwidth(2);f.setframerate(48000)
                    f.writeframes(np.clip(clip*32767,-32767,32767).astype('<i2').tobytes())
                assert path.stat().st_size<=1024*1024
                receipt['retained_clips'].append(dict(path=str(path),bytes=path.stat().st_size,sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
            assert sum(r['bytes']for r in receipt['retained_clips'])<=2*1024*1024
            (destination/'evidence.json').write_text(json.dumps(receipt,indent=2)+'\n')
        print('PASS actual rendered UI audio:',json.dumps(receipt),flush=True)
        return receipt
