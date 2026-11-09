//! Native queue-client authority. C++ supplies HTTP, login and connection IO.
//!
//! This is the replacement service's client protocol, not a recovered Xbox
//! Live wire implementation. Original readiness and full-variant predicates
//! remain shared with their separately authenticated original comparisons.
pub use crate::matchmaking_origin::UrlParts;
use crate::{matchmaking_catalog, matchmaking_json, matchmaking_readiness, matchmaking_variant};
use serde_json::{Value, json};
use sha2::{Digest, Sha256};
use std::collections::BTreeSet;
use std::sync::Mutex;

const LIMIT: usize = 65_536;
const PROGRESS: &str = "/v1/matchmaking/ticket?progress=1&ranks=1&start=1&failure=1&appearance=1";
const BOUND_START: &str = "/v1/matchmaking/ticket?start=1&failure=1&start_room=1";
const TICKET: &str = "/v1/matchmaking/ticket";
const SESSION: &str = "/v1/matchmaking/session";

pub fn runtime_compatibility(eos_transport_configured: bool) -> &'static str {
    if eos_transport_configured {
        "halo2-native-queue-v16"
    } else {
        "halo2-native-queue-v12"
    }
}

#[derive(Clone, Debug, Default)]
pub struct Config {
    pub input_valid: bool,
    pub endpoint: String,
    pub url: UrlParts,
    pub maps: Vec<String>,
    pub compatibility: String,
    pub language: u32,
    pub poll_ms: i64,
    pub loopback: bool,
    pub login_present: bool,
    pub terminal_recovery: bool,
    pub force_relay: bool,
}

#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum Status {
    #[default]
    Unconfigured,
    Idle,
    Connecting,
    Ready,
    Joining,
    Searching,
    CheckingConnections,
    AwaitingTransport,
    Loading,
    Failed,
}

#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct Playlist {
    pub id: String,
    pub revision: String,
    pub name: String,
    pub description: String,
    pub ranked: bool,
    pub quickmatch: bool,
    pub eligible: bool,
    pub minimum: u32,
    pub maximum: u32,
    pub level: Option<u32>,
    pub clan_match: bool,
}

#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct Player {
    pub name: String,
    pub team: u32,
    pub is_self: bool,
    pub level: Option<u32>,
    pub appearance: Option<[u8; 9]>,
}

#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct Match {
    pub map: String,
    pub variant: String,
    pub game_id: String,
    pub settings: Option<[u8; 304]>,
    pub players: Vec<Player>,
}

impl Match {
    fn matches_countdown(&self, observed: &Self) -> bool {
        self.map == observed.map
            && self.variant == observed.variant
            && self.game_id == observed.game_id
            && self.settings == observed.settings
            && self.players.len() == observed.players.len()
            && self
                .players
                .iter()
                .zip(&observed.players)
                .all(|(a, b)| a.name == b.name && a.team == b.team && a.is_self == b.is_self)
    }
}

#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct Gathering {
    pub players: Vec<Player>,
    pub minimum: u32,
    pub maximum: u32,
    pub wait_ms: Option<u32>,
}

#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct Snapshot {
    pub status: Status,
    pub playlists: Vec<Playlist>,
    pub playlist_id: String,
    pub message: String,
    pub game: Option<Match>,
    pub gathering: Option<Gathering>,
    pub queued_players: u32,
    pub revision: u64,
    pub start_supported: bool,
    pub start_ms: Option<u32>,
    pub failure_state: Option<i32>,
}

#[derive(Clone, Debug, Default)]
pub struct LocalPlayer {
    pub name: String,
    pub signed_in: bool,
    pub appearance: Option<[u8; 9]>,
}

#[derive(Clone, Debug, Default)]
pub struct Profile {
    pub name: String,
    pub appearance: Option<[u8; 9]>,
    pub locals: Vec<LocalPlayer>,
}

pub enum Command {
    Refresh(Profile),
    Join(String),
    Cancel,
    SignOut,
    Invalid,
    InvalidProfile,
}

#[derive(Clone, Debug, Default)]
pub struct Grant {
    pub authorization: Option<String>,
    pub steam: bool,
}

#[derive(Clone, Debug, Default)]
pub struct Identity {
    pub primary: Option<String>,
    pub locals: Vec<Grant>,
}

#[derive(Clone, Debug)]
pub struct Request {
    pub version: u64,
    pub method: String,
    pub url: String,
    pub body: String,
    pub bearer: String,
    pub cleanup: bool,
}

pub struct Response {
    pub status: i64,
    pub body: String,
}

#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum ConnectionPhase {
    #[default]
    Missing,
    Connecting,
    Loading,
    Failed,
    Other,
}

#[derive(Default)]
pub struct Connection {
    pub phase: ConnectionPhase,
    pub error: String,
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Seat {
    pub slot: u32,
    pub name: String,
    pub team: u32,
    pub identity: [u8; 8],
    pub peer: u32,
    pub guest: bool,
    pub level: Option<u32>,
}

#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct Eos {
    pub socket: String,
    pub peers: Vec<(u32, String)>,
    pub account_login: bool,
    pub playlist: String,
    pub revision: String,
    pub map: String,
    pub variant: String,
    pub force_relay: bool,
}

#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct Admission {
    pub endpoint: String,
    pub room: String,
    pub bearer: String,
    pub current: u32,
    pub seats: Vec<Seat>,
    pub loopback: bool,
    pub team_mode: Option<bool>,
    pub direct: bool,
    pub stun: Vec<String>,
    pub eos: Option<Eos>,
    pub terminal_recovery: bool,
}

