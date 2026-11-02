//! Bounded derived career-total cache. SQL/profile policy lives in the native
//! directory cursor. Only that cursor can prepare a value; publication follows
//! the adapter's successful owned SQL commit. This is modern service policy.
use crate::matchmaking_coordinator::id;
use crate::matchmaking_model::{self as m, Error, Result};
use serde_json::{Value, json};
use std::collections::BTreeMap;

struct Cached {
    revision: Option<i64>,
    value: Value,
    expires: f64,
    touched: u64,
}
struct Pending {
    owner: u64,
    player: String,
    revision: Option<i64>,
    generation: u64,
    expires: f64,
    cache_expires: f64,
    value: Option<Value>,
}
#[derive(Default)]
pub struct Profiles {
    now: f64,
    serial: u64,
    entries: BTreeMap<String, Cached>,
    pending: BTreeMap<String, Pending>,
    latest: BTreeMap<String, (u64, Option<i64>)>,
}
impl Profiles {
    pub fn observe(&mut self, now: f64) -> Result<()> {
        m::check(
            now >= 0. && (now + 300.).is_finite(),
            "Invalid profile cache clock",
        )?;
        self.now = self.now.max(now);
        self.entries.retain(|_, v| v.expires > self.now);
        self.pending.retain(|_, v| v.expires > self.now);
        self.prune();
        Ok(())
    }
    fn prune(&mut self) {
        self.latest.retain(|player, _| {
            self.entries.contains_key(player) || self.pending.values().any(|p| p.player == *player)
        });
    }
    fn next(&mut self) -> Result<u64> {
        self.serial = self
            .serial
            .checked_add(1)
            .ok_or(Error(503, "Profile cache generation exhausted"))?;
        Ok(self.serial)
    }
    /// Called only by a source-bound native directory event, never an HTTP
    /// cache-install operation. Revisions are canonical SQL observations.
    pub fn directory(&mut self, owner: u64, r: &Value) -> Result<Value> {
        match m::text(&r["op"], 32)? {
            "lookup" => {
                m::fields(r, &["op", "player", "revision"])?;
                let player = m::text(&r["player"], 64)?;
                let revision = if r["revision"].is_null() {
                    None
                } else {
                    Some(m::integer(&r["revision"], 0, i64::MAX)?)
                };
                // An older SQL snapshot may be read, but must not poison a
                // newer revision already observed by this cache owner.
                if self
                    .latest
                    .get(player)
                    .is_some_and(|(_, known)| *known > revision)
                {
                    return Ok(json!({"value":null,"token":null}));
                }
                let generation = self.next()?;
                if let Some(entry) = self.entries.get_mut(player) {
                    if entry.revision == revision && entry.expires > self.now {
                        entry.touched = generation;
                        return Ok(json!({"value":entry.value,"token":null}));
                    }
                }
                m::check(
                    self.pending.len() < 128,
                    "Profile cache pending capacity reached",
                )?;
                self.entries.remove(player);
                let token = id()?;
                self.latest
                    .insert(player.to_owned(), (generation, revision));
                self.pending.insert(
                    token.clone(),
                    Pending {
                        owner,
                        player: player.to_owned(),
                        revision,
                        generation,
                        expires: self.now + 30.,
                        cache_expires: self.now + 300.,
                        value: None,
                    },
                );
                Ok(json!({"value":null,"token":token}))
            }
            "stage" => {
                m::fields(r, &["op", "token", "value"])?;
                let token = m::text(&r["token"], 36)?;
                statistics(&r["value"])?;
                let plan = self
                    .pending
                    .get_mut(token)
                    .ok_or(Error(409, "Profile cache plan expired"))?;
                m::check(
                    plan.owner == owner && plan.value.is_none(),
                    "Profile cache plan owner differs",
                )?;
                plan.value = Some(r["value"].clone());
                Ok(Value::Null)
            }
            _ => Err(Error(400, "Unknown native profile cache event")),
        }
    }
    /// A cache is derived data. Retirement/expiry after SQL commit is a no-op,
    /// never a failed customization response after a successful durable write.
    pub fn committed(&mut self, owner: u64, now: f64) -> bool {
        if self.observe(now).is_err() {
            self.abort(owner);
            return false;
        }
        let tokens = self
            .pending
            .iter()
            .filter(|(_, p)| p.owner == owner)
            .map(|(t, _)| t.clone())
            .collect::<Vec<_>>();
        let mut published = false;
        for token in tokens {
            let p = self.pending.remove(&token).unwrap();
            if self.latest.get(&p.player) != Some(&(p.generation, p.revision))
                || p.cache_expires <= self.now
            {
                continue;
            }
            let Some(value) = p.value else {
                continue;
            };
            while self.entries.len() >= 1024 {
                let key = self
                    .entries
                    .iter()
                    .min_by_key(|(_, v)| v.touched)
                    .unwrap()
                    .0
                    .clone();
                self.entries.remove(&key);
            }
            self.entries.insert(
                p.player,
                Cached {
                    revision: p.revision,
                    value,
                    expires: p.cache_expires,
                    touched: p.generation,
                },
            );
            published = true;
        }
        self.prune();
        published
    }
    pub fn abort(&mut self, owner: u64) {
        self.pending.retain(|_, p| p.owner != owner);
        self.prune();
    }
    /// Operational invalidation/counts only. Python cannot install snapshots.
    pub fn execute(&mut self, r: &Value) -> Result<Value> {
        m::fields(r, &["op"])?;
        match m::text(&r["op"], 32)? {
            "clear" => {
                self.entries.clear();
                self.pending.clear();
                self.latest.clear();
                Ok(Value::Null)
            }
            "stats" => Ok(json!({"cached":self.entries.len(),"pending":self.pending.len()})),
            _ => Err(Error(
                400,
                "Profile cache publication requires a native SQL owner",
            )),
        }
    }
}
pub fn statistics(value: &Value) -> Result<()> {
    let fields = [
        "matches",
        "wins",
        "kills",
        "deaths",
        "assists",
        "shots",
        "hits",
        "accuracy_matches",
        "accuracy_complete",
    ];
    m::fields(value, &fields)?;
    for name in &fields[..8] {
        m::integer(&value[*name], 0, i64::MAX)?;
    }
    m::boolean(&value["accuracy_complete"])?;
    m::check(
        value["accuracy_complete"] == json!(value["accuracy_matches"] == value["matches"]),
        "Profile accuracy coverage differs",
    )
}

