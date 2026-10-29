//! Owned pregame countdown publication over original press/tick/writer kernels.
//! Directory/room/clock observations are trusted IO; transport authorization is
//! explicitly native service policy, not an original Live procedure claim.
use super::{Coordinator, fingerprint, id, m, party, playing, rooms, roster_pin, select};
use crate::matchmaking_model::{Error, Result, array, s};
use crate::party_countdown::{self as original, Parameter};
use serde_json::{Value, json};
use std::collections::{BTreeMap, HashMap};
fn require(ok: bool, status: u16, message: &'static str) -> Result<()> {
    super::reject(ok, status, message)
}
fn clock(r: &Value) -> Result<u32> {
    Ok(m::integer(&r["clock_ms"], i64::MIN, i64::MAX)? as u32)
}
fn statement(sql: &str, params: Value, changes: i64) -> Value {
    json!({"sql":sql,"params":params,"changes":changes})
}
#[derive(Clone)]
struct Row {
    serial: u64,
    epoch: u64,
    clock: u32,
    playlist: String,
    revision: String,
    party_version: i64,
    profile: String,
    roster: String,
    members: Value,
    parameter: Parameter<Option<String>>,
    complete: bool,
    consumed: bool,
}
impl Row {
    fn snapshot(&self) -> Value {
        json!({"serial":self.serial,"seconds":if self.consumed{Value::Null}else{json!(self.parameter.seconds)},
        "clock_ms":self.clock,"playlist":self.playlist,"revision":self.revision,"party_version":self.party_version,
        "mode":self.parameter.mode,"complete":self.complete,"requester":self.parameter.requester})
    }
    fn public(&self, p: &Value) -> Value {
        let requester = self
            .parameter
            .requester
            .as_ref()
            .and_then(|who| array(p, "members").iter().find(|m| m["player"] == *who))
            .map(|m| m["handle"].clone());
        json!({"serial":self.serial,"seconds":if self.consumed{Value::Null}else{json!(self.parameter.seconds)},"mode":self.parameter.mode,"complete":self.complete,"requester":requester})
    }
    fn valid(&self, core: &Coordinator) -> bool {
        core.policy_pins.get(&self.playlist) == Some(&self.profile)
            && self.members.as_array().unwrap().iter().all(|m| {
                core.sessions
                    .checked(&m["generation"], core.clock)
                    .is_ok_and(|session| session["key"] == m["session_key"])
            })
    }
}
#[derive(Clone)]
enum Change {
    Keep,
    Drop,
    Publish(Row),
}
#[derive(Clone)]
struct Pending {
    party: String,
    before: Option<u64>,
    actor: String,
    body: Value,
    facts: String,
    expires: f64,
    change: Change,
}
#[derive(Default)]
pub(super) struct Registry {
    serial: u64,
    epoch: u64,
    rows: BTreeMap<String, Row>,
    pending: HashMap<String, Pending>,
}
impl Registry {
    pub(super) fn invalidate(&mut self, ids: &[Value]) {
        self.rows.retain(|id, _| !ids.contains(&json!(id)));
        self.pending.retain(|_, p| !ids.contains(&json!(p.party)));
    }
    pub(super) fn retire_generation(&mut self, generation: u64) {
        let ids = self
            .rows
            .iter()
            .filter(|(_, r)| {
                r.members
                    .as_array()
                    .unwrap()
                    .iter()
                    .any(|m| m["generation"] == generation)
            })
            .map(|(id, _)| json!(id))
            .collect::<Vec<_>>();
        self.invalidate(&ids);
    }
    pub(super) fn cleanup(&mut self, pins: &HashMap<String, String>) {
        self.rows
            .retain(|_, r| pins.get(&r.playlist) == Some(&r.profile));
    }
    fn reconcile(&mut self, core: &Coordinator) {
        self.rows.retain(|_, r| r.valid(core));
        self.pending.retain(|_, p| p.expires > core.clock);
    }
    fn bump(&mut self) -> Result<u64> {
        self.epoch = self
            .epoch
            .checked_add(1)
            .ok_or(Error(503, "Countdown publication generation exhausted"))?;
        Ok(self.epoch)
    }
    pub(super) fn consume(&mut self, pid: &str) -> Result<()> {
        if self
            .rows
            .get(pid)
            .is_some_and(|r| r.complete && !r.consumed)
        {
            let epoch = self.bump()?;
            let row = self.rows.get_mut(pid).unwrap();
            row.parameter.publish(true, 0, 0, 0, &None);
            row.consumed = true;
            row.epoch = epoch;
        }
        Ok(())
    }
    fn context(&mut self, core: &Coordinator, who: &str, facts: &Value) -> Result<Option<Value>> {
        let p = party(facts)?;
        let Some(p) = p else {
            return Ok(None);
        };
        require(
            array(p, "members").iter().any(|m| m["player"] == who),
            403,
            "Account is not in the accepted party",
        )?;
        let pid = s(p, "id");
        if self.rows.get(pid).is_some_and(|row| {
            row.party_version != m::n(p, "version")
                || roster_pin(core, who, &row.playlist, facts).map_or(true, |pin| pin != row.roster)
        }) {
            self.invalidate(&[json!(pid)]);
        }
        Ok(Some(p.clone()))
    }
    fn prepare(&mut self, core: &Coordinator, r: &Value) -> Result<Value> {
        let body = &r["body"];
        let action = m::text(&body["action"], 64)?;
        match action {
            "countdown_start" => m::fields(
                body,
                &["action", "request_id", "version", "playlist_id", "revision"],
            )?,
            "countdown_cancel" | "countdown_request" => {
                m::fields(body, &["action", "request_id", "version"])?
            }
            _ => return Err(Error(400, "Invalid countdown command")),
        }
        let rid = super::super::sessions::uuid(&body["request_id"])?;
        let who = m::text(&r["player"], 64)?;
        m::fields(&r["requests"], &["payload", "count"])?;
        let count = m::integer(&r["requests"]["count"], 0, i64::MAX)?;
        if let Some(previous) = r["requests"]["payload"].as_str() {
            require(
                crate::matchmaking_coordinator::decode(previous.as_bytes())? == *body,
                409,
                "Party request ID already used differently",
            )?;
            return Ok(json!({"token":null,"statements":[],"replayed":true}));
        }
        m::check(
            r["requests"]["payload"].is_null(),
            "Invalid countdown request record",
        )?;
        require(count < 256, 429, "Party command limit reached")?;
        let p = self
            .context(core, who, &r["facts"])?
            .ok_or(Error(409, "Create or join a party first"))?;
        require(
            m::integer(&body["version"], 1, i64::MAX)? == m::n(&p, "version"),
            409,
            "Party changed; refresh it",
        )?;
        let observed = rooms(&r["rooms"])?;
        require(
            !array(&p, "members")
                .iter()
                .any(|m| playing(observed, s(m, "player"))),
            409,
            "Leave the current match first",
        )?;
        let leader = p["leader"] == who;
        let pid = s(&p, "id");
        let before = self.rows.get(pid).map(|r| r.epoch);
        let ms = clock(r)?;
        let change = match action {
            "countdown_start" => {
                require(leader, 403, "Only the party leader can do that")?;
                let playlist = m::text(&body["playlist_id"], 64)?;
                let revision = m::text(&body["revision"], 64)?;
                let policy = core
                    .policy(playlist)
                    .map_err(|_| Error(409, "Playlist changed; refresh it"))?;
                require(
                    policy["revision"] == revision,
                    409,
                    "Playlist changed; refresh it",
                )?;
                require(
                    self.rows
                        .get(pid)
                        .is_none_or(|r| r.playlist == playlist && r.revision == revision),
                    409,
                    "Cancel the current countdown before changing its playlist",
                )?;
                let selected = select(core, who, playlist, &r["facts"], &r["rooms"])?;
                let old = self.rows.get(pid);
                let seconds = original::press(
                    old.filter(|r| !r.consumed)
                        .map(|r| r.parameter.seconds)
                        .unwrap_or(-1),
                );
                let mut row = if let Some(old) = old {
                    old.clone()
                } else {
                    Row {
                        serial: 0,
                        epoch: 0,
                        clock: ms,
                        playlist: playlist.to_owned(),
                        revision: revision.to_owned(),
                        party_version: m::n(&p, "version"),
                        profile: core.policy_pins[playlist].clone(),
                        roster: roster_pin(core, who, playlist, &r["facts"])?,
                        members: selected["binding"]["members"].clone(),
                        parameter: Parameter::default(),
                        complete: false,
                        consumed: false,
                    }
                };
                let changed = row.parameter.publish(true, 1, seconds, 0, &None);
                if changed || row.consumed {
                    row.clock = ms;
                    row.complete = seconds == 0;
                    row.consumed = false;
                    Change::Publish(row)
                } else {
                    Change::Keep
                }
            }
            "countdown_cancel" => {
                require(leader, 403, "Only the party leader can do that")?;
                Change::Drop
            }
            "countdown_request" => {
                require(!leader, 409, "The leader cancels its own countdown")?;
                let mut row = self
                    .rows
                    .get(pid)
                    .filter(|r| !r.consumed && !r.complete && r.parameter.active != 0)
                    .cloned()
                    .ok_or(Error(409, "No active countdown"))?;
                if row
                    .parameter
                    .publish(false, 1, 10, 1, &Some(who.to_owned()))
                {
                    row.clock = ms;
                    row.complete = false;
                    Change::Publish(row)
                } else {
                    Change::Keep
                }
            }
            _ => unreachable!(),
        };
        require(
            self.pending.len() < 1000,
            503,
            "Countdown command capacity reached",
        )?;
        require(
            self.rows.contains_key(pid) || self.rows.len() < 1000,
            503,
            "Countdown capacity reached",
        )?;
        let expires = m::integer(&r["wall_seconds"], 0, 9007199254654591)? + 86400;
        let member = array(&p, "members")
            .iter()
            .find(|m| m["player"] == who)
            .unwrap();
        let token = id()?;
        let statements = vec![
            statement(
                "UPDATE parties SET version=version WHERE id=? AND version=? AND leader=?",
                json!([pid, p["version"], p["leader"]]),
                1,
            ),
            statement(
                "UPDATE party_members SET handle=handle WHERE party=? AND player=? AND handle=?",
                json!([pid, who, member["handle"]]),
                1,
            ),
            statement(
                "INSERT INTO party_requests(player,request,payload,expires) SELECT ?,?,?,? WHERE (SELECT count(*) FROM party_requests WHERE player=?)<256 AND NOT EXISTS(SELECT 1 FROM party_requests WHERE player=? AND request=?)",
                json!([who, rid, body.to_string(), expires, who, who, rid]),
                1,
            ),
        ];
        self.pending.insert(
            token.clone(),
            Pending {
                party: pid.to_owned(),
                before,
                actor: who.to_owned(),
                body: body.clone(),
                facts: fingerprint(&r["facts"]),
                expires: core.clock + core.proposal_lease as f64,
                change,
            },
        );
        Ok(json!({"token":token,"statements":statements,"replayed":false}))
    }
    fn commit(&mut self, core: &Coordinator, r: &Value) -> Result<Value> {
        let token = m::text(&r["token"], 64)?;
        let pending = self
            .pending
            .remove(token)
            .ok_or(Error(409, "Countdown command retired"))?;
        require(
            pending.expires > core.clock && pending.facts == fingerprint(&r["facts"]),
            409,
            "Countdown context changed",
        )?;
        let p = self
            .context(core, &pending.actor, &r["facts"])?
            .ok_or(Error(409, "Party changed; refresh it"))?;
        require(
            p["id"] == pending.party
                && p["version"] == pending.body["version"]
                && self.rows.get(&pending.party).map(|r| r.epoch) == pending.before,
            409,
            "Countdown publication changed",
        )?;
        let observed = rooms(&r["rooms"])?;
        require(
            !array(&p, "members")
                .iter()
                .any(|m| playing(observed, s(m, "player"))),
            409,
            "Leave the current match first",
        )?;
        if let Change::Publish(row) = &pending.change {
            require(
                row.valid(core)
                    && roster_pin(core, &pending.actor, &row.playlist, &r["facts"])? == row.roster,
                409,
                "Countdown profile or connection changed",
            )?;
        }
        match pending.change {
            Change::Keep => {}
            Change::Drop => {
                self.invalidate(&[json!(pending.party)]);
            }
            Change::Publish(mut row) => {
                self.serial = self
                    .serial
                    .checked_add(1)
                    .ok_or(Error(503, "Countdown serial exhausted"))?;
                row.serial = self.serial;
                row.epoch = self.bump()?;
                self.rows.insert(pending.party, row);
            }
        }
        Ok(json!(
            self.rows.values().map(Row::snapshot).collect::<Vec<_>>()
        ))
    }
    pub(super) fn execute(&mut self, core: &Coordinator, r: &Value) -> Result<Value> {
        self.reconcile(core);
        match m::text(&r["op"], 64)? {
            "party_countdown_prepare" => self.prepare(core, r),
            "party_countdown_commit" => self.commit(core, r),
            "party_countdown_abort" => {
                self.pending.remove(m::text(&r["token"], 64)?);
                Ok(Value::Null)
            }
            "party_countdown_keys" => Ok(json!(self.rows.keys().collect::<Vec<_>>())),
            "party_countdown_get" => Ok(self
                .rows
                .get(m::text(&r["party"], 64)?)
                .map(Row::snapshot)
                .unwrap_or(Value::Null)),
            "party_countdown_view" => {
                let who = m::text(&r["player"], 64)?;
                let p = self.context(core, who, &r["facts"])?;
                Ok(
                    json!({"version":p.as_ref().map(|p|p["version"].clone()),"countdown":p.as_ref().and_then(|p|self.rows.get(s(p,"id")).map(|row|row.public(p)))}),
                )
            }
            "party_countdown_tick" => {
                let ms = clock(r)?;
                let versions = r["versions"]
                    .as_array()
                    .ok_or(Error(400, "Party versions required"))?;
                m::check(versions.len() <= 1000, "Party version capacity exceeded")?;
                let mut current = HashMap::new();
                for row in versions {
                    m::fields(row, &["id", "version"])?;
                    let pid = m::text(&row["id"], 64)?;
                    m::integer(&row["version"], 1, i64::MAX)?;
                    m::check(
                        current.insert(pid, m::n(row, "version")).is_none(),
                        "Duplicate party version",
                    )?;
                }
                self.rows
                    .retain(|id, row| current.get(id.as_str()) == Some(&row.party_version));
                let ids = self
                    .rows
                    .iter()
                    .filter(|(_, r)| {
                        !r.consumed
                            && r.parameter.active != 0
                            && original::tick(r.parameter.seconds, ms.wrapping_sub(r.clock))
                                != r.parameter.seconds
                    })
                    .map(|(id, _)| id.clone())
                    .collect::<Vec<_>>();
                for id in ids {
                    let epoch = self.bump()?;
                    let row = self.rows.get_mut(&id).unwrap();
                    let next = original::tick(row.parameter.seconds, ms.wrapping_sub(row.clock));
                    row.parameter.publish(true, 1, next, 0, &None);
                    row.clock = ms;
                    row.complete = next == 0;
                    row.epoch = epoch;
                }
                Ok(json!(
                    self.rows.values().map(Row::snapshot).collect::<Vec<_>>()
                ))
            }
            _ => Err(Error(400, "Unknown native countdown operation")),
        }
    }
}
// Pure numerical fixture carrier; it cannot install or mutate a live row.
pub(super) fn sample(r: &Value) -> Result<Value> {
    let value = &r["value"];
    m::fields(
        value,
        &[
            "serial",
            "seconds",
            "clock_ms",
            "playlist",
            "revision",
            "party_version",
            "mode",
            "complete",
            "requester",
        ],
    )?;
    m::integer(&value["serial"], 1, i64::MAX)?;
    m::text(&value["playlist"], 64)?;
    m::text(&value["revision"], 64)?;
    m::integer(&value["party_version"], 1, i64::MAX)?;
    m::integer(&value["clock_ms"], 0, u32::MAX as i64)?;
    let mode = m::integer(&value["mode"], 0, 1)? as i32;
    m::boolean(&value["complete"])?;
    if !value["requester"].is_null() {
        m::text(&value["requester"], 64)?;
    }
    let seconds = if value["seconds"].is_null() {
        None
    } else {
        Some(m::integer(&value["seconds"], 0, i32::MAX as i64)? as i32)
    };
    let mut result = value.clone();
    if r["op"] == "party_countdown_sample_tick" {
        let now = clock(r)?;
        if let Some(seconds) = seconds {
            let mut p = Parameter {
                active: 1,
                seconds,
                mode,
                watermark: if mode != 0 { seconds } else { -1 },
                updates: 0,
                requester: value["requester"].as_str().map(str::to_owned),
            };
            let next = original::tick(
                seconds,
                now.wrapping_sub(value["clock_ms"].as_u64().unwrap() as u32),
            );
            if next != seconds {
                p.publish(true, 1, next, 0, &None);
                result["seconds"] = json!(next);
                result["clock_ms"] = json!(now);
                result["mode"] = json!(p.mode);
                result["requester"] = json!(p.requester);
                result["complete"] = json!(next == 0);
            }
        }
    } else {
        m::check(
            r["op"] == "party_countdown_sample",
            "Unknown countdown sample operation",
        )?;
    }
    Ok(result)
}