#[derive(Debug, PartialEq, Eq)]
pub enum Error {
    Invalid,
    Unavailable,
    Cancelled,
    Status(i64, String),
    Terminal(i32, String),
    UnsupportedCountdown,
}
pub type Result<T> = std::result::Result<T, Error>;

/// These operations observe or create actual native owners; none admits policy.
pub trait Io {
    fn now_ms(&self) -> u64;
    fn cancelled(&self, version: u64) -> bool;
    fn identity(&self, version: u64) -> Result<Identity>;
    fn request(&self, request: &Request) -> Result<Response>;
    fn connection(&self, version: u64) -> Result<Connection>;
    fn install(&self, version: u64, admission: &Admission) -> Result<bool>;
    fn disconnect(&self, version: u64);
    fn qos(&self, version: u64, endpoint: &str, bearer: &str, loopback: bool) -> Result<bool>;
}

fn check(value: bool) -> Result<()> {
    if value { Ok(()) } else { Err(Error::Invalid) }
}
fn field<'a>(v: &'a Value, key: &str) -> Result<&'a Value> {
    v.as_object().and_then(|v| v.get(key)).ok_or(Error::Invalid)
}
fn object(v: &Value, size: usize) -> Result<()> {
    check(v.as_object().is_some_and(|v| v.len() == size))
}
fn string(v: &Value, max: usize, empty: bool) -> Result<String> {
    let s = v.as_str().ok_or(Error::Invalid)?;
    check((empty || !s.is_empty()) && s.len() <= max && !s.bytes().any(|v| v < 32 || v == 127))?;
    Ok(s.to_owned())
}
fn text(v: &Value, key: &str) -> Result<String> {
    string(field(v, key)?, 64, false)
}
fn number(v: &Value, low: u32, high: u32) -> Result<u32> {
    let n = v.as_u64().ok_or(Error::Invalid)?;
    check(n >= u64::from(low) && n <= u64::from(high))?;
    Ok(n as u32)
}
fn flag(v: &Value, key: &str) -> Result<bool> {
    field(v, key)?.as_bool().ok_or(Error::Invalid)
}
fn array(v: &Value) -> Result<&Vec<Value>> {
    v.as_array().ok_or(Error::Invalid)
}
fn depth(v: &Value, level: u32) -> Result<()> {
    check(level <= 16)?;
    match v {
        Value::Array(v) => {
            for v in v {
                depth(v, level + 1)?;
            }
        }
        Value::Object(v) => {
            for v in v.values() {
                depth(v, level + 1)?;
            }
        }
        _ => {}
    }
    Ok(())
}
fn parse(body: &str) -> Result<Value> {
    let v = matchmaking_json::parse(body.as_bytes(), LIMIT).map_err(|_| Error::Invalid)?;
    depth(&v, 0)?;
    Ok(v)
}
fn hex<const N: usize>(v: &Value) -> Result<[u8; N]> {
    let s = string(v, N * 2, false)?;
    check(
        s.len() == N * 2
            && s.bytes()
                .all(|v| v.is_ascii_digit() || (b'a'..=b'f').contains(&v)),
    )?;
    let mut out = [0; N];
    for (i, v) in out.iter_mut().enumerate() {
        *v = u8::from_str_radix(&s[i * 2..i * 2 + 2], 16).map_err(|_| Error::Invalid)?;
    }
    Ok(out)
}
fn encoded(bytes: &[u8]) -> String {
    const DIGITS: &[u8; 16] = b"0123456789abcdef";
    let mut out = String::with_capacity(bytes.len() * 2);
    for byte in bytes {
        out.push(DIGITS[(byte >> 4) as usize] as char);
        out.push(DIGITS[(byte & 15) as usize] as char);
    }
    out
}
fn appearance(v: &Value) -> Result<Option<[u8; 9]>> {
    match v.get("appearance").filter(|v| !v.is_null()) {
        Some(v) => {
            let bytes = hex(v)?;
            check(bytes[4] <= 1)?;
            Ok(Some(bytes))
        }
        None => Ok(None),
    }
}
fn level(v: &Value) -> Result<Option<u32>> {
    v.get("level")
        .filter(|v| !v.is_null())
        .map(|v| number(v, 1, 50))
        .transpose()
}
fn optional_fields(v: &Value, names: &[&str]) -> usize {
    names.iter().filter(|name| v.get(**name).is_some()).count()
}

fn catalog(root: &Value) -> Result<Vec<Playlist>> {
    object(root, 1)?;
    let rows = array(field(root, "playlists")?)?;
    check(rows.len() <= 64)?;
    let mut ids = BTreeSet::new();
    let mut defaults = 0;
    let mut out = Vec::with_capacity(rows.len());
    for row in rows {
        object(row, 10 + optional_fields(row, &["clan_match"]))?;
        let mut p = Playlist {
            id: text(row, "id")?,
            revision: text(row, "revision")?,
            name: text(row, "name")?,
            description: string(field(row, "description")?, 256, false)?,
            ranked: flag(row, "ranked")?,
            quickmatch: flag(row, "quickmatch")?,
            eligible: flag(row, "eligible")?,
            clan_match: row
                .get("clan_match")
                .map(|v| v.as_bool().ok_or(Error::Invalid))
                .transpose()?
                .unwrap_or(false),
            minimum: number(field(row, "minimum_players")?, 2, 16)?,
            ..Default::default()
        };
        check(ids.insert(p.id.clone()) && (!p.clan_match || p.ranked))?;
        p.maximum = number(field(row, "maximum_players")?, p.minimum, 16)?;
        let rank = field(row, "level")?;
        if p.ranked && !(p.clan_match && rank.is_null()) {
            p.level = Some(number(rank, 1, 50)?);
        } else {
            check(rank.is_null())?;
        }
        check(!p.clan_match || p.level.is_some() || !p.eligible)?;
        defaults += usize::from(p.quickmatch);
        out.push(p);
    }
    check(defaults <= 1)?;
    Ok(out)
}

