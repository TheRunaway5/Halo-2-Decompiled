//! Packet/audio caller ABI only. Core Rust owns all policy and sequencing.
use crate::ffi;
use halo2_online_protocol::matchmaking_native_api as core;

fn io<T>(value: Result<T, cxx::Exception>) -> core::Result<T> {
    value.map_err(|error| core::Error(error.to_string()))
}
struct Packets(Vec<ffi::NativeSessionPacket>);
impl core::Received for Packets {
    type Bytes = [u8];
    fn first(&self) -> Option<(u32, &[u8])> {
        self.0.first().map(|p| (p.peer, p.bytes.as_slice()))
    }
}
struct Io<'a>(&'a ffi::NativeSessionApiIo);
impl core::Io for Io<'_> {
    type Received = Packets;
    fn lock(&self) -> core::Result<()> { io(self.0.api_lock()) }
    fn unlock(&self) -> core::Result<()> { io(self.0.api_unlock()) }
    fn fence(&self) -> core::Result<()> { io(self.0.api_fence()) }
    fn send(&self, peer: u32, bytes: &[u8]) -> core::Result<()> { io(self.0.api_send(peer, bytes)) }
    fn receive(&self) -> core::Result<Packets> { io(self.0.api_receive()).map(Packets) }
    fn receive_peer(&self, peer: u32) -> core::Result<()> { io(self.0.api_receive_peer(peer)) }
    fn receive_bytes(&self, bytes: &[u8]) -> core::Result<()> { io(self.0.api_receive_bytes(bytes)) }
    fn audio_present(&self) -> bool { self.0.api_audio_present() }
    fn audio_resume_allowed(&self) -> bool { self.0.api_audio_resume_allowed() }
    fn audio_resume(&self) -> core::Result<()> { io(self.0.api_audio_resume()) }
    fn audio_ready(&self) -> core::Result<bool> { io(self.0.api_audio_ready()) }
    fn audio_generation(&self) -> u64 { self.0.api_audio_generation() }
    fn audio_fence(&self) -> core::Result<u64> { io(self.0.api_audio_fence()) }
    fn apply_audio_fence(&self, generation: u64) -> core::Result<()> { io(self.0.api_apply_audio_fence(generation)) }
    fn voice(&self, target: u32, ready: bool) -> core::Result<()> { io(self.0.api_voice(target, ready)) }
    fn preferences(&self, target: u32, enabled: bool, muted: u16) -> core::Result<()> { io(self.0.api_preferences(target, enabled, muted)) }
    fn collect_voices(&self) -> core::Result<()> { io(self.0.api_collect_voices()) }
    fn audio_binding(&self) -> core::AudioBinding {
        let v=self.0.api_audio_binding();core::AudioBinding { existing:v.existing,same:v.same }
    }
    fn install_audio(&self) -> core::Result<()> { io(self.0.api_install_audio()) }
    fn bind_audio(&self) -> core::Result<()> { io(self.0.api_bind_audio()) }
    fn close_presence(&self) -> core::Result<()> { io(self.0.api_close_presence()) }
    fn pin_audio(&self) -> core::Result<bool> { io(self.0.api_pin_audio()) }
    fn close_audio(&self) -> core::Result<()> { io(self.0.api_close_audio()) }
    fn unpin_audio(&self) -> core::Result<()> { io(self.0.api_unpin_audio()) }
    fn stop_worker(&self) -> core::Result<()> { io(self.0.api_stop_worker()) }
    fn worker_joinable(&self) -> bool { self.0.api_worker_joinable() }
    fn join_worker(&self) -> core::Result<()> { io(self.0.api_join_worker()) }
}
fn protect<T>(call: impl FnOnce() -> core::Result<T>) -> Result<T,String> {
    std::panic::catch_unwind(std::panic::AssertUnwindSafe(call))
        .map_err(|_| "Native Session API caller failed".to_owned())?
        .map_err(|error| error.to_string())
}
pub fn native_api_send(v:&ffi::NativeSessionApiIo,peer:u32,bytes:&[u8])->Result<(),String> {
    protect(|| core::send(&Io(v),peer,bytes))
}
pub fn native_api_receive(v:&ffi::NativeSessionApiIo)->Result<bool,String> {
    protect(|| core::receive(&Io(v)))
}
pub fn native_api_voice(v:&ffi::NativeSessionApiIo,target:u32)->Result<(),String> {
    protect(|| core::voice(&Io(v),target))
}
pub fn native_api_preferences(v:&ffi::NativeSessionApiIo,target:u32,enabled:bool,muted:u16)->Result<(),String> {
    protect(|| core::preferences(&Io(v),target,enabled,muted))
}
pub fn native_api_voices(v:&ffi::NativeSessionApiIo)->Result<(),String> {
    protect(|| core::voices(&Io(v)))
}
pub fn native_api_retain_audio(v:&ffi::NativeSessionApiIo)->Result<bool,String> {
    protect(|| core::retain_audio(&Io(v)))
}
pub fn native_api_close(v:&ffi::NativeSessionApiIo)->Result<(),String> {
    protect(|| core::close(&Io(v)))
}
pub fn native_api_sync_audio(v:&ffi::NativeSessionApiIo)->Result<(),String> {
    protect(|| core::sync_audio(&Io(v)))
}
pub fn native_api_fence_audio(v:&ffi::NativeSessionApiIo)->Result<(),String> {
    protect(|| core::fence_audio(&Io(v)))
}
