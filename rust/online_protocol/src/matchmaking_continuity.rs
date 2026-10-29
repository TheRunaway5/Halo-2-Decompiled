//! Native legacy-relay authority generations and complete-world proofs.
//!
//! This is explicitly modern coordination, not recovered Xbox host ordering.
//! A digest is a peer claim: this owner neither imports nor recreates a world.
use crate::matchmaking_model::{self as m, Error, Result};
use serde_json::{Value, json};
use std::collections::{BTreeMap, BTreeSet};

#[derive(Clone, Debug, PartialEq, Eq)]
struct Seal {
    tick: u64,
    digest: String,
}
impl Seal {
    fn parse(r: &Value) -> Result<Self> {
        let tick = r["tick"]
            .as_u64()
            .ok_or(Error(400, "Invalid completed tick"))?;
        m::hex(&r["digest"], 32)?;
        Ok(Self {
            tick,
            digest: r["digest"].as_str().unwrap().to_owned(),
        })
    }
    fn value(&self) -> Value {
        json!({"tick":self.tick,"digest":self.digest})
    }
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
enum Phase {
    Running,
    Awaiting,
    Migrating,
    Rebinding,
    Empty,
    Unavailable,
}
impl Phase {
    fn name(self) -> &'static str {
        match self {
            Self::Running => "running",
            Self::Awaiting => "awaiting_world",
            Self::Migrating => "migrating",
            Self::Rebinding => "rebinding",
            Self::Empty => "empty",
            Self::Unavailable => "unavailable",
        }
    }
}
#[derive(Clone)]
pub struct Continuity {
    owners: Vec<usize>,
    active: BTreeSet<usize>,
    host: usize,
    epoch: u32,
    phase: Phase,
    timeout: f64,
    now: f64,
    deadline: Option<f64>,
    seals: BTreeMap<u64, Seal>,
    proofs: BTreeMap<usize, Seal>,
    selected: Option<Seal>,
    rebound: BTreeSet<usize>,
    failed: BTreeSet<usize>,
    quality: BTreeMap<usize, f64>,
}
impl Continuity {
    pub fn new(r: &Value) -> Result<Self> {
        m::fields(r, &["owners", "host", "timeout"])?;
        let rows = r["owners"]
            .as_array()
            .ok_or(Error(400, "Physical room owners required"))?;
        m::check(
            (2..=16).contains(&rows.len()),
            "Invalid physical match ownership",
        )?;
        let owners = rows
            .iter()
            .map(|p| Ok(m::integer(p, 0, (rows.len() - 1) as i64)? as usize))
            .collect::<Result<Vec<_>>>()?;
        let active = owners.iter().copied().collect::<BTreeSet<_>>();
        m::check(
            active
                .iter()
                .all(|p| owners[*p] == *p && owners.iter().filter(|v| *v == p).count() <= 4),
            "Invalid physical match ownership",
        )?;
        let host = m::integer(&r["host"], 0, 15)? as usize;
        m::check(active.contains(&host), "Invalid match authority")?;
        let timeout = m::real(&r["timeout"])?;
        m::check(timeout > 0., "Invalid handoff timeout")?;
        Ok(Self {
            owners,
            active,
            host,
            epoch: 0,
            phase: Phase::Running,
            timeout,
            now: 0.,
            deadline: None,
            seals: BTreeMap::new(),
            proofs: BTreeMap::new(),
            selected: None,
            rebound: BTreeSet::new(),
            failed: BTreeSet::new(),
            quality: BTreeMap::new(),
        })
    }
    pub fn epoch(&self) -> u32 {
        self.epoch
    }
    pub(crate) fn deadline(&self) -> Option<f64> {
        self.deadline
    }
    pub(crate) fn active(&self, owner: usize) -> bool {
        self.active.contains(&owner)
    }
    pub(crate) fn host(&self) -> usize {
        self.host
    }
    pub(crate) fn phase(&self) -> &'static str {
        self.phase.name()
    }
    fn members(&self) -> u32 {
        self.owners.iter().enumerate().fold(0, |mask, (i, p)| {
            mask | if self.active.contains(p) { 1 << i } else { 0 }
        })
    }
    pub fn control(&self) -> Value {
        let mut v = json!({"epoch":self.epoch,"host":if matches!(self.phase,Phase::Awaiting|Phase::Empty|Phase::Unavailable){None}else{Some(self.host)},
            "members":self.members(),"phase":self.phase.name()});
        if let Some(seal) = &self.selected {
            v["tick"] = json!(seal.tick);
            v["digest"] = json!(seal.digest);
        }
        v
    }
    fn state(&self) -> Value {
        json!({"owners":self.owners,"active":self.active,"host":self.host,"epoch":self.epoch,"phase":self.phase.name(),
            "members":self.members(),"timeout":self.timeout,"deadline":self.deadline,
            "rebound":self.rebound,"failed_candidates":self.failed,"quality":self.quality})
    }
    fn snapshot(&self) -> Value {
        json!({"owners":self.owners,"active":self.active,"host":self.host,"epoch":self.epoch,"phase":self.phase.name(),
            "members":self.members(),"timeout":self.timeout,"deadline":self.deadline,
            "seals":self.seals.iter().map(|(k,v)|(k.to_string(),v.value())).collect::<BTreeMap<_,_>>(),
            "proofs":self.proofs.iter().map(|(k,v)|(k.to_string(),v.value())).collect::<BTreeMap<_,_>>(),
            "selected":self.selected.as_ref().map(Seal::value),"rebound":self.rebound,"failed_candidates":self.failed,"quality":self.quality})
    }
    fn clock(&mut self, r: &Value) -> Result<()> {
        let now = m::real(&r["now"])?;
        m::check(now >= 0., "Invalid handoff clock")?;
        self.now = self.now.max(now);
        Ok(())
    }
    fn arm(&mut self) -> Result<()> {
        let deadline = self.now + self.timeout;
        m::check(deadline.is_finite(), "Invalid handoff deadline")?;
        self.deadline = Some(deadline);
        Ok(())
    }
    fn seal(&mut self, peer: usize, epoch: u32, seal: Seal) -> Result<Value> {
        m::check(
            self.phase == Phase::Running
                && peer == self.host
                && self.active.contains(&peer)
                && epoch == self.epoch,
            "Stale or unauthorized world seal",
        )?;
        m::check(
            self.seals
                .last_key_value()
                .is_none_or(|(tick, _)| seal.tick >= *tick),
            "World seal moved backwards",
        )?;
        m::check(
            self.seals.get(&seal.tick).is_none_or(|old| old == &seal),
            "Authority changed a sealed completed tick",
        )?;
        self.seals.insert(seal.tick, seal.clone());
        while self.seals.len() > 64 {
            self.seals.pop_first();
        }
        self.proofs.insert(peer, seal.clone());
        Ok(seal.value())
    }
    fn acknowledge(&mut self, peer: usize, epoch: u32, seal: Seal) -> Result<Value> {
        let expected = if self.phase == Phase::Awaiting {
            self.epoch.checked_sub(1)
        } else {
            Some(self.epoch)
        };
        m::check(
            self.active.contains(&peer)
                && Some(epoch) == expected
                && matches!(self.phase, Phase::Running | Phase::Awaiting),
            "Stale world acknowledgement",
        )?;
        m::check(
            self.seals.get(&seal.tick) == Some(&seal),
            "Standby simulation differs from the sealed world",
        )?;
        m::check(
            self.proofs.get(&peer).is_none_or(|p| seal.tick >= p.tick),
            "Standby simulation moved backwards",
        )?;
        self.proofs.insert(peer, seal);
        if self.phase == Phase::Awaiting {
            self.choose()?;
        }
        Ok(Value::Null)
    }
    fn elect(&mut self) -> Result<()> {
        self.epoch = self
            .epoch
            .checked_add(1)
            .ok_or(Error(503, "Authority generation exhausted"))?;
        self.phase = Phase::Migrating;
        self.rebound.clear();
        self.choose()
    }
    fn choose(&mut self) -> Result<()> {
        let newest = self.seals.last_key_value().map(|(tick, _)| *tick);
        let candidate = self
            .active
            .iter()
            .copied()
            .filter(|p| {
                !self.failed.contains(p)
                    && self.proofs.get(p).is_some_and(|proof| {
                        self.seals.get(&proof.tick) == Some(proof)
                            && match &self.selected {
                                Some(selected) => selected == proof,
                                None => Some(proof.tick) == newest,
                            }
                    })
            })
            .min_by(|a, b| {
                self.quality
                    .get(a)
                    .unwrap_or(&f64::INFINITY)
                    .total_cmp(self.quality.get(b).unwrap_or(&f64::INFINITY))
                    .then(a.cmp(b))
            });
        let Some(host) = candidate else {
            if self.selected.is_none() && !self.seals.is_empty() {
                self.phase = Phase::Awaiting;
                if self.deadline.is_none() {
                    self.arm()?;
                }
            } else {
                self.phase = Phase::Unavailable;
                self.deadline = None;
            }
            return Ok(());
        };
        self.host = host;
        self.selected = Some(self.proofs[&host].clone());
        self.phase = Phase::Migrating;
        self.arm()
    }
    fn disconnect(&mut self, peer: usize) -> Result<Value> {
        if !self.active.remove(&peer) {
            return Ok(json!(false));
        }
        self.proofs.remove(&peer);
        self.rebound.remove(&peer);
        if self.active.is_empty() {
            self.phase = Phase::Empty;
            self.deadline = None;
        } else if peer == self.host {
            self.elect()?;
        } else if self.phase == Phase::Rebinding {
            self.finish()?;
        }
        Ok(json!(true))
    }
    fn finish(&mut self) -> Result<()> {
        if self.active.is_subset(&self.rebound) {
            let seal = self
                .selected
                .as_ref()
                .ok_or(Error(503, "Retained world proof unavailable"))?
                .clone();
            self.phase = Phase::Running;
            self.deadline = None;
            self.proofs = self.active.iter().map(|p| (*p, seal.clone())).collect();
            self.seals = BTreeMap::from([(seal.tick, seal)]);
            self.selected = None;
            self.failed.clear();
        }
        Ok(())
    }
    fn accept(&mut self, peer: usize, epoch: u32, seal: Seal, rebind: bool) -> Result<Value> {
        m::check(
            self.phase
                == (if rebind {
                    Phase::Rebinding
                } else {
                    Phase::Migrating
                })
                && self.active.contains(&peer)
                && (rebind || peer == self.host)
                && epoch == self.epoch
                && self.selected.as_ref() == Some(&seal),
            "Authority handoff does not match the retained complete world",
        )?;
        if !rebind {
            self.phase = Phase::Rebinding;
            self.rebound.clear();
            self.arm()?;
        }
        self.rebound.insert(peer);
        self.finish()?;
        Ok(Value::Null)
    }
    fn expire(&mut self) -> Result<Value> {
        if self.deadline.is_none_or(|deadline| self.now < deadline) {
            return Ok(json!(false));
        }
        match self.phase {
            Phase::Awaiting => {
                self.phase = Phase::Unavailable;
                self.deadline = None;
            }
            Phase::Migrating => {
                self.failed.insert(self.host);
                self.elect()?;
            }
            Phase::Rebinding => {
                self.active.retain(|p| self.rebound.contains(p));
                self.proofs.retain(|p, _| self.active.contains(p));
                self.finish()?;
            }
            _ => return Err(Error(503, "Invalid handoff deadline state")),
        }
        Ok(json!(true))
    }
    fn operation(&mut self, op: &str, peer: usize, epoch: u32, seal: Seal) -> Result<Value> {
        match op {
            "seal" | "world_seal" => self.seal(peer, epoch, seal),
            "acknowledge" | "world_ack" => self.acknowledge(peer, epoch, seal),
            "accept" | "migration_accept" => self.accept(peer, epoch, seal, false),
            "rebind" | "migration_rebind" => self.accept(peer, epoch, seal, true),
            _ => Err(Error(400, "Unknown complete-world control")),
        }
    }
    /// The room owner supplies the physical sender and Rust result state.
    /// Validate the whole request before ignoring an in-flight old generation.
    pub(crate) fn recognizes(data: &Value) -> Result<bool> {
        let object = data
            .as_object()
            .ok_or(Error(400, "Invalid complete-world control"))?;
        Ok([
            "world_seal",
            "world_ack",
            "migration_accept",
            "migration_rebind",
        ]
        .iter()
        .any(|action| object.contains_key(*action)))
    }
    pub(crate) fn message(
        &mut self,
        peer: usize,
        data: &Value,
        finished: bool,
        now: f64,
    ) -> Result<Value> {
        let object = data
            .as_object()
            .ok_or(Error(400, "Invalid complete-world control"))?;
        if !Self::recognizes(data)? {
            return Ok(json!({"handled":false,"broadcast":null,"changed":false}));
        }
        m::check(
            object.len() == 1,
            "Invalid complete-world continuation control",
        )?;
        let (action, r) = object.iter().next().unwrap();
        m::fields(r, &["epoch", "tick", "digest"])?;
        let epoch = m::integer(&r["epoch"], 0, u32::MAX as i64)? as u32;
        let seal = Seal::parse(r)?;
        let ignored = (finished && ["world_seal", "world_ack"].contains(&action.as_str()))
            || (epoch < self.epoch
                && !(action == "world_ack"
                    && self.phase == Phase::Awaiting
                    && epoch.checked_add(1) == Some(self.epoch)));
        if ignored {
            return Ok(json!({"handled":true,"broadcast":null,"changed":false}));
        }
        let mut next = self.clone();
        next.clock(&json!({"now":now}))?;
        next.operation(action, peer, epoch, seal)?;
        let changed = next.control() != self.control();
        *self = next;
        Ok(
            json!({"handled":true,"broadcast":if action=="world_seal"{data.clone()}else{Value::Null},"changed":changed}),
        )
    }
    /// No snapshot installation or field setters exist on the production ABI.
    /// Failed operands leave even clock/deadline metadata unchanged.
    pub fn execute(&mut self, r: &Value) -> Result<Value> {
        let op = m::text(&r["op"], 64)?;
        if op == "snapshot" {
            return Ok(self.snapshot());
        }
        if op == "state" {
            return Ok(self.state());
        }
        if op == "control" {
            return Ok(self.control());
        }
        let mut next = self.clone();
        next.clock(r)?;
        let value = if op == "expire" {
            next.expire()?
        } else {
            let peer = m::integer(&r["peer"], 0, 15)? as usize;
            match op {
                "disconnect" => next.disconnect(peer)?,
                "quality" => {
                    let ms = m::real(&r["milliseconds"])?;
                    m::check(
                        next.active.contains(&peer) && (0.0..=60000.).contains(&ms),
                        "Invalid observed match quality",
                    )?;
                    // Python's previous numerical ordering treated both zeros
                    // as equal; preserve the physical-slot tie break.
                    next.quality.insert(peer, if ms == 0. { 0. } else { ms });
                    Value::Null
                }
                "seal" | "acknowledge" | "accept" | "rebind" => {
                    let epoch = m::integer(&r["epoch"], 0, u32::MAX as i64)? as u32;
                    next.operation(op, peer, epoch, Seal::parse(r)?)?
                }
                _ => return Err(Error(400, "Unknown native continuity operation")),
            }
        };
        *self = next;
        Ok(value)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn generation_exhaustion_fails_without_partial_departure() {
        let mut c = Continuity::new(&json!({"owners":[0,1],"host":0,"timeout":15})).unwrap();
        c.epoch = u32::MAX;
        let before = c.snapshot();
        assert_eq!(
            c.execute(&json!({"op":"disconnect","peer":0,"now":1}))
                .unwrap_err()
                .0,
            503
        );
        assert_eq!(c.snapshot(), before);
    }
}