fn player(v: &Value, team: bool) -> Result<Player> {
    object(
        v,
        (if team { 3 } else { 2 }) + optional_fields(v, &["level", "appearance"]),
    )?;
    Ok(Player {
        name: text(v, "name")?,
        team: if team {
            number(field(v, "team")?, 0, 15)?
        } else {
            0
        },
        is_self: flag(v, "self")?,
        level: level(v)?,
        appearance: appearance(v)?,
    })
}

fn ticket(root: &Value, s: &mut Snapshot, compatibility: &str) -> Result<()> {
    object(
        root,
        4 + optional_fields(
            root,
            &["progress", "start_countdown_ms", "start_room", "failure"],
        ),
    )?;
    s.start_supported = root.get("start_countdown_ms").is_some();
    s.start_ms = root
        .get("start_countdown_ms")
        .filter(|v| !v.is_null())
        .map(|v| number(v, 0, 10000))
        .transpose()?;
    if let Some(room) = root.get("start_room") {
        if s.start_ms.is_some() {
            string(room, 64, false)?;
        } else {
            check(s.start_supported && room.is_null())?;
        }
    }
    s.failure_state = None;
    if let Some(failure) = root.get("failure").filter(|v| !v.is_null()) {
        let (code, message) = match string(failure, 64, false)?.as_str() {
            "search_failed" => (1, "Matchmaking failed. Try again."),
            "missing_content" => (5, "A party member is missing required content."),
            "party_changed" => (6, "A member left the party."),
            "party_cancelled" => (4, "Matchmaking failed. Try again."),
            _ => return Err(Error::Invalid),
        };
        return Err(Error::Terminal(code, message.into()));
    }
    let state = text(root, "state")?;
    s.playlist_id = string(field(root, "playlist_id")?, 64, true)?;
    s.queued_players = number(field(root, "queued_players")?, 0, 1000)?;
    s.game = None;
    s.gathering = None;
    if let Some(v) = root.get("progress").filter(|v| !v.is_null()) {
        check(state == "searching")?;
        object(v, 5)?;
        let count = number(field(v, "players")?, 1, 16)?;
        let mut g = Gathering {
            minimum: number(field(v, "minimum_players")?, 2, 16)?,
            ..Default::default()
        };
        g.maximum = number(field(v, "maximum_players")?, g.minimum, 16)?;
        check(count <= g.maximum && count <= s.queued_players)?;
        if !field(v, "wait_ms")?.is_null() {
            g.wait_ms = Some(number(field(v, "wait_ms")?, 1, i32::MAX as u32)?);
            check(count >= g.minimum)?;
        }
        let rows = array(field(v, "roster")?)?;
        check(rows.len() == count as usize)?;
        g.players = rows
            .iter()
            .map(|v| player(v, false))
            .collect::<Result<_>>()?;
        check(g.players.iter().filter(|v| v.is_self).count() == 1)?;
        s.gathering = Some(g);
    }
    if state == "idle" {
        check(s.playlist_id.is_empty() && field(root, "match")?.is_null())?;
        s.status = Status::Ready;
        s.message = "Choose a playlist.".into();
        return Ok(());
    }
    check(!s.playlist_id.is_empty())?;
    if state == "searching" {
        check(field(root, "match")?.is_null())?;
        s.status = Status::Searching;
        s.message = "Searching for players...".into();
        return Ok(());
    }
    check(state == "checking_connections" || state == "awaiting_transport")?;
    let v = field(root, "match")?;
    let complete = v.get("game_id").is_some();
    check(complete || !matchmaking_catalog::OPTIONS.contains(&compatibility))?;
    object(v, if complete { 6 } else { 3 })?;
    let mut game = Match {
        map: text(v, "map")?,
        variant: text(v, "variant")?,
        ..Default::default()
    };
    if complete {
        game.game_id = text(v, "game_id")?;
        if !field(v, "settings")?.is_null() {
            let bytes = hex::<304>(field(v, "settings")?)?;
            check(
                hex::<32>(field(v, "settings_sha256")?)? == <[u8; 32]>::from(Sha256::digest(bytes))
                    && matchmaking_variant::valid(&bytes),
            )?;
            game.settings = Some(bytes);
        } else {
            check(field(v, "settings_sha256")?.is_null())?;
        }
    }
    check(
        game.settings.is_some()
            || !matchmaking_catalog::requires_variant_settings(compatibility, &game.variant),
    )?;
    let rows = array(field(v, "players")?)?;
    check((2..=16).contains(&rows.len()))?;
    game.players = rows
        .iter()
        .map(|v| player(v, true))
        .collect::<Result<_>>()?;
    check(game.players.iter().filter(|v| v.is_self).count() == 1)?;
    s.game = Some(game);
    s.status = if state == "checking_connections" {
        Status::CheckingConnections
    } else {
        Status::AwaitingTransport
    };
    s.message = if state == "checking_connections" {
        "Players found. Checking connections..."
    } else {
        "Match found. Connecting to game..."
    }
    .into();
    Ok(())
}

