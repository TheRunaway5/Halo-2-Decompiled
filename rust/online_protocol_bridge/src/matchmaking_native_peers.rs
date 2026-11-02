//! Typed conversion only; no peer scheduling or capability policy lives here.
use crate::{
    ffi,
    matchmaking_native_session::{NativeSession, context, packet, to_context},
};
use halo2_online_protocol::{
    matchmaking_native_peers as core, matchmaking_native_session as session,
};

pub struct NativePeerWorker(core::Worker);
pub fn native_session_epoch_packets(s: &NativeSession) -> bool {
    s.0.epoch_packets()
}
pub fn native_session_peer_worker(s: &NativeSession) -> Result<Box<NativePeerWorker>, String> {
    s.0.peer_config()
        .map(|c| Box::new(NativePeerWorker(core::Worker::new(c))))
        .map_err(|e| e.to_string())
}
fn poll(kind: ffi::NativePeerPoll) -> Option<core::Poll> {
    Some(match kind {
        ffi::NativePeerPoll::Authority => core::Poll::Authority,
        ffi::NativePeerPoll::Admissions => core::Poll::Admissions,
        ffi::NativePeerPoll::Common => core::Poll::Common,
        ffi::NativePeerPoll::Selection => core::Poll::Selection,
        ffi::NativePeerPoll::Snapshot => core::Poll::Snapshot,
        ffi::NativePeerPoll::Quality => core::Poll::Quality,
        ffi::NativePeerPoll::Agreement => core::Poll::Agreement,
        _ => return None,
    })
}
pub fn native_session_peer_plan(
    s: &NativeSession,
    o: &ffi::NativePeerResources,
) -> ffi::NativePeerPlan {
    let p = s.0.peer_plan(session::PeerResources {
        authority: o.authority_present.then(|| context(&o.authority)),
        common: o.common_present.then(|| context(&o.common)),
        selection: o.selection_present.then(|| context(&o.selection)),
        snapshot: o.snapshot_present.then(|| context(&o.snapshot)),
        features: o.features,
    });
    ffi::NativePeerPlan {
        common_supported: p.common_supported,
        selection_supported: p.selection_supported,
        snapshot_supported: p.snapshot_supported,
        native_control: p.native_control,
        common: p.common,
        handoff: p.handoff,
        failed: p.failed,
        setup: p.setup,
    }
}
pub fn native_peer_poll_allowed(p: &ffi::NativePeerPlan, kind: ffi::NativePeerPoll) -> bool {
    let p = session::PeerPlan {
        common_supported: p.common_supported,
        selection_supported: p.selection_supported,
        snapshot_supported: p.snapshot_supported,
        native_control: p.native_control,
        common: p.common,
        handoff: p.handoff,
        failed: p.failed,
        setup: p.setup,
    };
    poll(kind).is_some_and(|kind| p.allows(kind))
}
fn signal(s: &session::Signal) -> ffi::NativeSessionSignal {
    ffi::NativeSessionSignal {
        peer: s.peer as u32,
        kind: s.kind.clone(),
        payload: s.payload.clone(),
        mid: s.mid.clone(),
    }
}
fn direct(o: session::DirectObservation) -> ffi::NativeSessionDirect {
    ffi::NativeSessionDirect {
        now: o.now,
        authenticated: o.authenticated,
        ready: o.ready,
        failed: o.failed,
        links: o
            .links
            .into_iter()
            .map(|l| ffi::NativeSessionLink {
                peer: l.peer as u32,
                measured: l.measured,
                rtt: l.rtt,
                upload: l.upload,
                loss: l.loss,
            })
            .collect(),
        authority_present: false,
        authority: to_context(None),
        admitted_present: false,
        admitted: to_context(None),
    }
}
struct Io<'a>(&'a ffi::NativeMatchPeersIo);
fn io<T>(v: Result<T, cxx::Exception>) -> core::Result<T> {
    v.map_err(|_| core::Error)
}
impl core::Io for Io<'_> {
    fn cancelled(&self) -> bool {
        self.0.peers_cancelled()
    }
    fn wait(&self, ms: u32) {
        self.0.peers_wait(ms)
    }
    fn ready(&self, peer: u8) -> bool {
        self.0.peers_ready(peer)
    }
    fn failed(&self, peer: u8) -> bool {
        self.0.peers_failed(peer)
    }
    fn signals(&self) -> core::Result<Vec<session::Signal>> {
        io(self.0.peers_signals())?
            .into_iter()
            .map(|s| {
                Ok(session::Signal {
                    peer: s.peer.try_into().map_err(|_| core::Error)?,
                    kind: s.kind,
                    payload: s.payload,
                    mid: s.mid,
                })
            })
            .collect()
    }
    fn signal(&self, s: &session::Signal) -> core::Result<()> {
        io(self.0.peers_signal(&signal(s)))
    }
    fn sample(&self, ready: u16) -> core::Result<core::Sample> {
        let s = io(self.0.peers_sample(ready))?;
        let links = s
            .links
            .into_iter()
            .map(|l| {
                Ok(session::Link {
                    peer: l.peer.try_into().map_err(|_| core::Error)?,
                    measured: l.measured,
                    rtt: l.rtt,
                    upload: l.upload,
                    loss: l.loss,
                })
            })
            .collect::<core::Result<_>>()?;
        Ok(core::Sample {
            now: s.now,
            authenticated: s.authenticated,
            setup: s.setup,
            links,
        })
    }
    fn receive(&self) -> core::Result<Option<core::Datagram>> {
        let p = io(self.0.peers_receive())?;
        Ok(p.present.then_some(core::Datagram {
            peer: p.peer,
            epoch: p.epoch,
            bytes: p.bytes,
        }))
    }
    fn datagram(&self, peer: u32, epoch: u32, bytes: &[u8]) -> core::Result<()> {
        io(self.0.peers_datagram(peer, epoch, bytes))
    }
    fn rejected(&self) {
        self.0.peers_rejected()
    }
    fn drain(&self) -> core::Result<Vec<session::Packet>> {
        io(self.0.peers_drain())?
            .into_iter()
            .map(|p| {
                Ok(session::Packet {
                    peer: p.peer.try_into().map_err(|_| core::Error)?,
                    bytes: p.bytes,
                    epoch: p.epoch,
                    audio_generation: p.generation,
                })
            })
            .collect()
    }
    fn send_media(&self, p: &session::Packet, bytes: &[u8]) -> core::Result<()> {
        io(self.0.peers_send_media(&packet(p.clone()), bytes))
    }
    fn send(&self, peer: u32, bytes: &[u8]) -> core::Result<()> {
        io(self.0.peers_send(peer, bytes))
    }
    fn poll(
        &self,
        kind: core::Poll,
        budget: u32,
        ready: u16,
        index: u32,
    ) -> core::Result<Vec<core::Datagram>> {
        let kind = match kind {
            core::Poll::Authority => ffi::NativePeerPoll::Authority,
            core::Poll::Admissions => ffi::NativePeerPoll::Admissions,
            core::Poll::Common => ffi::NativePeerPoll::Common,
            core::Poll::Selection => ffi::NativePeerPoll::Selection,
            core::Poll::Snapshot => ffi::NativePeerPoll::Snapshot,
            core::Poll::Quality => ffi::NativePeerPoll::Quality,
            core::Poll::Agreement => ffi::NativePeerPoll::Agreement,
        };
        Ok(io(self.0.peers_poll(kind, budget, ready, index))?
            .into_iter()
            .map(|p| core::Datagram {
                peer: p.peer,
                epoch: p.epoch,
                bytes: p.bytes,
            })
            .collect())
    }
    fn drive(&self) -> core::Result<()> {
        io(self.0.peers_drive())
    }
    fn agreement_generations(&self) -> u32 {
        self.0.peers_agreement_generations()
    }
    fn finish(&self, o: session::DirectObservation) -> core::Result<bool> {
        io(self.0.peers_finish(&direct(o)))
    }
    fn failure(&self) {
        self.0.peers_failure()
    }
}
pub fn native_peer_run(worker: &mut NativePeerWorker, io: &ffi::NativeMatchPeersIo) {
    worker.0.run(&Io(io));
}
