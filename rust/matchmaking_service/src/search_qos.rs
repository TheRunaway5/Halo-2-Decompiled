//! Actual socket observations and IO destinations for the existing native QoS
//! owner. This module supplies neither measurements nor matching decisions.
use super::*;
use std::{
    collections::BTreeMap,
    sync::{
        Arc,
        atomic::{AtomicBool, Ordering},
    },
};
use tokio::sync::Notify;

#[derive(Clone)]
struct Socket {
    connection: u64,
    closed: Arc<AtomicBool>,
    wake: Arc<Notify>,
}
#[derive(Default)]
pub(super) struct Sockets {
    peers: BTreeMap<String, Socket>,
}
/// This lease stays with its actual WebSocket future. Cancellation immediately
/// publishes closure even before the blocking owner can process a detach.
pub struct SocketBinding {
    player: String,
    connection: u64,
    closed: Arc<AtomicBool>,
    pub wake: Arc<Notify>,
}
impl SocketBinding {
    pub fn actor(&self) -> Value {
        json!({"player":self.player,"connection":self.connection})
    }
    pub fn closed(&self) -> bool {
        self.closed.load(Ordering::Acquire)
    }
    pub fn close(&self) {
        self.closed.store(true, Ordering::Release);
        self.wake.notify_one();
    }
}
impl Drop for SocketBinding {
    fn drop(&mut self) {
        self.close();
    }
}
impl Sockets {
    fn facts(&self) -> Value {
        json!(self.peers.iter().map(|(player,p)|json!({"player":player,"connection":p.connection,"closed":p.closed.load(Ordering::Acquire)})).collect::<Vec<_>>())
    }
    fn drain(&self, owners: &mut Owners, source: u64, clock: &Clock) -> Result<()> {
        // Native peers/wake destinations are capped at1000. The bounded drain
        // never alters a measurement clock or accepts an HTTP destination.
        for _ in 0..33 {
            let wakes = owners.request(
                source,
                &json!({"op":"search_qos_wakeups","now":clock.monotonic}),
            )?;
            let rows = wakes
                .as_array()
                .ok_or(Error(503, "Native QoS wake destinations required"))?;
            for row in rows {
                if let Some(peer) = row["player"].as_str().and_then(|id| self.peers.get(id)) {
                    if row["connection"].as_u64() == Some(peer.connection) {
                        peer.wake.notify_one();
                    }
                }
            }
            if rows.len() < 32 {
                return Ok(());
            }
        }
        Err(Error(503, "QoS wake destinations exceed bound"))
    }
    pub(super) fn request(
        &self,
        owners: &mut Owners,
        source: u64,
        mut request: Value,
        clock: &Clock,
    ) -> Result<Value> {
        request["now"] = json!(clock.monotonic);
        request["qos_now"] = json!(clock.monotonic);
        request["io_peers"] = self.facts();
        let result = owners.request(source, &request);
        self.drain(owners, source, clock)?;
        result
    }
}
impl Runtime {
    fn qos_request(&mut self, request: Value) -> Result<Value> {
        let source = self.queue_source()?;
        let clock = self.clock()?;
        self.qos_sockets
            .request(&mut self.owners, source, request, &clock)
    }
    pub fn qos_authorize(&mut self, header: &str) -> Result<Value> {
        // Drop ended futures' physical observations before applying a fresh
        // native authorization; live replacement is still the owner's choice.
        let ended = self
            .qos_sockets
            .peers
            .iter()
            .filter(|(_, p)| p.closed.load(Ordering::Acquire))
            .map(|(player, p)| json!({"player":player,"connection":p.connection}))
            .collect::<Vec<_>>();
        for actor in ended {
            self.qos_detach(actor)?;
        }
        let session = self.matchmaking_auth(header)?;
        self.qos_request(json!({"op":"search_qos_authorize","generation":session["generation"],"key":session["key"],"prior":null}))
    }
    pub fn qos_attach(&mut self, prior: Value) -> Result<SocketBinding> {
        let row=self.qos_request(json!({"op":"search_qos_attach","generation":prior["generation"],"key":prior["key"],"prior":prior}))?;
        let player = row["player"]
            .as_str()
            .ok_or(Error(503, "Native QoS actor required"))?
            .to_owned();
        let connection = row["connection"]
            .as_u64()
            .ok_or(Error(503, "Native QoS connection required"))?;
        let binding = SocketBinding {
            player: player.clone(),
            connection,
            closed: Arc::new(AtomicBool::new(false)),
            wake: Arc::new(Notify::new()),
        };
        let socket = Socket {
            connection,
            closed: binding.closed.clone(),
            wake: binding.wake.clone(),
        };
        if let Some(previous) = self.qos_sockets.peers.insert(player, socket) {
            previous.closed.store(true, Ordering::Release);
            previous.wake.notify_one();
        }
        Ok(binding)
    }
    pub fn qos_tick(&mut self, mut actor: Value) -> Result<Value> {
        actor["op"] = json!("search_qos_tick");
        self.qos_request(actor)
    }
    pub fn qos_receive(
        &mut self,
        mut actor: Value,
        kind: &str,
        text: Option<String>,
    ) -> Result<()> {
        actor["op"] = json!("search_qos_receive");
        actor["kind"] = json!(kind);
        actor["text"] = json!(text);
        self.qos_request(actor)?;
        Ok(())
    }
    pub fn qos_detach(&mut self, mut actor: Value) -> Result<()> {
        actor["op"] = json!("search_qos_detach");
        let result = self.qos_request(actor.clone());
        if let Some(who) = actor["player"].as_str() {
            if self
                .qos_sockets
                .peers
                .get(who)
                .is_some_and(|p| Some(p.connection) == actor["connection"].as_u64())
            {
                self.qos_sockets.peers.remove(who);
            }
        }
        result?;
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    fn admitted() -> (Runtime, String) {
        let catalog =
            include_bytes!("../../../multiplayer/master_server/examples/matchmaking-online.json");
        let mut runtime = Runtime::open(Path::new(":memory:"), true, Some(catalog)).unwrap();
        let token = format!("h2s_{}", "A".repeat(43));
        runtime
            .db
            .execute(
                "INSERT INTO steam_login_sessions VALUES(?,?,?,?)",
                rusqlite::params![
                    authentication::steam_digest(&token).unwrap(),
                    "76561197960287930",
                    "Fixture",
                    database::wall_now().unwrap() + 3600
                ],
            )
            .unwrap();
        let identity = runtime.steam(&token).unwrap();
        let reply=runtime.matchmaking_login(json!({"screen_name":"Upgrade control","maps":["lockout"],"compatibility":"halo2-native-queue-v16","language":0}),vec![Some(identity)]).unwrap();
        (
            runtime,
            format!("Bearer {}", reply["token"].as_str().unwrap()),
        )
    }
    #[test]
    fn upgraded_stream_cannot_attach_an_expired_or_retired_preflight() {
        let (mut runtime, header) = admitted();
        let prior = runtime.qos_authorize(&header).unwrap();
        // Trusted service clock IO advances while the HTTP upgrade is pending.
        runtime.started = Instant::now() - std::time::Duration::from_secs(901);
        assert_eq!(runtime.qos_attach(prior).err().unwrap().0, 401);
        assert!(runtime.qos_sockets.peers.is_empty());
        let (mut runtime, header) = admitted();
        let prior = runtime.qos_authorize(&header).unwrap();
        runtime.matchmaking_logout(&header).unwrap();
        assert_eq!(runtime.qos_attach(prior).err().unwrap().0, 401);
        assert!(runtime.qos_sockets.peers.is_empty());
    }
}