#[cfg(test)]
mod tests {
    use super::*;
    fn totals(kills: u64) -> Value {
        json!({"matches":1,"wins":1,"kills":kills,"deaths":0,"assists":0,
        "shots":0,"hits":0,"accuracy_matches":0,"accuracy_complete":false})
    }
    fn lookup(p: &mut Profiles, owner: u64, player: &str, revision: u64) -> Value {
        p.directory(
            owner,
            &json!({"op":"lookup","player":player,"revision":revision}),
        )
        .unwrap()
    }
    fn stage(p: &mut Profiles, owner: u64, token: &Value, kills: u64) {
        p.directory(
            owner,
            &json!({"op":"stage","token":token,"value":totals(kills)}),
        )
        .unwrap();
    }
    #[test]
    fn publication_requires_sql_completion_and_superseded_snapshots_cannot_replace_newer_revision()
    {
        let mut p = Profiles::default();
        p.observe(100.).unwrap();
        let old = lookup(&mut p, 1, "player", 1);
        stage(&mut p, 1, &old["token"], 7);
        let new = lookup(&mut p, 2, "player", 2);
        stage(&mut p, 2, &new["token"], 8);
        assert!(p.committed(2, 101.));
        assert!(!p.committed(1, 102.));
        assert_eq!(lookup(&mut p, 3, "player", 2)["value"]["kills"], 8);
        assert_eq!(
            lookup(&mut p, 4, "player", 1),
            json!({"value":null,"token":null})
        );
        assert_eq!(p.execute(&json!({"op":"stats"})).unwrap()["cached"], 1);
        assert!(p.execute(&json!({"op":"install"})).is_err());
    }
    #[test]
    fn monotonic_expiry_abort_capacity_and_lru_are_bounded() {
        let mut p = Profiles::default();
        p.observe(100.).unwrap();
        let first = lookup(&mut p, 1, "a", 1);
        stage(&mut p, 1, &first["token"], 7);
        p.abort(1);
        assert!(!p.committed(1, 101.));
        assert_eq!(p.execute(&json!({"op":"stats"})).unwrap()["cached"], 0);
        for i in 0..1025 {
            let row = lookup(&mut p, i + 2, &format!("p{i}"), 1);
            stage(&mut p, i + 2, &row["token"], 1);
            assert!(p.committed(i + 2, 102.));
        }
        assert_eq!(p.entries.len(), 1024);
        assert!(!p.entries.contains_key("p0"));
        p.observe(99.).unwrap();
        assert_eq!(p.now, 102.);
        p.observe(403.).unwrap();
        assert!(p.entries.is_empty());
        for i in 0..128 {
            lookup(&mut p, i + 2000, &format!("pending{i}"), 1);
        }
        assert!(
            p.directory(9000, &json!({"op":"lookup","player":"over","revision":1}))
                .is_err()
        );
        p.observe(434.).unwrap();
        assert!(p.pending.is_empty());
        p.observe(100.).unwrap();
        assert_eq!(p.now, 434.);
    }
}
