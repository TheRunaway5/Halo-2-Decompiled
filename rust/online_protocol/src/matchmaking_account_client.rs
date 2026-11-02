//! Native account/accepted-party client authority. Provider, HTTP, entropy,
//! clock, native mutex and wake operations are IO; no UI snapshot holds grants.
//! This preserves the existing replacement service contract, not Xbox Live wire.
use crate::{matchmaking_json, matchmaking_origin, matchmaking_steam_request};
use serde_json::{Value, json};
use std::collections::BTreeSet;
use std::sync::Mutex;

const SESSION: &str = "/v1/account/session";
const PARTY: &str = "/v1/party/commands";
const FRIENDS: &str = "/v1/friends/commands";
const SECOND: i64 = 1_000_000_000;

#[derive(Clone, Debug, PartialEq, Eq)]
pub enum Error {
    Invalid,
    Transport(String),
    Http(String),
}
impl std::fmt::Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::Invalid => f.write_str("Invalid ONLINE response"),
            Self::Transport(s) | Self::Http(s) => f.write_str(s),
        }
    }
}
pub type Result<T> = std::result::Result<T, Error>;
fn need(value: bool) -> Result<()> {
    if value { Ok(()) } else { Err(Error::Invalid) }
}
fn field<'a>(v: &'a Value, key: &str) -> Result<&'a Value> {
    v.as_object().and_then(|v| v.get(key)).ok_or(Error::Invalid)
}
fn object(v: &Value, size: usize) -> Result<()> {
    need(v.as_object().is_some_and(|v| v.len() == size))
}
fn text(v: &Value, key: &str, maximum: usize, empty: bool) -> Result<String> {
    let s = field(v, key)?.as_str().ok_or(Error::Invalid)?;
    need((empty || !s.is_empty()) && s.len() <= maximum && !s.bytes().any(|c| c < 32 || c == 127))?;
    Ok(s.into())
}
fn flag(v: &Value, key: &str) -> Result<bool> {
    field(v, key)?.as_bool().ok_or(Error::Invalid)
}
fn number(v: &Value, key: &str, low: u64, high: u64) -> Result<u64> {
    let n = field(v, key)?.as_u64().ok_or(Error::Invalid)?;
    need((low..=high).contains(&n))?;
    Ok(n)
}
fn array(v: &Value) -> Result<&Vec<Value>> {
    v.as_array().ok_or(Error::Invalid)
}
fn version(v: &Value) -> Result<u64> {
    number(v, "version", 1, i64::MAX as u64)
}
fn depth(v: &Value, level: usize) -> Result<()> {
    need(level <= 16)?;
    match v {
        Value::Array(a) => {
            for child in a {
                depth(child, level + 1)?
            }
        }
        Value::Object(m) => {
            for child in m.values() {
                depth(child, level + 1)?
            }
        }
        _ => {}
    }
    Ok(())
}
fn parse(body: &str, limit: usize) -> Result<Value> {
    let value = matchmaking_json::parse(body.as_bytes(), limit).map_err(|_| Error::Invalid)?;
    depth(&value, 0)?;
    Ok(value)
}
pub fn empty_snapshot() -> Value {
    json!({"authenticated":false,"busy":false,"message":"","clan":null,"invitations":[],"revision":0,
    "party":null,"party_invitations":[],"party_message":"","party_command_serial":0,"party_command_successful":null,
    "profile_message":"","steam_friends":[],"steam_friends_state":"","steam_friends_message":"","friend_tag":"",
    "friends_message":"","friends_available":false,"friends":[],"friend_search":null})
}

