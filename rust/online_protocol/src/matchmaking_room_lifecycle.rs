//! Native room phases, departures, PCR authority and postgame effects.
//! Deadlines and direct/relay policies are declared replacements, not Live recovery.
use super::{Coordinator, Setup, Stage, call, m, reject};
use crate::matchmaking_model::{Error, Result};
use serde_json::{Value, json};
use std::collections::{BTreeMap, BTreeSet};

pub(super) struct Life {
    pub active: BTreeSet<String>,
    tickets: Vec<String>,
    pub now: f64,
    created: Option<f64>,
    pub(super) deadline: f64,
    connection_seconds: f64,
    pub(super) closed: bool,
    pub(super) departed: u32,
    recorded: Option<bool>,
    host_departed: bool,
}
impl Life {
    pub fn new(owners: &[String], compatibility: &str, tickets: Vec<String>) -> Self {
        let connection_seconds = if compatibility == "halo2-native-queue-v14"
            || crate::matchmaking_catalog::OPTIONS.contains(&compatibility)
        {
            120.
        } else if compatibility == "halo2-native-queue-v13" {
            60.
        } else {
            25.
        };
        Self {
            active: owners.iter().cloned().collect(),
            tickets,
            now: 0.,
            created: None,
            deadline: 0.,
            connection_seconds,
            closed: false,
            departed: 0,
            recorded: None,
            host_departed: false,
        }
    }
    pub fn observe(&mut self, r: &Value, fallback: f64) -> Result<()> {
        let now = if r.get("io_now").is_some() {
            m::real(&r["io_now"])?
        } else if self.created.is_none() {
            fallback
        } else {
            // Account/queue clocks cannot expire a transferred running room.
            self.now
        };
        m::check(now >= 0. && (now + 7200.).is_finite(), "Invalid room clock")?;
        self.now = self.now.max(now);
        if self.created.is_none() {
            self.created = Some(self.now);
            self.arm(self.connection_seconds);
        }
        Ok(())
    }
    pub fn arm(&mut self, seconds: f64) {
        self.deadline = self.now + seconds;
    }
}
fn effect() -> Value {
    json!({"close":null,"retire":[],"review":false,"completed":false,"commit":false,"publish":null,"notice":null})
}
fn close(reason: &str) -> Value {
    let mut e = effect();
    e["close"] = json!(reason);
    e
}
#[derive(Clone, Copy)]
struct Peer {
    handoff: bool,
    departure: bool,
}
fn peers(row: &Setup, r: &Value) -> Result<BTreeMap<String, Peer>> {
    let rows = r["peers"]
        .as_array()
        .ok_or(Error(400, "Room socket observations required"))?;
    m::check(rows.len() <= 16, "Too many physical room sockets")?;
    let mut out = BTreeMap::new();
    for p in rows {
        m::fields(p, &["player", "native_handoff", "departure_game_over"])?;
        let who = m::text(&p["player"], 64)?;
        row.actor(who)?;
        m::check(!out.contains_key(who), "Duplicate physical room socket")?;
        if let Some(native) = row.wire.features(who) {
            m::check(
                native["native_handoff"] == p["native_handoff"]
                    && native["departure_game_over"] == p["departure_game_over"],
                "Room socket features differ from native admission",
            )?;
        }
        out.insert(
            who.to_owned(),
            Peer {
                handoff: m::boolean(&p["native_handoff"])?,
                departure: m::boolean(&p["departure_game_over"])?,
            },
        );
    }
    Ok(out)
}
impl Setup {
    pub(super) fn phase(&self) -> &'static str {
        match self.stage {
            Stage::Connecting => "connecting",
            Stage::Loading => "loading",
            Stage::Reviewing => "reviewing",
            Stage::Postgame => "postgame",
            Stage::Running => self
                .handoff
                .as_ref()
                .map(|h| h.phase())
                .unwrap_or("running"),
        }
    }
    pub(super) fn authority(&self) -> Option<usize> {
        if self.stage == Stage::Running {
            self.handoff.as_ref().map(|h| h.host()).or(self.host)
        } else {
            self.host
        }
    }
    fn state(&self) -> Value {
        json!({"phase":self.phase(),"host":self.authority(),"closed":self.life.closed,"created":self.life.created,
            "deadline":self.life.deadline,"departed_slots":self.life.departed,"result_recorded":self.life.recorded,
            "setup_host_departed":self.life.host_departed,"active":self.life.active})
    }
    pub(super) fn control(&self) -> Value {
        let mut v = json!({"phase":self.phase(),"host":self.authority()});
        if self.continuity {
            v["continuity"] = self
                .handoff
                .as_ref()
                .map(|h| h.control())
                .unwrap_or(Value::Null);
            if self.handoff.is_some() {
                v["host"] = v["continuity"]["host"].clone();
            }
        }
        v
    }
    fn voice(&self, peers: &BTreeMap<String, Peer>) -> Value {
        let mask = self
            .players
            .iter()
            .enumerate()
            .fold(0u32, |bits, (slot, p)| {
                bits | if (self.direct && self.life.active.contains(&p.owner))
                    || (!self.direct && peers.contains_key(&p.owner))
                {
                    1 << slot
                } else {
                    0
                }
            });
        json!({"voice_host":self.authority(),"voice_members":mask})
    }
    fn retire(&mut self, core: &mut Coordinator, id: &str, who: &str) -> Result<bool> {
        self.actor(who)?;
        if !self.life.active.contains(who) {
            return Ok(false);
        }
        call(core, json!({"op":"result_revoke","id":id,"actor":who}))?;
        self.life.active.remove(who);
        self.wire.retire(who);
        self.packets.retire(who);
        for (slot, p) in self.players.iter().enumerate() {
            if p.owner == who {
                self.life.departed |= 1 << slot;
            }
        }
        Ok(true)
    }
    fn cleanup_tickets(&self, core: &mut Coordinator) -> Result<()> {
        for id in &self.life.tickets {
            if core.tickets.iter().any(|ticket| ticket["id"] == *id) {
                call(core, json!({"op":"cancel","id":id}))?;
            }
        }
        Ok(())
    }
    fn review(&mut self, core: &mut Coordinator, id: &str) -> Result<Value> {
        let mut e = effect();
        if self.stage != Stage::Reviewing || self.life.closed {
            return Ok(e);
        }
        let decision = call(core, json!({"op":"result_review","id":id}))?;
        if decision["present"] != true {
            return Ok(e);
        }
        if decision["recordable"] == true && !core.results.is_committed(id) {
            e["commit"] = json!(true);
            return Ok(e);
        }
        self.cleanup_tickets(core)?;
        self.life.recorded = Some(decision["recordable"] == true);
        self.stage = Stage::Postgame;
        self.life.arm(900.);
        e["completed"] = json!(true);
        Ok(e)
    }
    fn sync(&mut self, core: &mut Coordinator, id: &str) -> Result<Value> {
        let mut e = effect();
        if self.life.closed || self.handoff.is_none() {
            return Ok(e);
        }
        let h = self.handoff.as_mut().unwrap();
        h.execute(&json!({"op":"expire","now":self.life.now}))?;
        let state = h.execute(&json!({"op":"state"}))?;
        let active = state["active"].as_array().unwrap();
        let retired = self
            .life
            .active
            .iter()
            .filter(|who| {
                !active
                    .iter()
                    .any(|slot| slot.as_u64() == Some(self.slot(who).unwrap() as u64))
            })
            .cloned()
            .collect::<Vec<_>>();
        for who in &retired {
            self.retire(core, id, who)?;
        }
        e["retire"] = json!(retired);
        if ["empty", "unavailable"].contains(&state["phase"].as_str().unwrap()) {
            e["close"] = json!("No surviving peer has the required complete world.");
        }
        Ok(e)
    }
}
pub(super) fn execute(row: &mut Setup, core: &mut Coordinator, r: &Value) -> Result<Value> {
    row.life.observe(r, core.clock)?;
    let id = m::text(&r["id"], 64)?;
    let op = m::text(&r["op"], 64)?;
    if op == "room_state_get" {
        return Ok(row.state());
    }
    if op == "room_state_control" {
        return Ok(row.control());
    }
    if op == "room_state_close" {
        row.cleanup_tickets(core)?;
        row.life.closed = true;
        row.wire.clear();
        row.packets.clear();
        return Ok(row.state());
    }
    if op == "room_state_expired" {
        return Ok(json!(
            !row.life.closed
                && (row.life.now > row.life.deadline
                    || (row.stage == Stage::Connecting && core.proposal(id).is_err()))
        ));
    }
    if op == "room_state_voice" {
        return Ok(row.voice(&peers(row, r)?));
    }
    if op == "room_state_member" {
        let who = m::text(&r["player"], 64)?;
        return Ok(json!(
            !row.life.closed
                && row.life.now <= row.life.deadline
                && row
                    .players
                    .iter()
                    .any(|p| p.id == who && row.life.active.contains(&p.owner))
        ));
    }
    if op == "room_state_rejection" {
        return Ok(json!(
            if row.life.closed || row.life.now > row.life.deadline {
                "room"
            } else if row.direct
                && matches!(
                    row.stage,
                    Stage::Running | Stage::Reviewing | Stage::Postgame
                )
            {
                "depart"
            } else if row.continuity {
                "peer"
            } else {
                "room"
            }
        ));
    }
    if ["room_state_socket_lost", "room_state_synchronize"].contains(&op)
        && row.life.now > row.life.deadline
    {
        return Ok(close(
            "Match connection or loading timed out. Search again.",
        ));
    }
    reject(!row.life.closed, 409, "Native room is closed")?;
    reject(
        row.life.now <= row.life.deadline,
        409,
        "Native room lease expired",
    )?;
    match op {
        "room_state_socket_lost" => {
            if row.direct
                && matches!(
                    row.stage,
                    Stage::Running | Stage::Reviewing | Stage::Postgame
                )
            {
                return Ok(effect());
            }
            if row.continuity {
                let mut request = r.clone();
                request["op"] = json!("room_state_depart");
                request["direct"] = json!(false);
                return execute(row, core, &request);
            }
            if row.stage != Stage::Postgame {
                return Ok(close("A player disconnected. Search again."));
            }
            let who = m::text(&r["player"], 64)?;
            row.actor(who)?;
            let mut connected = peers(row, r)?;
            connected.remove(who);
            if connected.is_empty() {
                return Ok(close("Match completed."));
            }
            if row
                .host
                .is_none_or(|host| !connected.contains_key(&row.players[host].owner))
            {
                row.host = connected.keys().map(|p| row.slot(p).unwrap()).min();
            }
            let mut e = effect();
            e["voice"] = json!(true);
            Ok(e)
        }
        "room_state_review" => row.review(core, id),
        "room_state_synchronize" => row.sync(core, id),
        "room_state_retire" => {
            let who = m::text(&r["player"], 64)?;
            let mut e = effect();
            if row.retire(core, id, who)? {
                e["retire"] = json!([who]);
            }
            Ok(e)
        }
        "room_state_submit" => {
            let who = m::text(&r["player"], 64)?;
            let slot = row.actor(who)?;
            reject(
                row.life.active.contains(who) && core.results.is_active(id, who),
                403,
                "Result publisher is not active",
            )?;
            let peers = peers(row, r)?;
            let native = peers.get(who).is_some_and(|p| p.handoff);
            reject(
                Some(slot) == row.authority() || (row.life.host_departed && native),
                403,
                "Only the native authority may publish results",
            )?;
            reject(
                row.phase() == "running"
                    || (row.direct && matches!(row.stage, Stage::Reviewing | Stage::Postgame)),
                400,
                "Invalid postgame report phase",
            )?;
            reject(
                row.direct || peers.len() == row.life.active.len(),
                400,
                "Invalid postgame report membership",
            )?;
            let digest = call(
                core,
                json!({"op":"result_submit","id":id,"actor":who,"report":r["report"]}),
            )?;
            let publish = if row.stage == Stage::Running {
                row.host = Some(slot);
                row.stage = Stage::Reviewing;
                row.life.arm(30.);
                "all"
            } else {
                "sender"
            };
            let mut e = effect();
            e["digest"] = digest;
            e["publish"] = json!(publish);
            e["report"] = r["report"].clone();
            Ok(e)
        }
        "room_state_confirm" => {
            let who = m::text(&r["player"], 64)?;
            row.actor(who)?;
            reject(
                matches!(row.stage, Stage::Reviewing | Stage::Postgame),
                400,
                "Invalid result confirmation phase",
            )?;
            call(
                core,
                json!({"op":"result_confirm","id":id,"actor":who,"digest":r["digest"]}),
            )?;
            let mut e = effect();
            e["review"] = json!(row.stage == Stage::Reviewing);
            e["reply_completed"] = json!(row.stage == Stage::Postgame && row.direct);
            Ok(e)
        }
        "room_state_depart" => {
            let who = m::text(&r["player"], 64)?;
            let slot = row.actor(who)?;
            let explicit = m::boolean(&r["direct"])?;
            reject(explicit == row.direct, 400, "Invalid room departure route")?;
            let mut connected = peers(row, r)?;
            let mut e = effect();
            if !row.life.active.contains(who) {
                return Ok(e);
            }
            if row.direct {
                if !matches!(
                    row.stage,
                    Stage::Running | Stage::Reviewing | Stage::Postgame
                ) {
                    call(
                        core,
                        json!({"op":"api_room_failure","id":id,"phase":row.phase(),"kind":"party_changed","player":who}),
                    )?;
                    return Ok(close("A player left before the match started."));
                }
                if row.stage == Stage::Running && row.authority() == Some(slot) {
                    if !connected.values().all(|p| p.handoff) {
                        let remaining = row
                            .players
                            .iter()
                            .filter(|p| p.owner != who && row.life.active.contains(&p.owner))
                            .collect::<Vec<_>>();
                        let last = if row.teams {
                            remaining
                                .iter()
                                .map(|p| p.team)
                                .collect::<BTreeSet<_>>()
                                .len()
                                == 1
                        } else {
                            remaining.len() == 1
                        };
                        let ready = row
                            .life
                            .active
                            .iter()
                            .filter(|p| p.as_str() != who)
                            .all(|p| connected.get(p).is_some_and(|p| p.departure));
                        if !last || !ready {
                            return Ok(close(
                                "The host left before a surviving authority was ready.",
                            ));
                        }
                    } else {
                        row.life.host_departed = true;
                    }
                }
            } else if !row.continuity {
                return Ok(close("A player disconnected. Search again."));
            }
            row.retire(core, id, who)?;
            e["retire"] = json!([who]);
            connected.remove(who);
            if (row.direct && row.life.active.is_empty()) || (!row.direct && connected.is_empty()) {
                e["close"] = json!(if row.direct {
                    "Match completed."
                } else {
                    "No players remain."
                });
                return Ok(e);
            }
            if row.stage == Stage::Running {
                if row.direct {
                    e["notice"] = json!({"departed_players":row.life.departed});
                } else {
                    row.handoff
                        .as_mut()
                        .ok_or(Error(409, "Running continuity owner missing"))?
                        .execute(&json!({"op":"disconnect","peer":slot,"now":row.life.now}))?;
                    let sync = row.sync(core, id)?;
                    e["close"] = sync["close"].clone();
                    e["retire"]
                        .as_array_mut()
                        .unwrap()
                        .extend(sync["retire"].as_array().unwrap().iter().cloned());
                    e["control"] = json!(true);
                }
                return Ok(e);
            }
            if matches!(row.stage, Stage::Reviewing | Stage::Postgame) {
                if row.host == Some(slot) {
                    row.host = row
                        .life
                        .active
                        .iter()
                        .filter(|p| row.direct || connected.contains_key(*p))
                        .map(|p| row.slot(p).unwrap())
                        .min();
                }
                e["review"] = json!(row.stage == Stage::Reviewing);
                e["voice"] = json!(true);
            } else {
                e["close"] = json!("A player left before the match started.");
            }
            Ok(e)
        }
        _ => Err(Error(400, "Unknown native room lifecycle operation")),
    }
}
