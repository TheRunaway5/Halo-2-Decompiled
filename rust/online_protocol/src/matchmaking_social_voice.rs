//! Modern opt-in social Opus relay. Native account/membership capabilities,
//! sequence/burst policy and five-frame queues; no audio recording or OG claim.
use crate::matchmaking_coordinator::id;
use crate::matchmaking_model::{self as m, Error, Result};
use serde_json::{Value, json};
use std::collections::{BTreeMap, BTreeSet, VecDeque};

struct Frame {
    receipt: String,
    source: String,
    packet: String,
    created: f64,
}
struct Peer {
    id: String,
    key: String,
    player: String,
    group: String,
    wire: String,
    epoch: i64,
    sequence: Option<u32>,
    credits: f64,
    previous: f64,
    queue: VecDeque<Frame>,
}

#[cfg(test)]
mod tests {
    use super::*;
    fn relay() -> Relay {
        Relay::new(&json!({"kind":"party","account_source":1,"source":2})).unwrap()
    }
    fn live(r: &Relay) -> BTreeSet<String> {
        r.bindings().into_iter().map(|p| p.0).collect()
    }
    fn attach(r: &mut Relay, owner: u64, player: &str, group: &str) -> String {
        r.directory(owner,&json!({"op":"attach","player":player,"key":"11".repeat(32),"membership":{"group":group,"wire":"11111111-2222-3333-4444-555555555555","epoch":1}}),&live(r),true).unwrap();
        let result = r.committed(owner, &live(r)).unwrap().unwrap();
        result["peer"].as_str().unwrap().to_owned()
    }
    fn send(r: &mut Relay, owner: u64, peer: &str, sequence: u32, targets: &[&str]) -> Value {
        r.directory(owner,&json!({"op":"frame","peer":peer,"binary":true,"packet":format!("01{sequence:08x}7a"),"targets":targets}),&live(r),false).unwrap();
        r.committed(owner, &live(r)).unwrap().unwrap()
    }
    #[test]
    fn bounded_native_queue_preserves_wire_and_drops_oldest_frames() {
        let mut r = relay();
        let a = attach(&mut r, 1, "a", "g");
        let b = attach(&mut r, 2, "b", "g");
        for seq in 1..=8 {
            send(&mut r, 10 + seq as u64, &a, seq, &[&b]);
        }
        assert_eq!(r.peers[&b].queue.len(), 5);
        let receipt = r.peers[&b].queue[0].receipt.clone();
        r.directory(
            30,
            &json!({"op":"deliver","peer":b,"receipt":receipt,"allowed":true}),
            &live(&r),
            false,
        )
        .unwrap();
        let value = r.committed(30, &live(&r)).unwrap().unwrap();
        assert_eq!(
            value["packet"],
            "0111111111222233334444555555555555000000047a"
        );
        assert_eq!(value["again"], true);
        assert_eq!(r.peers[&b].queue.len(), 4);
    }
    #[test]
    fn replacement_fences_queued_audio_stale_completion_and_old_disconnect() {
        let mut r = relay();
        let a = attach(&mut r, 1, "a", "g");
        let b = attach(&mut r, 2, "b", "g");
        send(&mut r, 3, &a, 1, &[&b]);
        let receipt = r.peers[&b].queue[0].receipt.clone();
        r.directory(
            4,
            &json!({"op":"deliver","peer":b,"receipt":receipt,"allowed":true}),
            &live(&r),
            false,
        )
        .unwrap();
        let newer = attach(&mut r, 5, "a", "g");
        assert!(r.committed(4, &live(&r)).is_err());
        assert!(r.peers[&b].queue.is_empty());
        assert!(!r.peers.contains_key(&a));
        r.directory(6, &json!({"op":"detach","peer":a}), &live(&r), false)
            .unwrap();
        r.committed(6, &live(&r)).unwrap();
        assert!(r.peers.contains_key(&newer));
        assert!(send(&mut r, 7, &newer, 1, &[&b]).get("wake").is_some());
    }
    #[test]
    fn replay_burst_clock_and_stale_media_have_finite_native_limits() {
        let mut r = relay();
        let a = attach(&mut r, 1, "a", "g");
        let b = attach(&mut r, 2, "b", "g");
        for seq in 1..=10 {
            assert!(
                send(&mut r, 10 + seq as u64, &a, seq, &[&b])
                    .get("wake")
                    .is_some()
            );
        }
        r.observe(0.02).unwrap();
        assert!(send(&mut r, 30, &a, 11, &[&b]).get("wake").is_some());
        r.observe(0.).unwrap();
        assert_eq!(send(&mut r, 31, &a, 12, &[&b])["code"], 1008);
        assert!(!r.peers.contains_key(&a));
        let a = attach(&mut r, 32, "a", "g");
        send(&mut r, 33, &a, 1, &[&b]);
        let receipt = r.peers[&b].queue[0].receipt.clone();
        r.observe(5.02).unwrap();
        r.directory(
            34,
            &json!({"op":"deliver","peer":b,"receipt":receipt,"allowed":true}),
            &live(&r),
            false,
        )
        .unwrap();
        assert!(r.committed(34, &live(&r)).unwrap().unwrap()["packet"].is_null());
        assert_eq!(send(&mut r, 35, &a, 1, &[&b])["code"], 1008);
    }
    #[test]
    fn publication_requires_commit_live_account_and_matching_revision() {
        let mut r = relay();
        let request = json!({"op":"attach","player":"a","key":"11".repeat(32),"membership":{"group":"g","wire":"11111111-2222-3333-4444-555555555555","epoch":1}});
        r.directory(1, &request, &BTreeSet::new(), true).unwrap();
        assert!(r.peers.is_empty());
        r.abort(1);
        assert!(r.plans.is_empty());
        r.directory(2, &request, &BTreeSet::new(), true).unwrap();
        assert!(r.committed(2, &BTreeSet::new()).is_err());
        assert!(r.peers.is_empty());
        assert!(r.plans.is_empty());
        assert!(r.directory(3, &request, &BTreeSet::new(), false).is_err());
        assert!(r.execute(&json!({"op":"install","peers":[]})).is_err());
    }
    #[test]
    fn full_service_and_group_caps_allow_only_same_player_replacement() {
        let mut r = relay();
        for i in 0..256 {
            attach(&mut r, i + 1, &format!("p{i}"), &format!("g{}", i / 16));
        }
        assert_eq!(r.peers.len(), 256);
        assert_eq!(r.capacity("stranger", "g0").unwrap_err().0, 503);
        assert_eq!(r.capacity("p32", "g0").unwrap_err().0, 409);
        let before = r.peers.keys().cloned().collect::<BTreeSet<_>>();
        let replaced = attach(&mut r, 300, "p0", "g0");
        assert!(!before.contains(&replaced));
        assert_eq!(r.peers.len(), 256);
        assert_eq!(r.peers.values().filter(|p| p.player == "p0").count(), 1);
    }
}
impl Peer {
    fn view(&self) -> Value {
        json!({"id":self.id,"player":self.player,"group":self.group,"wire":self.wire,"epoch":self.epoch})
    }
}
enum Action {
    Attach(Peer),
    Frame {
        peer: String,
        sequence: u32,
        credits: f64,
        packet: String,
        targets: Vec<String>,
    },
    Deliver {
        peer: String,
        receipt: String,
        allowed: bool,
    },
    Close {
        peer: String,
        code: u16,
    },
    Detach(String),
}
struct Plan {
    revision: u64,
    expires: f64,
    action: Action,
}
pub struct Relay {
    kind: String,
    accounts: u64,
    source: Option<u64>,
    now: f64,
    revision: u64,
    peers: BTreeMap<String, Peer>,
    plans: BTreeMap<u64, Plan>,
}
fn uuid(v: &Value) -> Result<String> {
    let value = m::text(v, 36)?;
    m::check(
        value.len() == 36
            && value.bytes().enumerate().all(|(i, c)| {
                if [8, 13, 18, 23].contains(&i) {
                    c == b'-'
                } else {
                    c.is_ascii_digit() || (b'a'..=b'f').contains(&c)
                }
            }),
        "Invalid voice identity",
    )?;
    Ok(value.to_owned())
}
fn close(peer: &str, code: u16) -> Value {
    json!({"close":peer,"code":code,"packet":null,"again":false})
}
impl Relay {
    pub fn new(r: &Value) -> Result<Self> {
        m::fields(r, &["kind", "account_source", "source"])?;
        m::check(
            r["kind"] == "clan" || r["kind"] == "party",
            "Unknown voice audience",
        )?;
        let accounts = r["account_source"]
            .as_u64()
            .ok_or(Error(400, "Voice account source required"))?;
        let source = if r["source"].is_null() {
            None
        } else {
            Some(
                r["source"]
                    .as_u64()
                    .ok_or(Error(400, "Voice matchmaking source required"))?,
            )
        };
        Ok(Self {
            kind: r["kind"].as_str().unwrap().to_owned(),
            accounts,
            source,
            now: 0.,
            revision: 0,
            peers: BTreeMap::new(),
            plans: BTreeMap::new(),
        })
    }
    pub fn sources(&self) -> (u64, Option<u64>) {
        (self.accounts, self.source)
    }
    pub fn observe(&mut self, now: f64) -> Result<()> {
        m::check(now >= 0. && (now + 300.).is_finite(), "Invalid voice clock")?;
        self.now = self.now.max(now);
        self.plans.retain(|_, p| p.expires > self.now);
        Ok(())
    }
    pub fn bindings(&self) -> Vec<(String, String, String)> {
        let mut rows = self
            .peers
            .values()
            .map(|p| (p.id.clone(), p.key.clone(), p.player.clone()))
            .collect::<Vec<_>>();
        rows.extend(self.plans.values().filter_map(|p| {
            if let Action::Attach(p) = &p.action {
                Some((p.id.clone(), p.key.clone(), p.player.clone()))
            } else {
                None
            }
        }));
        rows
    }
    fn stage(&mut self, owner: u64, action: Action) -> Result<Value> {
        m::check(
            !self.plans.contains_key(&owner),
            "Voice plan already sealed",
        )?;
        m::check(self.plans.len() < 128, "Voice pending capacity reached")?;
        self.plans.insert(
            owner,
            Plan {
                revision: self.revision,
                expires: self.now + 30.,
                action,
            },
        );
        Ok(json!({"staged":true}))
    }
    fn capacity(&self, player: &str, group: &str) -> Result<()> {
        if self.peers.len() >= 256 && !self.peers.values().any(|p| p.player == player) {
            return Err(Error(503, "Voice service is full"));
        }
        if self
            .peers
            .values()
            .filter(|p| p.group == group && p.player != player)
            .count()
            >= 16
        {
            return Err(Error(
                409,
                if self.kind == "clan" {
                    "Clan voice is full (16 players)"
                } else {
                    "Party voice is full (16 players)"
                },
            ));
        }
        Ok(())
    }
    pub fn directory(
        &mut self,
        owner: u64,
        r: &Value,
        live: &BTreeSet<String>,
        admitted: bool,
    ) -> Result<Value> {
        let op = m::text(&r["op"], 32)?;
        if op == "context" {
            m::fields(r, &["op"])?;
            return Ok(json!({"kind":self.kind}));
        }
        if op == "preflight" || op == "attach" {
            m::fields(r, &["op", "player", "key", "membership"])?;
            if !admitted {
                return Err(Error(401, "ONLINE session expired; reconnect"));
            }
            let player = m::text(&r["player"], 64)?.to_owned();
            m::hex(&r["key"], 32)?;
            let member = &r["membership"];
            m::fields(member, &["group", "wire", "epoch"])?;
            let group = m::text(&member["group"], 64)?.to_owned();
            let wire = uuid(&member["wire"])?;
            let epoch = m::integer(&member["epoch"], 0, i64::MAX)?;
            self.capacity(&player, &group)?;
            if op == "preflight" {
                return Ok(Value::Null);
            }
            return self.stage(
                owner,
                Action::Attach(Peer {
                    id: id()?,
                    key: r["key"].as_str().unwrap().to_owned(),
                    player,
                    group,
                    wire,
                    epoch,
                    sequence: None,
                    credits: 10.,
                    previous: self.now,
                    queue: VecDeque::new(),
                }),
            );
        }
        let peer = m::text(&r["peer"], 36)?.to_owned();
        match op {
            "detach" => {
                m::fields(r, &["op", "peer"])?;
                self.stage(owner, Action::Detach(peer))
            }
            "close" => {
                m::fields(r, &["op", "peer", "code"])?;
                let code = m::integer(&r["code"], 1003, 1008)? as u16;
                self.stage(owner, Action::Close { peer, code })
            }
            "peer" => {
                m::fields(r, &["op", "peer"])?;
                Ok(self
                    .peers
                    .get(&peer)
                    .filter(|_| live.contains(&peer))
                    .map(Peer::view)
                    .unwrap_or(Value::Null))
            }
            "audience" => {
                m::fields(r, &["op", "peer"])?;
                let speaker = self
                    .peers
                    .get(&peer)
                    .ok_or(Error(409, "Voice peer retired"))?;
                Ok(json!(
                    self.peers
                        .values()
                        .filter(|p| p.id != peer
                            && p.group == speaker.group
                            && live.contains(&p.id))
                        .map(Peer::view)
                        .collect::<Vec<_>>()
                ))
            }
            "next" => {
                m::fields(r, &["op", "peer"])?;
                let listener = self
                    .peers
                    .get(&peer)
                    .ok_or(Error(409, "Voice peer retired"))?;
                Ok(listener
                    .queue
                    .front()
                    .map(|f| json!({"source":f.source,"receipt":f.receipt}))
                    .unwrap_or(Value::Null))
            }
            "deliver" => {
                m::fields(r, &["op", "peer", "receipt", "allowed"])?;
                self.stage(
                    owner,
                    Action::Deliver {
                        peer,
                        receipt: m::text(&r["receipt"], 36)?.to_owned(),
                        allowed: m::boolean(&r["allowed"])?,
                    },
                )
            }
            "frame" => {
                m::fields(r, &["op", "peer", "binary", "packet", "targets"])?;
                if !m::boolean(&r["binary"])? {
                    return self.stage(owner, Action::Close { peer, code: 1003 });
                }
                let packet = r["packet"]
                    .as_str()
                    .ok_or(Error(400, "Voice packet carrier required"))?;
                if !(12..=522).contains(&packet.len()) || packet.len() % 2 != 0 {
                    return self.stage(owner, Action::Close { peer, code: 1008 });
                }
                let bytes = m::hex(&r["packet"], packet.len() / 2)?;
                let current = self
                    .peers
                    .get(&peer)
                    .ok_or(Error(409, "Voice peer retired"))?;
                let sequence = u32::from_be_bytes(bytes[1..5].try_into().unwrap());
                let credits = (current.credits + (self.now - current.previous) * 50.).min(10.);
                if bytes[0] != 1 || current.sequence.is_some_and(|s| sequence <= s) || credits < 1.
                {
                    return self.stage(owner, Action::Close { peer, code: 1008 });
                }
                let targets = r["targets"]
                    .as_array()
                    .ok_or(Error(400, "Voice audience required"))?;
                m::check(targets.len() <= 15, "Voice audience exceeds bounds")?;
                let mut seen = BTreeSet::new();
                for target in targets {
                    let target = m::text(target, 36)?.to_owned();
                    let listener = self
                        .peers
                        .get(&target)
                        .ok_or(Error(409, "Voice listener retired"))?;
                    m::check(
                        target != peer && listener.group == current.group && seen.insert(target),
                        "Invalid voice audience",
                    )?;
                }
                let packet = format!("01{}{}", current.wire.replace('-', ""), &packet[2..]);
                self.stage(
                    owner,
                    Action::Frame {
                        peer,
                        sequence,
                        credits: credits - 1.,
                        packet,
                        targets: seen.into_iter().collect(),
                    },
                )
            }
            _ => Err(Error(400, "Unknown native voice event")),
        }
    }
    fn remove(&mut self, peer: &str) {
        self.peers.remove(peer);
        for target in self.peers.values_mut() {
            target.queue.retain(|f| f.source != peer);
        }
    }
    pub fn committed(&mut self, owner: u64, live: &BTreeSet<String>) -> Result<Option<Value>> {
        let plan = self
            .plans
            .remove(&owner)
            .ok_or(Error(409, "Voice commit plan retired"))?;
        if plan.revision != self.revision {
            return Err(Error(409, "Voice state changed; retry"));
        }
        let next = self
            .revision
            .checked_add(1)
            .ok_or(Error(503, "Voice generation exhausted"))?;
        let result = match plan.action {
            Action::Attach(peer) => {
                if !live.contains(&peer.id) {
                    return Err(Error(401, "ONLINE session expired; reconnect"));
                }
                self.capacity(&peer.player, &peer.group)?;
                let old = self
                    .peers
                    .values()
                    .find(|p| p.player == peer.player)
                    .map(|p| p.id.clone());
                if let Some(old) = &old {
                    self.remove(old);
                }
                let result = json!({"peer":peer.id,"replaced":old});
                self.peers.insert(peer.id.clone(), peer);
                result
            }
            Action::Detach(peer) => {
                self.remove(&peer);
                Value::Null
            }
            Action::Close { peer, code } => {
                self.remove(&peer);
                close(&peer, code)
            }
            Action::Frame {
                peer,
                sequence,
                credits,
                packet,
                targets,
            } => {
                if !live.contains(&peer) || !self.peers.contains_key(&peer) {
                    self.remove(&peer);
                    close(&peer, 1008)
                } else {
                    // Allocate every receipt before changing stream/queue state.
                    let frames = targets
                        .into_iter()
                        .filter(|t| live.contains(t) && self.peers.contains_key(t))
                        .map(|t| {
                            Ok((
                                t,
                                Frame {
                                    receipt: id()?,
                                    source: peer.clone(),
                                    packet: packet.clone(),
                                    created: self.now,
                                },
                            ))
                        })
                        .collect::<Result<Vec<_>>>()?;
                    let speaker = self.peers.get_mut(&peer).unwrap();
                    speaker.sequence = Some(sequence);
                    speaker.credits = credits;
                    speaker.previous = self.now;
                    let mut wake = Vec::new();
                    for (target, frame) in frames {
                        let listener = self.peers.get_mut(&target).unwrap();
                        if listener.queue.len() >= 5 {
                            listener.queue.pop_front();
                        }
                        listener.queue.push_back(frame);
                        wake.push(target);
                    }
                    json!({"wake":wake})
                }
            }
            Action::Deliver {
                peer,
                receipt,
                allowed,
            } => {
                if !live.contains(&peer) {
                    self.remove(&peer);
                    close(&peer, 1008)
                } else {
                    let listener = self
                        .peers
                        .get_mut(&peer)
                        .ok_or(Error(409, "Voice peer retired"))?;
                    let frame = listener
                        .queue
                        .front()
                        .ok_or(Error(409, "Voice queue changed"))?;
                    m::check(frame.receipt == receipt, "Voice receipt retired")?;
                    let frame = listener.queue.pop_front().unwrap();
                    json!({"packet":if allowed && live.contains(&frame.source) && frame.created+5.>self.now{json!(frame.packet)}else{Value::Null},"again":!listener.queue.is_empty()})
                }
            }
        };
        self.revision = next;
        Ok(Some(result))
    }
    pub fn abort(&mut self, owner: u64) {
        self.plans.remove(&owner);
    }
    pub fn execute(&mut self, r: &Value) -> Result<Value> {
        m::fields(r, &["op"])?;
        m::check(r["op"] == "stats", "Unknown voice operation")?;
        Ok(
            json!({"peers":self.peers.len(),"pending":self.plans.len(),"queued":self.peers.values().map(|p|p.queue.len()).sum::<usize>()}),
        )
    }
}
