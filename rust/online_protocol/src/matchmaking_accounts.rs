//! Native verified-account sessions used by party/clan APIs; not Xbox Live.
//! Identity/provider/SQLite IO supplies canonical facts, never bearer state.
use crate::matchmaking_coordinator::id;
use crate::matchmaking_model::{self as m, Error, Result};
use serde_json::{Value, json};
use sha2::{Digest, Sha256};
use std::collections::{BTreeMap, BTreeSet};

#[derive(Clone)]
struct Session {
    generation: u64,
    player: String,
    expires: f64,
    seen: f64,
    owner: Value,
}
struct Permit {
    owner: Value,
    existing: Option<String>,
    generation: u64,
    token: String,
    key: String,
    expires: f64,
}
struct Budget {
    tokens: f64,
    at: f64,
}
#[derive(Default)]
pub struct Accounts {
    now: f64,
    rate_now: f64,
    presence_wall: i64,
    next: u64,
    sessions: BTreeMap<String, Session>,
    pending: BTreeMap<String, Permit>,
    budgets: BTreeMap<(String, String), Budget>,
}
fn require(ok: bool, status: u16, message: &'static str) -> Result<()> {
    if ok {
        Ok(())
    } else {
        Err(Error(status, message))
    }
}
fn hash(value: &str) -> String {
    format!("{:x}", Sha256::digest(value.as_bytes()))
}
fn token() -> Result<String> {
    let mut bytes = [0; 32];
    getrandom::getrandom(&mut bytes).map_err(|_| Error(503, "Account entropy unavailable"))?;
    let alphabet = b"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    let mut out = String::new();
    let mut bits = 0u32;
    let mut count = 0;
    for byte in bytes {
        bits = (bits << 8) | u32::from(byte);
        count += 8;
        while count >= 6 {
            count -= 6;
            out.push(alphabet[((bits >> count) & 63) as usize] as char);
        }
    }
    if count > 0 {
        out.push(alphabet[((bits << (6 - count)) & 63) as usize] as char);
    }
    Ok(out)
}
fn identity(owner: &Value) -> Result<()> {
    let rows = owner
        .as_array()
        .ok_or(Error(401, "Verified account identity required"))?;
    require(rows.len() == 2, 401, "Verified account identity required")?;
    m::text(&rows[0], 64)?;
    m::text(&rows[1], 256)?;
    Ok(())
}
fn body(value: &Value) -> Result<&str> {
    m::fields(value, &["screen_name"])?;
    m::text(&value["screen_name"], 64)
}
impl Accounts {
    /// Presence reads never renew activity, bearer leases or SQL clock state.
    /// Called only through the actual account owner by the native IO bridge.
    pub fn presence(&mut self, now: f64, wall: i64) -> Result<(Vec<String>, i64)> {
        m::check(
            now >= 0. && (now + 900.).is_finite(),
            "Invalid account clock",
        )?;
        m::check(
            (0..=9007199254740991).contains(&wall),
            "Invalid presence wall clock",
        )?;
        self.now = self.now.max(now);
        self.presence_wall = self.presence_wall.max(wall);
        Ok((
            self.sessions
                .values()
                .filter(|s| crate::matchmaking_presence::recent(s.expires, s.seen, self.now))
                .map(|s| s.player.clone())
                .collect(),
            self.presence_wall,
        ))
    }
    fn observe(&mut self, r: &Value) -> Result<()> {
        let now = m::real(&r["now"])?;
        let rate = m::real(&r["rate_now"])?;
        m::check(
            now >= 0. && (now + 900.).is_finite() && rate >= 0. && (rate + 60.).is_finite(),
            "Invalid account clock",
        )?;
        self.now = self.now.max(now);
        self.rate_now = self.rate_now.max(rate);
        self.sessions.retain(|_, s| s.expires > self.now);
        self.pending.retain(|_, p| p.expires > self.now);
        Ok(())
    }
    fn snapshot(&self, key: &str, s: &Session) -> Value {
        json!({"key":key,"generation":s.generation,"player":s.player,"expires":s.expires,"seen":s.seen})
    }
    fn budget(&mut self, who: &str, bucket: &str) -> Result<()> {
        let maximum = match bucket {
            "social" => 120.,
            "countdown" => 600.,
            _ => return Err(Error(400, "Unknown account budget")),
        };
        let key = (who.to_owned(), bucket.to_owned());
        if self.budgets.len() >= 4096 {
            self.budgets.retain(|_, b| self.rate_now - b.at < 60.);
            require(
                self.budgets.contains_key(&key) || self.budgets.len() < 4096,
                429,
                "Rate limiter capacity reached",
            )?;
        }
        let b = self.budgets.entry(key).or_insert(Budget {
            tokens: maximum,
            at: self.rate_now,
        });
        b.tokens = maximum.min(b.tokens + (self.rate_now - b.at).max(0.) * maximum / 60.);
        b.at = self.rate_now;
        require(b.tokens >= 1., 429, "Rate limit exceeded")?;
        b.tokens -= 1.;
        Ok(())
    }
    /// The FFI calls this against the actual owner on every protected directory
    /// continuation. It never accepts a Python session projection as authority.
    pub fn binding(&mut self, key: &str, player: &str, now: f64) -> Result<bool> {
        m::check(
            now >= 0. && (now + 900.).is_finite(),
            "Invalid account clock",
        )?;
        self.now = self.now.max(now);
        Ok(self
            .sessions
            .get(key)
            .is_some_and(|s| s.player == player && s.expires > self.now))
    }
    /// A committed canonical adoption retires grants for the deleted player.
    /// This is invoked by the sealed native directory commit, without IO.
    pub fn revoke_canonical_player(&mut self, who: &str) {
        let owners = self
            .sessions
            .values()
            .filter(|s| s.player == who)
            .map(|s| s.owner.clone())
            .collect::<Vec<_>>();
        self.sessions.retain(|_, s| s.player != who);
        self.pending
            .retain(|_, p| p.existing.as_deref() != Some(who) && !owners.contains(&p.owner));
    }
    pub fn execute(&mut self, r: &Value) -> Result<Value> {
        self.observe(r)?;
        match m::text(&r["op"], 32)? {
            "body" => Ok(json!(body(&r["body"])?)),
            "prepare" => {
                let name = body(&r["body"])?;
                identity(&r["owner"])?;
                let existing = if r["existing"].is_null() {
                    None
                } else {
                    Some(m::text(&r["existing"], 64)?.to_owned())
                };
                let mut occupied = self
                    .sessions
                    .values()
                    .map(|s| s.player.as_str())
                    .collect::<BTreeSet<_>>();
                occupied.extend(self.pending.values().filter_map(|p| p.existing.as_deref()));
                let held = existing
                    .as_deref()
                    .is_some_and(|who| occupied.contains(who));
                require(
                    self.pending.len() < 64,
                    503,
                    "Account login capacity reached",
                )?;
                let reserved = self
                    .pending
                    .values()
                    .filter(|p| p.existing.is_none())
                    .count();
                require(
                    held || occupied.len() + reserved < 1000,
                    503,
                    "ONLINE session capacity reached",
                )?;
                let generation = self
                    .next
                    .checked_add(1)
                    .ok_or(Error(503, "Account session generation exhausted"))?;
                // All entropy and capacity are owned before canonical identity
                // writes begin. Failure cannot first persist a denied login.
                let permit = id()?;
                let token = token()?;
                let key = hash(&token);
                require(
                    !self.sessions.contains_key(&key)
                        && !self.pending.values().any(|p| p.key == key),
                    503,
                    "Account entropy collision",
                )?;
                self.next = generation;
                self.pending.insert(
                    permit.clone(),
                    Permit {
                        owner: r["owner"].clone(),
                        existing,
                        generation,
                        token,
                        key,
                        expires: self.now + 30.,
                    },
                );
                Ok(json!({"permit":permit,"name":name}))
            }
            "commit" => {
                let key = m::text(&r["permit"], 64)?;
                let who = m::text(&r["player"], 64)?;
                let pending = self
                    .pending
                    .get(key)
                    .ok_or(Error(409, "Account login permit retired"))?;
                require(
                    pending.owner == r["owner"]
                        && pending.existing.as_ref().is_none_or(|p| p == who),
                    409,
                    "Verified account identity changed",
                )?;
                require(
                    self.sessions.len() < 1000 || self.sessions.values().any(|s| s.player == who),
                    503,
                    "ONLINE session capacity reached",
                )?;
                require(
                    !self
                        .sessions
                        .values()
                        .any(|s| s.player == who && s.generation > pending.generation),
                    409,
                    "Account login permit superseded",
                )?;
                let pending = self.pending.remove(key).unwrap();
                self.sessions.retain(|_, s| s.player != who);
                self.sessions.insert(
                    pending.key,
                    Session {
                        generation: pending.generation,
                        player: who.to_owned(),
                        expires: self.now + 900.,
                        seen: self.now,
                        owner: pending.owner,
                    },
                );
                Ok(json!({"token":pending.token,"expires_in":900}))
            }
            "abort" => {
                self.pending.remove(m::text(&r["permit"], 64)?);
                Ok(Value::Null)
            }
            "auth" => {
                let header = r["header"]
                    .as_str()
                    .ok_or(Error(401, "ONLINE session required"))?;
                let token = header
                    .strip_prefix("Bearer ")
                    .filter(|_| header.len() <= 256)
                    .ok_or(Error(401, "ONLINE session required"))?;
                let key = hash(token);
                let who = self
                    .sessions
                    .get(&key)
                    .ok_or(Error(401, "ONLINE session expired; reconnect"))?
                    .player
                    .clone();
                if m::boolean(&r["rate"])? {
                    self.budget(&who, "social")?;
                }
                let session = self.sessions.get_mut(&key).unwrap();
                session.seen = self.now;
                Ok(json!({"key":key,"player":who,"generation":session.generation}))
            }
            "revoke" => {
                let retired = self.sessions.remove(m::text(&r["key"], 64)?);
                if let Some(s) = &retired {
                    self.pending.retain(|_, p| {
                        p.existing.as_deref() != Some(&s.player) && p.owner != s.owner
                    });
                }
                Ok(json!(retired.is_some()))
            }
            "revoke_player" => {
                let who = m::text(&r["player"], 64)?;
                self.revoke_canonical_player(who);
                Ok(Value::Null)
            }
            "clear" => {
                self.sessions.clear();
                self.pending.clear();
                Ok(Value::Null)
            }
            "keys" => Ok(json!(self.sessions.keys().collect::<Vec<_>>())),
            "get" => {
                let key = m::text(&r["key"], 64)?;
                Ok(self
                    .sessions
                    .get(key)
                    .map(|s| self.snapshot(key, s))
                    .unwrap_or(Value::Null))
            }
            "stats" => Ok(
                json!({"sessions":self.sessions.len(),"pending":self.pending.len(),"budgets":self.budgets.len()}),
            ),
            "cleanup" => Ok(Value::Null),
            "budget" => {
                let who = m::text(&r["player"], 64)?;
                require(
                    self.sessions.values().any(|s| s.player == who),
                    401,
                    "ONLINE session expired; reconnect",
                )?;
                self.budget(who, m::text(&r["bucket"], 32)?)?;
                Ok(Value::Null)
            }
            _ => Err(Error(400, "Unknown account owner operation")),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    fn send(a: &mut Accounts, mut r: Value, now: f64, rate: f64) -> Result<Value> {
        r["now"] = json!(now);
        r["rate_now"] = json!(rate);
        a.execute(&r)
    }
    fn prepare(a: &mut Accounts, owner: Value, existing: Value, now: f64) -> Value {
        send(a,json!({"op":"prepare","body":{"screen_name":"Pilot"},"owner":owner,"existing":existing}),now,10.).unwrap()
    }
    fn commit(a: &mut Accounts, p: &Value, owner: Value, who: &str, now: f64) -> Result<Value> {
        send(
            a,
            json!({"op":"commit","permit":p["permit"],"owner":owner,"player":who}),
            now,
            10.,
        )
    }
    #[test]
    fn rotation_and_revocation_fence_old_permits_and_exact_expiry_cannot_rewind() {
        let mut a = Accounts::default();
        let owner = json!(["discord", "verified-one"]);
        let old = prepare(&mut a, owner.clone(), Value::Null, 10.);
        let next = prepare(&mut a, owner.clone(), Value::Null, 10.);
        let token = commit(&mut a, &next, owner.clone(), "one", 10.).unwrap()["token"]
            .as_str()
            .unwrap()
            .to_owned();
        assert_eq!(
            commit(&mut a, &old, owner.clone(), "one", 10.)
                .unwrap_err()
                .0,
            409
        );
        assert_eq!(a.sessions.len(), 1);
        let pending = prepare(&mut a, owner.clone(), Value::Null, 10.);
        send(&mut a, json!({"op":"revoke","key":hash(&token)}), 10., 10.).unwrap();
        assert!(
            a.pending.is_empty(),
            "Even a pre-identity alias permit retires with its verified owner"
        );
        assert_eq!(
            commit(&mut a, &pending, owner.clone(), "one", 10.)
                .unwrap_err()
                .0,
            409
        );
        let p = prepare(&mut a, owner.clone(), json!("one"), 10.);
        let t = commit(&mut a, &p, owner, "one", 10.).unwrap()["token"]
            .as_str()
            .unwrap()
            .to_owned();
        assert!(a.binding(&hash(&t), "one", 909.999).unwrap());
        assert!(!a.binding(&hash(&t), "one", 910.).unwrap());
        assert!(!a.binding(&hash(&t), "one", 10.).unwrap());
        assert_eq!(
            send(
                &mut a,
                json!({"op":"auth","header":format!("Bearer {t}"),"rate":false}),
                10.,
                10.
            )
            .unwrap_err()
            .0,
            401
        );
    }
    #[test]
    fn pending_login_capacity_identity_and_deadlines_cannot_publish_partial_sessions() {
        let mut a = Accounts::default();
        let owner = json!(["steam", "76561198000000000"]);
        let first = prepare(&mut a, owner.clone(), json!("known"), 10.);
        assert_eq!(
            commit(&mut a, &first, json!(["discord", "wrong"]), "known", 10.)
                .unwrap_err()
                .0,
            409
        );
        assert_eq!(
            commit(&mut a, &first, owner.clone(), "other", 10.)
                .unwrap_err()
                .0,
            409
        );
        assert!(a.sessions.is_empty());
        for i in 1..64 {
            prepare(
                &mut a,
                json!(["fixture", format!("p{i}")]),
                Value::Null,
                10.,
            );
        }
        assert_eq!(a.pending.len(), 64);
        assert_eq!(send(&mut a,json!({"op":"prepare","body":{"screen_name":"Pilot"},"owner":owner,"existing":"known"}),10.,10.).unwrap_err().0,503);
        assert_eq!(
            commit(
                &mut a,
                &first,
                json!(["steam", "76561198000000000"]),
                "known",
                40.
            )
            .unwrap_err()
            .0,
            409
        );
        assert!(a.pending.is_empty());
        assert!(a.sessions.is_empty());
        for op in ["restore", "install", "session_set"] {
            assert_eq!(send(&mut a, json!({"op":op}), 40., 10.).unwrap_err().0, 400);
        }
    }
    #[test]
    fn independent_social_and_countdown_budgets_do_not_refill_on_clock_regression() {
        let mut a = Accounts::default();
        let owner = json!(["fixture", "one"]);
        let p = prepare(&mut a, owner.clone(), Value::Null, 10.);
        let token = commit(&mut a, &p, owner, "one", 10.).unwrap()["token"]
            .as_str()
            .unwrap()
            .to_owned();
        let request = json!({"op":"auth","header":format!("Bearer {token}"),"rate":true});
        for _ in 0..120 {
            send(&mut a, request.clone(), 10., 10.).unwrap();
        }
        assert_eq!(send(&mut a, request.clone(), 10., 10.).unwrap_err().0, 429);
        for _ in 0..600 {
            send(
                &mut a,
                json!({"op":"budget","player":"one","bucket":"countdown"}),
                10.,
                10.,
            )
            .unwrap();
        }
        assert_eq!(
            send(
                &mut a,
                json!({"op":"budget","player":"one","bucket":"countdown"}),
                10.,
                10.
            )
            .unwrap_err()
            .0,
            429
        );
        assert_eq!(send(&mut a, request.clone(), 10., 1.).unwrap_err().0, 429);
        send(&mut a, request, 10., 10.5).unwrap();
        assert_eq!(a.budgets.len(), 2);
    }
}
