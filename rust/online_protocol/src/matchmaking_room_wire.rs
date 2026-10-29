//! Native room capabilities, connection generations and bounded signaling.
//! This is modern transport policy, never recovered Xbox Live authentication.
use super::{Coordinator, Setup, Stage, m, reject};
use crate::matchmaking_model::{Error, Result};
use crate::matchmaking_packet_budget::{Budget, Stream};
use serde_json::{Value, json};
use sha2::{Digest, Sha256};
use std::collections::{BTreeMap, BTreeSet};
pub(super) fn entropy(bytes: usize) -> Result<String> {
    let mut data = vec![0; bytes];
    getrandom::getrandom(&mut data).map_err(|_| Error(503, "Room entropy unavailable"))?;
    Ok(data.iter().map(|v| format!("{v:02x}")).collect())
}
pub(super) fn same(a: &str, b: &str) -> bool {
    let a = Sha256::digest(a.as_bytes());
    let b = Sha256::digest(b.as_bytes());
    a.iter()
        .zip(b.iter())
        .fold(0u8, |bits, (a, b)| bits | (a ^ b))
        == 0
}
struct Capability {
    token: String,
    revision: u64,
}
struct Connection {
    id: u64,
    features: Value,
    budget: Budget,
    control: super::control::Connection,
}
struct Pending {
    source: String,
    packet: Value,
}
pub(super) struct Wire {
    compatibility: String,
    caps: BTreeMap<String, Capability>,
    connections: BTreeMap<String, Connection>,
    detached: BTreeMap<String, u64>,
    next: u64,
    socket: String,
    keys: BTreeMap<(String, String), String>,
    users: BTreeMap<usize, String>,
    verified: BTreeMap<String, String>,
    pending: BTreeMap<String, Vec<Pending>>,
    counts: BTreeMap<(usize, usize), u32>,
    descriptions: BTreeSet<(usize, usize)>,
}
impl Wire {
    pub fn detached(&self, who: &str, connection: u64) -> bool {
        self.connections.get(who).is_none() && self.detached.get(who) == Some(&connection)
    }
    pub fn consume_detach(&mut self, who: &str, connection: u64) {
        if self.detached(who, connection) {
            self.detached.remove(who);
        }
    }
    pub fn control(&mut self, who: &str) -> Result<&mut super::control::Connection> {
        Ok(&mut self
            .connections
            .get_mut(who)
            .ok_or(Error(401, "Native connection retired"))?
            .control)
    }
    pub fn connection(&self, who: &str) -> Option<u64> {
        self.connections.get(who).map(|connection| connection.id)
    }
    pub fn budget(&mut self, who: &str, stream: Stream, now: f64) -> Result<()> {
        self.connections
            .get_mut(who)
            .ok_or(Error(401, "Native connection retired"))?
            .budget
            .take(stream, now)
    }
    pub fn features(&self, who: &str) -> Option<&Value> {
        self.connections.get(who).map(|c| &c.features)
    }
    pub fn new(owners: &[String], compatibility: &str) -> Result<Self> {
        let mut caps = BTreeMap::new();
        let mut keys = BTreeMap::new();
        for who in owners {
            caps.insert(
                who.clone(),
                Capability {
                    token: entropy(32)?,
                    revision: 1,
                },
            );
        }
        let mut sorted = owners.to_vec();
        sorted.sort();
        if ["halo2-native-queue-v13", "halo2-native-queue-v14"].contains(&compatibility)
            || crate::matchmaking_catalog::OPTIONS.contains(&compatibility)
        {
            for (i, a) in sorted.iter().enumerate() {
                for b in &sorted[i + 1..] {
                    keys.insert((a.clone(), b.clone()), entropy(32)?);
                }
            }
        }
        Ok(Self {
            compatibility: compatibility.to_owned(),
            caps,
            connections: BTreeMap::new(),
            detached: BTreeMap::new(),
            next: 0,
            socket: entropy(16)?,
            keys,
            users: BTreeMap::new(),
            verified: BTreeMap::new(),
            pending: BTreeMap::new(),
            counts: BTreeMap::new(),
            descriptions: BTreeSet::new(),
        })
    }
    fn eos(&self) -> bool {
        ["halo2-native-queue-v13", "halo2-native-queue-v14"].contains(&self.compatibility.as_str())
            || self.account()
    }
    fn account(&self) -> bool {
        self.compatibility == "halo2-native-queue-v14"
            || crate::matchmaking_catalog::OPTIONS.contains(&self.compatibility.as_str())
    }
    pub fn retire(&mut self, who: &str) {
        self.caps.remove(who);
        self.connections.remove(who);
        self.detached.remove(who);
        self.verified.remove(who);
        self.pending.remove(who);
        for pending in self.pending.values_mut() {
            pending.retain(|p| p.source != who);
        }
    }
    pub fn clear(&mut self) {
        self.caps.clear();
        self.connections.clear();
        self.detached.clear();
        self.verified.clear();
        self.pending.clear();
    }
    fn snapshot(&self) -> Value {
        json!({"tokens":self.caps.iter().map(|(p,c)|(p.clone(),c.token.clone())).collect::<BTreeMap<_,_>>(),
            "eos_users":self.users,"eos_verified":self.verified,"pending_signals":self.pending.iter().map(|(p,v)|(p.clone(),v.len())).collect::<BTreeMap<_,_>>(),
            "signal_counts":self.counts.iter().map(|((a,b),n)|json!([a,b,n])).collect::<Vec<_>>(),"signaled_descriptions":self.descriptions})
    }
}
fn connected(r: &Value, who: &str) -> bool {
    r["peers"]
        .as_array()
        .is_some_and(|rows| rows.iter().any(|p| p["player"] == who))
}
fn live(row: &Setup) -> Result<()> {
    reject(
        !row.life.closed && row.life.now <= row.life.deadline,
        404,
        "Match no longer exists",
    )
}
fn auth(row: &Setup, r: &Value) -> Result<Value> {
    live(row)?;
    let route = m::text(&r["route"], 16)?;
    if route == "eos" {
        reject(
            row.wire.account(),
            409,
            "Match does not use account-authenticated EOS transport",
        )?;
    } else {
        reject(
            (row.direct && route == "signal") || (!row.direct && route == "relay"),
            409,
            "Wrong match transport",
        )?;
    }
    let header = r["authorization"]
        .as_str()
        .ok_or(Error(401, "Match authorization required"))?;
    reject(
        header.starts_with("Bearer ") && header.len() < 128,
        401,
        "Match authorization required",
    )?;
    let token = &header[7..];
    let who = row
        .wire
        .caps
        .iter()
        .find(|(who, c)| row.life.active.contains(*who) && same(&c.token, token))
        .map(|(who, _)| who)
        .ok_or(Error(401, "Match authorization required"))?;
    let cap = &row.wire.caps[who];
    let value =
        json!({"player":who,"revision":cap.revision,"name":row.players[row.slot(who)?].name});
    if !r["prior"].is_null() {
        reject(r["prior"] == value, 401, "Match capability changed")?;
    }
    if route != "eos" {
        let has = row.wire.connections.contains_key(who) || connected(r, who);
        reject(
            !has || (row.direct
                && matches!(
                    row.stage,
                    Stage::Running | Stage::Reviewing | Stage::Postgame
                )),
            409,
            "Already connected to match",
        )?;
    }
    Ok(value)
}
pub(super) fn current(row: &Setup, r: &Value) -> Result<String> {
    live(row)?;
    let who = m::text(&r["player"], 64)?;
    row.actor(who)?;
    reject(
        row.life.active.contains(who) && row.wire.caps.contains_key(who),
        401,
        "Match authorization required",
    )?;
    let id = r["connection"]
        .as_u64()
        .ok_or(Error(400, "Native connection generation required"))?;
    reject(
        row.wire.connections.get(who).is_some_and(|c| c.id == id),
        409,
        "Match connection replaced",
    )?;
    Ok(who.to_owned())
}
pub(super) fn execute(row: &mut Setup, core: &mut Coordinator, r: &Value) -> Result<Value> {
    row.life.observe(r, core.clock)?;
    let op = m::text(&r["op"], 64)?;
    if op == "room_wire_snapshot" {
        return Ok(row.wire.snapshot());
    }
    if op == "room_wire_authorize" {
        return auth(row, r);
    }
    if op == "room_wire_current" {
        return Ok(json!(current(row, r).is_ok()));
    }
    if op == "room_wire_detach" {
        let who = m::text(&r["player"], 64)?;
        row.actor(who)?;
        let id = r["connection"]
            .as_u64()
            .ok_or(Error(400, "Native connection generation required"))?;
        let current = row.wire.connections.get(who).is_some_and(|c| c.id == id);
        if current {
            row.wire.connections.remove(who);
            row.wire.detached.insert(who.to_owned(), id);
        }
        return Ok(json!(current));
    }
    live(row)?;
    match op {
        "room_wire_rotate" => {
            let who = m::text(&r["player"], 64)?;
            row.actor(who)?;
            reject(
                row.life.active.contains(who),
                401,
                "Match authorization required",
            )?;
            let cap = row
                .wire
                .caps
                .get(who)
                .ok_or(Error(401, "Match capability retired"))?;
            let revision = cap
                .revision
                .checked_add(1)
                .ok_or(Error(503, "Room capability generation exhausted"))?;
            let token = entropy(32)?;
            row.wire.caps.insert(
                who.to_owned(),
                Capability {
                    token: token.clone(),
                    revision,
                },
            );
            Ok(json!({"token":token,"revision":revision}))
        }
        "room_wire_attach" => {
            let admission = auth(row, r)?;
            let who = admission["player"].as_str().unwrap().to_owned();
            let has = row.wire.connections.contains_key(&who) || connected(r, &who);
            reject(
                !has || (row.direct
                    && matches!(
                        row.stage,
                        Stage::Running | Stage::Reviewing | Stage::Postgame
                    )),
                409,
                "Already connected to match",
            )?;
            let id = row
                .wire
                .next
                .checked_add(1)
                .ok_or(Error(503, "Room connection generation exhausted"))?;
            let features = json!({"completion_detail":r["completion"]=="1","native_handoff":r["completion"]=="1"&&r["native_handoff"]=="1","departure_game_over":r["departure_game_over"]=="1"});
            row.wire.next = id;
            row.wire.detached.remove(&who);
            row.wire.connections.insert(
                who.clone(),
                Connection {
                    id,
                    features: features.clone(),
                    budget: Budget::default(),
                    control: super::control::Connection::default(),
                },
            );
            let mut publications = Vec::new();
            if row.direct && matches!(row.stage, Stage::Reviewing | Stage::Postgame) {
                publications
                    .push(json!({"postgame":core.results.report_hex(m::text(&r["id"],64)?)?}));
            } else {
                publications.push(row.control());
                if row.direct
                    && row.stage == Stage::Running
                    && features["departure_game_over"] == true
                    && row.life.departed != 0
                {
                    publications.push(json!({"departed_players":row.life.departed}));
                }
            }
            Ok(
                json!({"player":who,"connection":id,"features":features,"publications":publications,"voice_auth":super::packets::auth(row,&who)?}),
            )
        }
        "room_wire_proof_validate" | "room_wire_proof_commit" => {
            let admission = auth(row, r)?;
            let who = admission["player"].as_str().unwrap();
            reject(
                row.wire.account(),
                409,
                "Match does not use account-authenticated EOS transport",
            )?;
            reject(
                r["account"] == who,
                403,
                "EOS identity belongs to another account",
            )?;
            m::hex(&r["puid"], 16)?;
            let puid = m::text(&r["puid"], 32)?;
            reject(
                row.wire.verified.get(who).is_none_or(|p| p == puid),
                409,
                "EOS room identity changed",
            )?;
            reject(
                !row.wire.verified.iter().any(|(p, v)| p != who && v == puid),
                409,
                "EOS identity reused",
            )?;
            if op == "room_wire_proof_commit" {
                row.wire.verified.insert(who.to_owned(), puid.to_owned());
            }
            Ok(admission)
        }
        "room_wire_descriptor" => {
            let who = m::text(&r["player"], 64)?;
            let slot = row.actor(who)?;
            reject(
                row.life.active.contains(who),
                401,
                "Match authorization required",
            )?;
            let token = &row
                .wire
                .caps
                .get(who)
                .ok_or(Error(401, "Match capability retired"))?
                .token;
            let id = m::text(&r["id"], 64)?;
            let identities = core.results.identities(id)?;
            let couch = [
                "halo2-native-queue-v9",
                "halo2-native-queue-v10",
                "halo2-native-queue-v11",
                "halo2-native-queue-v12",
                "halo2-native-queue-v13",
                "halo2-native-queue-v14",
            ]
            .contains(&row.wire.compatibility.as_str())
                || crate::matchmaking_catalog::OPTIONS.contains(&row.wire.compatibility.as_str());
            let players = row
                .players
                .iter()
                .map(|p| {
                    let mut v = json!({"name":p.name,"team":p.team,"identity":identities[&p.id]});
                    if couch {
                        v["peer"] = json!(row.slot(&p.owner).unwrap());
                        v["guest"] = json!(p.guest);
                    }
                    v
                })
                .collect::<Vec<_>>();
            let mut v = json!({"id":id,"token":token,"slot":slot,"players":players});
            if row.wire.compatibility != "halo2-native-queue-v9" && couch {
                v["team_mode"] = json!(row.teams);
            }
            if row.direct {
                v["transport"] = json!({"kind":"direct-udp-v1","stun_servers":r["stun_servers"]});
            }
            if row.wire.eos() {
                let keys=row.wire.keys.iter().filter(|((a,b),_)|a==who||b==who).map(|((a,b),key)|json!({"slot":row.slot(if a==who{b}else{a}).unwrap(),"key":key})).collect::<Vec<_>>();
                v["transport"] = json!({"kind":"eos-p2p-v1","socket":row.wire.socket,"peers":keys,"session":core.results.session_identity(id)?});
            }
            if row.continuity {
                v["continuity"] = json!("complete-world-v1");
            }
            if couch {
                v["local_slots"] = json!(
                    row.players
                        .iter()
                        .enumerate()
                        .filter_map(|(i, p)| (p.owner == who).then_some(i))
                        .collect::<Vec<_>>()
                );
            }
            Ok(v)
        }
        "room_wire_drain" => {
            let who = current(row, r)?;
            let mut packets = Vec::new();
            if let Some(pending) = row.wire.pending.get_mut(&who) {
                while !pending.is_empty() && packets.len() < 16 {
                    let first = pending.remove(0);
                    if row.life.active.contains(&first.source) {
                        packets.push(first.packet);
                    }
                }
                if pending.is_empty() {
                    row.wire.pending.remove(&who);
                }
            }
            Ok(json!(packets))
        }
        "room_wire_signal" => {
            let who = current(row, r)?;
            reject(row.direct, 400, "Peer signaling is unavailable")?;
            let signal = &r["signal"];
            m::fields(signal, &["peer", "type", "payload", "mid"])?;
            let source = row.actor(&who)?;
            let target = m::integer(&signal["peer"], 0, (row.players.len() - 1) as i64)? as usize;
            reject(
                target != source && row.slot(&row.players[target].owner)? == target,
                400,
                "Invalid peer signal destination",
            )?;
            let kind = signal["type"]
                .as_str()
                .ok_or(Error(400, "Invalid peer signal"))?;
            let payload = signal["payload"]
                .as_str()
                .ok_or(Error(400, "Invalid peer signal"))?;
            let mid = signal["mid"]
                .as_str()
                .ok_or(Error(400, "Invalid peer signal"))?;
            m::check(
                !payload.is_empty()
                    && payload.chars().count() <= 16384
                    && mid.chars().count() <= 32,
                "Invalid peer signal",
            )?;
            m::check(
                if row.wire.eos() {
                    kind == "eos"
                } else {
                    ["offer", "answer", "candidate"].contains(&kind)
                },
                "Invalid peer signal",
            )?;
            let destination = row.players[target].owner.clone();
            if !row.life.active.contains(&destination) {
                return Ok(json!({"deliver":null,"packet":null}));
            }
            let key = (source, target);
            let count = row.wire.counts.get(&key).copied().unwrap_or(0) + 1;
            m::check(count <= 65, "Too many peer candidates")?;
            if row.wire.eos() {
                m::hex(&signal["payload"], 16)?;
                m::check(mid.is_empty(), "Invalid EOS identity")?;
                reject(
                    !row.wire.account()
                        || row.wire.verified.get(&who).is_some_and(|p| p == payload),
                    403,
                    "EOS identity has not been verified for this match",
                )?;
                m::check(
                    row.wire.users.get(&source).is_none_or(|p| p == payload)
                        && !row
                            .wire
                            .users
                            .iter()
                            .any(|(p, v)| *p != source && v == payload),
                    "EOS identity changed or reused",
                )?;
            } else if kind != "candidate" {
                m::check(
                    matches!(row.stage, Stage::Connecting | Stage::Loading)
                        && mid.is_empty()
                        && !row.wire.descriptions.contains(&key)
                        && kind == if source < target { "offer" } else { "answer" },
                    "Invalid peer description",
                )?;
            } else {
                m::check(payload.chars().count() <= 1024, "Invalid peer candidate")?;
            }
            let immediate = connected(r, &destination);
            if immediate {
                reject(
                    !r["full"]
                        .as_array()
                        .is_some_and(|v| v.iter().any(|v| v == &destination)),
                    503,
                    "Peer signaling reader stalled",
                )?;
            } else {
                m::check(
                    row.wire.pending.get(&destination).map_or(0, Vec::len) < 512,
                    "Too many pending peer candidates",
                )?;
            }
            let packet =
                json!({"peer_signal":{"peer":source,"type":kind,"payload":payload,"mid":mid}});
            row.wire.counts.insert(key, count);
            if row.wire.eos() {
                row.wire.users.insert(source, payload.to_owned());
            } else if kind != "candidate" {
                row.wire.descriptions.insert(key);
            }
            if !immediate {
                row.wire
                    .pending
                    .entry(destination.clone())
                    .or_default()
                    .push(Pending {
                        source: who,
                        packet,
                    });
                return Ok(json!({"deliver":null,"packet":null}));
            }
            Ok(json!({"deliver":destination,"packet":packet}))
        }
        _ => Err(Error(400, "Unknown native room wire operation")),
    }
}