fn invites(v: &Value) -> Result<Value> {
    let rows = array(v)?;
    need(rows.len() <= 100)?;
    let mut ids = BTreeSet::new();
    let mut out = Vec::new();
    for row in rows {
        object(row, 2)?;
        let id = text(row, "id", 36, false)?;
        need(ids.insert(id.clone()))?;
        out.push(json!({"id":id,"name":text(row,"name",64,false)?}));
    }
    Ok(json!(out))
}
pub fn social(root: &Value, snapshot: &Value) -> Result<Value> {
    object(root, 2)?;
    let mut next = snapshot.clone();
    next["invitations"] = invites(field(root, "invitations")?)?;
    next["clan"] = Value::Null;
    let clan = field(root, "clan")?;
    if clan.is_null() {
        return Ok(next);
    }
    object(clan, 6)?;
    let role = number(clan, "role", 0, 3)?;
    let rows = array(field(clan, "members")?)?;
    need(!rows.is_empty() && rows.len() <= 100)?;
    let (mut selves, mut leaders) = (0, 0);
    let mut ids = BTreeSet::new();
    let mut members = Vec::new();
    for row in rows {
        object(row, 4 + usize::from(row.get("presence").is_some()))?;
        let id = text(row, "id", 36, false)?;
        need(ids.insert(id.clone()))?;
        let member_role = number(row, "role", 0, 3)?;
        let is_self = flag(row, "self")?;
        if is_self {
            selves += 1;
            need(member_role == role)?
        }
        if member_role == 3 {
            leaders += 1
        }
        let presence = if row.get("presence").is_some() {
            match text(row, "presence", 16, false)?.as_str() {
                "offline" => 1,
                "online" => 2,
                "playing" => 3,
                _ => return Err(Error::Invalid),
            }
        } else {
            0
        };
        members.push(json!({"id":id,"name":text(row,"name",64,false)?,"role":member_role,"self":is_self,"presence":presence}));
    }
    need(selves == 1 && leaders >= 1)?;
    let outgoing = invites(field(clan, "outgoing")?)?;
    let messages = array(field(clan, "messages")?)?;
    need(messages.len() <= 20)?;
    let mut parsed = Vec::new();
    for row in messages {
        object(row, 3)?;
        parsed.push(json!({"id":text(row,"id",36,false)?,"name":text(row,"name",64,false)?,"body":text(row,"body",256,false)?}));
    }
    next["clan"] = json!({"id":text(clan,"id",36,false)?,"name":text(clan,"name",15,false)?,"role":role,"members":members,"outgoing":outgoing,"messages":parsed});
    Ok(next)
}
pub fn valid_friend_tag(tag: &str) -> bool {
    tag.rfind('#').is_some_and(|at| {
        at > 0
            && tag.len() - at == 9
            && tag.len() <= 73
            && tag.as_bytes()[at + 1..].iter().all(u8::is_ascii_digit)
    })
}
fn friend(row: &Value, relationship: bool) -> Result<Value> {
    let id = text(row, "id", 36, false)?;
    let name = text(row, "name", 64, false)?;
    let tag = text(row, "friend_tag", 73, false)?;
    need(
        id.len() == 36
            && valid_friend_tag(&tag)
            && tag == format!("{}{}", name, &tag[tag.rfind('#').unwrap()..]),
    )?;
    let presence = text(row, "presence", 16, false)?;
    need(matches!(
        presence.as_str(),
        "offline" | "online" | "playing"
    ))?;
    let state = if relationship {
        let s = text(row, "state", 16, false)?;
        need(matches!(
            s.as_str(),
            "friend" | "incoming" | "outgoing" | "blocked"
        ))?;
        s
    } else {
        String::new()
    };
    let joinable = if row.get("party_joinable").is_some() {
        let b = flag(row, "party_joinable")?;
        need(!b || (relationship && state == "friend" && presence != "offline"))?;
        Some(b)
    } else {
        None
    };
    let context = if row.get("party_context").is_some_and(|v| !v.is_null()) {
        let s = text(row, "party_context", 36, false)?;
        need(s.len() == 36 && relationship && state == "friend" && presence != "offline")?;
        s
    } else {
        String::new()
    };
    need(joinable != Some(true) || !context.is_empty())?;
    Ok(
        json!({"id":id,"name":name,"tag":tag,"presence":presence,"state":state,"party_joinable":joinable,"party_context":context}),
    )
}
pub fn friends(root: &Value, snapshot: &Value) -> Result<Value> {
    let mut next = snapshot.clone();
    let tag = text(root, "friend_tag", 73, false)?;
    need(valid_friend_tag(&tag))?;
    let rows = array(field(root, "friends")?)?;
    need(rows.len() <= 250)?;
    let mut ids = BTreeSet::new();
    let mut out = Vec::new();
    for row in rows {
        let value = friend(row, true)?;
        need(ids.insert(value["id"].clone().as_str().unwrap().to_owned()))?;
        out.push(value)
    }
    next["friend_tag"] = json!(tag);
    next["friends"] = json!(out);
    next["friends_available"] = json!(true);
    next["friends_message"] = json!("");
    if let Some(row) = root.get("search_result") {
        next["friend_search"] = friend(row, false)?
    }
    Ok(next)
}
pub fn countdown(v: &Value) -> Result<Value> {
    if v.is_null() {
        return Ok(Value::Null);
    }
    object(v, 5)?;
    let serial = number(v, "serial", 1, i64::MAX as u64)?;
    let seconds = if field(v, "seconds")?.is_null() {
        None
    } else {
        Some(number(v, "seconds", 0, 10)?)
    };
    let mode = number(v, "mode", 0, 1)?;
    let complete = flag(v, "complete")?;
    let requester = if field(v, "requester")?.is_null() {
        String::new()
    } else {
        text(v, "requester", 36, false)?
    };
    need(if mode == 1 {
        !requester.is_empty()
    } else {
        requester.is_empty()
    })?;
    need(if complete {
        seconds.is_none_or(|s| s == 0)
    } else {
        seconds.is_some_and(|s| s > 0)
    })?;
    Ok(
        json!({"serial":serial,"seconds":seconds,"mode":mode,"complete":complete,"requester":requester}),
    )
}
fn appearance(row: &Value) -> Result<Value> {
    if row.get("appearance").is_none_or(Value::is_null) {
        return Ok(Value::Null);
    }
    let encoded = text(row, "appearance", 18, false)?;
    need(encoded.len() == 18)?;
    let nibble = |c: u8| -> Result<u8> {
        match c {
            b'0'..=b'9' => Ok(c - b'0'),
            b'a'..=b'f' => Ok(c - b'a' + 10),
            _ => Err(Error::Invalid),
        }
    };
    let mut bytes = Vec::new();
    for pair in encoded.as_bytes().chunks_exact(2) {
        bytes.push(nibble(pair[0])? * 16 + nibble(pair[1])?)
    }
    need(bytes[4] <= 1)?;
    Ok(json!(bytes))
}
pub fn party(root: &Value, snapshot: &Value) -> Result<Value> {
    object(root, 2)?;
    let mut next = snapshot.clone();
    next["party"] = Value::Null;
    let rows = array(field(root, "invitations")?)?;
    need(rows.len() <= 100)?;
    let mut ids = BTreeSet::new();
    let mut invitations = Vec::new();
    for row in rows {
        object(row, 3)?;
        let id = text(row, "id", 36, false)?;
        need(ids.insert(id.clone()))?;
        invitations.push(json!({"id":id,"name":text(row,"name",64,false)?,"version":version(row)?}))
    }
    next["party_invitations"] = json!(invitations);
    let p = field(root, "party")?;
    if p.is_null() {
        return Ok(next);
    }
    object(
        p,
        4 + usize::from(p.get("countdown").is_some()) + usize::from(p.get("privacy").is_some()),
    )?;
    let ver = version(p)?;
    let leader = flag(p, "leader")?;
    let privacy = if let Some(v) = p.get("privacy") {
        object(v, 3)?;
        let context = text(v, "context", 36, false)?;
        need(context.len() == 36)?;
        json!({"mode":number(v,"mode",0,2)?,"revision":number(v,"revision",1,i64::MAX as u64)?,"context":context})
    } else {
        Value::Null
    };
    let cd = p
        .get("countdown")
        .map(countdown)
        .transpose()?
        .unwrap_or(Value::Null);
    let rows = array(field(p, "members")?)?;
    need(!rows.is_empty() && rows.len() <= 16)?;
    let (mut selves, mut leaders) = (0, 0);
    ids.clear();
    let mut members = Vec::new();
    for row in rows {
        object(
            row,
            6 + usize::from(row.get("local_players").is_some())
                + usize::from(row.get("appearance").is_some())
                + usize::from(row.get("postgame").is_some()),
        )?;
        let handle = text(row, "handle", 36, false)?;
        need(ids.insert(handle.clone()))?;
        let is_self = flag(row, "self")?;
        let is_leader = flag(row, "leader")?;
        if is_self {
            selves += 1;
            need(is_leader == leader && (privacy.is_null() || privacy["context"] == handle))?
        }
        if is_leader {
            leaders += 1
        }
        let mut locals = Vec::new();
        if let Some(rows) = row.get("local_players") {
            let rows = array(rows)?;
            need(rows.len() <= 3)?;
            for local in rows {
                object(local, 2 + usize::from(local.get("appearance").is_some()))?;
                locals.push(json!({"name":text(local,"name",64,false)?,"guest":flag(local,"guest")?,"appearance":appearance(local)?}));
            }
        }
        let postgame = if row.get("postgame").is_some() {
            Some(flag(row, "postgame")?)
        } else {
            None
        };
        members.push(json!({"handle":handle,"name":text(row,"name",64,false)?,"ready_playlist":text(row,"ready_playlist",64,true)?,"self":is_self,
            "leader":is_leader,"online":flag(row,"online")?,"appearance":appearance(row)?,"postgame":postgame,"local_players":locals}));
    }
    need(selves == 1 && leaders == 1)?;
    if !cd.is_null() && cd["requester"].as_str().is_some_and(|s| !s.is_empty()) {
        need(
            members
                .iter()
                .any(|m| m["handle"] == cd["requester"] && m["leader"] == false),
        )?;
    }
    let outgoing = invites(field(p, "outgoing")?)?;
    need(leader || outgoing.as_array().unwrap().is_empty())?;
    next["party"] = json!({"version":ver,"leader":leader,"privacy":privacy,"countdown_supported":p.get("countdown").is_some(),"countdown":cd,"members":members,"outgoing":outgoing});
    Ok(next)
}
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct Link {
    pub party_id: String,
    pub join_secret: String,
    pub version: u64,
    pub current_size: u32,
    pub max_size: u32,
    pub expires: i64,
}
fn valid_secret(s: &str) -> bool {
    s.len() == 48
        && s.starts_with("h2p1_")
        && s.as_bytes()[5..]
            .iter()
            .all(|c| c.is_ascii_alphanumeric() || *c == b'_' || *c == b'-')
}
fn party_link(v: &Value, p: &Value, now: i64) -> Result<Link> {
    object(v, 6)?;
    let id = text(v, "party_id", 36, false)?;
    let secret = text(v, "join_secret", 48, false)?;
    need(id.len() == 36 && valid_secret(&secret))?;
    let version = version(v)?;
    let current = number(v, "current_size", 1, 15)? as u32;
    let max = number(v, "max_size", 16, 16)? as u32;
    need(
        p["version"] == version
            && p["members"]
                .as_array()
                .is_some_and(|m| m.len() == current as usize),
    )?;
    let seconds = number(v, "expires_in", 1, 600)? as i64;
    Ok(Link {
        party_id: id,
        join_secret: secret,
        version,
        current_size: current,
        max_size: max,
        expires: now.checked_add(seconds * SECOND).ok_or(Error::Invalid)?,
    })
}
#[derive(Clone, Debug, Default)]
pub struct Config {
    pub input_valid: bool,
    pub endpoint: String,
    pub url: matchmaking_origin::UrlParts,
    pub loopback: bool,
    pub login_present: bool,
    pub steam: bool,
    pub poll_ms: i64,
    pub parties: bool,
    pub profile_only: bool,
}
#[derive(Clone, Debug, Default)]
pub struct Identity {
    pub bearer: String,
    pub subject: String,
}
#[derive(Clone, Debug, Default)]
pub struct Reply {
    pub status: i64,
    pub body: String,
}
#[derive(Clone, Debug, Default)]
pub struct Operation {
    pub serial: u64,
    pub pending: bool,
    pub successful: bool,
    pub link: Option<Link>,
}
#[derive(Clone, Debug, Default)]
pub struct PartyCommand {
    pub action: String,
    pub value: String,
    pub playlist: String,
    pub revision: String,
    pub version: u64,
    pub privacy: Option<u32>,
    pub privacy_revision: u64,
    pub privacy_context: String,
    pub join_context: String,
}
#[derive(Clone, Debug, Default)]
pub struct Voice {
    pub origin: String,
    pub bearer: String,
    pub channel: String,
    pub player: String,
    pub loopback: bool,
    pub party: bool,
}

