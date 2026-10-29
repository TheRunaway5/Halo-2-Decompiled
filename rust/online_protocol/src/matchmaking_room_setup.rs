//! Native room setup/readiness/QoS producers over immutable admitted seats.
//! Original connected-team and start timer kernels are separate from explicitly
//! modern relay/direct scoring, upload ranking, and loaded-data validation.
use super::{Coordinator, m, reject};
use crate::matchmaking_continuity::Continuity;
use crate::matchmaking_model::{Error, Result, array, s};
use crate::matchmaking_readiness::{self as readiness, Seat};
use crate::matchmaking_start::Timer;
use serde_json::{Value, json};
use std::collections::{BTreeMap, BTreeSet, HashMap};
#[path = "matchmaking_room_control.rs"]
mod control;
#[path = "matchmaking_room_lifecycle.rs"]
mod lifecycle;
#[path = "matchmaking_room_packets.rs"]
mod packets;
#[path = "matchmaking_room_wire.rs"]
mod wire;

fn call(core: &mut Coordinator, mut r: Value) -> Result<Value> {
    r["now"] = json!(core.clock);
    core.execute(&r)
}
fn milliseconds(r: &Value) -> Result<u32> {
    Ok(m::integer(&r["clock_ms"], i64::MIN, i64::MAX)? as u32)
}
fn physical(p: &Value) -> &str {
    p["connection_owner"].as_str().unwrap_or(s(p, "id"))
}
#[derive(Clone)]
struct Player {
    id: String,
    owner: String,
    team: i32,
    name: String,
    guest: bool,
}
#[derive(Clone, Copy, PartialEq)]
enum Stage {
    Connecting,
    Loading,
    Running,
    Reviewing,
    Postgame,
}
struct Setup {
    life: lifecycle::Life,
    wire: wire::Wire,
    packets: packets::Packets,
    players: Vec<Player>,
    owners: Vec<String>,
    direct: bool,
    continuity: bool,
    teams: bool,
    samples: BTreeMap<String, Vec<f64>>,
    quality: BTreeMap<String, BTreeMap<usize, Value>>,
    loaded: BTreeMap<String, String>,
    host: Option<usize>,
    timer: Timer,
    active_clock: bool,
    stage: Stage,
    handoff: Option<Continuity>,
}
impl Setup {
    fn slot(&self, who: &str) -> Result<usize> {
        self.players
            .iter()
            .position(|p| p.id == who)
            .ok_or(Error(403, "Room peer is not admitted"))
    }
    fn actor(&self, who: &str) -> Result<usize> {
        reject(
            self.owners.iter().any(|owner| owner == who),
            403,
            "Physical room connection required",
        )?;
        self.slot(who)
    }
    fn snapshot(&self) -> Value {
        json!({"samples":self.samples,"direct_ready":self.quality,"loaded":self.loaded,
        "clock":if self.active_clock{json!(self.timer.clock)}else{Value::Null},"host":self.host})
    }
    fn peers<'a>(&self, r: &'a Value) -> Result<(Vec<&'a str>, Vec<&'a str>)> {
        let mut lists = Vec::new();
        for key in ["active", "connected"] {
            let list = r[key]
                .as_array()
                .ok_or(Error(400, "Room connection observations required"))?;
            m::check(list.len() <= 16, "Too many room connections")?;
            let mut seen = BTreeSet::new();
            let mut out = Vec::new();
            for who in list {
                let who = m::text(who, 64)?;
                m::check(seen.insert(who), "Duplicate room connection")?;
                self.actor(who)?;
                out.push(who);
            }
            lists.push(out);
        }
        Ok((lists.remove(0), lists.remove(0)))
    }
    fn ready(&self, r: &Value) -> Result<bool> {
        let (active, connected) = self.peers(r)?;
        if active.len() != connected.len()
            || active.iter().any(|owner| !connected.contains(owner))
            || self
                .players
                .iter()
                .any(|p| !active.contains(&p.owner.as_str()))
        {
            return Ok(false);
        }
        let Some(host) = self.host.and_then(|i| self.players.get(i)) else {
            return Ok(false);
        };
        let Some(current) = active.iter().position(|owner| *owner == host.owner) else {
            return Ok(false);
        };
        let masks = active
            .iter()
            .map(|owner| {
                active
                    .iter()
                    .enumerate()
                    .filter(|(_, other)| {
                        !self.direct
                            || owner == *other
                            || self
                                .quality
                                .get(*owner)
                                .and_then(|rows| rows.get(&self.slot(other).unwrap()))
                                .is_some_and(|row| !row["rtt_ms"].is_null())
                    })
                    .fold(0u32, |mask, (index, _)| mask | (1 << index))
            })
            .collect::<Vec<_>>();
        let seats = self
            .players
            .iter()
            .map(|p| Seat {
                occupied: true,
                peer: active.iter().position(|owner| *owner == p.owner).unwrap() as u32,
                team: p.team,
            })
            .collect::<Vec<_>>();
        Ok(readiness::eligible(
            self.teams,
            readiness::connected_teams(&masks, current, &seats),
            false,
            false,
            false,
            false,
            false,
        ))
    }
    fn tick(&mut self, r: &Value) -> Result<Value> {
        let ready = self.ready(r)?;
        if self.life.closed || self.stage != Stage::Loading || !ready {
            self.timer.clear();
            self.active_clock = false;
            return Ok(Value::Null);
        }
        self.active_clock = true;
        Ok(json!(self.timer.normal(milliseconds(r)?).remaining))
    }
    fn measured(&self) -> Result<Value> {
        if !self.quality.values().any(|rows| {
            rows.values()
                .any(|row| row.get("upload_bytes_per_second").is_some())
        }) {
            return Ok(Value::Null);
        }
        let mut ranked = BTreeMap::new();
        for owner in &self.owners {
            let mut loss = 0i64;
            let mut upload = 64 * 1024 * 1024i64;
            let mut rtt = 0i64;
            let mut paths = 0;
            let mut complete = true;
            for other in &self.owners {
                if owner == other {
                    continue;
                }
                let a = self
                    .quality
                    .get(owner)
                    .and_then(|rows| rows.get(&self.slot(other).unwrap()));
                let b = self
                    .quality
                    .get(other)
                    .and_then(|rows| rows.get(&self.slot(owner).unwrap()));
                let Some((a, b)) = a.zip(b) else {
                    complete = false;
                    break;
                };
                if [a, b].iter().any(|row| {
                    row["rtt_ms"].is_null()
                        || row["upload_bytes_per_second"].is_null()
                        || row["loss_per_mille"].is_null()
                }) {
                    complete = false;
                    break;
                }
                loss = loss.max(
                    a["loss_per_mille"]
                        .as_i64()
                        .unwrap()
                        .max(b["loss_per_mille"].as_i64().unwrap()),
                );
                upload = upload.min(a["upload_bytes_per_second"].as_i64().unwrap());
                rtt += a["rtt_ms"]
                    .as_i64()
                    .unwrap()
                    .max(b["rtt_ms"].as_i64().unwrap());
                paths += 1;
            }
            if complete {
                ranked.insert(
                    owner.clone(),
                    json!([loss, -upload, rtt / paths.max(1), self.slot(owner)?]),
                );
            }
        }
        Ok(json!(ranked))
    }
    fn direct_report(&mut self, r: &Value) -> Result<Value> {
        let who = m::text(&r["player"], 64)?;
        self.actor(who)?;
        reject(
            self.direct && self.stage == Stage::Connecting && !self.quality.contains_key(who),
            400,
            "Invalid direct connection admission",
        )?;
        let rows = r["rows"]
            .as_array()
            .ok_or(Error(400, "Direct peer reports required"))?;
        let expected = self
            .owners
            .iter()
            .filter(|owner| *owner != who)
            .map(|owner| self.slot(owner).unwrap())
            .collect::<BTreeSet<_>>();
        m::check(
            rows.len() == expected.len(),
            "Missing direct peer connections",
        )?;
        let mut reports = BTreeMap::new();
        for row in rows {
            m::fields_optional(
                row,
                &["peer", "rtt_ms"],
                &["upload_bytes_per_second", "loss_per_mille"],
            )?;
            let peer = m::integer(&row["peer"], 0, 15)? as usize;
            m::check(
                expected.contains(&peer) && !reports.contains_key(&peer),
                "Invalid direct peer quality",
            )?;
            if !row["rtt_ms"].is_null() {
                m::integer(&row["rtt_ms"], 0, 60000)?;
            }
            if row.get("upload_bytes_per_second").is_some() {
                m::check(
                    row.get("loss_per_mille").is_some(),
                    "Incomplete direct upload measurement",
                )?;
                if row["rtt_ms"].is_null() {
                    m::check(
                        row["upload_bytes_per_second"].is_null() && row["loss_per_mille"].is_null(),
                        "Invalid direct upload measurement",
                    )?;
                } else {
                    m::integer(&row["upload_bytes_per_second"], 1, 64 * 1024 * 1024)?;
                    m::integer(&row["loss_per_mille"], 0, 1000)?;
                }
            } else {
                m::check(
                    row.get("loss_per_mille").is_none(),
                    "Incomplete direct upload measurement",
                )?;
            }
            reports.insert(peer, row.clone());
        }
        self.quality.insert(who.to_owned(), reports);
        Ok(Value::Null)
    }
    fn load(&mut self, r: &Value) -> Result<Value> {
        let who = m::text(&r["player"], 64)?;
        self.actor(who)?;
        reject(
            self.stage == Stage::Loading,
            400,
            "Invalid game-data admission",
        )?;
        let signature = m::text(&r["signature"], 24)?;
        m::check(
            signature.len() == 24
                && signature
                    .bytes()
                    .all(|b| b.is_ascii_hexdigit() && !b.is_ascii_uppercase() || b == b':'),
            "Invalid game-data admission",
        )?;
        {
            let (hash, protocol) = signature.split_at(16);
            m::check(
                hash.bytes().all(|b| b.is_ascii_hexdigit())
                    && protocol.starts_with(':')
                    && protocol[1..].bytes().all(|b| b.is_ascii_hexdigit())
                    && u32::from_str_radix(&protocol[1..], 16).is_ok_and(|n| n <= 0xffff),
                "Invalid native continuation protocol admission",
            )?;
        }
        self.loaded.insert(who.to_owned(), signature.to_owned());
        Ok(Value::Null)
    }
}
fn median(values: &[f64]) -> f64 {
    let mut values = values.to_vec();
    values.sort_by(f64::total_cmp);
    values[values.len() / 2]
}
#[derive(Default)]
pub(super) struct Registry {
    rows: HashMap<String, Setup>,
}
impl Registry {
    pub(super) fn authority_epoch(&self, id: &str, required: bool) -> Result<u32> {
        match self.rows.get(id).and_then(|row| row.handoff.as_ref()) {
            Some(owner) => Ok(owner.epoch()),
            None if !required => Ok(0),
            None => Err(Error(409, "Native continuity source is not running")),
        }
    }
    pub(super) fn open(&mut self, core: &Coordinator, id: &str) -> Result<Value> {
        if self.rows.contains_key(id) {
            return Ok(Value::Null);
        }
        reject(
            self.rows.len() < 64,
            503,
            "Native room setup capacity reached",
        )?;
        let proposal = core.proposal(id)?;
        let group = core.group(&super::strings(&proposal["tickets"])?)?;
        let admitted = Coordinator::players(&group);
        let mut players = Vec::new();
        let mut owners = Vec::new();
        for (team, ids) in array(proposal, "teams").iter().enumerate() {
            for id in ids.as_array().unwrap() {
                let p = admitted
                    .iter()
                    .find(|p| p["id"] == *id)
                    .ok_or(Error(409, "Room roster differs from admission"))?;
                let owner = physical(p).to_owned();
                if !owners.contains(&owner) {
                    owners.push(owner.clone());
                }
                players.push(Player {
                    id: s(p, "id").to_owned(),
                    owner,
                    team: team as i32,
                    name: s(p, "screen_name").to_owned(),
                    guest: m::flag(p, "guest"),
                });
            }
        }
        let compatibility = s(&group[0], "compatibility");
        let direct = crate::matchmaking_catalog::OPTIONS.contains(&compatibility)
            || [
                "halo2-native-queue-v12",
                "halo2-native-queue-v13",
                "halo2-native-queue-v14",
            ]
            .contains(&compatibility);
        let state = Setup {
            wire: wire::Wire::new(&owners, compatibility)?,
            packets: packets::Packets::new(&owners)?,
            life: lifecycle::Life::new(
                &owners,
                compatibility,
                array(proposal, "tickets")
                    .iter()
                    .map(|id| id.as_str().unwrap().to_owned())
                    .collect(),
            ),
            players,
            owners,
            direct,
            continuity: compatibility == "halo2-native-queue-v11",
            teams: m::n(core.policy(s(proposal, "playlist"))?, "team_size") != 0,
            samples: BTreeMap::new(),
            quality: BTreeMap::new(),
            loaded: BTreeMap::new(),
            host: None,
            timer: Timer::default(),
            active_clock: false,
            stage: Stage::Connecting,
            handoff: None,
        };
        self.rows.insert(id.to_owned(), state);
        Ok(Value::Null)
    }
    pub(super) fn execute(&mut self, core: &mut Coordinator, r: &Value) -> Result<Value> {
        let id = m::text(&r["id"], 64)?;
        if r["op"] == "room_setup_open" {
            return self.open(core, id);
        }
        if r["op"] == "room_setup_close" {
            self.rows.remove(id);
            return Ok(Value::Null);
        }
        if r["op"] == "room_setup_clock" {
            return Ok(self
                .rows
                .get(id)
                .filter(|row| row.active_clock)
                .map(|row| json!(row.timer.clock))
                .unwrap_or(Value::Null));
        }
        let row = self
            .rows
            .get_mut(id)
            .ok_or(Error(409, "Native room setup retired"))?;
        dispatch(row, core, r)
    }
}