fn direct(compatibility: &str) -> bool {
    matches!(
        compatibility,
        "halo2-native-queue-v12"
            | "halo2-native-queue-v13"
            | "halo2-native-queue-v14"
            | "halo2-native-queue-v15"
            | "halo2-native-queue-v16"
    )
}
fn gathering(compatibility: &str) -> bool {
    direct(compatibility)
        || matches!(
            compatibility,
            "halo2-native-queue-v8" | "halo2-native-queue-v9" | "halo2-native-queue-v10"
        )
}
fn supports_locals(compatibility: &str) -> bool {
    direct(compatibility)
        || matches!(
            compatibility,
            "halo2-native-queue-v9" | "halo2-native-queue-v10"
        )
}

fn admission(
    root: &Value,
    config: &Config,
    profile: &Profile,
    snapshot: &Snapshot,
) -> Result<Admission> {
    let game = snapshot.game.as_ref().ok_or(Error::Invalid)?;
    let policy = snapshot
        .playlists
        .iter()
        .find(|p| p.id == snapshot.playlist_id)
        .ok_or(Error::Invalid)?;
    let is_direct = direct(&config.compatibility);
    let is_eos = is_direct && config.compatibility != "halo2-native-queue-v12";
    let couch = root.get("local_slots").is_some();
    object(
        root,
        if is_direct {
            7
        } else if couch {
            if config.compatibility == "halo2-native-queue-v10" {
                6
            } else {
                5
            }
        } else {
            4
        },
    )?;
    let team_mode = root
        .get("team_mode")
        .map(|v| v.as_bool().ok_or(Error::Invalid))
        .transpose()?;
    check(
        game.settings
            .as_ref()
            .is_none_or(|settings| team_mode == Some(settings[0x48] & 1 != 0)),
    )?;
    let mut out = Admission {
        endpoint: config.endpoint.clone(),
        room: text(root, "id")?,
        bearer: string(field(root, "token")?, 128, false)?,
        loopback: config.loopback,
        terminal_recovery: config.terminal_recovery,
        direct: is_direct,
        team_mode,
        ..Default::default()
    };
    if is_direct {
        check(couch)?;
        let v = field(root, "transport")?;
        if is_eos {
            object(v, 4)?;
            check(text(v, "kind")? == "eos-p2p-v1")?;
            let peers = array(field(v, "peers")?)?;
            check(peers.len() <= 15)?;
            let session = field(v, "session")?;
            object(session, 4)?;
            let mut eos = Eos {
                socket: string(field(v, "socket")?, 32, false)?,
                account_login: config.compatibility != "halo2-native-queue-v13",
                playlist: text(session, "playlist")?,
                revision: text(session, "revision")?,
                map: text(session, "map")?,
                variant: text(session, "variant")?,
                force_relay: config.force_relay,
                ..Default::default()
            };
            check(
                eos.playlist == snapshot.playlist_id
                    && eos.revision == policy.revision
                    && eos.map == game.map
                    && eos.variant == game.variant,
            )?;
            for peer in peers {
                object(peer, 2)?;
                eos.peers
                    .push((number(field(peer, "slot")?, 0, 15)?, text(peer, "key")?));
            }
            out.eos = Some(eos);
        } else {
            object(v, 2)?;
            check(text(v, "kind")? == "direct-udp-v1")?;
            let servers = array(field(v, "stun_servers")?)?;
            check(servers.len() <= 4)?;
            for v in servers {
                out.stun.push(v.as_str().ok_or(Error::Invalid)?.to_owned());
            }
        }
    }
    check(profile.locals.is_empty() || couch)?;
    let rows = array(field(root, "players")?)?;
    check(rows.len() == game.players.len())?;
    for (i, row) in rows.iter().enumerate() {
        object(row, if couch { 5 } else { 3 })?;
        let seat = Seat {
            slot: i as u32,
            name: text(row, "name")?,
            team: number(field(row, "team")?, 0, 15)?,
            identity: hex(field(row, "identity")?)?,
            peer: if couch {
                number(field(row, "peer")?, 0, rows.len() as u32 - 1)?
            } else {
                i as u32
            },
            guest: couch && flag(row, "guest")?,
            level: if policy.ranked && !(couch && flag(row, "guest")?) {
                game.players[i].level
            } else {
                check(game.players[i].level.is_none())?;
                None
            },
        };
        check(seat.name == game.players[i].name && seat.team == game.players[i].team)?;
        out.seats.push(seat);
    }
    out.current = number(field(root, "slot")?, 0, out.seats.len() as u32 - 1)?;
    check(game.players[out.current as usize].is_self)?;
    if couch {
        let slots = array(field(root, "local_slots")?)?;
        check(slots.len() == 1 + profile.locals.len())?;
        let mut local = BTreeSet::new();
        for (i, slot) in slots.iter().enumerate() {
            let slot = number(slot, 0, out.seats.len() as u32 - 1)?;
            let seat = &out.seats[slot as usize];
            check(local.insert(slot) && seat.peer == out.current)?;
            check(
                &seat.name
                    == if i == 0 {
                        &profile.name
                    } else {
                        &profile.locals[i - 1].name
                    },
            )?;
            check(seat.guest == (i != 0 && !profile.locals[i - 1].signed_in))?;
        }
        check(number(&slots[0], 0, 15)? == out.current && local.contains(&out.current))?;
        for seat in &out.seats {
            check((seat.peer == out.current) == local.contains(&seat.slot))?;
        }
    }
    Ok(out)
}

