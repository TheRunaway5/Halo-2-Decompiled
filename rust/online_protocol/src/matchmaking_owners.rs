//! Trusted IO bridge to the Rust matchmaking owner. No network pointers.
use crate::matchmaking_accounts::Accounts;
use crate::matchmaking_continuity::Continuity;
use crate::matchmaking_coordinator::{self as core, Coordinator};
use crate::matchmaking_friend_cache::Friends;
use crate::matchmaking_model::{Error, Result};
use crate::matchmaking_packet_budget::Budget;
use crate::matchmaking_party_directory::Directory;
use crate::matchmaking_persistence::{Database, DatabaseBank};
use crate::matchmaking_profiles::Profiles;
use crate::matchmaking_social_voice::Relay;
use crate::matchmaking_steam_cache::Cache as SteamCache;
use crate::matchmaking_steam_request::Request as SteamRequest;
use std::collections::{BTreeSet, HashMap};

#[derive(Default)]
pub struct Owners {
    next: u64,
    values: HashMap<u64, Owner>,
}
enum Owner {
    Queue(Box<Coordinator>),
    Database(Box<Database>),
    Bank(Box<DatabaseBank>),
    Continuity(Box<Continuity>),
    Budget(Box<Budget>),
    PartyDirectory(Box<Directory>),
    Accounts(Box<Accounts>),
    Profiles(Box<Profiles>),
    Friends(Box<Friends>),
    Voice(Box<Relay>),
    SteamCache(Box<SteamCache>),
    SteamRequest(Box<SteamRequest>),
}
impl Owner {
    fn execute(&mut self, request: &core::WireValue) -> Result<core::WireValue> {
        match self {
            Self::Queue(value) => value.execute(request),
            Self::Database(value) => value.execute(request),
            Self::Bank(value) => value.execute(request),
            Self::Continuity(value) => value.execute(request),
            Self::Budget(value) => value.execute(request),
            Self::PartyDirectory(value) => value.execute(request),
            Self::Accounts(value) => value.execute(request),
            Self::Profiles(value) => value.execute(request),
            Self::Friends(value) => value.execute(request),
            Self::Voice(value) => value.execute(request),
            Self::SteamCache(value) => value.execute(request),
            Self::SteamRequest(value) => value.execute(request),
        }
    }
}
fn open(owners: &mut Owners, request: &core::WireValue) -> Result<u64> {
    if owners.values.len() >= 4096 {
        return Err(Error(503, "Native owner capacity reached"));
    }
    let owner = if let Some(config) = request.get("steam_request") {
        crate::matchmaking_model::fields(request, &["steam_request"])?;
        if owners
            .values
            .values()
            .filter(|v| matches!(v, Owner::SteamRequest(_)))
            .count()
            >= 128
        {
            return Err(Error(503, "Steam request capacity reached"));
        }
        let lookup = SteamRequest::new(config)?;
        let (cache, account_source, key, player) = lookup.binding();
        let Some(Owner::SteamCache(cache)) = owners.values.get(&cache) else {
            return Err(Error(503, "Steam cache source retired"));
        };
        if cache.accounts() != account_source {
            return Err(Error(409, "Steam source accounts differ"));
        }
        let Some(Owner::Accounts(accounts)) = owners.values.get_mut(&account_source) else {
            return Err(Error(503, "Steam account source retired"));
        };
        if !accounts.binding(
            &key,
            &player,
            crate::matchmaking_model::real(&config["account_now"])?,
        )? {
            return Err(Error(401, "ONLINE session expired; reconnect"));
        }
        Owner::SteamRequest(Box::new(lookup))
    } else if let Some(config) = request.get("steam_cache") {
        crate::matchmaking_model::fields(request, &["steam_cache"])?;
        if owners
            .values
            .values()
            .filter(|v| matches!(v, Owner::SteamCache(_)))
            .count()
            >= 128
        {
            return Err(Error(503, "Steam cache capacity reached"));
        }
        let cache = SteamCache::new(config)?;
        if !matches!(
            owners.values.get(&cache.accounts()),
            Some(Owner::Accounts(_))
        ) {
            return Err(Error(503, "Steam account source retired"));
        }
        Owner::SteamCache(Box::new(cache))
    } else if let Some(config) = request.get("social_voice") {
        crate::matchmaking_model::fields(request, &["social_voice"])?;
        if owners
            .values
            .values()
            .filter(|v| matches!(v, Owner::Voice(_)))
            .count()
            >= 128
        {
            return Err(Error(503, "Voice owner capacity reached"));
        }
        let relay = Relay::new(config)?;
        let (accounts, source) = relay.sources();
        if !matches!(owners.values.get(&accounts), Some(Owner::Accounts(_))) {
            return Err(Error(503, "Voice account source retired"));
        }
        if source.is_some_and(|s| !matches!(owners.values.get(&s), Some(Owner::Queue(_)))) {
            return Err(Error(503, "Voice matchmaking source retired"));
        }
        Owner::Voice(Box::new(relay))
    } else if let Some(friends) = request.get("friend_cache") {
        crate::matchmaking_model::fields(request, &["friend_cache"])?;
        crate::matchmaking_model::fields(friends, &[])?;
        if owners
            .values
            .values()
            .filter(|v| matches!(v, Owner::Friends(_)))
            .count()
            >= 128
        {
            return Err(Error(503, "Friend cache owner capacity reached"));
        }
        Owner::Friends(Box::default())
    } else if let Some(profiles) = request.get("profile_cache") {
        crate::matchmaking_model::fields(request, &["profile_cache"])?;
        crate::matchmaking_model::fields(profiles, &[])?;
        if owners
            .values
            .values()
            .filter(|v| matches!(v, Owner::Profiles(_)))
            .count()
            >= 128
        {
            return Err(Error(503, "Profile owner capacity reached"));
        }
        Owner::Profiles(Box::default())
    } else if let Some(accounts) = request.get("accounts") {
        crate::matchmaking_model::fields(accounts, &[])?;
        Owner::Accounts(Box::default())
    } else if let Some(directory) = request.get("party_directory") {
        if owners
            .values
            .values()
            .filter(|v| matches!(v, Owner::PartyDirectory(_)))
            .count()
            >= 128
        {
            return Err(Error(503, "Party transaction capacity reached"));
        }
        let directory = Directory::new(directory)?;
        if let Some(source) = directory.identity_account_source() {
            if !matches!(owners.values.get(&source), Some(Owner::Accounts(_))) {
                return Err(Error(503, "Canonical account source retired"));
            }
        }
        if let Some(source) = directory.source() {
            if !matches!(owners.values.get(&source), Some(Owner::Queue(_))) {
                return Err(Error(503, "Party source owner retired"));
            }
        }
        if let Some((source, key, player, now)) = directory.account_binding() {
            let Some(Owner::Accounts(accounts)) = owners.values.get_mut(&source) else {
                return Err(Error(503, "Account source owner retired"));
            };
            if !accounts.binding(&key, &player, now)? {
                return Err(Error(401, "ONLINE session expired; reconnect"));
            }
        }
        if let Some(source) = directory.profile_source() {
            let Some(Owner::Profiles(profiles)) = owners.values.get_mut(&source) else {
                return Err(Error(503, "Profile source owner retired"));
            };
            profiles.observe(crate::matchmaking_model::real(
                &request["party_directory"]["profile_now"],
            )?)?;
        }
        if let Some(source) = directory.friend_source() {
            let Some(Owner::Friends(friends)) = owners.values.get_mut(&source) else {
                return Err(Error(503, "Friend cache source retired"));
            };
            friends.observe(crate::matchmaking_model::real(
                &request["party_directory"]["friend_now"],
            )?)?;
        }
        if let Some(voice_source) = directory.voice_source() {
            let Some(Owner::Voice(relay)) = owners.values.get_mut(&voice_source) else {
                return Err(Error(503, "Voice source retired"));
            };
            let (accounts, source) = relay.sources();
            if source != directory.source()
                || directory.account_binding().is_some_and(|a| a.0 != accounts)
            {
                return Err(Error(409, "Voice directory sources differ"));
            }
            relay.observe(crate::matchmaking_model::real(
                &request["party_directory"]["voice_now"],
            )?)?;
        }
        if let Some(source) = directory.steam_source() {
            let Some(Owner::SteamCache(cache)) = owners.values.get_mut(&source) else {
                return Err(Error(503, "Steam source retired"));
            };
            if directory
                .account_binding()
                .is_none_or(|a| a.0 != cache.accounts())
            {
                return Err(Error(409, "Steam directory account source differs"));
            }
            cache.observe(crate::matchmaking_model::real(
                &request["party_directory"]["steam_now"],
            )?)?;
            cache.configure_credential(&request["party_directory"]["steam_credential"])?;
        }
        Owner::PartyDirectory(Box::new(directory))
    } else if let Some(database) = request.get("database") {
        if database["mode"] == "snapshot_bank" {
            Owner::Bank(Box::new(DatabaseBank::new(database)?))
        } else if database["mode"] == "result_commit" {
            use crate::matchmaking_model as m;
            m::fields(database, &["mode", "source", "match", "completed"])?;
            let key = database["source"]
                .as_u64()
                .ok_or(Error(400, "Result source owner required"))?;
            let Some(Owner::Queue(source)) = owners.values.get(&key) else {
                return Err(Error(503, "Result source owner retired"));
            };
            let context = source.result_commit_context(m::text(&database["match"], 64)?)?;
            Owner::Database(Box::new(Database::result(
                context,
                m::integer(&database["completed"], 0, 9007199254740991)?,
            )?))
        } else if database["mode"] == "public_rank" {
            use crate::matchmaking_model as m;
            m::fields(database, &["mode", "source", "playlist", "player", "clan"])?;
            let key = database["source"]
                .as_u64()
                .ok_or(Error(400, "Catalog source owner required"))?;
            let Some(Owner::Queue(source)) = owners.values.get(&key) else {
                return Err(Error(503, "Catalog source owner retired"));
            };
            let clan = if database["clan"].is_null() {
                None
            } else {
                Some(m::text(&database["clan"], 64)?)
            };
            let setup = source.public_rank_setup(
                m::text(&database["playlist"], 64)?,
                m::text(&database["player"], 64)?,
                clan,
            )?;
            Owner::Database(Box::new(Database::public_rank(&setup)?))
        } else {
            Owner::Database(Box::new(Database::new(database)?))
        }
    } else if let Some(budget) = request.get("packet_budget") {
        crate::matchmaking_model::fields(request, &["packet_budget"])?;
        crate::matchmaking_model::fields(budget, &[])?;
        Owner::Budget(Box::default())
    } else if let Some(continuity) = request.get("continuity") {
        crate::matchmaking_model::fields(request, &["continuity"])?;
        Owner::Continuity(Box::new(Continuity::new(continuity)?))
    } else {
        if owners
            .values
            .values()
            .filter(|v| matches!(v, Owner::Queue(_)))
            .count()
            >= 1024
        {
            return Err(Error(503, "Native queue owner capacity reached"));
        }
        Owner::Queue(Box::new(Coordinator::new(request)?))
    };
    let id = owners
        .next
        .checked_add(1)
        .ok_or(Error(503, "Native owner generation exhausted"))?;
    owners.next = id;
    owners.values.insert(id, owner);
    Ok(id)
}

