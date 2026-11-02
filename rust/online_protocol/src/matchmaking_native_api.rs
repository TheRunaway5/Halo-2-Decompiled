//! Complete native Session packet/audio API caller policy.
//! Actual mutexes, exception identities, packet banks, devices and workers are
//! IO. No Rust Session borrow may span callbacks into those genuine owners.

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Error(pub String);
impl std::fmt::Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str(&self.0)
    }
}
impl std::error::Error for Error {}
pub type Result<T> = std::result::Result<T, Error>;

/// Owns the actual returned packet banks. A first-packet observation borrows
/// those banks; it never clones them or borrows the Session that produced them.
pub trait Received {
    type Bytes: ?Sized;
    fn first(&self) -> Option<(u32, &Self::Bytes)>;
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct AudioBinding {
    pub existing: bool,
    pub same: bool,
}
pub trait Io {
    type Received: Received;
    fn lock(&self) -> Result<()>;
    fn unlock(&self) -> Result<()>;
    fn fence(&self) -> Result<()>;
    fn send(&self, peer: u32, bytes: &[u8]) -> Result<()>;
    fn receive(&self) -> Result<Self::Received>;
    fn receive_peer(&self, peer: u32) -> Result<()>;
    fn receive_bytes(&self, bytes: &<Self::Received as Received>::Bytes) -> Result<()>;
    fn audio_present(&self) -> bool;
    fn audio_resume_allowed(&self) -> bool;
    fn audio_resume(&self) -> Result<()>;
    fn audio_ready(&self) -> Result<bool>;
    fn audio_generation(&self) -> u64;
    fn audio_fence(&self) -> Result<u64>;
    fn apply_audio_fence(&self, generation: u64) -> Result<()>;
    fn voice(&self, target: u32, ready: bool) -> Result<()>;
    fn preferences(&self, target: u32, enabled: bool, muted: u16) -> Result<()>;
    fn collect_voices(&self) -> Result<()>;
    fn audio_binding(&self) -> AudioBinding;
    fn install_audio(&self) -> Result<()>;
    fn bind_audio(&self) -> Result<()>;
    fn close_presence(&self) -> Result<()>;
    fn pin_audio(&self) -> Result<bool>;
    fn close_audio(&self) -> Result<()>;
    fn unpin_audio(&self) -> Result<()>;
    fn stop_worker(&self) -> Result<()>;
    fn worker_joinable(&self) -> bool;
    fn join_worker(&self) -> Result<()>;
}
fn locked<T>(io: &impl Io, call: impl FnOnce() -> Result<T>) -> Result<T> {
    io.lock()?;
    let value = call();
    let unlocked = io.unlock();
    value.and_then(|value| unlocked.map(|()| value))
}
/// The caller already holds the generation mutex. Used both inside the public
/// API drivers below and by existing generation-locked native effect callers.
pub fn sync_audio(io: &impl Io) -> Result<()> {
    if io.audio_present() && io.audio_resume_allowed() {
        io.audio_resume()?;
    }
    Ok(())
}
/// The caller already holds the generation mutex. Only the genuine device may
/// issue a new stamp; absence retains the existing full-width generation.
pub fn fence_audio(io: &impl Io) -> Result<()> {
    let generation = if io.audio_present() {
        io.audio_fence()?
    } else {
        io.audio_generation()
    };
    io.apply_audio_fence(generation)
}
pub fn send(io: &impl Io, peer: u32, bytes: &[u8]) -> Result<()> {
    locked(io, || {
        io.fence()?;
        io.send(peer, bytes)
    })
}
pub fn receive(io: &impl Io) -> Result<bool> {
    locked(io, || {
        io.fence()?;
        // All returned banks retire before unlock, including unselected banks
        // and the selected bank when either native output operation refuses.
        let received = io.receive()?;
        let Some((peer, bytes)) = received.first() else {
            return Ok(false);
        };
        io.receive_peer(peer)?;
        io.receive_bytes(bytes)?;
        Ok(true)
    })
}
pub fn voice(io: &impl Io, target: u32) -> Result<()> {
    locked(io, || {
        io.fence()?;
        sync_audio(io)?;
        let ready = if io.audio_present() {
            io.audio_ready()?
        } else {
            true
        };
        io.voice(target, ready)
    })
}
pub fn preferences(io: &impl Io, target: u32, enabled: bool, muted: u16) -> Result<()> {
    locked(io, || {
        io.fence()?;
        io.preferences(target, enabled, muted)
    })
}
pub fn voices(io: &impl Io) -> Result<()> {
    locked(io, || {
        io.fence()?;
        io.collect_voices()
    })
}
/// False is the exact existing incompatible-native-owner refusal. The native
/// ABI applies that typed result using its existing exception constructor.
pub fn retain_audio(io: &impl Io) -> Result<bool> {
    locked(io, || {
        let observed = io.audio_binding();
        if observed.existing && !observed.same {
            return Ok(false);
        }
        if observed.same {
            return Ok(true);
        }
        io.install_audio()?;
        if io.audio_present() {
            io.bind_audio()?;
        }
        fence_audio(io)?;
        sync_audio(io)?;
        Ok(true)
    })
}
pub fn close(io: &impl Io) -> Result<()> {
    locked(io, || io.close_presence())?;
    // Pin under the separate real native owner lock, then close outside it.
    // The pin retires before worker stop/join, even on a close refusal.
    if io.pin_audio()? {
        let closed = io.close_audio();
        let unpinned = io.unpin_audio();
        closed.and(unpinned)?;
    }
    io.stop_worker()?;
    if io.worker_joinable() {
        io.join_worker()?;
    }
    Ok(())
}