#[derive(Clone, Default)]
struct Intent {
    active: bool,
    input_valid: bool,
    playlist_valid: bool,
    profile: Profile,
    playlist: String,
    cancel: bool,
}

struct Shared {
    version: u64,
    intent: Intent,
    snapshot: Snapshot,
    connected: Option<Match>,
    room: String,
    transferred: bool,
    rejected_content: bool,
}

#[derive(Default)]
struct Worker {
    previous: Option<u64>,
    session: String,
    configuration: String,
    authorization: String,
    expires: u64,
    queued: bool,
    accepted: bool,
    request_id: String,
    qos_session: String,
    terminal: Option<u64>,
    unsupported_countdown: Option<u64>,
}

/// Commands and publication are serialized independently of blocking IO.
/// The worker's private lease/idempotency state is never a UI credential store.
pub struct Client {
    config: Config,
    configured: bool,
    shared: Mutex<Shared>,
    worker: Mutex<Worker>,
}

fn configure(c: &Config) -> Result<String> {
    let endpoint = crate::matchmaking_origin::pinned_origin(&c.endpoint, &c.url, c.loopback)
        .map_err(|_| Error::Invalid)?;
    check(
        c.login_present
            && !c.maps.is_empty()
            && c.maps.len() <= 256
            && c.poll_ms >= 10
            && c.language <= 8,
    )?;
    string(&json!(c.compatibility), 64, false)?;
    let mut maps = BTreeSet::new();
    for map in &c.maps {
        string(&json!(map), 64, false)?;
        check(maps.insert(map))?;
    }
    Ok(endpoint)
}

impl Client {
    pub fn new(mut config: Config) -> Self {
        let mut snapshot = Snapshot {
            revision: 1,
            message: "No matchmaking service configured.".into(),
            ..Default::default()
        };
        let configured = if !config.input_valid {
            snapshot.status = Status::Failed;
            snapshot.message = "Matchmaking configuration is invalid.".into();
            false
        } else if config.endpoint.is_empty() {
            false
        } else {
            match configure(&config) {
                Ok(endpoint) => {
                    config.endpoint = endpoint;
                    snapshot.status = Status::Idle;
                    snapshot.message = "Choose a playlist to connect.".into();
                    true
                }
                Err(_) => {
                    snapshot.status = Status::Failed;
                    snapshot.message = "Matchmaking configuration is invalid.".into();
                    false
                }
            }
        };
        Self {
            config,
            configured,
            shared: Mutex::new(Shared {
                version: 0,
                intent: Intent::default(),
                snapshot,
                connected: None,
                room: String::new(),
                transferred: false,
                rejected_content: false,
            }),
            worker: Mutex::new(Worker::default()),
        }
    }

    pub fn configured(&self) -> bool {
        self.configured
    }
    pub fn current(&self, version: u64) -> bool {
        self.shared.lock().unwrap().version == version
    }
    pub fn active(&self) -> bool {
        self.configured && self.shared.lock().unwrap().intent.active
    }
    pub fn snapshot(&self) -> Snapshot {
        self.shared.lock().unwrap().snapshot.clone()
    }

    pub fn command(&self, command: Command) -> u64 {
        let mut s = self.shared.lock().unwrap();
        if !self.configured {
            return s.version;
        }
        let cancel = matches!(command, Command::Cancel) && !s.transferred;
        match command {
            Command::Refresh(profile) => {
                s.intent = Intent {
                    active: true,
                    input_valid: true,
                    playlist_valid: true,
                    profile,
                    ..Default::default()
                }
            }
            Command::Join(playlist) => {
                s.intent.active = true;
                s.intent.playlist_valid = true;
                s.intent.playlist = playlist;
            }
            Command::Cancel => {
                s.intent.active = true;
                s.intent.playlist_valid = true;
                s.intent.playlist.clear();
            }
            Command::SignOut => s.intent = Intent::default(),
            Command::Invalid => {
                s.intent.active = true;
                s.intent.playlist_valid = false;
            }
            Command::InvalidProfile => {
                s.intent.active = true;
                s.intent.input_valid = false;
                s.intent.playlist.clear();
            }
        }
        s.intent.cancel = cancel;
        s.version = s.version.wrapping_add(1);
        s.connected = None;
        s.room.clear();
        s.transferred = false;
        s.rejected_content = false;
        let mut next = Snapshot {
            revision: s.snapshot.revision.wrapping_add(1),
            playlists: s.snapshot.playlists.clone(),
            playlist_id: s.intent.playlist.clone(),
            ..Default::default()
        };
        next.status = if s.intent.active {
            if s.intent.playlist.is_empty() {
                Status::Connecting
            } else {
                Status::Joining
            }
        } else {
            Status::Idle
        };
        next.message = if s.intent.active {
            "Connecting to matchmaking..."
        } else {
            "Signed out of matchmaking."
        }
        .into();
        if !s.intent.active {
            next.playlists.clear();
        }
        s.snapshot = next;
        s.version
    }

    pub fn reject_content(&self) -> bool {
        let mut s = self.shared.lock().unwrap();
        if s.room.is_empty() {
            return false;
        }
        s.rejected_content = true;
        true
    }

