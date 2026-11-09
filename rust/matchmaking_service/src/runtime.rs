//! One exclusive owner for native state, SQL transactions and publication.
use crate::{
    authentication::{self, Identity},
    database::{self, Clock},
};
use halo2_online_protocol::{
    matchmaking_model::{Error, Result},
    matchmaking_owners::Owners,
};
use rusqlite::{Connection, OptionalExtension, TransactionBehavior};
use serde_json::{Value, json};
use std::{path::Path, time::Instant};
#[path = "matchmaking.rs"]
mod matchmaking;
#[path = "search_qos.rs"]
mod search_qos;
pub use search_qos::SocketBinding;
#[path = "eos_binding.rs"]
mod eos_binding;
#[path = "room_io.rs"]
pub(crate) mod room_io;
#[path = "rooms.rs"]
mod rooms;
#[path = "social.rs"]
mod social;
#[path = "social_voice.rs"]
mod social_voice;
pub(crate) use social_voice::{VoiceBinding, VoiceGrant, VoiceKind};
#[path = "friend_cloud_runtime.rs"]
mod friend_cloud;
#[cfg(test)]
#[path = "registration_clock_tests.rs"]
mod registration_clock_tests;
#[cfg(test)]
#[path = "variant_catalog_tests.rs"]
mod variant_catalog_tests;
#[path = "steam_friends.rs"]
mod steam_friends;
pub(crate) use steam_friends::Begin as SteamFriendsBegin;
#[path = "steam_sessions.rs"]
mod steam_sessions;
pub use eos_binding::EosOwner;
pub use room_io::{Binding as RoomBinding, Outbound as RoomOutbound, Queued as RoomQueued};