fn execute(owners: &mut Owners, id: u64, request: &core::WireValue) -> Result<core::WireValue> {
    use crate::matchmaking_model as m;
    if matches!(owners.values.get(&id), Some(Owner::SteamRequest(_))) {
        return execute_steam(owners, id, request);
    }
    if request["op"] == "presence" {
        m::fields(
            request,
            &["op", "view", "source", "now", "queue_now", "wall", "rooms"],
        )?;
        let view = m::text(&request["view"], 16)?;
        m::check(
            ["population", "observed", "rooms"].contains(&view),
            "Unknown presence view",
        )?;
        let source = if request["source"].is_null() {
            None
        } else {
            Some(
                request["source"]
                    .as_u64()
                    .ok_or(Error(400, "Presence source owner required"))?,
            )
        };
        if source.is_some_and(|s| !matches!(owners.values.get(&s), Some(Owner::Queue(_)))) {
            return Err(Error(503, "Presence source owner retired"));
        }
        let Some(Owner::Accounts(accounts)) = owners.values.get_mut(&id) else {
            return Err(Error(503, "Presence account owner retired"));
        };
        let (accounts, wall) = accounts.presence(
            m::real(&request["now"])?,
            m::integer(&request["wall"], 0, 9007199254740991)?,
        )?;
        return match source.and_then(|s| owners.values.get_mut(&s)) {
            Some(Owner::Queue(queue)) => queue.presence(
                m::real(&request["queue_now"])?,
                &request["rooms"],
                &accounts,
                wall,
                view,
            ),
            None => {
                m::check(
                    request["rooms"].as_array().is_some_and(|r| r.is_empty()),
                    "Room observations require a native queue",
                )?;
                Coordinator::presence_without_queue(&accounts, wall, view)
            }
            _ => unreachable!(),
        };
    }
    // Acknowledge sealed cache plans and cloud IO claims after SQL commit.
    // All userdata writes were authorized at the final SQL reply; a new bearer
    // check here would falsely reject an already durable transaction.
    if request["op"] == "committed"
        && matches!(owners.values.get(&id), Some(Owner::PartyDirectory(_)))
    {
        m::fields_optional(
            request,
            &["op"],
            &[
                "account_now",
                "profile_now",
                "friend_now",
                "voice_now",
                "voice_account_now",
                "steam_now",
                "steam_credential",
            ],
        )?;
        let Some(Owner::PartyDirectory(directory)) = owners.values.get_mut(&id) else {
            unreachable!();
        };
        let source = directory.take_profile_commit();
        let friend_source = directory.take_friend_commit();
        let voice_source = directory.take_voice_commit();
        let steam_source = directory.take_steam_commit();
        let identity_revoke = directory.take_identity_commit();
        if let Some((source, player)) = identity_revoke {
            let Some(Owner::Accounts(accounts)) = owners.values.get_mut(&source) else {
                return Err(Error(503, "Canonical account source retired"));
            };
            accounts.revoke_canonical_player(&player);
        }
        let profile_changed = match source.and_then(|s| owners.values.get_mut(&s)) {
            Some(Owner::Profiles(profiles)) => {
                profiles.committed(id, m::real(&request["profile_now"]).unwrap_or(-1.))
            }
            _ => false,
        };
        let friend_changed = match friend_source.and_then(|s| owners.values.get_mut(&s)) {
            Some(Owner::Friends(friends)) => {
                friends.committed(id, m::real(&request["friend_now"]).unwrap_or(-1.))
            }
            _ => false,
        };
        if let Some(source) = voice_source {
            observe_voice(owners, source, m::real(&request["voice_now"])?)?;
            let live = voice_live(owners, source, m::real(&request["voice_account_now"])?)?;
            let Some(Owner::Voice(relay)) = owners.values.get_mut(&source) else {
                unreachable!();
            };
            if let Some(value) = relay.committed(id, &live)? {
                return Ok(serde_json::json!({"voice":value}));
            }
        }
        let steam_changed = match steam_source.and_then(|s| owners.values.get_mut(&s)) {
            Some(Owner::SteamCache(cache)) => cache.committed(id),
            _ => false,
        };
        return Ok(core::WireValue::from(
            profile_changed || friend_changed || steam_changed,
        ));
    }
    let binding = match owners.values.get(&id) {
        Some(Owner::PartyDirectory(directory)) => Some((
            directory.account_binding(),
            directory.profile_source(),
            directory.source(),
            directory.friend_source(),
            directory.voice_source(),
            directory.steam_source(),
            directory.identity_account_source(),
        )),
        Some(_) => return owners.values.get_mut(&id).unwrap().execute(request),
        None => return Err(Error(503, "Rust matchmaking owner retired")),
    };
    let (account, profiles, source, friends, voice, steam, identity) = binding.unwrap();
    if identity.is_some_and(|s| !matches!(owners.values.get(&s), Some(Owner::Accounts(_)))) {
        return Err(Error(503, "Canonical account source retired"));
    }
    let attach_binding = account.clone();
    let mut inner = request.clone();
    if let Some((source, key, player, issued)) = account {
        let now = m::real(&request["account_now"])?;
        let Some(Owner::Accounts(accounts)) = owners.values.get_mut(&source) else {
            return Err(Error(503, "Account source owner retired"));
        };
        if !accounts.binding(&key, &player, now.max(issued))? {
            return Err(Error(401, "ONLINE session expired; reconnect"));
        }
        inner
            .as_object_mut()
            .ok_or(Error(400, "Directory request required"))?
            .remove("account_now");
    }
    if let Some(profile_source) = profiles {
        let Some(Owner::Profiles(cache)) = owners.values.get_mut(&profile_source) else {
            return Err(Error(503, "Profile source owner retired"));
        };
        cache.observe(m::real(&request["profile_now"])?)?;
        inner
            .as_object_mut()
            .ok_or(Error(400, "Directory request required"))?
            .remove("profile_now");
    }
    if let Some(source) = source {
        if !matches!(owners.values.get(&source), Some(Owner::Queue(_))) {
            return Err(Error(503, "Party source owner retired"));
        }
    }
    if let Some(friend_source) = friends {
        let Some(Owner::Friends(cache)) = owners.values.get_mut(&friend_source) else {
            return Err(Error(503, "Friend cache source retired"));
        };
        cache.observe(m::real(&request["friend_now"])?)?;
        inner
            .as_object_mut()
            .ok_or(Error(400, "Directory request required"))?
            .remove("friend_now");
    }
    if let Some(voice_source) = voice {
        observe_voice(owners, voice_source, m::real(&request["voice_now"])?)?;
        inner
            .as_object_mut()
            .ok_or(Error(400, "Directory request required"))?
            .remove("voice_now");
        m::real(&request["voice_account_now"])?;
        inner.as_object_mut().unwrap().remove("voice_account_now");
    }
    if let Some(source) = steam {
        let Some(Owner::SteamCache(cache)) = owners.values.get_mut(&source) else {
            return Err(Error(503, "Steam source retired"));
        };
        cache.observe(m::real(&request["steam_now"])?)?;
        cache.configure_credential(&request["steam_credential"])?;
        if !cache.directory_live(id) {
            return Err(Error(403, "Steam invitation proof retired"));
        }
        inner.as_object_mut().unwrap().remove("steam_now");
        inner.as_object_mut().unwrap().remove("steam_credential");
    }
    let mut event = owners.values.get_mut(&id).unwrap().execute(&inner)?;
    for _ in 0..1024 {
        let result = match event["kind"].as_str() {
            Some("core") => {
                let source = source.ok_or(Error(503, "Party source owner required"))?;
                let Some(Owner::Queue(queue)) = owners.values.get_mut(&source) else {
                    return Err(Error(503, "Party source owner retired"));
                };
                queue.execute(&event["request"])?
            }
            Some("profiles") => {
                let source = profiles.ok_or(Error(503, "Profile source owner required"))?;
                let Some(Owner::Profiles(cache)) = owners.values.get_mut(&source) else {
                    return Err(Error(503, "Profile source owner retired"));
                };
                cache.directory(id, &event["request"])?
            }
            Some("friends") => {
                let source = friends.ok_or(Error(503, "Friend cache source required"))?;
                let Some(Owner::Friends(cache)) = owners.values.get_mut(&source) else {
                    return Err(Error(503, "Friend cache source retired"));
                };
                cache.directory(id, &event["request"])?
            }
            Some("voice") => {
                let source = voice.ok_or(Error(503, "Voice source required"))?;
                let live = voice_live(owners, source, m::real(&request["voice_account_now"])?)?;
                let Some(Owner::Voice(relay)) = owners.values.get_mut(&source) else {
                    return Err(Error(503, "Voice source retired"));
                };
                let admitted = attach_binding
                    .as_ref()
                    .is_some_and(|(accounts, key, player, _)| {
                        *accounts == relay.sources().0
                            && event["request"]["key"] == *key
                            && event["request"]["player"] == *player
                    });
                relay.directory(id, &event["request"], &live, admitted)?
            }
            Some("steam") => {
                let source = steam.ok_or(Error(403, "Native Steam proof source required"))?;
                let Some(Owner::SteamCache(cache)) = owners.values.get_mut(&source) else {
                    return Err(Error(503, "Steam source retired"));
                };
                cache.directory(id, &event["request"])?
            }
            _ => return Ok(event),
        };
        let mut continuation = core::WireValue::Object(Default::default());
        continuation["op"] = core::WireValue::from("reply");
        continuation["frame"] = event["frame"].clone();
        continuation["value"] = result;
        event = owners.values.get_mut(&id).unwrap().execute(&continuation)?;
    }
    Err(Error(503, "Party owner continuation exceeds bounds"))
}