fn dispatch(row: &mut Setup, core: &mut Coordinator, r: &Value) -> Result<Value> {
    let id = m::text(&r["id"], 64)?;
    if r["op"]
        .as_str()
        .is_some_and(|op| op.starts_with("room_control_"))
    {
        return control::execute(row, core, r);
    }
    if r["op"]
        .as_str()
        .is_some_and(|op| op.starts_with("room_packets_"))
    {
        return packets::execute(row, core, r);
    }
    if r["op"]
        .as_str()
        .is_some_and(|op| op.starts_with("room_wire_"))
    {
        return wire::execute(row, core, r);
    }
    if r["op"]
        .as_str()
        .is_some_and(|op| op.starts_with("room_state_"))
    {
        return lifecycle::execute(row, core, r);
    }
    row.life.observe(r, core.clock)?;
    if let Some(op) = r["op"]
        .as_str()
        .and_then(|op| op.strip_prefix("room_continuity_"))
    {
        if op == "message" && !Continuity::recognizes(&r["data"])? {
            return Ok(json!({"handled":false,"broadcast":null,"changed":false}));
        }
        let actor = if op == "message" {
            Some(row.actor(m::text(&r["player"], 64)?)?)
        } else {
            None
        };
        let handoff = row
            .handoff
            .as_mut()
            .ok_or(Error(400, "Complete-world continuation is unavailable"))?;
        if let Some(actor) = actor {
            return handoff.message(actor, &r["data"], core.results.has_report(id), row.life.now);
        }
        let mut request = r.clone();
        request["op"] = json!(op);
        return handoff.execute(&request);
    }
    match m::text(&r["op"], 64)? {
        "room_setup_snapshot" => Ok(row.snapshot()),
        "room_setup_ready" => Ok(json!(row.ready(r)?)),
        "room_setup_start" => row.tick(r),
        "room_setup_measured_hosts" => row.measured(),
        "room_setup_direct_report" => row.direct_report(r),
        "room_setup_loaded" => row.load(r),
        "room_setup_sample" => {
            let who = m::text(&r["player"], 64)?;
            row.actor(who)?;
            let ms = m::real(&r["milliseconds"])?;
            m::check(
                (0.0..=60000.).contains(&ms),
                "Invalid observed room round trip",
            )?;
            let samples = row.samples.entry(who.to_owned()).or_default();
            samples.push(ms);
            if samples.len() > 3 {
                samples.remove(0);
            }
            Ok(Value::Null)
        }
        "room_setup_advance" => {
            let noop = || json!({"transition":null,"close":null});
            let (active, connected) = row.peers(r)?;
            if row.life.closed {
                return Ok(noop());
            }
            if row.stage == Stage::Connecting {
                if core.proposal(id).is_err() {
                    return Ok(
                        json!({"transition":null,"close":"Match connection or loading timed out. Search again."}),
                    );
                }
                if !row
                    .owners
                    .iter()
                    .all(|p| row.samples.get(p).is_some_and(|v| v.len() >= 3))
                    || (row.direct && row.quality.len() != row.owners.len())
                {
                    return Ok(noop());
                }
                for a in &row.players {
                    for b in &row.players {
                        if a.id >= b.id {
                            continue;
                        }
                        let ms = if a.owner == b.owner {
                            Some(0.)
                        } else if row.direct {
                            let ab = row.quality[&a.owner].get(&row.slot(&b.owner)?).unwrap();
                            let ba = row.quality[&b.owner].get(&row.slot(&a.owner)?).unwrap();
                            ab["rtt_ms"]
                                .as_i64()
                                .zip(ba["rtt_ms"].as_i64())
                                .map(|(a, b)| a.max(b) as f64)
                        } else {
                            Some(median(&row.samples[&a.owner]) + median(&row.samples[&b.owner]))
                        };
                        if let Some(ms) = ms {
                            let score = (100 - (ms / 5.) as i64).max(0);
                            call(
                                core,
                                json!({"op":"connection","host":a.id,"player":b.id,"score":score}),
                            )?;
                            call(
                                core,
                                json!({"op":"connection","host":b.id,"player":a.id,"score":score}),
                            )?;
                        } else {
                            call(core, json!({"op":"connection_delete","key":[a.id,b.id]}))?;
                            call(core, json!({"op":"connection_delete","key":[b.id,a.id]}))?;
                        }
                    }
                }
                let quality = if row.direct {
                    row.measured()?
                } else {
                    Value::Null
                };
                let proposal = call(
                    core,
                    json!({"op":"refresh","id":id,"optimize":row.direct,"quality":quality}),
                )?;
                let Some(host) = proposal["host"].as_str() else {
                    return Ok(
                        json!({"transition":null,"close":"Connection quality is insufficient for this playlist."}),
                    );
                };
                let physical = &row
                    .players
                    .iter()
                    .find(|p| p.id == host)
                    .ok_or(Error(409, "Selected host left the admitted room"))?
                    .owner;
                row.host = Some(row.slot(physical)?);
                if !row.ready(r)? {
                    row.timer.clear();
                    row.active_clock = false;
                    return Ok(noop());
                }
                for p in &row.players {
                    call(
                        core,
                        json!({"op":"acknowledge","id":id,"player":p.id,"optimize":row.direct,"quality":quality}),
                    )?;
                }
                row.stage = Stage::Loading;
                row.life.arm(180.);
                row.timer.clear();
                row.timer.normal(milliseconds(r)?);
                row.active_clock = true;
                return Ok(
                    json!({"transition":"loading","close":null,"host":row.host,"timeout_seconds":180,"continuity":null}),
                );
            }
            if row.stage == Stage::Loading {
                let remaining = row.tick(r)?;
                if row.loaded.len() != row.owners.len() {
                    return Ok(noop());
                }
                if row.loaded.values().collect::<BTreeSet<_>>().len() != 1 {
                    return Ok(
                        json!({"transition":null,"close":"Map, game data, or native protocol differs between players."}),
                    );
                }
                if remaining != 0 {
                    return Ok(noop());
                }
                let continuity = if row.continuity {
                    let mut owner = Continuity::new(
                        &json!({"owners":row.players.iter().map(|p|row.slot(&p.owner).unwrap()).collect::<Vec<_>>(),
                        "host":row.host,"timeout":15}),
                    )?;
                    for (who, values) in &row.samples {
                        owner.execute(&json!({"op":"quality","peer":row.slot(who)?,"milliseconds":median(values),"now":row.life.now}))?;
                    }
                    row.handoff = Some(owner);
                    json!(true)
                } else {
                    Value::Null
                };
                row.stage = Stage::Running;
                row.life.arm(7200.);
                row.timer.clear();
                row.active_clock = false;
                return Ok(
                    json!({"transition":"running","close":null,"host":row.host,"timeout_seconds":7200,"continuity":continuity}),
                );
            }
            let _ = (active, connected);
            Ok(noop())
        }
        _ => Err(Error(400, "Unknown native room setup operation")),
    }
}
