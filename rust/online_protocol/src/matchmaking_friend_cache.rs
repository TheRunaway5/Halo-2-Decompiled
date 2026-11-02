//! Derived bilateral block cache. Every lookup is bound to a SQL epoch.
//! Native directory plans alone stage values, published after owned SQL commit.
use crate::matchmaking_coordinator::id;
use crate::matchmaking_model::{self as m, Error, Result};
use serde_json::{Value, json};
use sha2::{Digest, Sha256};
use std::collections::BTreeMap;

struct Entry {
    value: bool,
    expires: f64,
    touched: u64,
}
struct Pending {
    owner: u64,
    pair: (String, String),
    revision: i64,
    value: Option<bool>,
    expires: f64,
}
struct Cloud {
    prepared: Option<u64>,
    finishing: Option<u64>,
    context: Value,
    expires: f64,
}
#[derive(Default)]
pub struct Friends {
    now: f64,
    revision: i64,
    serial: u64,
    entries: BTreeMap<(String, String), Entry>,
    pending: BTreeMap<String, Pending>,
    clouds: BTreeMap<String, Cloud>,
}
impl Friends {
    pub fn observe(&mut self, now: f64) -> Result<()> {
        m::check(
            now >= 0. && (now + 300.).is_finite(),
            "Invalid friend cache clock",
        )?;
        self.now = self.now.max(now);
        self.entries.retain(|_, e| e.expires > self.now);
        self.pending.retain(|_, e| e.expires > self.now);
        self.clouds.retain(|_, e| e.expires > self.now);
        Ok(())
    }
    pub fn directory(&mut self, owner: u64, r: &Value) -> Result<Value> {
        match m::text(&r["op"], 32)? {
            "cloud_prepare" => {
                m::fields(r, &["op", "context"])?;
                let context = &r["context"];
                m::fields(
                    context,
                    &[
                        "player", "revision", "payload", "digest", "puid", "name", "failures",
                    ],
                )?;
                m::text(&context["player"], 64)?;
                m::integer(&context["revision"], 1, i64::MAX)?;
                m::text(&context["payload"], 32768)?;
                m::hex(&context["digest"], 32)?;
                if context["digest"]
                    != format!(
                        "{:x}",
                        Sha256::digest(context["payload"].as_str().unwrap().as_bytes())
                    )
                {
                    return Err(Error(503, "Friend cloud record digest differs"));
                }
                m::hex(&context["puid"], 16)?;
                m::text(&context["name"], 64)?;
                m::integer(&context["failures"], 0, i64::MAX)?;
                m::check(
                    self.clouds.len() < 32,
                    "Friend cloud claim capacity reached",
                )?;
                let token = id()?;
                self.clouds.insert(
                    token.clone(),
                    Cloud {
                        prepared: Some(owner),
                        finishing: None,
                        context: context.clone(),
                        expires: self.now + 180.,
                    },
                );
                Ok(json!(token))
            }
            "cloud_context" => {
                m::fields(r, &["op", "claim"])?;
                let token = m::text(&r["claim"], 36)?;
                let cloud = self
                    .clouds
                    .get_mut(token)
                    .ok_or(Error(409, "Friend cloud claim retired"))?;
                m::check(
                    cloud.prepared.is_none() && cloud.finishing.is_none(),
                    "Friend cloud claim unavailable",
                )?;
                cloud.finishing = Some(owner);
                Ok(cloud.context.clone())
            }
            "lookup" => {
                m::fields(r, &["op", "a", "b", "revision"])?;
                let a = m::text(&r["a"], 64)?.to_owned();
                let b = m::text(&r["b"], 64)?.to_owned();
                let pair = if a < b { (a, b) } else { (b, a) };
                let revision = m::integer(&r["revision"], 0, i64::MAX)?;
                if revision < self.revision {
                    return Ok(json!({"value":null,"token":null}));
                }
                if revision > self.revision {
                    self.revision = revision;
                    self.entries.clear();
                }
                self.serial = self
                    .serial
                    .checked_add(1)
                    .ok_or(Error(503, "Friend cache generation exhausted"))?;
                if let Some(entry) = self.entries.get_mut(&pair) {
                    entry.touched = self.serial;
                    return Ok(json!({"value":entry.value,"token":null}));
                }
                m::check(
                    self.pending.len() < 128,
                    "Friend cache pending capacity reached",
                )?;
                let token = id()?;
                self.pending.insert(
                    token.clone(),
                    Pending {
                        owner,
                        pair,
                        revision,
                        value: None,
                        expires: self.now + 30.,
                    },
                );
                Ok(json!({"value":null,"token":token}))
            }
            "stage" => {
                m::fields(r, &["op", "token", "value"])?;
                let token = m::text(&r["token"], 36)?;
                let value = m::boolean(&r["value"])?;
                let pending = self
                    .pending
                    .get_mut(token)
                    .ok_or(Error(409, "Friend cache plan retired"))?;
                m::check(
                    pending.owner == owner && pending.value.is_none(),
                    "Friend cache plan owner differs",
                )?;
                pending.value = Some(value);
                Ok(Value::Null)
            }
            _ => Err(Error(400, "Unknown native friend cache event")),
        }
    }
    pub fn committed(&mut self, owner: u64, now: f64) -> bool {
        if self.observe(now).is_err() {
            self.abort(owner);
            return false;
        }
        let tokens = self
            .pending
            .iter()
            .filter(|(_, p)| p.owner == owner)
            .map(|(k, _)| k.clone())
            .collect::<Vec<_>>();
        let mut changed = false;
        self.clouds.retain(|_, cloud| {
            if cloud.finishing == Some(owner) {
                changed = true;
                return false;
            }
            if cloud.prepared == Some(owner) {
                cloud.prepared = None;
                changed = true;
            }
            true
        });
        for token in tokens {
            let plan = self.pending.remove(&token).unwrap();
            let Some(value) = plan.value else {
                continue;
            };
            if plan.revision != self.revision {
                continue;
            }
            if self.entries.len() >= 4096 && !self.entries.contains_key(&plan.pair) {
                let oldest = self
                    .entries
                    .iter()
                    .min_by_key(|(_, v)| v.touched)
                    .map(|(k, _)| k.clone());
                if let Some(oldest) = oldest {
                    self.entries.remove(&oldest);
                }
            }
            self.entries.insert(
                plan.pair,
                Entry {
                    value,
                    expires: self.now + 300.,
                    touched: self.serial,
                },
            );
            changed = true;
        }
        changed
    }
    pub fn abort(&mut self, owner: u64) {
        self.pending.retain(|_, p| p.owner != owner);
        self.clouds.retain(|_, cloud| {
            if cloud.prepared == Some(owner) {
                return false;
            }
            if cloud.finishing == Some(owner) {
                cloud.finishing = None;
            }
            true
        });
    }
    pub fn execute(&mut self, r: &Value) -> Result<Value> {
        m::fields(r, &["op"])?;
        match m::text(&r["op"], 32)? {
            "clear" => {
                self.entries.clear();
                self.pending.clear();
                self.clouds.clear();
                Ok(Value::Null)
            }
            "stats" => Ok(
                json!({"cached":self.entries.len(),"pending":self.pending.len(),"revision":self.revision,"cloud_claims":self.clouds.len()}),
            ),
            _ => Err(Error(400, "Unknown friend cache operation")),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn cloud_claim_is_sealed_by_sql_commit_and_retryable_after_aborted_completion() {
        let mut c = Friends::default();
        c.observe(10.).unwrap();
        let context = json!({"player":"a","revision":1,"payload":"record","digest":format!("{:x}",Sha256::digest(b"record")),"puid":"aa".repeat(16),"name":"Name","failures":0});
        let token = c
            .directory(1, &json!({"op":"cloud_prepare","context":context}))
            .unwrap();
        assert!(
            c.directory(2, &json!({"op":"cloud_context","claim":token}))
                .is_err()
        );
        assert!(c.committed(1, 10.));
        assert_eq!(
            c.directory(3, &json!({"op":"cloud_context","claim":token}))
                .unwrap(),
            context
        );
        assert!(
            c.directory(4, &json!({"op":"cloud_context","claim":token}))
                .is_err()
        );
        c.abort(3);
        assert_eq!(
            c.directory(4, &json!({"op":"cloud_context","claim":token}))
                .unwrap(),
            context
        );
        assert!(c.committed(4, 10.));
        assert!(
            c.directory(5, &json!({"op":"cloud_context","claim":token}))
                .is_err()
        );
        let abandoned = c
            .directory(6, &json!({"op":"cloud_prepare","context":context}))
            .unwrap();
        c.abort(6);
        assert!(
            c.directory(7, &json!({"op":"cloud_context","claim":abandoned}))
                .is_err()
        );
    }
    fn lookup(c: &mut Friends, owner: u64, revision: i64) -> Value {
        c.directory(
            owner,
            &json!({"op":"lookup","a":"a","b":"b","revision":revision}),
        )
        .unwrap()
    }
    #[test]
    fn sql_epoch_retires_cached_allow_and_stale_commit_without_poisoning_newer_block() {
        let mut c = Friends::default();
        c.observe(10.).unwrap();
        let old = lookup(&mut c, 1, 0);
        c.directory(1, &json!({"op":"stage","token":old["token"],"value":false}))
            .unwrap();
        let new = lookup(&mut c, 2, 1);
        c.directory(2, &json!({"op":"stage","token":new["token"],"value":true}))
            .unwrap();
        assert!(c.committed(2, 10.));
        assert!(!c.committed(1, 10.));
        assert_eq!(lookup(&mut c, 3, 1)["value"], true);
        assert_eq!(lookup(&mut c, 4, 0), json!({"value":null,"token":null}));
        assert!(
            c.directory(5, &json!({"op":"stage","token":new["token"],"value":false}))
                .is_err()
        );
        c.observe(310.).unwrap();
        c.observe(9.).unwrap();
        assert!(lookup(&mut c, 6, 1)["value"].is_null());
        c.abort(6);
        assert_eq!(c.execute(&json!({"op":"stats"})).unwrap()["pending"], 0);
    }
}
