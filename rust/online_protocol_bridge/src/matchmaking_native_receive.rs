//! Receive ABI conversion. All routing/order/phase/epoch/history policy is core Rust.
use crate::ffi;
use halo2_online_protocol::matchmaking_native_receive as core;
fn target(t: core::Target) -> ffi::NativeReceiveTarget {
    match t {
        core::Target::Selection => ffi::NativeReceiveTarget::Selection,
        core::Target::Snapshot => ffi::NativeReceiveTarget::Snapshot,
        core::Target::Common => ffi::NativeReceiveTarget::Common,
        core::Target::Agreement => ffi::NativeReceiveTarget::Agreement,
        core::Target::Quality => ffi::NativeReceiveTarget::Quality,
        core::Target::Admissions => ffi::NativeReceiveTarget::Admissions,
        core::Target::Authority => ffi::NativeReceiveTarget::Authority,
    }
}
fn io<T>(v: Result<T, cxx::Exception>) -> core::Result<T> {
    v.map_err(|_| core::Error)
}
struct Io<'a>(&'a ffi::NativeReceiveIo);
impl core::Io for Io<'_> {
    fn lock(&self) -> core::Result<()> {
        io(self.0.receive_lock())
    }
    fn unlock(&self) -> core::Result<()> {
        io(self.0.receive_unlock())
    }
    fn session(&self) -> core::Result<core::Session> {
        let s = io(self.0.receive_session())?;
        Ok(core::Session {
            common: s.common,
            native_control: s.native_control,
            handoff: s.handoff,
            direct: s.direct,
            epochs: s.epochs,
        })
    }
    fn supported(&self, t: core::Target) -> core::Result<bool> {
        io(self.0.receive_supported(target(t)))
    }
    fn pin(&self, t: core::Target) -> core::Result<()> {
        io(self.0.receive_pin(target(t)))
    }
    fn present(&self, t: core::Target) -> core::Result<bool> {
        io(self.0.receive_present(target(t)))
    }
    fn epoch(&self, t: core::Target) -> core::Result<u32> {
        io(self.0.receive_epoch(target(t)))
    }
    fn agreement_binding(&self) -> core::Result<Option<u32>> {
        Ok(if io(self.0.receive_binding_present())? {
            Some(io(self.0.receive_binding_epoch())?)
        } else {
            None
        })
    }
    fn history_len(&self) -> core::Result<usize> {
        io(self.0.receive_history_len())?
            .try_into()
            .map_err(|_| core::Error)
    }
    fn history_epoch(&self, n: usize) -> core::Result<u32> {
        io(self.0.receive_history_epoch(n as u64))
    }
    fn history_receive(&self, n: usize, source: u32, b: &[u8]) -> core::Result<()> {
        io(self.0.receive_history_packet(n as u64, source, b))
    }
    fn receive(&self, t: core::Target, source: u32, b: &[u8]) -> core::Result<()> {
        io(self.0.receive_packet(target(t), source, b))
    }
    fn selection_abort(&self) -> core::Result<()> {
        io(self.0.receive_abort())
    }
    fn selection_progress(&self) -> core::Result<()> {
        io(self.0.receive_progress())
    }
    fn fence(&self) -> core::Result<()> {
        io(self.0.receive_fence())
    }
    fn general_receive(&self, source: u32, epoch: u32, b: &[u8]) -> core::Result<bool> {
        io(self.0.receive_general(source, epoch, b))
    }
    fn observe_host(&self) -> core::Result<()> {
        io(self.0.receive_observe_host())
    }
    fn refresh(&self) -> core::Result<()> {
        io(self.0.receive_refresh())
    }
}
pub fn native_receive_dispatch(
    io: &ffi::NativeReceiveIo,
    source: u32,
    packet: &[u8],
    epoch: u32,
) -> Result<(), String> {
    std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
        core::dispatch(&Io(io), source, packet, epoch)
    }))
    .map_err(|_| "Native receive dispatch failed".to_owned())?
    .map_err(|e| e.to_string())
}
