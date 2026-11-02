use halo2_online_protocol::matchmaking_native_api::*;
use std::cell::{Cell, RefCell};
use std::rc::Rc;

struct Bank {
    peer: u32,
    bytes: Vec<u8>,
    held: Rc<Cell<bool>>,
    drops: Rc<RefCell<Vec<bool>>>,
}
impl Drop for Bank {
    fn drop(&mut self) { self.drops.borrow_mut().push(self.held.get()); }
}
struct Packets(Vec<Bank>);
impl Received for Packets {
    type Bytes=[u8];
    fn first(&self)->Option<(u32,&[u8])> { self.0.first().map(|p|(p.peer,p.bytes.as_slice())) }
}
struct Fixture {
    held: Rc<Cell<bool>>,
    drops: Rc<RefCell<Vec<bool>>>,
    events: RefCell<Vec<String>>,
    failures: Vec<&'static str>,
    audio: Cell<bool>,
    incoming: bool,
    same: bool,
    allowed: Cell<bool>,
    allowed_after_fence: Option<bool>,
    ready: bool,
    generation: Cell<u64>,
    next_generation: u64,
    packet_count: usize,
    peer: Cell<u32>,
    bytes: RefCell<Vec<u8>>,
    pinned: Cell<bool>,
    joinable: bool,
}
impl Default for Fixture {
    fn default()->Self { Self {
        held:Rc::new(Cell::new(false)),drops:Rc::new(RefCell::new(vec![])),events:RefCell::new(vec![]),failures:vec![],
        audio:Cell::new(true),incoming:true,same:true,allowed:Cell::new(true),allowed_after_fence:None,
        ready:true,generation:Cell::new(u64::MAX),next_generation:u64::MAX-1,packet_count:2,
        peer:Cell::new(123),bytes:RefCell::new(vec![0xee]),pinned:Cell::new(false),joinable:true,
    }}
}
impl Fixture {
    fn event(&self,name:&'static str,data:impl std::fmt::Display)->Result<()> {
        self.events.borrow_mut().push(format!("{name}:{}:{data}",self.held.get()));
        if self.failures.contains(&name) {Err(Error(format!("native detail {name}")))} else {Ok(())}
    }
    fn has(&self,name:&str)->bool { self.events.borrow().iter().any(|v|v.starts_with(&format!("{name}:"))) }
}
impl Io for Fixture {
    type Received=Packets;
    fn lock(&self)->Result<()> {assert!(!self.held.get());self.event("lock","")?;self.held.set(true);Ok(())}
    fn unlock(&self)->Result<()> {assert!(self.held.get());self.event("unlock","")?;self.held.set(false);Ok(())}
    fn fence(&self)->Result<()> {assert!(self.held.get());self.event("fence","")?;if let Some(v)=self.allowed_after_fence {self.allowed.set(v);}Ok(())}
    fn send(&self,peer:u32,bytes:&[u8])->Result<()> {assert!(self.held.get());assert_eq!(bytes,[0,255,17]);self.event("send",peer)}
    fn receive(&self)->Result<Packets> {
        assert!(self.held.get());self.event("receive","")?;
        Ok(Packets((0..self.packet_count).map(|i|Bank {peer:256+i as u32,bytes:vec![i as u8,255],held:self.held.clone(),drops:self.drops.clone()}).collect()))
    }
    fn receive_peer(&self,peer:u32)->Result<()> {assert!(self.held.get());self.event("peer",peer)?;self.peer.set(peer);Ok(())}
    fn receive_bytes(&self,bytes:&[u8])->Result<()> {assert!(self.held.get());self.event("bytes","")?;*self.bytes.borrow_mut()=bytes.to_vec();Ok(())}
    fn audio_present(&self)->bool {self.audio.get()}
    fn audio_resume_allowed(&self)->bool {self.allowed.get()}
    fn audio_resume(&self)->Result<()> {assert!(self.audio.get()&&self.held.get());self.event("resume",self.generation.get())}
    fn audio_ready(&self)->Result<bool> {assert!(self.audio.get()&&self.held.get());self.event("ready",self.generation.get())?;Ok(self.ready)}
    fn audio_generation(&self)->u64 {self.generation.get()}
    fn audio_fence(&self)->Result<u64> {assert!(self.audio.get()&&self.held.get());self.event("device_fence","")?;Ok(self.next_generation)}
    fn apply_audio_fence(&self,g:u64)->Result<()> {assert!(self.held.get());self.event("stamp",g)?;self.generation.set(g);Ok(())}
    fn voice(&self,target:u32,ready:bool)->Result<()> {assert!(self.held.get());self.event("voice",format!("{target}:{ready}"))}
    fn preferences(&self,target:u32,enabled:bool,muted:u16)->Result<()> {assert!(self.held.get());self.event("prefs",format!("{target}:{enabled}:{muted}"))}
    fn collect_voices(&self)->Result<()> {assert!(self.held.get());self.event("voices","")}
    fn audio_binding(&self)->AudioBinding {AudioBinding {existing:self.audio.get(),same:self.same}}
    fn install_audio(&self)->Result<()> {assert!(self.held.get());self.event("install","")?;self.audio.set(self.incoming);Ok(())}
    fn bind_audio(&self)->Result<()> {assert!(self.held.get()&&self.audio.get());self.event("bind","")}
    fn close_presence(&self)->Result<()> {assert!(self.held.get());self.event("close_presence","")}
    fn pin_audio(&self)->Result<bool> {assert!(!self.held.get());self.event("pin","")?;self.pinned.set(self.audio.get());Ok(self.audio.get())}
    fn close_audio(&self)->Result<()> {assert!(!self.held.get()&&self.pinned.get());self.event("close_audio","")}
    fn unpin_audio(&self)->Result<()> {assert!(!self.held.get()&&self.pinned.get());self.event("unpin","")?;self.pinned.set(false);Ok(())}
    fn stop_worker(&self)->Result<()> {assert!(!self.held.get()&&!self.pinned.get());self.event("stop","")}
    fn worker_joinable(&self)->bool {self.joinable}
    fn join_worker(&self)->Result<()> {assert!(!self.held.get()&&!self.pinned.get());self.event("join","")}
}
#[test]
fn stream_fences_before_every_operation_and_keeps_wide_targets() {
    for target in [0,15,16,255,256,257,u32::MAX] {
        let f=Fixture::default();send(&f,target,&[0,255,17]).unwrap();
        assert_eq!(*f.events.borrow(),["lock:false:","fence:true:",&format!("send:true:{target}"),"unlock:true:"]);
        let f=Fixture::default();preferences(&f,target,false,0xffff).unwrap();
        assert_eq!(*f.events.borrow(),["lock:false:","fence:true:",&format!("prefs:true:{target}:false:65535"),"unlock:true:"]);
    }
    let f=Fixture::default();voices(&f).unwrap();assert_eq!(*f.events.borrow(),["lock:false:","fence:true:","voices:true:","unlock:true:"]);
}
#[test]
fn receive_empty_preserves_outputs_and_first_packet_writes_peer_before_bytes() {
    for n in [0,1,3] {
        let f=Fixture {packet_count:n,..Fixture::default()};assert_eq!(receive(&f).unwrap(),n!=0);
        assert!(!f.held.get());assert_eq!(*f.drops.borrow(),vec![true;n]);
        assert_eq!(f.peer.get(),if n==0 {123}else{256});assert_eq!(*f.bytes.borrow(),if n==0 {vec![0xee]}else{vec![0,255]});
        if n!=0 {assert_eq!(*f.events.borrow(),["lock:false:","fence:true:","receive:true:","peer:true:256","bytes:true:","unlock:true:"]);}
    }
}
#[test]
fn receive_refusals_keep_actual_banks_under_lock_and_preserve_partial_outputs() {
    for fail in ["peer","bytes"] {
        let f=Fixture {failures:vec![fail],..Fixture::default()};let e=receive(&f).unwrap_err();
        assert_eq!(e.0,format!("native detail {fail}"));assert!(!f.held.get());assert_eq!(*f.drops.borrow(),[true,true]);
        assert_eq!(*f.bytes.borrow(),[0xee]);assert_eq!(f.peer.get(),if fail=="peer" {123}else{256});
    }
}
#[test]
fn voice_resamples_after_fence_and_missing_device_is_ready_without_resume() {
    for audio in [false,true] {for allowed in [false,true] {for ready in [false,true] {
        let f=Fixture {audio:Cell::new(audio),allowed:Cell::new(!allowed),allowed_after_fence:Some(allowed),ready,..Fixture::default()};
        voice(&f,u32::MAX).unwrap();assert_eq!(f.has("resume"),audio&&allowed);assert_eq!(f.has("ready"),audio);
        assert!(f.events.borrow().contains(&format!("voice:true:{}:{}",u32::MAX,!audio||ready)));assert!(!f.held.get());
    }}}
}
#[test]
fn standalone_audio_helpers_leave_outer_lock_owned_and_keep_real_generation() {
    for audio in [false,true] {for generation in [0,1,u64::MAX] {
        let f=Fixture {audio:Cell::new(audio),generation:Cell::new(generation),..Fixture::default()};f.held.set(true);
        fence_audio(&f).unwrap();assert_eq!(f.generation.get(),if audio {f.next_generation}else{generation});
        sync_audio(&f).unwrap();assert_eq!(f.has("resume"),audio);assert!(f.held.get());assert!(!f.has("lock")&&!f.has("unlock"));
    }}
}
#[test]
fn retain_uses_real_identity_observation_and_only_installed_device_can_issue_stamp() {
    for existing in [false,true] {for same in [false,true] {for incoming in [false,true] {
        let f=Fixture {audio:Cell::new(existing),same,incoming,..Fixture::default()};let accepted=retain_audio(&f).unwrap();
        assert_eq!(accepted,!existing||same);assert!(!f.held.get());let install=!existing&&!same;
        assert_eq!(f.has("install"),install);assert_eq!(f.has("bind"),install&&incoming);
        assert_eq!(f.has("device_fence"),install&&incoming);assert_eq!(f.has("stamp"),install);
        assert_eq!(f.audio.get(),if install {incoming}else{existing});
    }}}
}
#[test]
fn close_retires_pin_before_stop_and_joins_only_actual_joinable_worker() {
    for audio in [false,true] {for joinable in [false,true] {
        let f=Fixture {audio:Cell::new(audio),joinable,..Fixture::default()};close(&f).unwrap();
        let expected=if audio {vec!["lock:false:","close_presence:true:","unlock:true:","pin:false:","close_audio:false:","unpin:false:","stop:false:"]}else{vec!["lock:false:","close_presence:true:","unlock:true:","pin:false:","stop:false:"]};
        assert_eq!(&f.events.borrow()[..expected.len()],expected.as_slice());assert_eq!(f.has("join"),joinable);assert!(!f.pinned.get()&&!f.held.get());
    }}
    let f=Fixture {failures:vec!["close_audio"],..Fixture::default()};assert_eq!(close(&f).unwrap_err().0,"native detail close_audio");assert!(f.has("unpin")&&!f.has("stop"));assert!(!f.pinned.get());
}
#[test]
fn ordinary_refusals_unlock_stop_later_effects_and_keep_original_error_detail() {
    for fail in ["lock","fence","resume","ready","voice"] {
        let f=Fixture {failures:vec![fail],..Fixture::default()};assert_eq!(voice(&f,0).unwrap_err().0,format!("native detail {fail}"));assert!(!f.held.get());
    }
    for fail in ["install","bind","device_fence","stamp","resume"] {
        let f=Fixture {audio:Cell::new(false),same:false,failures:vec![fail],..Fixture::default()};assert_eq!(retain_audio(&f).unwrap_err().0,format!("native detail {fail}"));assert!(!f.held.get());
    }
    for fail in ["close_presence","pin","close_audio","stop","join"] {
        let f=Fixture {failures:vec![fail],..Fixture::default()};assert_eq!(close(&f).unwrap_err().0,format!("native detail {fail}"));assert!(!f.held.get());
    }
    let f=Fixture {failures:vec!["close_audio","unpin"],..Fixture::default()};assert_eq!(close(&f).unwrap_err().0,"native detail close_audio");assert!(!f.has("stop"));
}
