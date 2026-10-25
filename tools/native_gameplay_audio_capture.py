#!/usr/bin/env python3
"""Owned Pulse sink/monitor for the game under test; never records a default sink."""
import array,json,math,os,subprocess,sys,time,uuid
from pathlib import Path

class NativeGameplayAudioCapture:
    def __init__(self,scratch):
        owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
        self.scratch=Path(scratch).resolve(strict=True)
        if not self.scratch.is_relative_to(owner):raise RuntimeError('Audio capture requires its owned scratch directory')
        self.name='halo2_audit_'+uuid.uuid4().hex
        self.module=None;self.process=None;self.stream=None;self.clients={};self.summary=None
        self.game=None;self.game_pid=None;self.game_executable=None;self.routing=[]
        self.path=self.scratch/'native-output.f32';self.started=None
    def __enter__(self):
        try:
            self.default_sink=subprocess.check_output(['pactl','get-default-sink'],text=True).strip()
            self.module=int(subprocess.check_output(['pactl','load-module','module-null-sink',
                'sink_name='+self.name,'rate=48000','channels=2','format=float32le',
                'sink_properties=device.description='+self.name],text=True).strip())
            sinks=json.loads(subprocess.check_output(['pactl','--format=json','list','sinks'],text=True))
            selected=[s for s in sinks if s['name']==self.name]
            if len(selected)!=1:raise RuntimeError('Owned native audio sink was not published exactly once')
            self.index=selected[0]['index'];self.stream=self.path.open('wb');self.started=time.monotonic()
            if subprocess.check_output(['pactl','get-default-sink'],text=True).strip()!=self.default_sink:
                raise RuntimeError('Creating the private audio sink changed the desktop default')
            self.process=subprocess.Popen(['parec','--raw','--device='+self.name+'.monitor',
                '--format=float32le','--channels=2','--rate=48000','--latency-msec=20'],
                stdout=self.stream,stderr=subprocess.DEVNULL)
            return self
        except BaseException:self.__exit__(None,None,None);raise
    def environment(self):return dict(SDL_AUDIODRIVER='pulseaudio',PULSE_SINK=self.name)
    def bind_process(self,game,process_id=None,executable=None):
        if self.game is not None:raise RuntimeError('Audio capture already has an owned game process')
        if not isinstance(game,subprocess.Popen):raise TypeError('Bind the actual owned Popen process')
        self.game=game;self.game_pid=game.pid if process_id is None else int(process_id)
        self.game_executable=Path(executable).resolve(strict=True) if executable else None
        if not self._owned_process_live():raise RuntimeError('Native audio process is not the live owned game child')
    def _owned_process_live(self):
        if self.game is None or self.game.poll() is not None:return False
        try:
            if self.game_executable and Path('/proc/'+str(self.game_pid)+'/exe').resolve(strict=True)!=self.game_executable:
                raise RuntimeError('Owned game executable changed before audio routing')
            current=self.game_pid;seen=set()
            while current!=self.game.pid:
                if current<=1 or current in seen:raise RuntimeError('Native audio process escaped its owned debugger parent')
                seen.add(current)
                status=Path('/proc/'+str(current)+'/status').read_text()
                current=int(next(line.split()[1] for line in status.splitlines() if line.startswith('PPid:')))
            return True
        except FileNotFoundError:return False
    def observe(self):
        rows=json.loads(subprocess.check_output(['pactl','--format=json','list','sink-inputs'],text=True))
        if self._owned_process_live():
            # PipeWire policy can override PULSE_SINK. Move only this live
            # child process's streams; never change the desktop default or
            # inspect/capture PCM from another application's endpoint.
            for row in rows:
                pid=row.get('properties',{}).get('application.process.id')
                if str(pid)==str(self.game_pid) and row['sink']!=self.index:
                    if not self._owned_process_live():break
                    subprocess.run(['pactl','move-sink-input',str(row['index']),self.name],
                        check=True,stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
                    self.routing.append(dict(index=row['index'],process_id=pid,
                        from_sink=row['sink'],to_sink=self.index))
            if self.routing:
                rows=json.loads(subprocess.check_output(['pactl','--format=json','list','sink-inputs'],text=True))
        for row in rows:
            if row['sink']==self.index:
                properties=row.get('properties',{})
                if self.game is not None and str(properties.get('application.process.id'))!=str(self.game_pid):
                    raise RuntimeError('Foreign process connected to the private game monitor')
                self.clients[row['index']]=dict(index=row['index'],sink=row['sink'],
                    process_id=properties.get('application.process.id'),
                    application=properties.get('application.name'),
                    sample_specification=row.get('sample_specification'))
    def __exit__(self,kind,value,traceback):
        try:
            if self.process:
                if self.process.poll() is None:self.process.terminate()
                try:self.process.wait(timeout=5)
                except subprocess.TimeoutExpired:self.process.kill();self.process.wait()
            if self.stream:self.stream.close()
            if self.path.exists():
                values=array.array('f');size=self.path.stat().st_size
                if size%8:raise RuntimeError('Native audio monitor has an incomplete stereo frame')
                with self.path.open('rb') as stream:values.fromfile(stream,size//4)
                if sys.byteorder!='little':values.byteswap()
                if not all(math.isfinite(v) for v in values):raise RuntimeError('Native output contains nonfinite PCM')
                self.summary=dict(sink_name=self.name,sink_index=getattr(self,'index',None),
                    monitor_is_private_owned_sink=True,default_sink_or_microphone_recorded=False,
                    clients=list(self.clients.values()),frames=len(values)//2,bytes=size,
                    owned_game_process_id=self.game_pid,
                    owned_stream_routing=self.routing,
                    peak=max((abs(v) for v in values),default=0),
                    rms=math.sqrt(sum(v*v for v in values)/len(values)) if values else 0,
                    nonzero_samples=sum(v!=0 for v in values),
                    physical_speaker_audibility_verified=False,raw_pcm_retained=False)
                self.path.unlink()
        finally:
            if self.module is not None:
                subprocess.run(['pactl','unload-module',str(self.module)],check=True,
                    stdout=subprocess.DEVNULL,stderr=subprocess.PIPE)
                self.module=None
                sinks=json.loads(subprocess.check_output(['pactl','--format=json','list','sinks'],text=True))
                removed=not any(s['name']==self.name for s in sinks)
                preserved=subprocess.check_output(['pactl','get-default-sink'],text=True).strip()==self.default_sink
                if self.summary is not None:self.summary.update(sink_removed=removed,default_sink_preserved=preserved)
                if not removed or not preserved:raise RuntimeError('Private audio capture did not leave the desktop sink unchanged')
