//! Owned SQLite issuance/renewal and the private pending-state publication seam.
use super::*;
use crate::steam_login::{self as steam, Callback, Confirmation, Owner, Plan, Poll, Reply, need};
use std::{
    sync::atomic::{AtomicBool, Ordering},
    time::Instant,
};
pub(super) fn schema(db: &Connection) -> Result<()> {
    db.execute_batch("CREATE TABLE IF NOT EXISTS steam_login_nonces(nonce TEXT PRIMARY KEY,expires INTEGER NOT NULL);CREATE TABLE IF NOT EXISTS steam_login_refresh(token_hash TEXT PRIMARY KEY,subject TEXT NOT NULL,name TEXT NOT NULL,expires INTEGER NOT NULL);CREATE TABLE IF NOT EXISTS steam_login_refresh_sessions(session_hash TEXT PRIMARY KEY,refresh_hash TEXT NOT NULL);CREATE INDEX IF NOT EXISTS steam_refresh_expiry ON steam_login_refresh(expires);CREATE INDEX IF NOT EXISTS steam_refresh_subject ON steam_login_refresh(subject,expires);CREATE INDEX IF NOT EXISTS steam_refresh_issued ON steam_login_refresh_sessions(refresh_hash);").map_err(database::failure)
}
fn cleanup(db: &Connection, wall: i64) -> Result<()> {
    for sql in [
        "DELETE FROM steam_login_sessions WHERE expires<=?",
        "DELETE FROM steam_login_nonces WHERE expires<=?",
        "DELETE FROM steam_login_refresh WHERE expires<=?",
    ] {
        db.execute(sql, [wall]).map_err(database::failure)?;
    }
    db.execute("DELETE FROM steam_login_refresh_sessions WHERE session_hash NOT IN (SELECT token_hash FROM steam_login_sessions)",[]).map_err(database::failure)?;
    Ok(())
}
fn session(db: &Connection, owner: &Owner, token: &str, wall: i64) -> Result<()> {
    db.execute("DELETE FROM steam_login_sessions WHERE subject=? AND token_hash NOT IN (SELECT token_hash FROM steam_login_sessions WHERE subject=? ORDER BY expires DESC,rowid DESC LIMIT 3)",[owner.subject(),owner.subject()]).map_err(database::failure)?;
    let count = db
        .query_row("SELECT count(*) FROM steam_login_sessions", [], |r| {
            r.get::<_, i64>(0)
        })
        .map_err(database::failure)?;
    need(count < 1000, 503, "Steam identity capacity reached")?;
    db.execute(
        "INSERT INTO steam_login_sessions VALUES(?,?,?,?)",
        rusqlite::params![
            steam::digest(token),
            owner.subject(),
            owner.name(),
            wall + steam::SESSION_LIFETIME
        ],
    )
    .map_err(database::failure)?;
    Ok(())
}
impl Runtime {
    pub fn configure_steam(&mut self, issuer: &str, allow_http: bool) -> Result<()> {
        if !self.steam_enabled {
            return Ok(());
        }
        self.steam_logins.configure(issuer, allow_http)
    }
    fn steam_enabled(&self) -> Result<()> {
        need(self.steam_enabled, 503, "Steam sign-in is not configured")
    }
    pub fn steam_start(
        &mut self,
        body: Value,
        source: String,
        prefix: String,
        admit: impl FnOnce() -> Result<()>,
    ) -> Result<Reply> {
        self.steam_enabled()?;
        let clock = self.clock()?;
        self.steam_logins
            .start(&body, source, prefix, &clock, admit)
    }
    pub fn steam_callback(
        &mut self,
        state: &str,
        query: Vec<(String, String)>,
    ) -> Result<Callback> {
        self.steam_enabled()?;
        let clock = self.clock()?;
        let db = &self.db;
        self.steam_logins.callback(state, query, &clock, |nonce| {
            Ok(db
                .query_row(
                    "SELECT 1 FROM steam_login_nonces WHERE nonce=?",
                    [nonce],
                    |_| Ok(()),
                )
                .optional()
                .map_err(database::failure)?
                .is_some())
        })
    }
    pub fn steam_failed(&mut self, plan: &Plan) {
        self.steam_logins.failed(plan);
    }
    pub fn steam_verified(
        &mut self,
        plan: &Plan,
        owner: Owner,
        alive: &AtomicBool,
        deadline: Instant,
    ) -> Result<Reply> {
        let valid = || {
            need(
                alive.load(Ordering::Acquire) && Instant::now() < deadline,
                503,
                "Steam verification request expired",
            )
        };
        valid()?;
        let cookie = steam::random()?;
        let started = self.started;
        let tx = self
            .db
            .transaction_with_behavior(TransactionBehavior::Immediate)
            .map_err(database::failure)?;
        let clock = Clock {
            monotonic: started.elapsed().as_secs_f64(),
            wall: database::wall_now()?,
        };
        self.steam_logins.verify_live(plan, &owner, &clock)?;
        valid()?;
        let exists = tx
            .query_row(
                "SELECT 1 FROM steam_login_nonces WHERE nonce=?",
                [&plan.nonce],
                |_| Ok(()),
            )
            .optional()
            .map_err(database::failure)?
            .is_some();
        need(!exists, 400, "Steam response replayed")?;
        cleanup(&tx, clock.wall)?;
        tx.execute(
            "INSERT INTO steam_login_nonces VALUES(?,?)",
            rusqlite::params![plan.nonce, clock.wall + 600],
        )
        .map_err(database::failure)?;
        let fresh = Clock {
            monotonic: started.elapsed().as_secs_f64(),
            wall: database::wall_now()?,
        };
        self.steam_logins.verify_live(plan, &owner, &fresh)?;
        valid()?;
        tx.commit().map_err(database::failure)?;
        Ok(self.steam_logins.publish_verified(plan, owner, cookie))
    }
    pub fn steam_confirmation(
        &mut self,
        state: &str,
        cookie: &str,
        origin: Option<&str>,
    ) -> Result<Confirmation> {
        let clock = self.clock()?;
        let prior = self.steam_logins.confirm(state, cookie, &clock)?;
        if let Some(origin) = origin {
            self.steam_logins.origin(origin)?;
        }
        Ok(prior)
    }
    pub fn steam_confirmation_page(&mut self, state: &str, cookie: &str) -> Result<Reply> {
        let prior = self.steam_confirmation(state, cookie, None)?;
        Ok(self.steam_logins.page(&prior))
    }
    pub fn steam_confirmed(&mut self, prior: Confirmation, body: Vec<u8>) -> Result<Reply> {
        let clock = self.clock()?;
        self.steam_logins.confirmed(&prior, &body, &clock)
    }
    pub fn steam_poll(&mut self, body: Value) -> Result<Reply> {
        self.steam_enabled()?;
        let clock = self.clock()?;
        let (state, generation, owner) = match self.steam_logins.poll(&body, &clock)? {
            Poll::Waiting(reply) => return Ok(reply),
            Poll::Complete {
                state,
                generation,
                owner,
            } => (state, generation, owner),
        };
        let token = format!("h2s_{}", steam::random()?);
        let refresh = format!("h2r_{}", steam::random()?);
        let started = self.started;
        let tx = self
            .db
            .transaction_with_behavior(TransactionBehavior::Immediate)
            .map_err(database::failure)?;
        let clock = Clock {
            monotonic: started.elapsed().as_secs_f64(),
            wall: database::wall_now()?,
        };
        self.steam_logins.poll_live(&state, generation, &clock)?;
        cleanup(&tx, clock.wall)?;
        session(&tx, &owner, &token, clock.wall)?;
        tx.execute("DELETE FROM steam_login_refresh WHERE subject=? AND token_hash NOT IN (SELECT token_hash FROM steam_login_refresh WHERE subject=? ORDER BY expires DESC,rowid DESC LIMIT 3)",[owner.subject(),owner.subject()]).map_err(database::failure)?;
        let count = tx
            .query_row("SELECT count(*) FROM steam_login_refresh", [], |r| {
                r.get::<_, i64>(0)
            })
            .map_err(database::failure)?;
        let remembered = if count < 100000 {
            tx.execute(
                "INSERT INTO steam_login_refresh VALUES(?,?,?,?)",
                rusqlite::params![
                    steam::digest(&refresh),
                    owner.subject(),
                    owner.name(),
                    clock.wall + steam::REFRESH_LIFETIME
                ],
            )
            .map_err(database::failure)?;
            tx.execute(
                "INSERT INTO steam_login_refresh_sessions VALUES(?,?)",
                rusqlite::params![steam::digest(&token), steam::digest(&refresh)],
            )
            .map_err(database::failure)?;
            Some(refresh)
        } else {
            None
        };
        let fresh = Clock {
            monotonic: started.elapsed().as_secs_f64(),
            wall: database::wall_now()?,
        };
        self.steam_logins.poll_live(&state, generation, &fresh)?;
        tx.commit().map_err(database::failure)?;
        self.steam_logins.publish_poll(&state);
        Ok(owner.signed_in(token, remembered))
    }
    pub fn steam_resume(&mut self, header: &str, body: Value) -> Result<Reply> {
        self.steam_enabled()?;
        halo2_online_protocol::matchmaking_model::fields(&body, &[])?;
        let hash = steam::refresh(header)?;
        let token = format!("h2s_{}", steam::random()?);
        let tx = self
            .db
            .transaction_with_behavior(TransactionBehavior::Immediate)
            .map_err(database::failure)?;
        let wall = database::wall_now()?;
        cleanup(&tx, wall)?;
        let row = tx
            .query_row(
                "SELECT subject,name FROM steam_login_refresh WHERE token_hash=? AND expires>?",
                rusqlite::params![hash, wall],
                |r| Ok((r.get::<_, String>(0)?, r.get::<_, String>(1)?)),
            )
            .optional()
            .map_err(database::failure)?
            .ok_or(Error(401, "Saved Steam sign-in expired"))?;
        let owner = Owner::stored(row.0, row.1)
            .map_err(|_| Error(503, "Saved Steam identity unavailable"))?;
        session(&tx, &owner, &token, wall)?;
        tx.execute(
            "INSERT INTO steam_login_refresh_sessions VALUES(?,?)",
            rusqlite::params![steam::digest(&token), hash],
        )
        .map_err(database::failure)?;
        tx.commit().map_err(database::failure)?;
        Ok(owner.signed_in(token, None))
    }
    pub fn steam_forget(&mut self, header: &str, body: Value) -> Result<Reply> {
        halo2_online_protocol::matchmaking_model::fields(&body, &[])?;
        let hash = steam::refresh(header)?;
        let tx = self
            .db
            .transaction_with_behavior(TransactionBehavior::Immediate)
            .map_err(database::failure)?;
        for sql in [
            "DELETE FROM steam_login_refresh WHERE token_hash=?",
            "DELETE FROM steam_login_sessions WHERE token_hash IN (SELECT session_hash FROM steam_login_refresh_sessions WHERE refresh_hash=?)",
            "DELETE FROM steam_login_refresh_sessions WHERE refresh_hash=?",
        ] {
            tx.execute(sql, [&hash]).map_err(database::failure)?;
        }
        tx.commit().map_err(database::failure)?;
        Ok(Reply::json(json!({"state":"signed_out"})))
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::{
        path::PathBuf,
        time::{Duration, SystemTime, UNIX_EPOCH},
    };
    struct Scratch(PathBuf);
    impl Scratch {
        fn new() -> Self {
            let mut bytes = [0; 8];
            getrandom::getrandom(&mut bytes).unwrap();
            let path = PathBuf::from(
                std::env::var_os("HALO2_TEST_SCRATCH").expect("run_test.py storage required"),
            )
            .join(format!("steam-sql-{}", u64::from_le_bytes(bytes)));
            std::fs::create_dir(&path).unwrap();
            Self(path)
        }
    }
    impl Drop for Scratch {
        fn drop(&mut self) {
            let _ = std::fs::remove_dir_all(&self.0);
        }
    }
    fn begun() -> (Runtime, Plan, Value) {
        use base64::Engine as _;
        use sha2::{Digest, Sha256};
        let mut runtime = Runtime::open(Path::new(":memory:"), true, None).unwrap();
        runtime
            .configure_steam("https://steam-fixture.invalid", false)
            .unwrap();
        let hash = |s: &str| {
            base64::engine::general_purpose::URL_SAFE_NO_PAD.encode(Sha256::digest(s.as_bytes()))
        };
        let verifier = "v".repeat(43);
        let started = runtime
            .steam_start(
                json!({"challenge":hash(&verifier),"confirmation":hash("A1B2C3D4E5F6")}),
                "source".into(),
                "prefix".into(),
                || Ok(()),
            )
            .unwrap();
        let device: Value = serde_json::from_str(&started.body).unwrap();
        let state = device["device"].as_str().unwrap();
        let wall = database::wall_now().unwrap();
        let nonce = format!(
            "{}{}",
            chrono::DateTime::from_timestamp(wall, 0)
                .unwrap()
                .format("%Y-%m-%dT%H:%M:%SZ"),
            state
        );
        let values = vec![
            ("openid.ns", steam::NS),
            ("openid.mode", "id_res"),
            ("openid.op_endpoint", steam::OP),
            (
                "openid.claimed_id",
                "https://steamcommunity.com/openid/id/76561197960287930",
            ),
            (
                "openid.identity",
                "https://steamcommunity.com/openid/id/76561197960287930",
            ),
            (
                "openid.return_to",
                &format!("https://steam-fixture.invalid/v1/auth/steam/callback/{state}"),
            ),
            ("openid.response_nonce", &nonce),
            ("openid.assoc_handle", "unit"),
            (
                "openid.signed",
                "op_endpoint,claimed_id,identity,return_to,response_nonce,assoc_handle",
            ),
            ("openid.sig", "private-unit-provider-receipt"),
        ]
        .into_iter()
        .map(|(k, v)| (k.to_owned(), v.to_owned()))
        .collect();
        let Callback::Verify(plan) = runtime.steam_callback(state, values).unwrap() else {
            panic!("Unit callback did not request provider IO")
        };
        (runtime, plan, json!({"device":state,"verifier":verifier}))
    }
    fn confirmed() -> (Runtime, Value) {
        let (mut runtime, plan, body) = begun();
        let reply = runtime
            .steam_verified(
                &plan,
                Owner::stored("76561197960287930".into(), "Fixture".into()).unwrap(),
                &AtomicBool::new(true),
                Instant::now() + Duration::from_secs(10),
            )
            .unwrap();
        let cookie = reply
            .headers
            .iter()
            .find(|(name, _)| *name == "set-cookie")
            .unwrap()
            .1
            .split(';')
            .next()
            .unwrap()
            .strip_prefix("steam-confirm=")
            .unwrap();
        let prior = runtime
            .steam_confirmation(&plan.state, cookie, Some("https://steam-fixture.invalid"))
            .unwrap();
        runtime
            .steam_confirmed(prior, b"code=A1B2C3D4E5F6".to_vec())
            .unwrap();
        (runtime, body)
    }
    #[test]
    fn remembered_quota_does_not_reject_verified_login_or_evict_another_subject() {
        let (mut runtime, body) = confirmed();
        let expires = database::wall_now().unwrap() + steam::REFRESH_LIFETIME;
        runtime.db.execute("WITH RECURSIVE rows(n) AS (VALUES(1) UNION ALL SELECT n+1 FROM rows WHERE n<100000) INSERT INTO steam_login_refresh SELECT printf('%064x',n),printf('%017d',76561197960300000+n),'Foreign',? FROM rows",[expires]).unwrap();
        let reply = runtime.steam_poll(body).unwrap();
        let value: Value = serde_json::from_str(&reply.body).unwrap();
        assert_eq!(value["state"], "signed_in");
        assert!(value.get("refresh_token").is_none());
        assert_eq!(
            runtime
                .db
                .query_row("SELECT count(*) FROM steam_login_refresh", [], |r| r
                    .get::<_, i64>(0))
                .unwrap(),
            100000
        );
        assert!(runtime.steam(value["token"].as_str().unwrap()).is_ok());
    }
    #[test]
    fn active_credential_capacity_retains_confirmed_pending_until_retry() {
        let (mut runtime, body) = confirmed();
        let expires = database::wall_now().unwrap() + steam::SESSION_LIFETIME;
        runtime.db.execute("WITH RECURSIVE rows(n) AS (VALUES(1) UNION ALL SELECT n+1 FROM rows WHERE n<1000) INSERT INTO steam_login_sessions SELECT printf('%064x',n),printf('%017d',76561197960300000+n),'Foreign',? FROM rows",[expires]).unwrap();
        assert_eq!(runtime.steam_poll(body.clone()).err().unwrap().0, 503);
        assert_eq!(
            runtime
                .db
                .query_row("SELECT count(*) FROM steam_login_sessions", [], |r| r
                    .get::<_, i64>(0))
                .unwrap(),
            1000
        );
        runtime.db.execute("DELETE FROM steam_login_sessions WHERE token_hash=(SELECT min(token_hash) FROM steam_login_sessions)",[]).unwrap();
        let reply = runtime.steam_poll(body.clone()).unwrap();
        let value: Value = serde_json::from_str(&reply.body).unwrap();
        assert_eq!(value["state"], "signed_in");
        assert_eq!(runtime.steam_poll(body).err().unwrap().0, 401);
    }
    #[test]
    fn caller_cancellation_and_issuer_change_fence_private_verifier_continuations() {
        for changed in [false, true] {
            let (mut runtime, plan, _) = begun();
            if changed {
                runtime
                    .configure_steam("https://changed.invalid", false)
                    .unwrap();
            }
            let owner = Owner::stored("76561197960287930".into(), "Fixture".into()).unwrap();
            let result = runtime.steam_verified(
                &plan,
                owner,
                &AtomicBool::new(changed),
                Instant::now() + Duration::from_secs(10),
            );
            assert_eq!(result.err().unwrap().0, if changed { 400 } else { 503 });
            assert_eq!(
                runtime
                    .db
                    .query_row("SELECT count(*) FROM steam_login_nonces", [], |r| r
                        .get::<_, i64>(0))
                    .unwrap(),
                0
            );
        }
    }
    #[test]
    fn expired_steam_proof_after_owned_sql_wait_cannot_allocate_a_canonical_player() {
        let scratch = Scratch::new();
        let path = scratch.0.join("registry.sqlite3");
        let mut runtime = Runtime::open(&path, true, None).unwrap();
        let token = format!("h2s_{}", "A".repeat(43));
        let header = format!("Bearer {token}");
        runtime
            .db
            .execute(
                "INSERT INTO steam_login_sessions VALUES(?,?,?,?)",
                rusqlite::params![
                    steam::digest(&token),
                    "76561197960287930",
                    "Steam Fixture",
                    database::wall_now().unwrap() + 3600
                ],
            )
            .unwrap();
        let body = json!({"discord_token":"verified-provider-fixture"});
        runtime.link_discord(&header, body.clone(), None).unwrap();
        runtime.steam(&token).unwrap();
        let (ready, received) = std::sync::mpsc::sync_channel(1);
        let blocker = std::thread::spawn(move || {
            let mut db = Connection::open(path).unwrap();
            let tx = db
                .transaction_with_behavior(TransactionBehavior::Immediate)
                .unwrap();
            let expiry = database::wall_now().unwrap() + 2;
            tx.execute("UPDATE steam_login_sessions SET expires=?", [expiry])
                .unwrap();
            ready.send(expiry).unwrap();
            let deadline =
                UNIX_EPOCH + Duration::from_secs(expiry as u64) + Duration::from_millis(200);
            std::thread::sleep(
                deadline
                    .duration_since(SystemTime::now())
                    .unwrap_or_default(),
            );
            tx.commit().unwrap();
        });
        let expires = received.recv().unwrap();
        assert!(database::wall_now().unwrap() < expires);
        let identity = Identity {
            provider: "discord".into(),
            subject: "123456789012345678".into(),
            username: "retained_pilot".into(),
            steam_digest: None,
        };
        let result = runtime.link_discord(&header, body, Some(identity));
        blocker.join().unwrap();
        let written = runtime
            .db
            .query_row("SELECT count(*) FROM players", [], |r| r.get::<_, i64>(0))
            .unwrap();
        assert_eq!(
            result.err().map(|e| e.0),
            Some(401),
            "Expired proof wrote {written} canonical players after the actual SQL wait"
        );
        assert_eq!(written, 0);
    }
}