fn execute_steam(
    owners: &mut Owners,
    id: u64,
    request: &core::WireValue,
) -> Result<core::WireValue> {
    use crate::matchmaking_model as m;
    let Some(Owner::SteamRequest(lookup)) = owners.values.get_mut(&id) else {
        return Err(Error(503, "Steam request retired"));
    };
    let (cache, account_source, key, player) = lookup.binding();
    lookup.observe(m::real(&request["now"])?, &request["credential"])?;
    let now = lookup.now();
    let Some(Owner::Accounts(accounts)) = owners.values.get_mut(&account_source) else {
        return Err(Error(503, "Steam account source retired"));
    };
    if !accounts.binding(&key, &player, m::real(&request["account_now"])?)? {
        return Err(Error(401, "ONLINE session expired; reconnect"));
    }
    if !matches!(owners.values.get(&cache), Some(Owner::SteamCache(_))) {
        return Err(Error(503, "Steam cache source retired"));
    }
    let mut inner = request.clone();
    for field in ["now", "account_now", "credential"] {
        inner
            .as_object_mut()
            .ok_or(Error(400, "Steam request required"))?
            .remove(field);
    }
    let mut event = owners.values.get_mut(&id).unwrap().execute(&inner)?;
    for _ in 0..32 {
        if event["kind"] != "steam_cache" {
            return Ok(event);
        }
        let Some(Owner::SteamCache(cache)) = owners.values.get_mut(&cache) else {
            return Err(Error(503, "Steam cache source retired"));
        };
        cache.observe(now)?;
        let reply = cache.request(id, &event["request"])?;
        event = owners
            .values
            .get_mut(&id)
            .unwrap()
            .execute(&serde_json::json!({"op":"reply","frame":event["frame"],"value":reply}))?;
    }
    Err(Error(503, "Steam continuation exceeds bounds"))
}