    pub fn speaker_binding(&self, row: u32, name: &str, team: i32, slot: u32, peer: u32) -> bool {
        let s = self.shared.lock().unwrap();
        s.connected
            .as_ref()
            .and_then(|m| m.players.get(row as usize))
            .is_some_and(|p| {
                p.name == name && i64::from(p.team) == i64::from(team) && slot == row && peer == row
            })
    }

    pub fn take_launch(
        &self,
        mut observation: matchmaking_readiness::NativeLaunchObservation,
        seats: &[matchmaking_readiness::Seat],
    ) -> bool {
        let mut s = self.shared.lock().unwrap();
        observation.transferred = s.transferred;
        observation.start_supported = s.snapshot.start_supported;
        observation.start_ms = s.snapshot.start_ms;
        if s.snapshot.status != Status::Loading
            || s.room.is_empty()
            || s.connected.is_none()
            || !matchmaking_readiness::native_launch_ready(observation, seats)
        {
            return false;
        }
        s.transferred = true;
        true
    }

    fn ensure(&self, io: &impl Io, version: u64) -> Result<()> {
        if self.current(version) && !io.cancelled(version) {
            Ok(())
        } else {
            Err(Error::Cancelled)
        }
    }

    fn publish(&self, mut next: Snapshot, version: u64) {
        let mut s = self.shared.lock().unwrap();
        if s.version != version {
            return;
        }
        next.revision = s.snapshot.revision.wrapping_add(1);
        s.snapshot = next;
    }

    fn request(
        &self,
        io: &impl Io,
        version: u64,
        method: &str,
        path: &str,
        body: &Value,
        bearer: &str,
    ) -> Result<Response> {
        self.ensure(io, version)?;
        io.request(&Request {
            version,
            method: method.into(),
            url: self.config.endpoint.clone() + path,
            body: if body.is_null() {
                String::new()
            } else {
                body.to_string()
            },
            bearer: bearer.into(),
            cleanup: false,
        })
    }

    fn call(
        &self,
        io: &impl Io,
        version: u64,
        w: &Worker,
        method: &str,
        path: &str,
        body: &Value,
    ) -> Result<Value> {
        let response = self.request(io, version, method, path, body, &w.session)?;
        if response.status != 200 && response.status != 201 {
            let body = parse(&response.body)?;
            let message = if body.is_object() && body.get("error").is_some() {
                string(field(&body, "error")?, 256, false)?
            } else {
                String::new()
            };
            return Err(Error::Status(response.status, message));
        }
        parse(&response.body)
    }

    fn cleanup(&self, io: &impl Io, version: u64, w: &Worker, path: &str) {
        if !w.session.is_empty() {
            // This deliberately uses the old owned capability after cancellation.
            // A stale successful POST must still have its ticket revoked.
            let _ = io.request(&Request {
                version,
                method: "DELETE".into(),
                url: self.config.endpoint.clone() + path,
                body: String::new(),
                bearer: w.session.clone(),
                cleanup: true,
            });
        }
    }

    fn disconnect(&self, io: &impl Io, version: u64) {
        io.disconnect(version);
        let mut s = self.shared.lock().unwrap();
        if s.version == version {
            s.connected = None;
            s.room.clear();
            s.transferred = false;
        }
    }

    fn transferred(&self, version: u64) -> bool {
        let s = self.shared.lock().unwrap();
        s.version == version && s.transferred
    }

    fn reset_queue(w: &mut Worker) {
        w.queued = false;
        w.accepted = false;
        w.request_id.clear();
    }

    fn qos(&self, io: &impl Io, version: u64, w: &mut Worker, token: &str) -> Result<()> {
        if w.qos_session != token {
            if !io.qos(version, &self.config.endpoint, token, self.config.loopback)? {
                return Err(Error::Cancelled);
            }
            w.qos_session = token.into();
        }
        Ok(())
    }

    fn session_body(&self, profile: &Profile, identity: &Identity) -> Result<Value> {
        string(&json!(profile.name), 64, false)?;
        let mut body = json!({"screen_name":profile.name,"maps":self.config.maps,"compatibility":self.config.compatibility});
        if let Some(bytes) = profile.appearance {
            check(bytes[4] <= 1)?;
            body["appearance"] = json!(encoded(&bytes));
        }
        if gathering(&self.config.compatibility) {
            body["language"] = json!(self.config.language);
        }
        check(profile.locals.len() <= 3 && identity.locals.len() == profile.locals.len())?;
        if !profile.locals.is_empty() {
            check(supports_locals(&self.config.compatibility))?;
            let mut locals = Vec::new();
            for (player, grant) in profile.locals.iter().zip(&identity.locals) {
                string(&json!(player.name), 64, false)?;
                let mut row = json!({"screen_name":player.name});
                if let Some(bytes) = player.appearance {
                    check(bytes[4] <= 1)?;
                    row["appearance"] = json!(encoded(&bytes));
                }
                if player.signed_in {
                    let token = grant.authorization.as_ref().ok_or(Error::Invalid)?;
                    row[if grant.steam {
                        "identity_token"
                    } else {
                        "discord_token"
                    }] = json!(token);
                }
                locals.push(row);
            }
            body["local_players"] = json!(locals);
        }
        Ok(body)
    }