pub struct Runtime {
    db: Connection,
    owners: Owners,
    accounts: u64,
    profiles: u64,
    friends: u64,
    steam_friends: u64,
    started: Instant,
    steam_enabled: bool,
    queue: Option<u64>,
    catalog_sha256: Option<String>,
    qos_sockets: search_qos::Sockets,
    room_sockets: room_io::Sockets,
    steam_logins: crate::steam_login::Broker,
    friend_cloud_enabled: bool,
    friend_cloud_wake: std::sync::Arc<tokio::sync::Notify>,
    voices: social_voice::Voices,
}
impl Runtime {
    pub fn open(path: &Path, steam_enabled: bool, catalog: Option<&[u8]>) -> Result<Self> {
        let db = Connection::open(path).map_err(database::failure)?;
        db.busy_timeout(std::time::Duration::from_secs(5))
            .map_err(database::failure)?;
        db.execute_batch("PRAGMA foreign_keys=ON;CREATE TABLE IF NOT EXISTS clock_floor(id INTEGER PRIMARY KEY CHECK(id=1),value INTEGER NOT NULL);INSERT OR IGNORE INTO clock_floor VALUES(1,0);CREATE TABLE IF NOT EXISTS steam_login_sessions(token_hash TEXT PRIMARY KEY,subject TEXT NOT NULL,name TEXT NOT NULL,expires INTEGER NOT NULL);").map_err(database::failure)?;
        steam_sessions::schema(&db)?;
        let mut owners = Owners::new();
        let accounts = owners.open(&json!({"accounts":{}}))?;
        let profiles = owners.open(&json!({"profile_cache":{}}))?;
        let friends = owners.open(&json!({"friend_cache":{}}))?;
        let steam_friends = owners.open(&json!({"steam_cache":{"account_source":accounts}}))?;
        let (queue, catalog_sha256) = if let Some(raw) = catalog {
            use sha2::{Digest, Sha256};
            let entries = halo2_online_protocol::matchmaking_catalog::parse(raw)?;
            let mut seeds = [0; 12];
            getrandom::getrandom(&mut seeds)
                .map_err(|_| Error(503, "Matchmaking entropy unavailable"))?;
            let queue=owners.open(&json!({"catalog":entries,"playlists":entries.iter().map(|e|e["policy"].clone()).collect::<Vec<_>>(),
                "lease":90,"proposal_lease":30,"qos_lease":30,"capacity":1000,"search_budget":10000,
                "draw_seed":u32::from_le_bytes(seeds[..4].try_into().unwrap()),"native_seed":u32::from_le_bytes(seeds[4..8].try_into().unwrap())}))?;
            owners.request(queue,&json!({"op":"gather_init","now":0.,"seed":u32::from_le_bytes(seeds[8..].try_into().unwrap())}))?;
            (Some(queue), Some(format!("{:x}", Sha256::digest(raw))))
        } else {
            (None, None)
        };
        let voices = social_voice::Voices::new(&mut owners, accounts, queue)?;
        let mut runtime = Self {
            db,
            owners,
            accounts,
            profiles,
            friends,
            steam_friends,
            started: Instant::now(),
            steam_enabled,
            queue,
            catalog_sha256,
            qos_sockets: search_qos::Sockets::default(),
            room_sockets: room_io::Sockets::default(),
            steam_logins: crate::steam_login::Broker::default(),
            friend_cloud_enabled: false,
            friend_cloud_wake: std::sync::Arc::new(tokio::sync::Notify::new()),
            voices,
        };
        runtime.directory("identity", "schema", None, None, Value::Null, Value::Null)?;
        for mode in [
            "player_schema",
            "clan_schema",
            "result_schema",
            "history_schema",
        ] {
            runtime.plan(json!({"database":{"mode":mode}}))?;
        }
        for family in ["clan", "friends", "profile", "party", "voice"] {
            runtime.directory(family, "schema", None, None, Value::Null, Value::Null)?;
        }
        runtime.directory(
            "friends",
            "cloud_schema",
            None,
            None,
            Value::Null,
            Value::Null,
        )?;
        runtime.voice_schema()?;
        runtime.bind_catalog_revisions()?;
        Ok(runtime)
    }
    fn bind_catalog_revisions(&mut self) -> Result<()> {
        use halo2_online_protocol::matchmaking_catalog::Catalog;
        use sha2::{Digest, Sha256};

        let Some(queue) = self.queue else {
            return Ok(());
        };
        let entries = self
            .owners
            .request(queue, &json!({"op":"catalog_get","id":null,"now":0.}))?;
        let catalog = Catalog::new(&entries)?;
        let tx = self
            .db
            .transaction_with_behavior(TransactionBehavior::Immediate)
            .map_err(database::failure)?;
        tx.execute_batch(
            "CREATE TABLE IF NOT EXISTS matchmaking_catalog_revisions (
                playlist TEXT NOT NULL,
                revision TEXT NOT NULL,
                profile_sha256 TEXT NOT NULL CHECK(length(profile_sha256)=64),
                PRIMARY KEY (playlist, revision)
            );",
        )
        .map_err(database::failure)?;
        for policy in catalog.policies() {
            let playlist = policy["id"].as_str().unwrap();
            let revision = policy["revision"].as_str().unwrap();
            let profile = catalog.profile(&policy)?;
            let pin = format!("{:x}", Sha256::digest(profile.to_string().as_bytes()));
            let previous: Option<String> = tx
                .query_row(
                    "SELECT profile_sha256 FROM matchmaking_catalog_revisions WHERE playlist=? AND revision=?",
                    [playlist, revision],
                    |row| row.get(0),
                )
                .optional()
                .map_err(database::failure)?;
            match previous {
                Some(previous) if previous != pin => {
                    eprintln!(
                        "Rust matchmaking catalog revision changed for {playlist} at {revision}"
                    );
                    return Err(Error(
                        409,
                        "Matchmaking playlist revision has different content",
                    ));
                }
                Some(_) => {}
                None => {
                    tx.execute(
                        "INSERT INTO matchmaking_catalog_revisions VALUES(?,?,?)",
                        [playlist, revision, pin.as_str()],
                    )
                    .map_err(database::failure)?;
                }
            }
        }
        tx.commit().map_err(database::failure)
    }
    fn clock(&self) -> Result<Clock> {
        Ok(Clock {
            monotonic: self.started.elapsed().as_secs_f64(),
            wall: database::wall_now()?,
        })
    }
    fn account(&mut self, op: &str, mut fields: Value, clock: &Clock) -> Result<Value> {
        fields["op"] = json!(op);
        fields["now"] = json!(clock.monotonic);
        fields["rate_now"] = json!(clock.monotonic);
        self.owners.request(self.accounts, &fields)
    }
    fn config(
        &self,
        family: &str,
        mode: &str,
        player: Option<&str>,
        key: Option<&str>,
        body: Value,
        features: Value,
        clock: &Clock,
    ) -> Value {
        let profile = (family == "profile" || family == "history").then_some(self.profiles);
        let friends = (family == "friends").then_some(self.friends);
        json!({"party_directory":{"family":family,"mode":mode,"source":self.queue,"player":player,"wall":clock.wall,"clock":clock.monotonic,
            "account_source":key.map(|_|self.accounts),"account_key":key,"account_now":key.map(|_|clock.monotonic),
            "profile_source":profile,"profile_now":profile.map(|_|clock.monotonic),
            "friend_source":friends,"friend_now":friends.map(|_|clock.monotonic),
            "transaction_owned":true,"friend_cloud":self.friend_cloud_enabled,"body":body,"features":features}})
    }
    fn plan(&mut self, setup: Value) -> Result<Value> {
        self.plan_reply(setup, None, None)
    }
    fn plan_reply(
        &mut self,
        setup: Value,
        view: Option<Value>,
        alive: Option<&std::sync::atomic::AtomicBool>,
    ) -> Result<Value> {
        self.plan_reply_checked(setup, view, alive, None)
    }
    fn plan_reply_checked(
        &mut self,
        mut setup: Value,
        mut view: Option<Value>,
        alive: Option<&std::sync::atomic::AtomicBool>,
        verified: Option<(Identity, Instant)>,
    ) -> Result<Value> {
        let started = self.started;
        let observations = self.room_sockets.observations(self.started);
        let sources = self.sources(&observations);
        let mut pending = None;
        let mut applied_clock = None;
        let result = database::transaction(
            &mut self.owners,
            &mut self.db,
            self.queue,
            |owners, db| {
                // BEGIN IMMEDIATE can wait. No directory, credential expiry or
                // registration clock may use a snapshot from before that wait.
                let clock = Clock {
                    monotonic: started.elapsed().as_secs_f64(),
                    wall: database::wall_now()?,
                };
                if let Some((identity, deadline)) = &verified {
                    if Instant::now() >= *deadline {
                        return Err(Error(503, "Service request expired"));
                    }
                    if let Some(digest) = &identity.steam_digest {
                        let subject=db.query_row("SELECT subject FROM steam_login_sessions WHERE token_hash=? AND expires>?",
                            rusqlite::params![digest,clock.wall],|r|r.get::<_,String>(0)).optional().map_err(database::failure)?;
                        if subject.as_deref() != Some(&identity.subject) {
                            return Err(Error(401, "Steam sign-in expired"));
                        }
                    }
                }
                if alive.is_some_and(|a| !a.load(std::sync::atomic::Ordering::Acquire)) {
                    return Err(Error(503, "Steam request cancelled"));
                }
                for setup in std::iter::once(&mut setup).chain(view.iter_mut()) {
                    if let Some(config) = setup.get_mut("party_directory") {
                        config["clock"] = json!(clock.monotonic);
                        config["wall"] = json!(clock.wall);
                        for (field, source) in [
                            ("account_now", "account_source"),
                            ("profile_now", "profile_source"),
                            ("friend_now", "friend_source"),
                            ("voice_now", "voice_source"),
                            ("voice_account_now", "voice_source"),
                            ("steam_now", "steam_source"),
                        ] {
                            if !config[source].is_null() {
                                config[field] = json!(clock.monotonic);
                            }
                        }
                        if config["family"] == "identity"
                            && matches!(config["mode"].as_str(), Some("link" | "link_preflight"))
                        {
                            config["features"]["registration_now"] = json!(clock.wall);
                        }
                    }
                }
                applied_clock = Some(clock);
                let (id, value) = database::drive(owners, db, setup.clone(), &clock, sources)?;
                pending = Some(id);
                let value = if let Some(view) = &view {
                    let (id, value) = database::drive(owners, db, view.clone(), &clock, sources)?;
                    owners.retire(id);
                    value
                } else {
                    value
                };
                if alive.is_some_and(|a| !a.load(std::sync::atomic::Ordering::Acquire)) {
                    return Err(Error(503, "Steam request cancelled"));
                }
                Ok(value)
            },
        );
        let acknowledgement = if result.is_ok() {
            let clock = Clock {
                monotonic: started.elapsed().as_secs_f64(),
                wall: applied_clock.unwrap().wall,
            };
            database::committed_reply(&mut self.owners, pending.unwrap(), &setup, &clock)
        } else {
            Ok(Value::Null)
        };
        if let Some(id) = pending {
            self.owners.retire(id);
        }
        let value = result?;
        let acknowledgement = acknowledgement?;
        if self.friend_cloud_enabled
            && setup["party_directory"]["family"] == "friends"
            && matches!(
                setup["party_directory"]["mode"].as_str(),
                Some("command" | "cloud_enqueue")
            )
        {
            self.friend_cloud_wake.notify_one();
        }
        Ok(acknowledgement.get("voice").cloned().unwrap_or(value))
    }
    fn sources<'a>(&self, rooms: &'a room_io::Observations) -> database::Sources<'a> {
        database::Sources {
            accounts: self.accounts,
            queue: self.queue,
            rooms,
        }
    }
    fn directory(
        &mut self,
        family: &str,
        mode: &str,
        player: Option<&str>,
        key: Option<&str>,
        body: Value,
        features: Value,
    ) -> Result<Value> {
        let config = self.config(family, mode, player, key, body, features, &self.clock()?);
        self.plan(config)
    }
    pub fn validate_login(&mut self, body: &Value) -> Result<()> {
        let clock = self.clock()?;
        self.account("body", json!({"body":body}), &clock)?;
        Ok(())
    }
    pub fn steam(&self, token: &str) -> Result<Identity> {
        if !self.steam_enabled {
            return Err(Error(503, "Steam sign-in is not configured"));
        }
        let digest = authentication::steam_digest(token)?;
        let row = self
            .db
            .query_row(
                "SELECT subject,name FROM steam_login_sessions WHERE token_hash=? AND expires>?",
                rusqlite::params![digest, self.clock()?.wall],
                |r| Ok((r.get::<_, String>(0)?, r.get::<_, String>(1)?)),
            )
            .optional()
            .map_err(database::failure)?
            .ok_or(Error(401, "Steam sign-in expired"))?;
        Ok(Identity {
            provider: "steam".into(),
            subject: row.0,
            username: row.1,
            steam_digest: Some(digest),
        })
    }
    pub fn login(&mut self, identity: Identity, body: Value) -> Result<Value> {
        let clock = self.clock()?;
        let owner = json!([identity.provider, identity.subject]);
        let mut lookup = self.config(
            "identity",
            "lookup",
            None,
            None,
            owner.clone(),
            Value::Null,
            &clock,
        );
        let mut resolve = self.config(
            "identity",
            "resolve",
            None,
            None,
            json!({"owner":owner,"screen_name":body["screen_name"],"username":identity.username}),
            Value::Null,
            &clock,
        );
        let mut registration = self.config(
            "identity",
            "registration_clock",
            None,
            None,
            Value::Null,
            Value::Null,
            &clock,
        );
        let started = self.started;
        let accounts = self.accounts;
        let observations = self.room_sockets.observations(self.started);
        let sources = self.sources(&observations);
        let cloud_enabled = self.friend_cloud_enabled;
        let mut cloud_pending = None;
        let tx = self
            .db
            .transaction_with_behavior(TransactionBehavior::Immediate)
            .map_err(database::failure)?;
        // BEGIN can wait. Provider expiry keeps its wall clock; registration
        // separately observes the persisted floor inside this transaction.
        let clock = Clock {
            monotonic: started.elapsed().as_secs_f64(),
            wall: database::wall_now()?,
        };
        for setup in [&mut lookup, &mut resolve, &mut registration] {
            setup["party_directory"]["clock"] = json!(clock.monotonic);
            setup["party_directory"]["wall"] = json!(clock.wall);
        }
        // Re-read the actual provider credential in the same SQL transaction
        // that creates/updates its canonical player; a receipt is not authority.
        if let Some(digest) = identity.steam_digest {
            let subject = tx
                .query_row(
                    "SELECT subject FROM steam_login_sessions WHERE token_hash=? AND expires>?",
                    rusqlite::params![digest, clock.wall],
                    |r| r.get::<_, String>(0),
                )
                .optional()
                .map_err(database::failure)?;
            if subject.as_deref() != owner[1].as_str() {
                return Err(Error(401, "Steam sign-in expired"));
            }
        }
        let (id, existing) = database::drive(&mut self.owners, &tx, lookup, &clock, sources)?;
        self.owners.retire(id);
        let plan=self.owners.request(accounts,&json!({"op":"prepare","now":clock.monotonic,"rate_now":clock.monotonic,"body":body,"owner":owner,"existing":existing}))?;
        let result = (|| {
            let (id, registered) =
                database::drive(&mut self.owners, &tx, registration, &clock, sources)?;
            self.owners.retire(id);
            resolve["party_directory"]["wall"] = registered;
            let (id, who) = database::drive(&mut self.owners, &tx, resolve, &clock, sources)?;
            self.owners.retire(id);
            let (id, _) = database::drive(
                &mut self.owners,
                &tx,
                json!({"database":{"mode":"history_identity","player":who}}),
                &clock,
                sources,
            )?;
            self.owners.retire(id);
            if cloud_enabled {
                let mut setup = json!({"party_directory":{"family":"friends","mode":"cloud_enqueue","source":self.queue,"player":who,"wall":clock.wall,"clock":clock.monotonic,
                    "friend_source":self.friends,"friend_now":clock.monotonic,"transaction_owned":true,"friend_cloud":true}});
                setup["party_directory"]["account_source"] = Value::Null;
                let (id, _) =
                    database::drive(&mut self.owners, &tx, setup.clone(), &clock, sources)?;
                cloud_pending = Some((id, setup));
            }
            tx.commit().map_err(database::failure)?;
            if let Some((id, setup)) = &cloud_pending {
                database::committed(&mut self.owners, *id, setup, &clock)?;
            }
            self.owners.request(accounts,&json!({"op":"commit","now":clock.monotonic,"rate_now":clock.monotonic,"permit":plan["permit"],"owner":owner,"player":who}))
        })();
        let _=self.owners.request(accounts,&json!({"op":"abort","now":clock.monotonic,"rate_now":clock.monotonic,"permit":plan["permit"]}));
        if let Some((id, _)) = cloud_pending {
            self.owners.retire(id);
        }
        if result.is_ok() && cloud_enabled {
            self.friend_cloud_wake.notify_one();
        }
        result
    }
    fn session(&mut self, header: &str) -> Result<Value> {
        let clock = self.clock()?;
        self.account("auth", json!({"header":header,"rate":true}), &clock)
    }
    pub fn logout(&mut self, header: &str) -> Result<Value> {
        let auth = self.session(header)?;
        let clock = self.clock()?;
        self.account("revoke", json!({"key":auth["key"]}), &clock)?;
        Ok(json!({"state":"signed_out"}))
    }
    pub fn clan(&mut self, header: &str, body: Option<Value>, features: Value) -> Result<Value> {
        let auth = self.session(header)?;
        let who = auth["player"]
            .as_str()
            .ok_or(Error(503, "Native account player missing"))?;
        let key = auth["key"]
            .as_str()
            .ok_or(Error(503, "Native account key missing"))?;
        if let Some(body) = body {
            self.directory(
                "clan",
                "command",
                Some(who),
                Some(key),
                body,
                features.clone(),
            )?;
        }
        self.directory("clan", "get", Some(who), Some(key), Value::Null, features)
    }
    pub fn profile(&mut self, header: &str, body: Option<Value>) -> Result<Value> {
        let auth = self.session(header)?;
        let who = auth["player"]
            .as_str()
            .ok_or(Error(503, "Native account player missing"))?;
        let key = auth["key"]
            .as_str()
            .ok_or(Error(503, "Native account key missing"))?;
        self.directory(
            "profile",
            if body.is_some() { "update" } else { "get" },
            Some(who),
            Some(key),
            body.unwrap_or(Value::Null),
            Value::Null,
        )
    }
    pub fn history(&mut self, mode: &str, target: Option<&str>, features: Value) -> Result<Value> {
        let mut setup = self.config(
            "history",
            mode,
            None,
            None,
            Value::Null,
            features,
            &self.clock()?,
        );
        setup["party_directory"]["target"] = json!(target);
        self.plan(setup)
    }
    pub fn link_discord(
        &mut self,
        header: &str,
        body: Value,
        verified: Option<Identity>,
    ) -> Result<Value> {
        let clock = self.clock()?;
        let mut setup=self.config("identity",if verified.is_some(){"link"}else{"link_preflight"},None,None,body,
            json!({"header":header,"steam_enabled":self.steam_enabled,"registration_now":clock.wall}),&clock);
        setup["party_directory"]["identity_account_source"] = json!(self.accounts);
        if let Some(identity) = verified {
            setup["party_directory"]["verified_owner"] = json!({"provider":identity.provider,"subject":identity.subject,"username":identity.username});
        }
        self.plan(setup)
    }
    pub fn health(&self) -> Result<Value> {
        self.db
            .query_row("SELECT 1", [], |_| Ok(()))
            .map_err(database::failure)?;
        let mut capabilities = vec![
            "account_sessions",
            "clans",
            "profiles",
            "history",
            "accepted_parties",
            "friends",
            "observed_population",
            "voice_preferences",
            "social_voice",
        ];
        if self.steam_enabled && self.steam_logins.configured() {
            capabilities.push("steam_identity");
        }
        if self.friend_cloud_enabled {
            capabilities.push("friend_storage");
        }
        if self.queue.is_some() {
            capabilities.extend([
                "matchmaking_sessions",
                "playlist_eligibility",
                "party_countdown",
                "queue_control",
                "search_qos",
                "room_descriptors",
                "room_signal_relay",
            ]);
        }
        Ok(
            json!({"status":"ok","ready":false,"component_ready":true,"capabilities":capabilities,"catalog_sha256":self.catalog_sha256,"uptime_seconds":self.started.elapsed().as_secs()}),
        )
    }
}