fn observe_voice(owners: &mut Owners, source: u64, now: f64) -> Result<()> {
    let Some(Owner::Voice(relay)) = owners.values.get_mut(&source) else {
        return Err(Error(503, "Voice source retired"));
    };
    relay.observe(now)
}
fn voice_live(owners: &mut Owners, source: u64, now: f64) -> Result<BTreeSet<String>> {
    let Some(Owner::Voice(relay)) = owners.values.get(&source) else {
        return Err(Error(503, "Voice source retired"));
    };
    let (account_source, core_source) = relay.sources();
    let bindings = relay.bindings();
    if core_source.is_some_and(|s| !matches!(owners.values.get(&s), Some(Owner::Queue(_)))) {
        return Err(Error(503, "Voice matchmaking source retired"));
    }
    let Some(Owner::Accounts(accounts)) = owners.values.get_mut(&account_source) else {
        return Err(Error(503, "Voice account source retired"));
    };
    let mut live = BTreeSet::new();
    for (peer, key, player) in bindings {
        if accounts.binding(&key, &player, now)? {
            live.insert(peer);
        }
    }
    Ok(live)
}

fn retire(owners: &mut Owners, id: u64) {
    match owners.values.remove(&id) {
        Some(Owner::PartyDirectory(directory)) => {
            if let Some(source) = directory.profile_source() {
                if let Some(Owner::Profiles(profiles)) = owners.values.get_mut(&source) {
                    profiles.abort(id);
                }
            }
            if let Some(source) = directory.friend_source() {
                if let Some(Owner::Friends(friends)) = owners.values.get_mut(&source) {
                    friends.abort(id);
                }
            }
            if let Some(source) = directory.voice_source() {
                if let Some(Owner::Voice(relay)) = owners.values.get_mut(&source) {
                    relay.abort(id);
                }
            }
            if let Some(source) = directory.steam_source() {
                if let Some(Owner::SteamCache(cache)) = owners.values.get_mut(&source) {
                    cache.abort_directory(id);
                }
            }
        }
        Some(Owner::SteamRequest(lookup)) => {
            if let Some(Owner::SteamCache(cache)) = owners.values.get_mut(&lookup.binding().0) {
                cache.abort_request(id);
            }
        }
        _ => {}
    }
}