    pub fn poll(&self, io: &impl Io) {
        if !self.configured {
            return;
        }
        let mut w = self.worker.lock().unwrap();
        let (version, intent, rejected, mut next) = {
            let mut s = self.shared.lock().unwrap();
            let rejected = if std::mem::take(&mut s.rejected_content) {
                Some(s.room.clone())
            } else {
                None
            };
            (s.version, s.intent.clone(), rejected, s.snapshot.clone())
        };
        let changed = w.previous != Some(version);
        if changed {
            if w.queued {
                self.cleanup(
                    io,
                    version,
                    &w,
                    if intent.cancel {
                        "/v1/matchmaking/ticket?cancel=1"
                    } else {
                        TICKET
                    },
                );
            }
            Self::reset_queue(&mut w);
            w.previous = Some(version);
        }
        let result = self.step(io, version, &intent, rejected, changed, &mut w, &mut next);
        if let Err(error) = result {
            self.failure(io, version, &mut w, &mut next, error);
        }
    }

    fn step(
        &self,
        io: &impl Io,
        version: u64,
        intent: &Intent,
        rejected: Option<String>,
        changed: bool,
        w: &mut Worker,
        next: &mut Snapshot,
    ) -> Result<()> {
        let identity = if intent.active {
            self.ensure(io, version)?;
            io.identity(version)?
        } else {
            Identity::default()
        };
        if !intent.active || identity.primary.is_none() {
            self.qos(io, version, w, "")?;
            self.cleanup(io, version, w, SESSION);
            w.session.clear();
            w.configuration.clear();
            w.authorization.clear();
            *next = Snapshot {
                status: Status::Idle,
                message: "Sign in with Discord to use matchmaking.".into(),
                ..Default::default()
            };
            self.publish(next.clone(), version);
            return Ok(());
        }
        if w.unsupported_countdown == Some(version) || w.terminal == Some(version) {
            return Ok(());
        }
        check(intent.input_valid && intent.playlist_valid)?;
        if let Some(room) = rejected.filter(|room| !room.is_empty() && !w.session.is_empty()) {
            let _ = self.call(
                io,
                version,
                w,
                "POST",
                "/v1/matchmaking/failure",
                &json!({"match_id":room,"kind":"missing_content"}),
            );
            return Err(Error::Terminal(
                5,
                "Required match content is unavailable.".into(),
            ));
        }
        let authorization = identity.primary.as_ref().ok_or(Error::Invalid)?;
        let mut body = self.session_body(&intent.profile, &identity)?;
        let configuration = body.to_string();
        if w.session.is_empty()
            || configuration != w.configuration
            || authorization != &w.authorization
            || io.now_ms() >= w.expires
        {
            if !w.session.is_empty() {
                body["renew_session"] = json!(w.session);
            }
            let response = self.request(io, version, "POST", SESSION, &body, authorization)?;
            if response.status != 201 {
                return Err(Error::Status(response.status, String::new()));
            }
            let body = parse(&response.body)?;
            object(&body, 2)?;
            let token = string(field(&body, "token")?, 128, false)?;
            check(
                token.len() >= 32
                    && token
                        .bytes()
                        .all(|v| v.is_ascii_alphanumeric() || v == b'-' || v == b'_'),
            )?;
            let rotated = token != w.session;
            // Own a successful exchange even if a concurrent command cancels
            // later IO. The next generation must be able to revoke this token.
            w.session = token;
            w.configuration = configuration;
            w.authorization = authorization.clone();
            if rotated {
                self.qos(io, version, w, "")?;
                Self::reset_queue(w);
                next.playlists.clear();
                if !self.transferred(version) {
                    self.disconnect(io, version);
                }
            }
            w.expires = io
                .now_ms()
                .saturating_add(u64::from(number(field(&body, "expires_in")?, 1, 900)?) * 750);
            if gathering(&self.config.compatibility) {
                let token = w.session.clone();
                self.qos(io, version, w, &token)?;
            }
        }
        // Gameplay owns the transferred connection and its immutable room.
        // Lease/configuration renewal cannot become an implicit new search.
        // Explicit commands retire this fence in command(), as before.
        if self.transferred(version) {
            return Ok(());
        }
        if changed || next.playlists.is_empty() {
            next.playlists = catalog(&self.call(
                io,
                version,
                w,
                "GET",
                "/v1/matchmaking/playlists",
                &Value::Null,
            )?)?;
        }
        if intent.playlist.is_empty() && !w.accepted {
            ticket(
                &self.call(io, version, w, "GET", PROGRESS, &Value::Null)?,
                next,
                &self.config.compatibility,
            )?;
            if next.status != Status::Ready {
                w.queued = true;
                w.accepted = true;
            }
        } else if !w.accepted {
            let entry = next
                .playlists
                .iter()
                .find(|p| p.id == intent.playlist)
                .ok_or(Error::Invalid)?;
            check(entry.eligible)?;
            if w.request_id.is_empty() {
                let mut bytes = [0; 16];
                getrandom::getrandom(&mut bytes).map_err(|_| Error::Unavailable)?;
                bytes[6] = (bytes[6] & 15) | 64;
                bytes[8] = (bytes[8] & 63) | 128;
                let hex = encoded(&bytes);
                w.request_id = format!(
                    "{}-{}-{}-{}-{}",
                    &hex[..8],
                    &hex[8..12],
                    &hex[12..16],
                    &hex[16..20],
                    &hex[20..]
                );
            }
            let body = json!({"playlist_id":intent.playlist,"revision":entry.revision,"request_id":w.request_id});
            w.queued = true;
            ticket(
                &self.call(io, version, w, "POST", TICKET, &body)?,
                next,
                &self.config.compatibility,
            )?;
            w.accepted = true;
            if next.game.is_some() {
                ticket(
                    &self.call(io, version, w, "GET", PROGRESS, &Value::Null)?,
                    next,
                    &self.config.compatibility,
                )?;
            }
        } else {
            self.ensure(io, version)?;
            let (transferred, connected, installed_room) = {
                let s = self.shared.lock().unwrap();
                (s.transferred, s.connected.clone(), s.room.clone())
            };
            if transferred {
                return Ok(());
            }
            let connection = io.connection(version)?;
            if connection.phase != ConnectionPhase::Missing {
                next.game = connected;
                if connection.phase == ConnectionPhase::Failed {
                    let mut outcome = Snapshot::default();
                    ticket(
                        &self.call(
                            io,
                            version,
                            w,
                            "GET",
                            "/v1/matchmaking/ticket?failure=1",
                            &Value::Null,
                        )?,
                        &mut outcome,
                        &self.config.compatibility,
                    )?;
                    self.disconnect(io, version);
                    self.cleanup(io, version, w, TICKET);
                    Self::reset_queue(w);
                    return Err(Error::Terminal(1, connection.error));
                }
                next.status = if connection.phase == ConnectionPhase::Connecting {
                    Status::CheckingConnections
                } else {
                    Status::Loading
                };
                next.message = if next.status == Status::Loading {
                    "Loading ONLINE match..."
                } else {
                    "Checking connections..."
                }
                .into();
                next.gathering = None;
                let receipt = self.call(io, version, w, "GET", BOUND_START, &Value::Null)?;
                let mut progress = Snapshot::default();
                ticket(&receipt, &mut progress, &self.config.compatibility)?;
                next.start_supported = progress.start_supported;
                next.start_ms = progress.start_ms;
                if next.status == Status::Loading {
                    let binding = receipt.get("start_room");
                    if !next.start_supported || binding.is_none() {
                        return Err(Error::UnsupportedCountdown);
                    }
                    if next.start_ms.is_some() {
                        check(
                            binding.and_then(Value::as_str) == Some(installed_room.as_str())
                                && next.playlist_id == progress.playlist_id
                                && next.game.as_ref().zip(progress.game.as_ref()).is_some_and(
                                    |(installed, observed)| installed.matches_countdown(observed),
                                ),
                        )?;
                    }
                } else {
                    next.start_ms = None;
                }
            } else {
                ticket(
                    &self.call(io, version, w, "GET", PROGRESS, &Value::Null)?,
                    next,
                    &self.config.compatibility,
                )?;
            }
            if next.status == Status::Ready {
                Self::reset_queue(w);
            }
        }
        if next.game.is_some()
            && matches!(
                next.status,
                Status::CheckingConnections | Status::AwaitingTransport
            )
            && io.connection(version)?.phase == ConnectionPhase::Missing
        {
            let descriptor = self.call(
                io,
                version,
                w,
                "GET",
                "/v1/matchmaking/transport",
                &Value::Null,
            )?;
            let admitted = admission(&descriptor, &self.config, &intent.profile, next)?;
            self.ensure(io, version)?;
            if io.install(version, &admitted)? {
                let mut s = self.shared.lock().unwrap();
                if s.version == version {
                    s.connected = next.game.clone();
                    s.room = admitted.room;
                }
            }
        }
        self.publish(next.clone(), version);
        Ok(())
    }

