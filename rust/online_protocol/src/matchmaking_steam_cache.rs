//! Modern Steam upstream cache, serialized work leases and sealed invitations.
//! Only native request/directory owners can publish data or issue/consume proof.
use crate::matchmaking_coordinator::id;
use crate::matchmaking_model::{self as m, Error, Result};
use serde_json::{Value, json};
use sha2::{Digest, Sha256};
use std::collections::BTreeMap;

const MAX_BYTES: usize = 64 * 1024 * 1024;
pub fn credential_context(v: &Value) -> Result<String> {
    let s = v
        .as_str()
        .ok_or(Error(503, "Steam credential configuration required"))?;
    if s.is_empty() {
        return Err(Error(503, "Steam friends are unavailable"));
    }
    if s.len() > 512 || s.chars().any(|c| c < ' ' || c == '\x7f') {
        return Err(Error(503, "Invalid Steam credential configuration"));
    }
    Ok(format!("{:x}", Sha256::digest(s.as_bytes())))
}
struct Entry {
    value: Value,
    expires: f64,
    bytes: usize,
}
struct Lease {
    owner: u64,
    subject: String,
    expires: f64,
    started: f64,
}
struct Waiting {
    subject: String,
    expires: f64,
}
struct Grant {
    player: String,
    key: String,
    subject: String,
    target: String,
    payload: String,
    expires: f64,
    locked: Option<u64>,
}
struct Rate {
    tokens: f64,
    at: f64,
}
pub struct Cache {
    accounts: u64,
    now: f64,
    epoch: u64,
    context: Option<String>,
    bytes: usize,
    entries: BTreeMap<String, Entry>,
    active: Option<Lease>,
    waiting: BTreeMap<u64, Waiting>,
    grants: BTreeMap<String, Grant>,
    pending: BTreeMap<u64, String>,
    rates: BTreeMap<String, Rate>,
}
pub(crate) fn steam_id(v: &Value) -> Result<&str> {
    let s = m::text(v, 17)?;
    m::check(
        s.len() == 17 && s.as_bytes()[0] != b'0' && s.bytes().all(|b| b.is_ascii_digit()),
        "Invalid Steam identity",
    )?;
    Ok(s)
}
pub(crate) fn clean(value: &Value) -> Result<()> {
    let private = value["state"] == "private";
    m::fields(
        value,
        if private {
            &["state", "friends"]
        } else {
            &["state", "friends", "truncated"]
        },
    )?;
    m::check(
        private || value["state"] == "ready",
        "Invalid Steam cache state",
    )?;
    let rows = value["friends"]
        .as_array()
        .ok_or(Error(502, "Invalid Steam cache rows"))?;
    m::check(
        rows.len() <= 1000 && (!private || rows.is_empty()),
        "Steam cache rows exceed bounds",
    )?;
    if !private {
        m::boolean(&value["truncated"])?;
    }
    let mut ids = std::collections::BTreeSet::new();
    for row in rows {
        m::fields(row, &["id", "name"])?;
        m::check(
            ids.insert(steam_id(&row["id"])?),
            "Duplicate Steam cache identity",
        )?;
        m::text(&row["name"], 128)?;
    }
    Ok(())
}
impl Cache {
    pub fn new(r: &Value) -> Result<Self> {
        m::fields(r, &["account_source"])?;
        Ok(Self {
            accounts: r["account_source"]
                .as_u64()
                .ok_or(Error(400, "Steam account source required"))?,
            now: 0.,
            epoch: 0,
            context: None,
            bytes: 0,
            entries: BTreeMap::new(),
            active: None,
            waiting: BTreeMap::new(),
            grants: BTreeMap::new(),
            pending: BTreeMap::new(),
            rates: BTreeMap::new(),
        })
    }
    pub fn accounts(&self) -> u64 {
        self.accounts
    }
    pub fn configure_credential(&mut self, value: &Value) -> Result<()> {
        let context = credential_context(value)?;
        self.context(&json!(context))
    }
    pub fn observe(&mut self, now: f64) -> Result<()> {
        m::check(
            now >= 0. && (now + 300.).is_finite(),
            "Invalid Steam cache clock",
        )?;
        self.now = self.now.max(now);
        self.entries.retain(|_, e| e.expires > self.now);
        self.bytes = self.entries.values().map(|e| e.bytes).sum();
        if self.active.as_ref().is_some_and(|e| e.expires <= self.now) {
            self.active = None;
        }
        self.waiting.retain(|_, w| w.expires > self.now);
        self.grants.retain(|_, g| g.expires > self.now);
        Ok(())
    }
    fn clear(&mut self) -> Result<()> {
        self.epoch = self
            .epoch
            .checked_add(1)
            .ok_or(Error(503, "Steam cache generation exhausted"))?;
        self.entries.clear();
        self.bytes = 0;
        self.active = None;
        self.waiting.clear();
        self.grants.clear();
        // A directory which held a grant must observe invalidation before its
        // next SQL reply; do not erase that pending binding until drop/commit.
        Ok(())
    }
    fn context(&mut self, value: &Value) -> Result<()> {
        m::hex(value, 32)?;
        let context = value.as_str().unwrap();
        if self.context.as_deref() != Some(context) {
            self.clear()?;
            self.context = Some(context.to_owned());
        }
        Ok(())
    }
    pub fn request(&mut self, owner: u64, r: &Value) -> Result<Value> {
        match m::text(&r["op"], 32)? {
            "acquire" => {
                m::fields(r, &["op", "subject", "context"])?;
                self.context(&r["context"])?;
                let subject = steam_id(&r["subject"])?.to_owned();
                if let Some(e) = self.entries.get(&subject) {
                    self.waiting.remove(&owner);
                    return Ok(
                        json!({"state":"hit","value":e.value,"epoch":self.epoch,"expires":e.expires}),
                    );
                }
                if self.entries.len() >= 1000 {
                    return Err(Error(503, "Steam friends are busy; try again"));
                }
                if !self.waiting.contains_key(&owner) {
                    m::check(self.waiting.len() < 128, "Steam wait capacity reached")?;
                    self.waiting.insert(
                        owner,
                        Waiting {
                            subject: subject.clone(),
                            expires: self.now + 75.,
                        },
                    );
                }
                m::check(
                    self.waiting[&owner].subject == subject,
                    "Steam waiter identity changed",
                )?;
                if self.active.is_none()
                    && self
                        .waiting
                        .first_key_value()
                        .is_some_and(|(id, _)| *id == owner)
                {
                    self.waiting.remove(&owner);
                    self.active = Some(Lease {
                        owner,
                        subject: subject.clone(),
                        expires: self.now + 75.,
                        started: self.now,
                    });
                }
                if let Some(lease) = self
                    .active
                    .as_ref()
                    .filter(|l| l.owner == owner && l.subject == subject)
                {
                    Ok(json!({"state":"active","epoch":self.epoch,"expires":lease.started+60.}))
                } else {
                    Ok(json!({"state":"wait"}))
                }
            }
            "publish" => {
                m::fields(
                    r,
                    &[
                        "op",
                        "subject",
                        "context",
                        "epoch",
                        "expires",
                        "value",
                        "invitation",
                    ],
                )?;
                m::hex(&r["context"], 32)?;
                let subject = steam_id(&r["subject"])?.to_owned();
                if self.context.as_deref() != r["context"].as_str()
                    || r["epoch"].as_u64() != Some(self.epoch)
                {
                    return Err(Error(409, "Steam lookup configuration changed"));
                }
                let expires = m::real(&r["expires"])?;
                if expires <= self.now {
                    return Err(Error(409, "Steam lookup expired; refresh friends"));
                }
                clean(&r["value"])?;
                let active = self.active.as_ref().is_some_and(|l| {
                    l.owner == owner && l.subject == subject && expires == l.started + 60.
                });
                let hit = self
                    .entries
                    .get(&subject)
                    .is_some_and(|e| e.value == r["value"] && e.expires == expires);
                m::check(active || hit, "Steam publication lease retired")?;
                let bytes = r["value"].to_string().len();
                let existing = self.entries.get(&subject).map(|e| e.bytes).unwrap_or(0);
                if !hit
                    && (self.entries.len() >= 1000 && !self.entries.contains_key(&subject)
                        || self.bytes - existing + bytes > MAX_BYTES)
                {
                    return Err(Error(503, "Steam friends cache is full"));
                }
                let mut issued = None;
                if !r["invitation"].is_null() {
                    let invitation = &r["invitation"];
                    m::fields(invitation, &["player", "key", "target", "payload"])?;
                    let target = steam_id(&invitation["target"])?;
                    let found = r["value"]["state"] == "ready"
                        && r["value"]["friends"]
                            .as_array()
                            .unwrap()
                            .iter()
                            .any(|v| v["id"] == target);
                    if found {
                        m::text(&invitation["player"], 64)?;
                        m::hex(&invitation["key"], 32)?;
                        m::hex(&invitation["payload"], 32)?;
                        if self.grants.len() >= 128 {
                            return Err(Error(503, "Steam invitation proof capacity reached"));
                        }
                        let token = id()?;
                        issued = Some((
                            token,
                            Grant {
                                player: invitation["player"].as_str().unwrap().to_owned(),
                                key: invitation["key"].as_str().unwrap().to_owned(),
                                subject: subject.clone(),
                                target: target.to_owned(),
                                payload: invitation["payload"].as_str().unwrap().to_owned(),
                                expires: expires.min(self.now + 30.),
                                locked: None,
                            },
                        ));
                    }
                }
                if !hit {
                    self.bytes = self.bytes - existing + bytes;
                    self.entries.insert(
                        subject,
                        Entry {
                            value: r["value"].clone(),
                            expires,
                            bytes,
                        },
                    );
                }
                if active {
                    self.active = None;
                }
                let token = issued.as_ref().map(|v| v.0.clone());
                if let Some((token, grant)) = issued {
                    self.grants.insert(token, grant);
                }
                Ok(json!({"claim":token}))
            }
            _ => Err(Error(400, "Unknown Steam request cache event")),
        }
    }
    pub fn directory(&mut self, owner: u64, r: &Value) -> Result<Value> {
        m::fields(r, &["op", "claim", "player", "key", "target", "payload"])?;
        m::check(r["op"] == "authorize", "Unknown Steam directory event")?;
        let token = m::text(&r["claim"], 36)?;
        let grant = self.grants.get_mut(token).ok_or(Error(
            403,
            "Refresh Steam friends before inviting this player",
        ))?;
        if grant.player != r["player"]
            || grant.key != r["key"]
            || grant.target != r["target"]
            || grant.payload != r["payload"]
            || grant.locked.is_some()
        {
            return Err(Error(403, "Steam invitation proof differs"));
        }
        m::check(
            self.pending.len() < 128,
            "Steam directory proof capacity reached",
        )?;
        grant.locked = Some(owner);
        self.pending.insert(owner, token.to_owned());
        Ok(json!({"subject":grant.subject,"target":grant.target}))
    }
    pub fn directory_live(&self, owner: u64) -> bool {
        self.pending.get(&owner).is_none_or(|token| {
            self.grants
                .get(token)
                .is_some_and(|g| g.locked == Some(owner) && g.expires > self.now)
        })
    }
    pub fn committed(&mut self, owner: u64) -> bool {
        if let Some(token) = self.pending.remove(&owner) {
            self.grants.remove(&token);
            true
        } else {
            false
        }
    }
    pub fn abort_directory(&mut self, owner: u64) {
        if let Some(token) = self.pending.remove(&owner) {
            if let Some(g) = self.grants.get_mut(&token) {
                if g.locked == Some(owner) {
                    g.locked = None;
                }
            }
        }
    }
    pub fn abort_request(&mut self, owner: u64) {
        self.waiting.remove(&owner);
        if self.active.as_ref().is_some_and(|l| l.owner == owner) {
            self.active = None;
        }
    }
    pub fn execute(&mut self, r: &Value) -> Result<Value> {
        let op = m::text(&r["op"], 16)?;
        m::fields(
            r,
            if op == "rate" {
                &["op", "now", "source"]
            } else {
                &["op", "now"]
            },
        )?;
        self.observe(m::real(&r["now"])?)?;
        match op {
            "stats" => Ok(
                json!({"cached":self.entries.len(),"bytes":self.bytes,"active":self.active.is_some(),"waiting":self.waiting.len(),"grants":self.grants.len(),"pending":self.pending.len(),"epoch":self.epoch}),
            ),
            "clear" => {
                self.clear()?;
                Ok(Value::Null)
            }
            "rate" => {
                let source = m::text(&r["source"], 128)?.to_owned();
                if self.rates.len() >= 4096 {
                    self.rates.retain(|_, r| self.now - r.at < 60.);
                }
                if self.rates.len() >= 4096 && !self.rates.contains_key(&source) {
                    return Err(Error(429, "Rate limiter capacity reached"));
                }
                let rate = self.rates.entry(source).or_insert(Rate {
                    tokens: 30.,
                    at: self.now,
                });
                rate.tokens = (rate.tokens + (self.now - rate.at) * 0.5).min(30.);
                rate.at = self.now;
                if rate.tokens < 1. {
                    return Err(Error(429, "Rate limit exceeded"));
                }
                rate.tokens -= 1.;
                Ok(Value::Null)
            }
            _ => Err(Error(400, "Unknown Steam cache operation")),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    fn cache() -> Cache {
        Cache::new(&json!({"account_source":7})).unwrap()
    }
    fn acquire(c: &mut Cache, owner: u64, subject: &str) -> Value {
        c.request(owner,&json!({"op":"acquire","subject":subject,"context":credential_context(&json!("fixture-key")).unwrap()})).unwrap()
    }
    fn publish(
        c: &mut Cache,
        owner: u64,
        subject: &str,
        lease: &Value,
        invitation: Value,
    ) -> Value {
        c.request(owner,&json!({"op":"publish","subject":subject,"context":credential_context(&json!("fixture-key")).unwrap(),"epoch":lease["epoch"],"expires":lease["expires"],"value":{"state":"ready","friends":[{"id":"76561197960287931","name":"Friend"}],"truncated":false},"invitation":invitation})).unwrap()
    }
    #[test]
    fn fifo_waiters_coalesce_hits_and_cancelled_owners_release_the_slot() {
        let mut c = cache();
        let subject = "76561197960287930";
        assert_eq!(acquire(&mut c, 10, subject)["state"], "active");
        assert_eq!(acquire(&mut c, 11, subject)["state"], "wait");
        assert_eq!(acquire(&mut c, 12, "76561197960287932")["state"], "wait");
        c.abort_request(10);
        assert_eq!(acquire(&mut c, 12, "76561197960287932")["state"], "wait");
        let lease = acquire(&mut c, 11, subject);
        assert_eq!(lease["state"], "active");
        publish(&mut c, 11, subject, &lease, Value::Null);
        assert_eq!(acquire(&mut c, 13, subject)["state"], "hit");
        assert_eq!(acquire(&mut c, 12, "76561197960287932")["state"], "active");
        c.abort_request(12);
        assert!(c.active.is_none());
        assert!(c.waiting.is_empty());
    }
    #[test]
    fn epoch_and_exact_expiry_fence_old_publication_and_invitation_proof() {
        let mut c = cache();
        let subject = "76561197960287930";
        let lease = acquire(&mut c, 1, subject);
        c.configure_credential(&json!("rotated-key")).unwrap();
        let old = json!({"op":"publish","subject":subject,"context":credential_context(&json!("fixture-key")).unwrap(),"epoch":lease["epoch"],"expires":lease["expires"],"value":{"state":"private","friends":[]},"invitation":null});
        assert_eq!(c.request(1, &old).unwrap_err().0, 409);
        assert!(c.entries.is_empty());
        let lease = acquire(&mut c, 2, subject);
        let invitation = json!({"player":"actor","key":"11".repeat(32),"target":"76561197960287931","payload":"22".repeat(32)});
        let issued = publish(&mut c, 2, subject, &lease, invitation);
        assert_eq!(c.grants.len(), 1);
        c.observe(30.).unwrap();
        assert!(c.grants.is_empty());
        assert_eq!(c.directory(3,&json!({"op":"authorize","claim":issued["claim"],"player":"actor","key":"11".repeat(32),"target":"76561197960287931","payload":"22".repeat(32)})).unwrap_err().0,403);
        c.observe(60.).unwrap();
        assert!(c.entries.is_empty());
        c.observe(0.).unwrap();
        assert_eq!(c.now, 60.);
    }
    #[test]
    fn invitation_is_single_use_after_commit_and_retryable_after_abort() {
        let mut c = cache();
        let subject = "76561197960287930";
        let lease = acquire(&mut c, 1, subject);
        let invitation = json!({"player":"actor","key":"11".repeat(32),"target":"76561197960287931","payload":"22".repeat(32)});
        let issued = publish(&mut c, 1, subject, &lease, invitation);
        let r = json!({"op":"authorize","claim":issued["claim"],"player":"actor","key":"11".repeat(32),"target":"76561197960287931","payload":"22".repeat(32)});
        let mut wrong = r.clone();
        wrong["player"] = json!("other");
        assert!(c.directory(2, &wrong).is_err());
        c.directory(2, &r).unwrap();
        assert!(c.directory(3, &r).is_err());
        c.abort_directory(2);
        c.directory(3, &r).unwrap();
        assert!(c.committed(3));
        assert!(c.directory(4, &r).is_err());
        assert!(c.grants.is_empty());
        assert!(c.pending.is_empty());
    }
    #[test]
    fn cached_row_and_endpoint_rate_capacity_are_finite_and_clock_cannot_refill() {
        let mut c = cache();
        for i in 0..1000 {
            let subject = (76561197960300000u64 + i).to_string();
            let lease = acquire(&mut c, i + 1, &subject);
            publish(&mut c, i + 1, &subject, &lease, Value::Null);
        }
        assert_eq!(c.entries.len(), 1000);
        assert_eq!(c.request(1002,&json!({"op":"acquire","subject":"76561197960287930","context":credential_context(&json!("fixture-key")).unwrap()})).unwrap_err().0,503);
        assert_eq!(acquire(&mut c, 1003, "76561197960300000")["state"], "hit");
        for _ in 0..30 {
            c.execute(&json!({"op":"rate","now":0.,"source":"loopback"}))
                .unwrap();
        }
        assert_eq!(
            c.execute(&json!({"op":"rate","now":0.,"source":"loopback"}))
                .unwrap_err()
                .0,
            429
        );
        c.execute(&json!({"op":"rate","now":2.,"source":"loopback"}))
            .unwrap();
        assert_eq!(
            c.execute(&json!({"op":"rate","now":0.,"source":"loopback"}))
                .unwrap_err()
                .0,
            429
        );
        assert!(c.execute(&json!({"op":"publish","now":2.})).is_err());
    }
}
