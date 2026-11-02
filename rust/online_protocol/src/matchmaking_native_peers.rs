//! Direct peer worker policy for the replacement protocol. Native IO supplies
//! actual transport/resource observations; no callback grants native Ready.
use crate::matchmaking_native_session::{DirectObservation, Link, Packet, Signal};

pub const COMMON_FEATURE: u32 = 512;
pub const SNAPSHOT_FEATURE: u32 = 1024;
pub const SELECTION_FEATURE: u32 = 2048;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Poll {
    Authority,
    Admissions,
    Common,
    Selection,
    Snapshot,
    Quality,
    Agreement,
}
#[derive(Clone, Copy, Debug)]
pub struct Config {
    pub self_peer: u8,
    pub physical: u16,
    pub epochs: bool,
}
pub struct Sample {
    pub now: u64,
    pub authenticated: bool,
    pub setup: bool,
    pub links: Vec<Link>,
}
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Datagram {
    pub peer: u32,
    pub epoch: u32,
    pub bytes: Vec<u8>,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Error;
pub type Result<T> = std::result::Result<T, Error>;

pub trait Io {
    fn cancelled(&self) -> bool;
    fn wait(&self, milliseconds: u32);
    fn ready(&self, peer: u8) -> bool;
    fn failed(&self, peer: u8) -> bool;
    fn signals(&self) -> Result<Vec<Signal>>;
    fn signal(&self, signal: &Signal) -> Result<()>;
    fn sample(&self, ready: u16) -> Result<Sample>;
    fn receive(&self) -> Result<Option<Datagram>>;
    fn datagram(&self, peer: u32, epoch: u32, bytes: &[u8]) -> Result<()>;
    fn rejected(&self);
    /// Acquires the actual generation lock, fences and synchronizes audio,
    /// drains Rust-owned media, then pumps the actual postgame device owner.
    fn drain(&self) -> Result<Vec<Packet>>;
    /// Rechecks the Rust media predicate under the actual generation lock and
    /// sends without releasing that lock. A stale media packet is discarded.
    fn send_media(&self, packet: &Packet, framed: &[u8]) -> Result<()>;
    fn send(&self, peer: u32, bytes: &[u8]) -> Result<()>;
    /// Native owners are copied/rechecked under the generation lock. Rust's
    /// resource/capability plan gates each actual poll at that boundary.
    fn poll(&self, kind: Poll, budget: u32, ready: u16, index: u32) -> Result<Vec<Datagram>>;
    fn drive(&self) -> Result<()>;
    /// Zero while handoff is unavailable; otherwise includes the current
    /// owner plus retained native owners (at most sixteen generations).
    fn agreement_generations(&self) -> u32;
    fn finish(&self, observation: DirectObservation) -> Result<bool>;
    fn failure(&self);
}

pub struct Worker {
    config: Config,
    agreement_cursor: u32,
}
impl Worker {
    /// Only the admitted native Session creates a production configuration.
    pub fn new(config: Config) -> Self {
        Self {
            config,
            agreement_cursor: 0,
        }
    }
    fn framed(epoch: u32, bytes: &[u8], epochs: bool) -> Vec<u8> {
        let mut result = Vec::with_capacity(bytes.len() + if epochs { 4 } else { 0 });
        if epochs {
            result.extend_from_slice(&epoch.to_le_bytes());
        }
        result.extend_from_slice(bytes);
        result
    }
    fn dispatch(&self, io: &impl Io, packet: Datagram) -> Result<()> {
        let offset = if self.config.epochs { 4 } else { 0 };
        if packet.bytes.len() < offset + 7 {
            return Err(Error);
        }
        let epoch = if self.config.epochs {
            u32::from_le_bytes(packet.bytes[..4].try_into().map_err(|_| Error)?)
        } else {
            0
        };
        io.datagram(packet.peer, epoch, &packet.bytes[offset..])
    }
    fn poll(&self, io: &impl Io, kind: Poll, budget: u32, ready: u16, index: u32) -> Result<()> {
        let epochs = self.config.epochs
            || matches!(kind, Poll::Selection | Poll::Snapshot | Poll::Agreement);
        for packet in io.poll(kind, budget, ready, index)? {
            io.send(
                packet.peer,
                &Self::framed(packet.epoch, &packet.bytes, epochs),
            )?;
        }
        Ok(())
    }
    fn iteration(&mut self, io: &impl Io) -> Result<bool> {
        let (mut ready, mut failed) = (0u16, 0u16);
        for peer in 0..16 {
            if peer != self.config.self_peer && self.config.physical & (1u16 << peer) != 0 {
                if io.ready(peer) {
                    ready |= 1u16 << peer;
                }
                if io.failed(peer) {
                    failed |= 1u16 << peer;
                }
            }
        }
        for signal in io.signals()? {
            io.signal(&signal)?;
        }
        let sample = io.sample(ready)?;
        let links = sample
            .links
            .into_iter()
            .filter(|link| link.peer < 16 && self.config.physical & (1u16 << link.peer) != 0)
            .collect();
        let observation = DirectObservation {
            now: sample.now,
            authenticated: sample.authenticated,
            ready,
            failed,
            links,
            authority: None,
            admitted: None,
        };
        for _ in 0..64 {
            let Some(packet) = io.receive()? else {
                break;
            };
            if self.dispatch(io, packet).is_err() {
                io.rejected();
            }
        }
        for packet in io.drain()? {
            io.send_media(
                &packet,
                &Self::framed(packet.epoch, &packet.bytes, self.config.epochs),
            )?;
        }
        // Gameplay and microphone IO precede separately budgeted native owners.
        // The native authority stream retains its own rate/window accounting.
        for (kind, budget) in [
            (Poll::Authority, 2),
            (Poll::Admissions, 2),
            (Poll::Common, 4),
            (Poll::Selection, 4),
            (Poll::Snapshot, 4),
            (Poll::Quality, if sample.setup { 16 } else { 4 }),
        ] {
            self.poll(io, kind, budget, ready, 0)?;
        }
        io.drive()?;
        // The wrapping cursor is owned here, including empty poll slots. A
        // membership change never drops a retained learner's generation.
        for _ in 0..4 {
            let count = io.agreement_generations();
            if count == 0 {
                break;
            }
            if count > 16 {
                return Err(Error);
            }
            let index = self.agreement_cursor % count;
            self.agreement_cursor = self.agreement_cursor.wrapping_add(1);
            self.poll(io, Poll::Agreement, 1, ready, index)?;
        }
        io.finish(observation)
    }
    pub fn run(&mut self, io: &impl Io) {
        while !io.cancelled() {
            match self.iteration(io) {
                Ok(true) => io.wait(2),
                Ok(false) => return,
                Err(_) => {
                    io.failure();
                    return;
                }
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::{
        cell::{Cell, RefCell},
        collections::VecDeque,
    };
    #[derive(Default)]
    struct Fixture {
        events: RefCell<Vec<String>>,
        received: RefCell<VecDeque<Datagram>>,
        observations: RefCell<Vec<DirectObservation>>,
        sent: RefCell<Vec<(u32, Vec<u8>)>>,
        rejected: Cell<u32>,
        count: Cell<u32>,
        setup: Cell<bool>,
        stop: Cell<bool>,
        failed_send: Cell<bool>,
        stale_media: Cell<bool>,
    }
    impl Io for Fixture {
        fn cancelled(&self) -> bool {
            self.stop.get()
        }
        fn wait(&self, ms: u32) {
            self.events.borrow_mut().push(format!("wait:{ms}"));
            self.stop.set(true);
        }
        fn ready(&self, p: u8) -> bool {
            self.events.borrow_mut().push(format!("ready:{p}"));
            true
        }
        fn failed(&self, p: u8) -> bool {
            self.events.borrow_mut().push(format!("failed:{p}"));
            p == 9
        }
        fn signals(&self) -> Result<Vec<Signal>> {
            Ok(vec![Signal {
                peer: 9,
                kind: "candidate".into(),
                payload: "x".into(),
                mid: String::new(),
            }])
        }
        fn signal(&self, s: &Signal) -> Result<()> {
            self.events.borrow_mut().push(format!("signal:{}", s.peer));
            Ok(())
        }
        fn sample(&self, ready: u16) -> Result<Sample> {
            self.events.borrow_mut().push(format!("sample:{ready}"));
            Ok(Sample {
                now: 100,
                authenticated: true,
                setup: self.setup.get(),
                links: (0..16)
                    .map(|peer| Link {
                        peer,
                        measured: false,
                        rtt: 0,
                        upload: 0,
                        loss: 1000,
                    })
                    .collect(),
            })
        }
        fn receive(&self) -> Result<Option<Datagram>> {
            Ok(self.received.borrow_mut().pop_front())
        }
        fn datagram(&self, p: u32, e: u32, b: &[u8]) -> Result<()> {
            self.events
                .borrow_mut()
                .push(format!("receive:{p}:{e}:{}", b.len()));
            if p == 99 { Err(Error) } else { Ok(()) }
        }
        fn rejected(&self) {
            self.rejected.set(self.rejected.get() + 1)
        }
        fn drain(&self) -> Result<Vec<Packet>> {
            self.events.borrow_mut().push("drain".into());
            Ok(vec![Packet {
                peer: 9,
                epoch: 0x78563412,
                bytes: vec![7; 7],
                audio_generation: 2,
            }])
        }
        fn send_media(&self, p: &Packet, b: &[u8]) -> Result<()> {
            self.events.borrow_mut().push("media".into());
            if !self.stale_media.get() {
                self.send(p.peer as u32, b)?;
            }
            Ok(())
        }
        fn send(&self, p: u32, b: &[u8]) -> Result<()> {
            if self.failed_send.get() {
                return Err(Error);
            }
            self.sent.borrow_mut().push((p, b.to_vec()));
            Ok(())
        }
        fn poll(&self, k: Poll, b: u32, r: u16, i: u32) -> Result<Vec<Datagram>> {
            self.events
                .borrow_mut()
                .push(format!("poll:{k:?}:{b}:{r}:{i}"));
            Ok(vec![Datagram {
                peer: 9,
                epoch: 0x78563412,
                bytes: vec![8; 7],
            }])
        }
        fn drive(&self) -> Result<()> {
            self.events.borrow_mut().push("drive".into());
            Ok(())
        }
        fn agreement_generations(&self) -> u32 {
            self.count.get()
        }
        fn finish(&self, o: DirectObservation) -> Result<bool> {
            self.events.borrow_mut().push("finish".into());
            self.observations.borrow_mut().push(o);
            Ok(true)
        }
        fn failure(&self) {
            self.events.borrow_mut().push("failure".into());
        }
    }
    fn worker(epochs: bool) -> Worker {
        Worker::new(Config {
            self_peer: 2,
            physical: (1 << 2) | (1 << 9),
            epochs,
        })
    }
    fn packet(peer: u32, epochs: bool) -> Datagram {
        Datagram {
            peer,
            epoch: 0,
            bytes: Worker::framed(0x78563412, &[1; 7], epochs),
        }
    }
    #[test]
    fn real_io_order_budgets_and_sparse_physical_members_are_preserved() {
        let io = Fixture::default();
        io.count.set(3);
        io.setup.set(true);
        worker(true).iteration(&io).unwrap();
        let e = io.events.borrow();
        assert_eq!(
            &e[..6],
            [
                "ready:9",
                "failed:9",
                "signal:9",
                "sample:512",
                "drain",
                "media"
            ]
        );
        let polls = e
            .iter()
            .filter(|e| e.starts_with("poll:"))
            .cloned()
            .collect::<Vec<_>>();
        assert_eq!(
            polls,
            [
                "poll:Authority:2:512:0",
                "poll:Admissions:2:512:0",
                "poll:Common:4:512:0",
                "poll:Selection:4:512:0",
                "poll:Snapshot:4:512:0",
                "poll:Quality:16:512:0",
                "poll:Agreement:1:512:0",
                "poll:Agreement:1:512:1",
                "poll:Agreement:1:512:2",
                "poll:Agreement:1:512:0"
            ]
        );
        assert_eq!(e[e.len() - 1], "finish");
        let obs = io.observations.borrow();
        assert_eq!(obs[0].ready, 512);
        assert_eq!(obs[0].failed, 512);
        assert_eq!(
            obs[0].links.iter().map(|l| l.peer).collect::<Vec<_>>(),
            vec![2, 9]
        );
        for (_, bytes) in io.sent.borrow().iter() {
            assert_eq!(&bytes[..4], &0x78563412u32.to_le_bytes());
            assert_eq!(bytes.len(), 11);
        }
    }
    #[test]
    fn receive_budget_rejects_bad_packets_without_skipping_media_or_evicting_peers() {
        let io = Fixture::default();
        io.received.borrow_mut().extend((0..70).map(|i| {
            if i % 2 == 0 {
                Datagram {
                    bytes: vec![0; 10],
                    ..packet(9, true)
                }
            } else {
                packet(99, true)
            }
        }));
        worker(true).iteration(&io).unwrap();
        assert_eq!(io.rejected.get(), 64);
        assert_eq!(io.received.borrow().len(), 6);
        assert!(io.events.borrow().iter().any(|e| e == "media"));
        assert_eq!(io.observations.borrow()[0].failed, 512);
    }
    #[test]
    fn legacy_frames_and_generation_protocols_keep_their_distinct_prefix_contracts() {
        let io = Fixture::default();
        io.count.set(1);
        io.received.borrow_mut().push_back(packet(9, false));
        worker(false).iteration(&io).unwrap();
        assert!(io.events.borrow().iter().any(|e| e == "receive:9:0:7"));
        let sent = io.sent.borrow();
        // media, authority, admissions, common: no epoch negotiated.
        for index in [0, 1, 2, 3, 6] {
            assert_eq!(sent[index].1.len(), 7);
        }
        // Selection, snapshots and retained agreement always carry an epoch.
        for index in [4, 5, 7, 8, 9, 10] {
            assert_eq!(sent[index].1.len(), 11);
        }
    }
    #[test]
    fn delayed_generation_rotation_wraps_and_keeps_the_full_sixteen_owner_chain() {
        let io = Fixture::default();
        io.count.set(16);
        let mut worker = worker(true);
        worker.agreement_cursor = u32::MAX - 1;
        worker.iteration(&io).unwrap();
        let e = io.events.borrow();
        let polls = e
            .iter()
            .filter(|e| e.starts_with("poll:Agreement"))
            .cloned()
            .collect::<Vec<_>>();
        assert_eq!(
            polls,
            [
                "poll:Agreement:1:512:14",
                "poll:Agreement:1:512:15",
                "poll:Agreement:1:512:0",
                "poll:Agreement:1:512:1"
            ]
        );
        drop(e);
        io.count.set(17);
        assert_eq!(worker.iteration(&io), Err(Error));
    }
    #[test]
    fn stop_send_error_and_actual_media_fence_have_bounded_effects() {
        let io = Fixture::default();
        io.stop.set(true);
        worker(true).run(&io);
        assert!(io.events.borrow().is_empty());
        let io = Fixture::default();
        io.failed_send.set(true);
        worker(true).run(&io);
        assert_eq!(io.events.borrow().last().unwrap(), "failure");
        assert!(io.observations.borrow().is_empty());
        let io = Fixture::default();
        io.stale_media.set(true);
        worker(true).run(&io);
        assert_eq!(io.sent.borrow().len(), 6);
        assert_eq!(io.events.borrow().last().unwrap(), "wait:2");
    }
}