    fn failure(
        &self,
        io: &impl Io,
        version: u64,
        w: &mut Worker,
        next: &mut Snapshot,
        error: Error,
    ) {
        if error == Error::Cancelled {
            return;
        }
        next.status = Status::Failed;
        next.game = None;
        next.gathering = None;
        next.start_ms = None;
        next.message = match error {
            Error::Terminal(state, message) => {
                self.disconnect(io, version);
                self.cleanup(io, version, w, TICKET);
                Self::reset_queue(w);
                w.terminal = Some(version);
                next.failure_state = Some(state);
                if message.is_empty() {
                    "Matchmaking failed. Try again.".into()
                } else {
                    message
                }
            }
            Error::UnsupportedCountdown => {
                self.disconnect(io, version);
                self.cleanup(io, version, w, TICKET);
                Self::reset_queue(w);
                w.unsupported_countdown = Some(version);
                next.start_supported = false;
                "Update the matchmaking service to support the room-bound game-start countdown."
                    .into()
            }
            Error::Status(status, message) => {
                if status == 401 {
                    let _ = self.qos(io, version, w, "");
                    w.session.clear();
                    w.configuration.clear();
                    w.authorization.clear();
                    Self::reset_queue(w);
                }
                if status == 409 {
                    self.cleanup(io, version, w, TICKET);
                    Self::reset_queue(w);
                    next.playlists.clear();
                }
                if !message.is_empty() && status != 401 {
                    message
                } else if status == 503 {
                    "Matchmaking playlists are not configured on this service.".into()
                } else if status == 401 {
                    "Matchmaking authorization expired. Reconnecting...".into()
                } else {
                    "Matchmaking request failed. Reconnecting...".into()
                }
            }
            _ => "Matchmaking service unavailable. Retrying...".into(),
        };
        self.publish(next.clone(), version);
    }

    pub fn shutdown(&self, io: &impl Io) {
        let mut w = self.worker.lock().unwrap();
        let version = self.shared.lock().unwrap().version;
        let _ = self.qos(io, version, &mut w, "");
        self.cleanup(io, version, &w, SESSION);
        w.session.clear();
        w.configuration.clear();
        w.authorization.clear();
        Self::reset_queue(&mut w);
    }
}
