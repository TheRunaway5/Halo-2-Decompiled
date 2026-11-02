//! Actual room socket handles, bounded queues and cancellation observations.
use serde_json::{Value, json};
use std::{
    collections::BTreeMap,
    sync::{
        Arc, Mutex,
        atomic::{AtomicBool, AtomicUsize, Ordering},
    },
    time::Instant,
};
use tokio::sync::{Notify, mpsc};

const QUEUE_BYTES: usize = 2 * 1024 * 1024;
const MAX_MESSAGE: usize = 120000;
#[derive(Clone)]
pub enum Outbound {
    Text(String),
    Packet {
        bytes: Vec<u8>,
        target: u8,
        origin: u64,
        recipient: u64,
    },
}
impl Outbound {
    fn len(&self) -> usize {
        match self {
            Self::Text(s) => s.len(),
            Self::Packet { bytes, .. } => bytes.len(),
        }
    }
}
pub struct Queued {
    pub message: Outbound,
    bytes: Arc<AtomicUsize>,
    size: usize,
}
impl Drop for Queued {
    fn drop(&mut self) {
        self.bytes.fetch_sub(self.size, Ordering::AcqRel);
    }
}
#[derive(Clone)]
struct Sender {
    channel: mpsc::Sender<Queued>,
    bytes: Arc<AtomicUsize>,
}
impl Sender {
    fn full(&self) -> bool {
        self.channel.is_closed()
            || self.channel.capacity() == 0
            || self.bytes.load(Ordering::Acquire) > QUEUE_BYTES - MAX_MESSAGE
    }
    fn send(&self, message: Outbound) -> bool {
        let size = message.len();
        if size > MAX_MESSAGE {
            return false;
        }
        if self
            .bytes
            .fetch_update(Ordering::AcqRel, Ordering::Acquire, |b| {
                b.checked_add(size).filter(|n| *n <= QUEUE_BYTES)
            })
            .is_err()
        {
            return false;
        }
        self.channel
            .try_send(Queued {
                message,
                bytes: self.bytes.clone(),
                size,
            })
            .is_ok()
    }
}
#[derive(Clone)]
struct Peer {
    connection: u64,
    features: Value,
    sender: Sender,
    closed: Arc<AtomicBool>,
    reason: Arc<Mutex<Option<(u16, String)>>>,
    wake: Arc<Notify>,
}
#[derive(Default)]
pub(super) struct Sockets {
    peers: BTreeMap<(String, String), Peer>,
    writes_retry_at: BTreeMap<String, Instant>,
}
pub struct Binding {
    pub room: String,
    pub player: String,
    pub connection: u64,
    pub incoming: mpsc::Receiver<Queued>,
    pub wake: Arc<Notify>,
    closed: Arc<AtomicBool>,
    reason: Arc<Mutex<Option<(u16, String)>>>,
}
impl Binding {
    pub fn actor(&self) -> Value {
        json!({"id":self.room,"player":self.player,"connection":self.connection})
    }
    pub fn closed(&self) -> bool {
        self.closed.load(Ordering::Acquire)
    }
    pub fn close(&self) {
        self.closed.store(true, Ordering::Release);
        self.wake.notify_one();
    }
    pub fn reason(&self) -> (u16, String) {
        self.reason
            .lock()
            .unwrap_or_else(|e| e.into_inner())
            .clone()
            .unwrap_or((1000, String::new()))
    }
}
impl Drop for Binding {
    fn drop(&mut self) {
        self.close()
    }
}
#[derive(Clone)]
struct Observation {
    room: String,
    player: String,
    connection: u64,
    closed: Arc<AtomicBool>,
}
pub struct Observations {
    peers: Vec<Observation>,
    started: Instant,
}
impl Observations {
    pub fn now(&self) -> f64 {
        self.started.elapsed().as_secs_f64()
    }
    pub fn facts(&self) -> Value {
        let now = self.now();
        let mut rooms: BTreeMap<String, Vec<Value>> = BTreeMap::new();
        for p in &self.peers {
            rooms
                .entry(p.room.clone())
                .or_default()
                .push(json!({"player":p.player,"connection":p.connection,
                "closed":p.closed.load(Ordering::Acquire),"control_now":now}));
        }
        json!(
            rooms
                .into_iter()
                .map(|(id, peers)| json!({"id":id,"io_now":now,"peers":peers}))
                .collect::<Vec<_>>()
        )
    }
}
impl Sockets {
    pub fn observations(&self, started: Instant) -> Observations {
        Observations {
            started,
            peers: self
                .peers
                .iter()
                .map(|((room, player), p)| Observation {
                    room: room.clone(),
                    player: player.clone(),
                    connection: p.connection,
                    closed: p.closed.clone(),
                })
                .collect(),
        }
    }
    pub fn attach(&mut self, room: &str, admission: &Value) -> crate::runtime::Result<Binding> {
        use crate::runtime::Error;
        let player = admission["player"]
            .as_str()
            .ok_or(Error(503, "Native room actor required"))?
            .to_owned();
        let connection = admission["connection"]
            .as_u64()
            .ok_or(Error(503, "Native room connection required"))?;
        let (channel, incoming) = mpsc::channel(512);
        let binding = Binding {
            room: room.to_owned(),
            player: player.clone(),
            connection,
            incoming,
            closed: Arc::new(AtomicBool::new(false)),
            reason: Arc::new(Mutex::new(None)),
            wake: Arc::new(Notify::new()),
        };
        let peer = Peer {
            connection,
            features: admission["features"].clone(),
            sender: Sender {
                channel,
                bytes: Arc::new(AtomicUsize::new(0)),
            },
            closed: binding.closed.clone(),
            reason: binding.reason.clone(),
            wake: binding.wake.clone(),
        };
        if let Some(old) = self.peers.insert((room.to_owned(), player), peer) {
            *old.reason.lock().unwrap_or_else(|e| e.into_inner()) =
                Some((1001, "Master connection replaced".into()));
            old.closed.store(true, Ordering::Release);
            old.wake.notify_one();
        }
        Ok(binding)
    }
    pub fn peers(&self, room: &str) -> Value {
        json!(self.peers.iter().filter(|((id,_),p)|id==room && !p.closed.load(Ordering::Acquire)).map(|((_,who),p)|json!({"player":who,
            "native_handoff":p.features["native_handoff"],"departure_game_over":p.features["departure_game_over"]})).collect::<Vec<_>>())
    }
    pub fn full(&self, room: &str) -> Value {
        json!(
            self.peers
                .iter()
                .filter(|((id, _), p)| id == room && p.sender.full())
                .map(|((_, who), _)| who)
                .collect::<Vec<_>>()
        )
    }
    pub fn send(&self, room: &str, player: &str, connection: u64, message: Outbound) -> bool {
        self.peers
            .get(&(room.to_owned(), player.to_owned()))
            .is_some_and(|p| {
                p.connection == connection
                    && !p.closed.load(Ordering::Acquire)
                    && p.sender.send(message)
            })
    }
    pub fn current(&self, room: &str, player: &str, connection: u64) -> bool {
        self.peers
            .get(&(room.to_owned(), player.to_owned()))
            .is_some_and(|p| p.connection == connection && !p.closed.load(Ordering::Acquire))
    }
    pub fn remove(&mut self, room: &str, player: &str, connection: u64) {
        let key = (room.to_owned(), player.to_owned());
        if self
            .peers
            .get(&key)
            .is_some_and(|p| p.connection == connection)
        {
            if let Some(peer) = self.peers.remove(&key) {
                peer.closed.store(true, Ordering::Release);
                peer.wake.notify_one();
            }
        }
    }
    pub fn close(&mut self, room: &str) {
        self.writes_retry_at.remove(room);
        let peers = self
            .peers
            .iter()
            .filter(|((id, _), _)| id == room)
            .map(|((_, who), p)| (who.clone(), p.connection))
            .collect::<Vec<_>>();
        for (who, connection) in peers {
            self.remove(room, &who, connection);
        }
    }
    pub fn write_ready(&self, room: &str) -> bool {
        self.writes_retry_at
            .get(room)
            .is_none_or(|at| Instant::now() >= *at)
    }
    pub fn failed_write(&mut self, room: &str) {
        self.writes_retry_at.insert(
            room.to_owned(),
            Instant::now() + std::time::Duration::from_secs(1),
        );
    }
    pub fn wrote(&mut self, room: &str) {
        self.writes_retry_at.remove(room);
    }
    pub fn disconnect(&self, room: &str, player: &str, connection: Option<u64>) {
        self.disconnect_reason(room, player, connection, 1008, "Match reader stalled");
    }
    pub fn disconnect_reason(
        &self,
        room: &str,
        player: &str,
        connection: Option<u64>,
        code: u16,
        reason: &str,
    ) {
        if let Some(p) = self
            .peers
            .get(&(room.to_owned(), player.to_owned()))
            .filter(|p| connection.is_none_or(|c| p.connection == c))
        {
            let mut reason = reason.to_owned();
            while reason.len() > 120 {
                reason.pop();
            }
            *p.reason.lock().unwrap_or_else(|e| e.into_inner()) = Some((code, reason));
            p.closed.store(true, Ordering::Release);
            p.wake.notify_one();
        }
    }
    pub fn close_reason(&mut self, room: &str, code: u16, reason: &str) {
        let players = self
            .peers
            .keys()
            .filter(|(id, _)| id == room)
            .map(|(_, who)| who.clone())
            .collect::<Vec<_>>();
        for who in players {
            self.disconnect_reason(room, &who, None, code, reason);
        }
        self.close(room);
    }
    pub fn ended(&self) -> Vec<Value> {
        self.peers
            .iter()
            .filter(|(_, p)| p.closed.load(Ordering::Acquire))
            .map(|((id, player), p)| json!({"id":id,"player":player,"connection":p.connection}))
            .collect()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn actual_queue_bytes_and_cancelled_generations_remain_bounded() {
        let mut sockets = Sockets::default();
        let started = Instant::now();
        let admission = json!({"player":"a","connection":1,"features":{"native_handoff":false,"departure_game_over":false}});
        let mut first = sockets.attach("room", &admission).unwrap();
        let observed = sockets.observations(started);
        for _ in 0..17 {
            assert!(sockets.send("room", "a", 1, Outbound::Text("x".repeat(MAX_MESSAGE))));
        }
        assert_eq!(sockets.full("room"), json!(["a"]));
        assert!(!sockets.send("room", "a", 1, Outbound::Text("x".repeat(MAX_MESSAGE))));
        drop(first.incoming.try_recv().unwrap());
        assert_eq!(sockets.full("room"), json!([]));
        let mut replacement = admission.clone();
        replacement["connection"] = json!(2);
        let next = sockets.attach("room", &replacement).unwrap();
        assert!(first.closed());
        sockets.remove("room", "a", 1);
        assert!(!next.closed());
        assert!(observed.facts()[0]["peers"][0]["closed"] == true);
        drop(next);
        assert_eq!(sockets.peers("room"), json!([]));
    }
}