/// Public command callers already hold the actual native generation mutex.
/// poll() explicitly acquires/releases that same mutex around capture/publish.
/// No Rust data guard spans a provider/HTTP/clock/native-mutex callback.
pub trait Io {
    fn lock(&self) -> Result<()>;
    fn unlock(&self) -> Result<()>;
    fn now(&self) -> i64;
    fn identity(&self) -> Result<Option<Identity>>;
    fn request(
        &self,
        method: &str,
        url: &str,
        body: &str,
        bearer: &str,
        cleanup: bool,
    ) -> Result<Reply>;
    fn cancelled(&self) -> bool;
    fn reset_cancellation(&self) -> Result<()>;
    fn cancel(&self);
    fn wake(&self);
    fn entropy(&self) -> Result<[u8; 16]>;
    fn eos_profiles(&self) -> Result<bool>;
    fn eos_identity(&self, token: &str) -> Result<(String, String)>;
}
fn uuid(io: &impl Io) -> Result<String> {
    let mut bytes = io.entropy()?;
    bytes[6] = (bytes[6] & 15) | 64;
    bytes[8] = (bytes[8] & 63) | 128;
    let mut out = String::new();
    for (i, b) in bytes.into_iter().enumerate() {
        if matches!(i, 4 | 6 | 8 | 10) {
            out.push('-')
        }
        out.push_str(&format!("{b:02x}"))
    }
    Ok(out)
}
#[derive(Clone)]
struct Data {
    state: Value,
    configured: bool,
    active: bool,
    pending: bool,
    fetch_steam: bool,
    name: String,
    profile: Option<Value>,
    profile_serial: u64,
    voice_bearer: String,
    voice_expires: i64,
    subject: String,
    command: Option<Value>,
    command_subject: String,
    path: String,
    operation: Operation,
    operation_subject: String,
    link: Option<Link>,
    link_subject: String,
    next_operation: u64,
    queued_operation: u64,
    queued_issuer: bool,
    generation: u64,
}
#[derive(Default)]
struct Worker {
    session: String,
    name: String,
    subject: String,
    sent_profile: u64,
    attempted_profile: u64,
    next_profile: i64,
    linked_session: String,
    next_identity: i64,
    next_social: i64,
    expires: i64,
}
pub struct Client {
    config: Config,
    data: Mutex<Data>,
    worker: Mutex<Option<Worker>>,
}
fn native_locked<T>(io: &impl Io, action: impl FnOnce() -> Result<T>) -> Result<T> {
    io.lock()?;
    let value = action();
    let unlocked = io.unlock();
    value.and_then(|v| unlocked.map(|_| v))
}
fn cleanup(w: &mut Worker, c: &Config, io: &impl Io) {
    if !w.session.is_empty() {
        retire_grant(&w.session, c, io);
    }
    w.session.clear();
    w.name.clear();
    w.subject.clear();
}
fn retire_grant(token: &str, c: &Config, io: &impl Io) {
    // This exchange owns the returned opaque grant, even if its generation was
    // cancelled. Cleanup must use a fresh, uncancelled native request token.
    let _ = io.request(
        "DELETE",
        &format!("{}{}", c.endpoint, SESSION),
        "",
        token,
        true,
    );
}
fn call(
    w: &mut Worker,
    next: &mut Value,
    c: &Config,
    io: &impl Io,
    method: &str,
    path: &str,
    body: &str,
    bearer: &str,
) -> Result<Value> {
    let url = format!("{}{}", c.endpoint, path);
    let reply = io.request(method, &url, body, bearer, false)?;
    let limit = matchmaking_steam_request::account_http_response_limit(method, &url);
    if io.cancelled() {
        if method == "POST" && path == SESSION && (200..300).contains(&reply.status) {
            if let Ok(value) = parse(&reply.body, limit) {
                if let Ok(token) = text(&value, "token", 128, false) {
                    retire_grant(&token, c, io)
                }
            }
        }
        return Err(Error::Invalid);
    }
    let value = parse(&reply.body, limit)?;
    if !(200..300).contains(&reply.status) {
        if reply.status == 401 {
            w.session.clear();
            *next = empty_snapshot()
        }
        return Err(Error::Http(text(&value, "error", 256, false)?));
    }
    Ok(value)
}
fn session_call(
    w: &mut Worker,
    next: &mut Value,
    c: &Config,
    io: &impl Io,
    method: &str,
    path: &str,
    body: &str,
) -> Result<Value> {
    let bearer = w.session.clone();
    call(w, next, c, io, method, path, body, &bearer)
}
fn consume_command(reply: &Value, next: &Value, path: &str, command: &Value) -> Result<Value> {
    if path == PARTY {
        let mut parsed = party(reply, next)?;
        if command["action"] == "join_friend" {
            need(
                !parsed["party"].is_null()
                    && parsed["party"]["members"]
                        .as_array()
                        .unwrap()
                        .iter()
                        .any(|m| m["handle"] == command["target_context"]),
            )?
        }
        parsed["party_message"] = json!("");
        Ok(parsed)
    } else if path == FRIENDS {
        friends(reply, next)
    } else {
        social(reply, next)
    }
}
fn steam_friends(reply: &Value, next: &mut Value) -> Result<()> {
    let state = text(reply, "state", 16, false)?;
    need(matches!(state.as_str(), "ready" | "private"))?;
    let rows = array(field(reply, "friends")?)?;
    need(rows.len() <= 1000)?;
    let mut ids = BTreeSet::new();
    let mut out = Vec::new();
    for row in rows {
        let id = text(row, "id", 17, false)?;
        need(
            id.len() == 17
                && !id.starts_with('0')
                && id.bytes().all(|b| b.is_ascii_digit())
                && ids.insert(id.clone()),
        )?;
        out.push(json!({"id":id,"name":text(row,"name",128,false)?,"registered":flag(row,"registered")?,"halo_online":flag(row,"halo_online")?}));
    }
    // Preserve the original evaluation order: a private list does not inspect
    // the optional truncation member at all.
    let message = if state == "private" {
        "Your Steam friend list is private. Make Friends List visible in Steam privacy settings, or invite clan members."
    } else if reply
        .get("truncated")
        .map(|v| v.as_bool().ok_or(Error::Invalid))
        .transpose()?
        .unwrap_or(false)
    {
        "Showing the first 1,000 Steam friends. Refresh to update online status."
    } else {
        "Online Steam friends. Players must have signed into Halo ONLINE before receiving party invites."
    };
    next["steam_friends"] = json!(out);
    next["steam_friends_state"] = json!(state);
    next["steam_friends_message"] = json!(message);
    Ok(())
}
impl Client {
    pub fn new(mut c: Config) -> Self {
        let endpoint = matchmaking_origin::pinned_origin(&c.endpoint, &c.url, c.loopback);
        let configured = c.input_valid && c.login_present && c.poll_ms >= 10 && endpoint.is_ok();
        if let Ok(url) = endpoint {
            c.endpoint = url
        }
        let mut state = empty_snapshot();
        state["revision"] = json!(1);
        state["message"] = json!(if configured {
            format!(
                "Sign in with {}.",
                if c.steam { "Steam" } else { "Discord" }
            )
        } else {
            "ONLINE account service configuration is invalid.".into()
        });
        Self {
            config: c,
            data: Mutex::new(Data {
                state,
                configured,
                active: false,
                pending: false,
                fetch_steam: false,
                name: String::new(),
                profile: None,
                profile_serial: 0,
                voice_bearer: String::new(),
                voice_expires: 0,
                subject: String::new(),
                command: None,
                command_subject: String::new(),
                path: "/v1/clans/commands".into(),
                operation: Operation::default(),
                operation_subject: String::new(),
                link: None,
                link_subject: String::new(),
                next_operation: 0,
                queued_operation: 0,
                queued_issuer: false,
                generation: 0,
            }),
            worker: Mutex::new(Some(Worker::default())),
        }
    }
    fn data(&self) -> std::sync::MutexGuard<'_, Data> {
        self.data
            .lock()
            .unwrap_or_else(std::sync::PoisonError::into_inner)
    }
    pub fn configured(&self) -> bool {
        self.data().configured
    }
    pub fn active(&self) -> bool {
        self.data().active
    }
    pub fn pending(&self) -> bool {
        self.data().pending
    }
    pub fn cadence_ms(&self) -> i64 {
        let d = self.data();
        let p = &d.state["party"];
        if !p.is_null() && p["countdown_supported"] == true {
            if p["countdown"]["seconds"].is_number() {
                100
            } else {
                500
            }
        } else {
            self.config.poll_ms
        }
    }
    pub fn snapshot(&self) -> Value {
        self.data().state.clone()
    }
    pub fn endpoint(&self) -> &str {
        &self.config.endpoint
    }
    pub fn connect(&self, name: String, io: &impl Io) {
        let mut d = self.data();
        if !d.configured || (d.active && d.name == name) {
            return;
        }
        d.active = true;
        d.name = name;
        d.command = None;
        d.pending = true;
        d.generation = d.generation.wrapping_add(1);
        d.operation = Operation::default();
        d.operation_subject.clear();
        d.queued_operation = 0;
        d.link = None;
        d.link_subject.clear();
        d.voice_bearer.clear();
        d.subject.clear();
        let revision = d.state["revision"].as_u64().unwrap().wrapping_add(1);
        d.state = empty_snapshot();
        d.state["busy"] = json!(true);
        d.state["message"] = json!("Verifying ONLINE identity...");
        d.state["revision"] = json!(revision);
        drop(d);
        io.cancel();
        io.wake();
    }
    pub fn sign_out(&self, io: &impl Io) {
        let mut d = self.data();
        if !d.active && d.state["authenticated"] != true {
            return;
        }
        d.active = false;
        d.pending = true;
        d.command = None;
        d.generation = d.generation.wrapping_add(1);
        d.operation = Operation::default();
        d.operation_subject.clear();
        d.queued_operation = 0;
        d.link = None;
        d.link_subject.clear();
        d.voice_bearer.clear();
        d.subject.clear();
        let revision = d.state["revision"].as_u64().unwrap().wrapping_add(1);
        d.state = empty_snapshot();
        d.state["message"] = json!("Signed out.");
        d.state["revision"] = json!(revision);
        drop(d);
        io.cancel();
        io.wake();
    }
    pub fn profile(&self, bytes: [u8; 9], tag: &str, io: &impl Io) {
        if tag.len() > 4
            || !tag
                .bytes()
                .all(|b| b.is_ascii_uppercase() || b.is_ascii_digit())
        {
            return;
        }
        let encoded = bytes.iter().map(|b| format!("{b:02x}")).collect::<String>();
        let value = json!({"appearance":encoded,"service_tag":tag});
        let mut d = self.data();
        if d.profile.as_ref() == Some(&value) {
            return;
        }
        d.profile = Some(value);
        d.profile_serial = d.profile_serial.wrapping_add(1);
        d.pending = true;
        drop(d);
        io.wake();
    }
    pub fn clan_command(
        &self,
        action: &str,
        value: &str,
        role: u32,
        confirmed: bool,
        io: &impl Io,
    ) -> Result<bool> {
        {
            let d = self.data();
            if self.config.profile_only
                || d.state["authenticated"] != true
                || d.state["busy"] == true
                || !d.active
            {
                return Ok(false);
            }
        }
        let mut body = json!({"request_id":uuid(io)?,"action":action});
        match action {
            "create" => body["name"] = json!(value),
            "invite" => body["username"] = json!(value),
            "broadcast" => body["message"] = json!(value),
            "leave" => {}
            _ => body["target"] = json!(value),
        }
        if action == "role" {
            body["role"] = json!(role);
            body["confirmed"] = json!(confirmed)
        }
        let mut d = self.data();
        d.command = Some(body);
        d.pending = true;
        d.state["busy"] = json!(true);
        d.command_subject = d.subject.clone();
        d.generation = d.generation.wrapping_add(1);
        d.path = "/v1/clans/commands?presence=1".into();
        d.state["message"] = json!("Updating clan...");
        increment(&mut d.state, "revision");
        drop(d);
        io.wake();
        Ok(true)
    }
    pub fn party_command(&self, c: PartyCommand, activity: u32, io: &impl Io) -> Result<u64> {
        let initial = self.data().clone();
        let s = &initial.state;
        let p = &s["party"];
        if self.config.profile_only
            || !self.config.parties
            || s["authenticated"] != true
            || s["busy"] == true
            || !initial.active
        {
            return Ok(0);
        }
        if activity != 0
            && io
                .identity()?
                .is_none_or(|id| id.subject != initial.subject)
        {
            return Ok(0);
        }
        if activity == 1
            && (p.is_null() || p["leader"] != true || p["members"].as_array().unwrap().len() >= 16)
        {
            return Ok(0);
        }
        if activity == 1
            && (c.value.is_empty()
                || c.value.len() > 20
                || !c.value.bytes().all(|b| b.is_ascii_digit()))
        {
            return Ok(0);
        }
        if activity == 2 && !p.is_null() {
            return Ok(0);
        }
        let mut body = json!({"request_id":uuid(io)?,"action":c.action});
        if c.action != "privacy"
            && (c.privacy.is_some() || c.privacy_revision != 0 || !c.privacy_context.is_empty())
        {
            return Ok(0);
        }
        if c.action != "join_friend" && !c.join_context.is_empty() {
            return Ok(0);
        }
        match c.action.as_str() {
            "join_activity" => {
                if c.version != 0
                    || !c.playlist.is_empty()
                    || !c.revision.is_empty()
                    || !valid_secret(&c.value)
                {
                    return Ok(0);
                }
                body["join_secret"] = json!(c.value)
            }
            "invite" => body["username"] = json!(c.value),
            "invite_discord" => body["discord_id"] = json!(c.value),
            "invite_steam" => body["steam_id"] = json!(c.value),
            "invite_member" | "invite_friend" | "join_friend" | "accept" | "decline" | "block"
            | "cancel" | "remove" | "transfer" => body["target"] = json!(c.value),
            "ready" | "countdown_start" => {
                body["playlist_id"] = json!(c.playlist);
                body["revision"] = json!(c.revision)
            }
            "privacy" => {
                if p.is_null()
                    || p["leader"] != true
                    || p["privacy"].is_null()
                    || c.privacy.is_none_or(|v| v > 2)
                    || c.privacy_revision == 0
                    || p["privacy"]["revision"] != c.privacy_revision
                    || p["privacy"]["context"] != c.privacy_context
                    || !c.value.is_empty()
                    || !c.playlist.is_empty()
                    || !c.revision.is_empty()
                {
                    return Ok(0);
                }
                body["privacy"] = json!(c.privacy.unwrap());
                body["privacy_revision"] = json!(c.privacy_revision);
                body["privacy_context"] = json!(c.privacy_context);
            }
            _ => {}
        }
        if c.action == "join_friend" {
            if !p.is_null()
                || c.version != 0
                || c.value.is_empty()
                || !c.playlist.is_empty()
                || !c.revision.is_empty()
            {
                return Ok(0);
            }
            let target = s["friends"]
                .as_array()
                .unwrap()
                .iter()
                .find(|f| f["id"] == c.value);
            if target.is_none_or(|f| {
                f["state"] != "friend"
                    || f["party_joinable"] != true
                    || c.join_context.is_empty()
                    || f["party_context"] != c.join_context
            }) {
                return Ok(0);
            }
            body["target_context"] = json!(c.join_context);
        }
        if !matches!(
            c.action.as_str(),
            "create" | "decline" | "block" | "join_activity" | "join_friend"
        ) {
            let v = if c.version != 0 {
                c.version
            } else {
                p["version"].as_u64().unwrap_or(0)
            };
            if v == 0 {
                return Ok(0);
            }
            body["version"] = json!(v);
        }
        let mut d = self.data();
        d.command = Some(body);
        d.path = PARTY.into();
        d.command_subject = d.subject.clone();
        d.pending = true;
        d.state["busy"] = json!(true);
        d.state["message"] = json!("Updating party...");
        increment(&mut d.state, "party_command_serial");
        d.state["party_command_successful"] = Value::Null;
        d.generation = d.generation.wrapping_add(1);
        if activity != 0 {
            d.next_operation = d.next_operation.wrapping_add(1);
            d.queued_operation = d.next_operation;
            d.queued_issuer = activity == 1;
            d.operation = Operation {
                serial: d.queued_operation,
                pending: true,
                ..Operation::default()
            };
            d.operation_subject = d.subject.clone()
        }
        if !matches!(
            c.action.as_str(),
            "invite"
                | "invite_discord"
                | "invite_steam"
                | "invite_member"
                | "invite_friend"
                | "cancel"
                | "decline"
                | "block"
                | "countdown_start"
                | "countdown_cancel"
                | "countdown_request"
                | "privacy"
        ) {
            d.link = None;
            d.link_subject.clear();
            d.operation.link = None;
        }
        increment(&mut d.state, "revision");
        let result = if activity != 0 { d.queued_operation } else { 1 };
        drop(d);
        io.wake();
        Ok(result)
    }
    pub fn friend_command(&self, action: &str, value: &str, io: &impl Io) -> Result<bool> {
        {
            let d = self.data();
            if self.config.profile_only
                || !d.active
                || d.state["authenticated"] != true
                || d.state["busy"] == true
            {
                return Ok(false);
            }
        }
        if !matches!(
            action,
            "search" | "request" | "accept" | "decline" | "cancel" | "remove" | "block" | "unblock"
        ) {
            return Ok(false);
        }
        let mut body = json!({"request_id":uuid(io)?,"action":action});
        body[if action == "search" { "tag" } else { "target" }] = json!(value);
        let mut d = self.data();
        d.command = Some(body);
        d.path = FRIENDS.into();
        d.command_subject = d.subject.clone();
        d.pending = true;
        d.state["busy"] = json!(true);
        d.state["friends_message"] = json!(if action == "search" {
            "Searching players..."
        } else {
            "Updating friends..."
        });
        if action == "search" {
            d.state["friend_search"] = Value::Null
        }
        d.generation = d.generation.wrapping_add(1);
        increment(&mut d.state, "revision");
        drop(d);
        io.wake();
        Ok(true)
    }
    pub fn refresh_steam(&self, io: &impl Io) {
        let mut d = self.data();
        if !d.active
            || d.state["authenticated"] != true
            || !self.config.steam
            || self.config.profile_only
        {
            return;
        }
        d.fetch_steam = true;
        d.pending = true;
        d.state["steam_friends_state"] = json!("loading");
        d.state["steam_friends_message"] = json!("Loading Steam friends...");
        increment(&mut d.state, "revision");
        drop(d);
        io.wake();
    }
    pub fn voice(&self, is_party: bool, io: &impl Io) -> Result<Option<Voice>> {
        if self.config.profile_only {
            return Ok(None);
        }
        let identity = io.identity()?;
        let (subject, expires) = {
            let d = self.data();
            if !d.active || d.state["authenticated"] != true || d.voice_bearer.is_empty() {
                return Ok(None);
            }
            (d.subject.clone(), d.voice_expires)
        };
        if io.now() >= expires || identity.is_none_or(|id| id.subject != subject) {
            return Ok(None);
        }
        let d = self.data();
        let group = &d.state[if is_party { "party" } else { "clan" }];
        if group.is_null() {
            return Ok(None);
        }
        for m in group["members"].as_array().unwrap() {
            if m["self"] == true {
                return Ok(Some(Voice {
                    origin: self.config.endpoint.clone(),
                    bearer: d.voice_bearer.clone(),
                    channel: if is_party {
                        "party".into()
                    } else {
                        group["id"].as_str().unwrap().into()
                    },
                    player: m[if is_party { "handle" } else { "id" }]
                        .as_str()
                        .unwrap()
                        .into(),
                    loopback: self.config.loopback,
                    party: is_party,
                }));
            }
        }
        Ok(None)
    }
    pub fn discord_operation(&self, serial: u64, io: &impl Io) -> Result<Operation> {
        let identity = io.identity()?;
        let d = self.data();
        let r = &d.operation;
        let subject = identity.as_ref().map(|id| id.subject.as_str());
        let failed = || Operation {
            serial,
            ..Operation::default()
        };
        if r.serial != serial
            || !d.active
            || d.state["authenticated"] != true
            || subject != Some(d.subject.as_str())
            || subject != Some(d.operation_subject.as_str())
        {
            return Ok(failed());
        }
        if let Some(r) = &r.link {
            let p = &d.state["party"];
            let link = d.link.as_ref();
            if p.is_null()
                || p["leader"] != true
                || link.is_none()
                || Some(d.link_subject.as_str()) != subject
                || link.is_none_or(|l| {
                    l.join_secret != r.join_secret
                        || l.party_id != r.party_id
                        || p["version"] != l.version
                        || p["members"].as_array().unwrap().len() != l.current_size as usize
                })
            {
                return Ok(failed());
            }
            let expires = link.unwrap().expires;
            let voice_expires = d.voice_expires;
            let mut out = d.operation.clone();
            out.link = d.link.clone();
            drop(d);
            if io.now() >= expires || io.now() >= voice_expires {
                return Ok(failed());
            }
            return Ok(out);
        }
        let mut out = r.clone();
        if out.link.is_some() {
            out.link = d.link.clone()
        }
        Ok(out)
    }
    pub fn discord_link(&self, io: &impl Io) -> Result<Option<Link>> {
        let identity = io.identity()?;
        let d = self.data();
        let link = d.link.as_ref();
        let subject = identity.as_ref().map(|id| id.subject.as_str());
        let p = &d.state["party"];
        if link.is_none()
            || !d.active
            || d.state["authenticated"] != true
            || subject != Some(d.subject.as_str())
            || subject != Some(d.link_subject.as_str())
            || p.is_null()
            || p["leader"] != true
            || link.is_none_or(|l| {
                p["version"] != l.version
                    || p["members"].as_array().unwrap().len() != l.current_size as usize
            })
        {
            return Ok(None);
        }
        let link = link.cloned().unwrap();
        let voice_expires = d.voice_expires;
        drop(d);
        if io.now() >= link.expires || io.now() >= voice_expires {
            return Ok(None);
        }
        Ok(Some(link))
    }
    fn publish(
        &self,
        mut next: Value,
        generation: u64,
        bearer: &str,
        expires: i64,
        mut activity: Option<Operation>,
        subject: &str,
        mut link: Option<Link>,
        io: &impl Io,
    ) -> Result<()> {
        native_locked(io, || {
            if self.data().generation != generation {
                return Ok(());
            }
            let identity = io.identity()?;
            if next["authenticated"] == true
                && identity.as_ref().is_none_or(|id| id.subject != subject)
            {
                next = empty_snapshot();
                next["message"] = json!("Account identity changed. Reconnecting ONLINE...");
                if let Some(activity) = &mut activity {
                    activity.successful = false;
                    activity.link = None
                }
            }
            {
                let mut d = self.data();
                d.voice_bearer = if next["authenticated"] == true {
                    bearer.into()
                } else {
                    String::new()
                };
                d.voice_expires = expires;
                d.subject = if next["authenticated"] == true {
                    subject.into()
                } else {
                    String::new()
                };
                if let Some(activity) = activity {
                    d.operation = activity;
                    d.operation_subject = if next["authenticated"] == true {
                        subject.into()
                    } else {
                        String::new()
                    };
                }
            }
            let p = &next["party"];
            if next["authenticated"] != true
                || p.is_null()
                || p["leader"] != true
                || link.as_ref().is_none_or(|l| {
                    p["version"] != l.version
                        || p["members"].as_array().unwrap().len() != l.current_size as usize
                })
                || io.now() >= link.as_ref().unwrap().expires
            {
                link = None
            }
            let mut d = self.data();
            d.link = link;
            d.link_subject = if d.link.is_some() {
                subject.into()
            } else {
                String::new()
            };
            next["revision"] = json!(d.state["revision"].as_u64().unwrap().wrapping_add(1));
            d.state = next;
            Ok(())
        })
    }
    pub fn poll(&self, io: &impl Io) -> Result<()> {
        let mut worker = self
            .worker
            .lock()
            .unwrap_or_else(std::sync::PoisonError::into_inner)
            .take()
            .ok_or(Error::Invalid)?;
        let result = self.poll_inner(&mut worker, io);
        *self
            .worker
            .lock()
            .unwrap_or_else(std::sync::PoisonError::into_inner) = Some(worker);
        result
    }
    pub fn shutdown(&self, io: &impl Io) {
        let worker = self
            .worker
            .lock()
            .unwrap_or_else(std::sync::PoisonError::into_inner)
            .take();
        if let Some(mut w) = worker {
            cleanup(&mut w, &self.config, io);
            *self
                .worker
                .lock()
                .unwrap_or_else(std::sync::PoisonError::into_inner) = Some(w)
        }
    }
    fn poll_inner(&self, w: &mut Worker, io: &impl Io) -> Result<()> {
        let captured = native_locked(io, || {
            let initial = self.data().clone();
            {
                let mut d = self.data();
                d.fetch_steam = false;
                d.command = None;
                d.pending = false;
                d.queued_operation = 0;
            }
            io.reset_cancellation()?;
            Ok(initial)
        })?;
        let mut next = captured.state.clone();
        let was_authenticated = next["authenticated"] == true;
        let mut link = captured.link.clone();
        let mut activity = if captured.queued_operation != 0 {
            Some(Operation {
                serial: captured.queued_operation,
                ..Operation::default()
            })
        } else {
            None
        };
        if !captured.active {
            cleanup(w, &self.config, io);
            return Ok(());
        }
        let mut acknowledged = false;
        enum Done {
            Full,
            Countdown,
            Profile,
            Unsigned,
        }
        let mut execute = || -> Result<Done> {
            let Some(authorization) = io.identity()? else {
                cleanup(w, &self.config, io);
                next = empty_snapshot();
                next["message"] = json!(format!(
                    "Sign in with {}.",
                    if self.config.steam {
                        "Steam"
                    } else {
                        "Discord"
                    }
                ));
                return Ok(Done::Unsigned);
            };
            if captured.command.is_some() && authorization.subject != captured.command_subject {
                cleanup(w, &self.config, io);
                next = empty_snapshot();
                return Err(Error::Http(
                    "Account identity changed. Refresh ONLINE before trying again.".into(),
                ));
            }
            if captured.link_subject != authorization.subject {
                link = None
            }
            if activity.is_none()
                && captured.operation.serial != 0
                && captured.operation_subject != authorization.subject
            {
                let mut value = captured.operation.clone();
                value.successful = false;
                value.link = None;
                activity = Some(value);
            }
            if captured.command.is_none()
                && !captured.fetch_steam
                && next["authenticated"] == true
                && !next["party"].is_null()
                && next["party"]["countdown_supported"] == true
                && !w.session.is_empty()
                && w.subject == authorization.subject
                && w.name == captured.name
                && io.now() < w.expires
                && io.now() < w.next_social
                && captured.profile_serial == w.sent_profile
            {
                let reply = session_call(
                    w,
                    &mut next,
                    &self.config,
                    io,
                    "GET",
                    "/v1/party/countdown",
                    "",
                )?;
                object(&reply, 2)?;
                let rv = field(&reply, "version")?;
                if (rv.is_i64() || rv.is_u64()) && rv == &next["party"]["version"] {
                    let cd = countdown(field(&reply, "countdown")?)?;
                    if !cd.is_null() && !cd["requester"].as_str().unwrap().is_empty() {
                        need(
                            next["party"]["members"]
                                .as_array()
                                .unwrap()
                                .iter()
                                .any(|m| m["handle"] == cd["requester"] && m["leader"] == false),
                        )?
                    }
                    next["party"]["countdown"] = cd;
                } else {
                    w.next_social = 0
                }
                return Ok(Done::Countdown);
            }
            if w.session.is_empty()
                || w.name != captured.name
                || w.subject != authorization.subject
                || io.now() >= w.expires
            {
                if !w.subject.is_empty() && w.subject != authorization.subject {
                    next = empty_snapshot()
                }
                cleanup(w, &self.config, io);
                let reply = call(
                    w,
                    &mut next,
                    &self.config,
                    io,
                    "POST",
                    SESSION,
                    &json!({"screen_name":captured.name}).to_string(),
                    &authorization.bearer,
                )?;
                let token = text(&reply, "token", 128, false)?;
                let lease = (|| -> Result<i64> {
                    object(&reply, 2)?;
                    let now = io.now();
                    let seconds = number(&reply, "expires_in", 1, 900)? as i64;
                    now.checked_add(seconds * SECOND)
                        .and_then(|n| n.checked_sub(SECOND))
                        .ok_or(Error::Invalid)
                })();
                let expires = match lease {
                    Ok(value) => value,
                    Err(error) => {
                        retire_grant(&token, &self.config, io);
                        return Err(error);
                    }
                };
                // Adopt only a completely validated lease. An earlier future
                // expiry must never make a rejected replacement token usable.
                w.session = token;
                w.expires = expires;
                w.name = captured.name.clone();
                w.subject = authorization.subject.clone();
                w.sent_profile = 0;
                w.attempted_profile = 0;
            }
            if let Some(profile) = &captured.profile {
                if w.sent_profile != captured.profile_serial
                    && (w.attempted_profile != captured.profile_serial
                        || io.now() >= w.next_profile)
                {
                    w.attempted_profile = captured.profile_serial;
                    w.next_profile = io.now().checked_add(60 * SECOND).ok_or(Error::Invalid)?;
                    match session_call(
                        w,
                        &mut next,
                        &self.config,
                        io,
                        "PUT",
                        "/v1/account/profile",
                        &profile.to_string(),
                    ) {
                        Ok(_) => {
                            w.sent_profile = captured.profile_serial;
                            next["profile_message"] = json!("")
                        }
                        Err(e) => {
                            if io.cancelled() || w.session.is_empty() {
                                return Err(e);
                            }
                            next["profile_message"] =
                                json!("Player profile sync is temporarily unavailable.")
                        }
                    }
                }
            }
            if w.linked_session != w.session && io.now() >= w.next_identity {
                w.next_identity = io.now().checked_add(60 * SECOND).ok_or(Error::Invalid)?;
                let result = (|| -> Result<()> {
                    if io.eos_profiles()? {
                        let credential = session_call(
                            w,
                            &mut next,
                            &self.config,
                            io,
                            "POST",
                            "/v1/account/eos-token",
                            "{}",
                        )?;
                        let token = text(&credential, "token", 8192, false)?;
                        let (puid, token) = io.eos_identity(&token)?;
                        need(!io.cancelled())?;
                        need(
                            io.identity()?
                                .is_some_and(|id| id.subject == authorization.subject),
                        )?;
                        session_call(
                            w,
                            &mut next,
                            &self.config,
                            io,
                            "POST",
                            "/v1/account/eos-proof",
                            &json!({"puid":puid,"token":token}).to_string(),
                        )?;
                        w.linked_session = w.session.clone();
                        next["profile_message"] = json!("");
                    }
                    Ok(())
                })();
                if result.is_err() {
                    next["profile_message"] =
                        json!("Career profile sync is temporarily unavailable.")
                }
            }
            if self.config.profile_only {
                session_call(
                    w,
                    &mut next,
                    &self.config,
                    io,
                    "GET",
                    "/v1/account/profile",
                    "",
                )?;
                next["authenticated"] = json!(true);
                next["busy"] = json!(false);
                next["message"] = json!("ONLINE profile connected.");
                return Ok(Done::Profile);
            }
            if captured.fetch_steam && self.config.steam {
                let result = session_call(
                    w,
                    &mut next,
                    &self.config,
                    io,
                    "GET",
                    "/v1/account/steam-friends",
                    "",
                )
                .and_then(|v| steam_friends(&v, &mut next));
                if let Err(e) = result {
                    if io.cancelled() || w.session.is_empty() {
                        return Err(e);
                    }
                    next["steam_friends"] = json!([]);
                    next["steam_friends_state"] = json!("unavailable");
                    next["steam_friends_message"] =
                        json!("Steam friends are temporarily unavailable. Refresh to try again.")
                }
            }
            if let Some(command) = &captured.command {
                if io
                    .identity()?
                    .is_none_or(|id| id.subject != captured.command_subject)
                {
                    cleanup(w, &self.config, io);
                    next = empty_snapshot();
                    return Err(Error::Http(
                        "Account identity changed. Refresh ONLINE before trying again.".into(),
                    ));
                }
                let path = if captured.path == PARTY {
                    format!("{PARTY}?countdown=1&privacy=1")
                } else if captured.path == FRIENDS {
                    format!("{FRIENDS}?party=1")
                } else {
                    captured.path.clone()
                };
                let consume = |w: &mut Worker, next: &mut Value| -> Result<()> {
                    let reply = session_call(
                        w,
                        next,
                        &self.config,
                        io,
                        "POST",
                        &path,
                        &command.to_string(),
                    )?;
                    *next = consume_command(&reply, next, &captured.path, command)?;
                    Ok(())
                };
                if let Err(error) = consume(w, &mut next) {
                    match error {
                        Error::Http(_) => return Err(error),
                        _ => {
                            if io.cancelled() {
                                return Err(error);
                            }
                            consume(w, &mut next)?
                        }
                    }
                }
                if captured.path == PARTY {
                    next["party_command_successful"] = json!(true);
                    acknowledged = true
                }
                if let Some(activity) = &mut activity {
                    if captured.queued_issuer {
                        let p = &next["party"];
                        need(
                            !p.is_null()
                                && p["leader"] == true
                                && p["members"].as_array().unwrap().len() < 16,
                        )?;
                        let version = p["version"].clone();
                        let old = link.take();
                        let reply = session_call(
                            w,
                            &mut next,
                            &self.config,
                            io,
                            "POST",
                            "/v1/party/activity",
                            &json!({"version":version}).to_string(),
                        )?;
                        let mut created = party_link(&reply, &next["party"], io.now())?;
                        if let Some(old) = old {
                            if old.party_id == created.party_id
                                && old.join_secret == created.join_secret
                            {
                                created.expires = created.expires.min(old.expires)
                            }
                        }
                        activity.link = Some(created.clone());
                        link = Some(created);
                    }
                    activity.successful = true;
                }
            } else {
                let v = session_call(
                    w,
                    &mut next,
                    &self.config,
                    io,
                    "GET",
                    "/v1/clans?presence=1",
                    "",
                )?;
                next = social(&v, &next)?
            }
            if captured.command.is_some() && matches!(captured.path.as_str(), PARTY | FRIENDS) {
                let v = session_call(
                    w,
                    &mut next,
                    &self.config,
                    io,
                    "GET",
                    "/v1/clans?presence=1",
                    "",
                )?;
                next = social(&v, &next)?
            }
            if captured.command.is_none() || captured.path != FRIENDS {
                let result = session_call(
                    w,
                    &mut next,
                    &self.config,
                    io,
                    "GET",
                    "/v1/friends?party=1",
                    "",
                )
                .and_then(|v| friends(&v, &next));
                match result {
                    Ok(v) => next = v,
                    Err(e) => {
                        if io.cancelled() || w.session.is_empty() {
                            return Err(e);
                        }
                        next["friends_message"] =
                            json!("Friends are temporarily unavailable. Try again shortly.");
                        next["friends_available"] = json!(false);
                        next["friends"] = json!([]);
                        next["friend_search"] = Value::Null
                    }
                }
            }
            if self.config.parties && (captured.command.is_none() || captured.path != PARTY) {
                let v = session_call(
                    w,
                    &mut next,
                    &self.config,
                    io,
                    "GET",
                    "/v1/party?local_players=1&appearance=1&postgame=1&countdown=1&privacy=1",
                    "",
                );
                match v {
                    Ok(v) => {
                        next = party(&v, &next)?;
                        next["party_message"] = json!("")
                    }
                    Err(Error::Http(e)) => {
                        if w.session.is_empty() {
                            return Err(Error::Http(e));
                        }
                        next["party_message"] = json!(e)
                    }
                    Err(e) => return Err(e),
                }
            }
            if link.is_some()
                && !next["party"].is_null()
                && next["party"]["leader"] == true
                && next["party"]["members"].as_array().unwrap().len() < 16
                && io.now() < link.as_ref().unwrap().expires
                && (next["party"]["version"] != link.as_ref().unwrap().version
                    || next["party"]["members"].as_array().unwrap().len()
                        != link.as_ref().unwrap().current_size as usize)
            {
                let old = link.take().unwrap();
                let result = (|| -> Result<Link> {
                    let version = next["party"]["version"].clone();
                    let reply = session_call(
                        w,
                        &mut next,
                        &self.config,
                        io,
                        "POST",
                        "/v1/party/activity",
                        &json!({"version":version,"join_secret":old.join_secret}).to_string(),
                    )?;
                    let mut refreshed = party_link(&reply, &next["party"], io.now())?;
                    need(
                        refreshed.party_id == old.party_id
                            && refreshed.join_secret == old.join_secret,
                    )?;
                    refreshed.expires = refreshed.expires.min(old.expires);
                    Ok(refreshed)
                })();
                match result {
                    Ok(value) => link = Some(value),
                    Err(e) => {
                        if w.session.is_empty() {
                            return Err(e);
                        }
                    }
                }
            }
            next["authenticated"] = json!(true);
            next["busy"] = json!(false);
            if captured.command.is_some() || !was_authenticated {
                next["message"] = json!(if captured.command.is_some() {
                    if captured.path == PARTY {
                        "Party updated."
                    } else if captured.path == FRIENDS {
                        "Friends updated."
                    } else {
                        "Clan updated."
                    }
                } else {
                    "ONLINE connected."
                })
            }
            Ok(Done::Full)
        };
        match execute() {
            Ok(Done::Unsigned) => {
                return self.publish(next, captured.generation, "", 0, activity, "", None, io);
            }
            Ok(Done::Countdown) => {
                return self.publish(
                    next,
                    captured.generation,
                    &w.session,
                    w.expires,
                    activity,
                    &w.subject,
                    link,
                    io,
                );
            }
            Ok(Done::Profile) => {
                if !io.cancelled() {
                    self.publish(
                        next,
                        captured.generation,
                        &w.session,
                        w.expires,
                        None,
                        &w.subject,
                        None,
                        io,
                    )?
                }
                return Ok(());
            }
            Ok(Done::Full) => {}
            Err(error) => {
                next["busy"] = json!(false);
                let mut message = error.to_string();
                if activity.is_some()
                    || captured
                        .command
                        .as_ref()
                        .is_some_and(|c| c["action"] == "join_activity")
                {
                    message = "Discord party operation failed. Refresh ONLINE and try again.".into()
                }
                if message.len() > 256 {
                    message = "ONLINE connection failed. Retry shortly.".into()
                }
                next["message"] = json!(message);
                if captured.command.is_some() && captured.path == PARTY {
                    next["party_message"] = next["message"].clone();
                    if !acknowledged {
                        next["party_command_successful"] = json!(false)
                    }
                }
                if captured.command.is_some() && captured.path == FRIENDS {
                    next["friends_message"] = next["message"].clone()
                }
            }
        }
        let interval = if !next["party"].is_null() && next["party"]["countdown_supported"] == true {
            self.config.poll_ms.max(2000)
        } else {
            self.config.poll_ms
        };
        w.next_social = io
            .now()
            .checked_add(interval.checked_mul(1_000_000).ok_or(Error::Invalid)?)
            .ok_or(Error::Invalid)?;
        if !io.cancelled() {
            self.publish(
                next,
                captured.generation,
                &w.session,
                w.expires,
                activity,
                &w.subject,
                link,
                io,
            )?
        }
        Ok(())
    }
}
fn increment(v: &mut Value, key: &str) {
    v[key] = json!(v[key].as_u64().unwrap().wrapping_add(1))
}
pub fn postgame_follower(leader: bool, count: usize) -> bool {
    !leader && count > 1
}
pub fn postgame_leader_left(
    leader: bool,
    count: usize,
    members: impl Iterator<Item = (bool, bool, Option<bool>)>,
) -> bool {
    if !postgame_follower(leader, count) {
        return false;
    }
    let (mut own, mut host) = (false, false);
    for (is_self, is_leader, postgame) in members {
        if is_self {
            own = postgame == Some(true)
        }
        if is_leader {
            host = postgame == Some(false)
        }
    }
    own && host
}
pub fn party_follower(p: &Value) -> bool {
    postgame_follower(
        p["leader"] == true,
        p["members"].as_array().map_or(0, Vec::len),
    )
}
pub fn leader_left_postgame(p: &Value) -> bool {
    let Some(rows) = p["members"].as_array() else {
        return false;
    };
    postgame_leader_left(
        p["leader"] == true,
        rows.len(),
        rows.iter().map(|row| {
            (
                row["self"] == true,
                row["leader"] == true,
                row["postgame"].as_bool(),
            )
        }),
    )
}