/// Private, safe Rust access to the same typed owners used by the native ABI.
/// IDs and constructors are trusted in-process values, never a public RPC.
impl Owners {
    pub fn new() -> Self {
        Self::default()
    }
    pub fn open(&mut self, request: &core::WireValue) -> Result<u64> {
        open(self, request)
    }
    pub fn request(&mut self, id: u64, request: &core::WireValue) -> Result<core::WireValue> {
        execute(self, id, request)
    }
    /// Keep native queue publication atomic with an enclosing storage commit.
    /// The caller holds exclusive access and must return success only after
    /// COMMIT. A rejection restores the complete queue, including private
    /// session/consent/RNG/continuation state; no provisional reply may escape.
    /// Other owners, accounting and handle generations are not rewound.
    pub fn queue_transaction<T>(
        &mut self,
        id: u64,
        operation: impl FnOnce(&mut Self) -> Result<T>,
    ) -> Result<T> {
        let checkpoint = match self.values.get(&id) {
            Some(Owner::Queue(queue)) => queue.clone(),
            _ => return Err(Error(503, "Actual queue transaction source required")),
        };
        let result = operation(self);
        if result.is_err() {
            self.values.insert(id, Owner::Queue(checkpoint));
        }
        result
    }
    pub fn retire(&mut self, id: u64) {
        retire(self, id)
    }
    pub fn len(&self) -> usize {
        self.values.len()
    }
    pub fn is_empty(&self) -> bool {
        self.values.is_empty()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use serde_json::{Value, json};
    #[test]
    fn rejected_storage_commit_restores_native_sessions_and_rng() {
        let entries=crate::matchmaking_catalog::parse(json!({"version":1,"playlists":[{
            "id":"native","revision":"v1","name":"Native","description":"Synthetic transaction control",
            "ranked":false,"quickmatch":true,"team_size":0,"minimum_players":2,"maximum_players":4,"maximum_party":4,
            "games":[{"id":"slayer","map":"lockout","variant":"Slayer","weight":1}],
            "stages":[{"after_seconds":0,"rank_numerator":1,"rank_denominator":1,"minimum_connection":50}],
            "skill":{"kind":"custom","radius":6},"provenance":"Synthetic; no original service claim"}]}).to_string().as_bytes()).unwrap();
        let mut owners = Owners::new();
        let queue=owners.open(&json!({"catalog":entries,"playlists":entries.iter().map(|e|e["policy"].clone()).collect::<Vec<_>>(),
            "lease":90,"proposal_lease":30,"qos_lease":30,"capacity":1000,"search_budget":10000,"draw_seed":2,"native_seed":3})).unwrap();
        let admission = json!({"op":"session_register","now":0.,"body":{"screen_name":"Pilot","maps":["lockout"],"compatibility":"halo2-native-queue-v16","language":0},
            "identities":[{"owner":["steam","fixture"],"player":"player","separate_party":false}],"resolved":["player"],"rooms":[]});
        let first = owners.request(queue, &admission).unwrap();
        let mut provisional = Value::Null;
        let failed: Result<()> = owners.queue_transaction(queue, |owners| {
            owners.request(
                queue,
                &json!({"op":"setting_set","now":0.,"name":"draw_seed","value":99}),
            )?;
            provisional = owners.request(queue, &admission)?;
            Err(Error(500, "Synthetic failed COMMIT"))
        });
        assert_eq!(failed.unwrap_err().0, 500);
        let auth = |token: &Value| json!({"op":"session_auth","now":0.,"header":format!("Bearer {}",token.as_str().unwrap())});
        assert!(
            owners
                .request(queue, &auth(&first["response"]["token"]))
                .is_ok()
        );
        assert_eq!(
            owners
                .request(queue, &auth(&provisional["response"]["token"]))
                .unwrap_err()
                .0,
            401
        );
        assert_eq!(
            owners
                .request(
                    queue,
                    &json!({"op":"setting_get","now":0.,"name":"draw_seed"})
                )
                .unwrap(),
            2
        );
        let accounts = owners.open(&json!({"accounts":{}})).unwrap();
        assert_eq!(
            owners
                .queue_transaction(accounts, |_| Ok(()))
                .unwrap_err()
                .0,
            503
        );
        let committed = owners
            .queue_transaction(queue, |owners| owners.request(queue, &admission))
            .unwrap();
        assert!(
            owners
                .request(queue, &auth(&committed["response"]["token"]))
                .is_ok()
        );
        assert_eq!(
            owners
                .request(queue, &auth(&first["response"]["token"]))
                .unwrap_err()
                .0,
            401
        );
    }
    fn grant(owners: &mut Owners, account: u64, player: &str) -> String {
        let owner = json!(["fixture", player]);
        let plan=owners.request(account,&json!({"op":"prepare","now":0.,"rate_now":0.,"body":{"screen_name":"Pilot"},"owner":owner,"existing":null})).unwrap();
        owners.request(account,&json!({"op":"commit","now":0.,"rate_now":0.,"permit":plan["permit"],"owner":owner,"player":player})).unwrap()["token"].as_str().unwrap().to_owned()
    }
    #[test]
    fn independent_registries_own_their_grants_and_typed_source_lifetimes() {
        let mut a = Owners::new();
        let mut b = Owners::new();
        let aa = a.open(&json!({"accounts":{}})).unwrap();
        let bb = b.open(&json!({"accounts":{}})).unwrap();
        let token_a = grant(&mut a, aa, "a");
        let token_b = grant(&mut b, bb, "b");
        let auth = |token: &str| json!({"op":"auth","now":0.,"rate_now":0.,"header":format!("Bearer {token}"),"rate":false});
        assert_eq!(b.request(bb, &auth(&token_a)).unwrap_err().0, 401);
        assert_eq!(a.request(aa, &auth(&token_b)).unwrap_err().0, 401);
        assert_eq!(b.request(bb, &auth(&token_b)).unwrap()["player"], "b");
        let profile = a.open(&json!({"profile_cache":{}})).unwrap();
        assert_eq!(
            a.open(&json!({"steam_cache":{"account_source":profile}}))
                .unwrap_err()
                .0,
            503
        );
        a.retire(aa);
        assert_eq!(a.request(aa, &auth(&token_a)).unwrap_err().0, 503);
        assert_eq!(b.request(bb, &auth(&token_b)).unwrap()["player"], "b");
    }
}
