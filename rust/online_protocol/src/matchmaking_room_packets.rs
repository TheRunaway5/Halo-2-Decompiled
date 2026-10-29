//! Native packet roles, voice proofs/privacy and bounded relay dispositions.
//! Modern H2N2/Ed25519/abuse policy is distinct from original Xbox wire recovery.
use super::wire;
use super::{Coordinator, Setup, Stage, m, reject};
use crate::matchmaking_model::{Error, Result};
use crate::matchmaking_packet_budget::Stream;
use ed25519_dalek::{Signature, SigningKey, VerifyingKey};
use serde_json::{Value, json};
use std::collections::{BTreeMap, BTreeSet};
struct VoiceKey {
    signing: Option<SigningKey>,
    verifying: VerifyingKey,
}
pub(super) struct Packets {
    keys: BTreeMap<String, VoiceKey>,
}
fn hex(bytes: &[u8]) -> String {
    bytes.iter().map(|b| format!("{b:02x}")).collect()
}
impl Packets {
    pub fn new(owners: &[String]) -> Result<Self> {
        let mut keys = BTreeMap::new();
        for owner in owners {
            let mut seed = [0; 32];
            getrandom::getrandom(&mut seed).map_err(|_| Error(503, "Voice entropy unavailable"))?;
            let signing = SigningKey::from_bytes(&seed);
            let verifying = signing.verifying_key();
            seed.fill(0);
            keys.insert(
                owner.clone(),
                VoiceKey {
                    signing: Some(signing),
                    verifying,
                },
            );
        }
        Ok(Self { keys })
    }
    pub fn retire(&mut self, who: &str) {
        if let Some(key) = self.keys.get_mut(who) {
            key.signing = None;
        }
    }
    pub fn clear(&mut self) {
        self.keys.clear();
    }
}
fn packet(r: &Value) -> Result<Vec<u8>> {
    let s = r["packet"]
        .as_str()
        .ok_or(Error(400, "Packet bytes required"))?;
    m::check(
        s.len() <= 2 * 1405 && s.len() % 2 == 0,
        "Invalid game packet",
    )?;
    m::hex(&r["packet"], s.len() / 2)
}
fn drop() -> Value {
    json!({"kind":"drop","deliver":null,"close_peer":null})
}
fn now(row: &Setup, r: &Value) -> Result<f64> {
    r.get("packet_now").map(m::real).unwrap_or(Ok(row.life.now))
}
fn budget(row: &mut Setup, r: &Value, who: &str, stream: Stream) -> Result<()> {
    let now = now(row, r)?;
    row.wire.budget(who, stream, now)
}
fn subject<'a>(row: &'a Setup, slot: usize) -> &'a str {
    let p = &row.players[slot];
    if p.guest { &p.owner } else { &p.id }
}
pub(super) fn pairs(row: &Setup, who: &str) -> Result<Vec<(String, String)>> {
    row.actor(who)?;
    let local = row
        .players
        .iter()
        .enumerate()
        .filter(|(_, p)| p.owner == who)
        .map(|(i, _)| subject(row, i))
        .collect::<BTreeSet<_>>();
    let other = row
        .players
        .iter()
        .enumerate()
        .filter(|(_, p)| p.owner != who)
        .map(|(i, _)| subject(row, i))
        .collect::<BTreeSet<_>>();
    Ok(local
        .iter()
        .flat_map(|a| {
            other
                .iter()
                .map(move |b| ((*a).to_owned(), (*b).to_owned()))
        })
        .collect())
}
pub(super) fn mask(row: &Setup, who: &str, facts: &Value) -> Result<u32> {
    let expected = pairs(row, who)?;
    let rows = facts
        .as_array()
        .ok_or(Error(400, "Voice relationship facts required"))?;
    m::check(
        rows.len() == expected.len(),
        "Incomplete voice relationship facts",
    )?;
    let mut allowed = BTreeMap::new();
    for v in rows {
        m::fields(v, &["a", "b", "allowed"])?;
        let key = (
            m::text(&v["a"], 64)?.to_owned(),
            m::text(&v["b"], 64)?.to_owned(),
        );
        m::check(
            expected.contains(&key) && !allowed.contains_key(&key),
            "Foreign or duplicate voice relationship",
        )?;
        allowed.insert(key, m::boolean(&v["allowed"])?);
    }
    Ok(row
        .players
        .iter()
        .enumerate()
        .fold(0u32, |bits, (slot, p)| {
            bits | if p.owner != who
                && allowed
                    .iter()
                    .any(|((_, b), ok)| b == subject(row, slot) && !*ok)
            {
                1 << slot
            } else {
                0
            }
        }))
}
fn privacy(row: &Setup, who: &str, facts: &Value) -> Result<Option<u32>> {
    if facts.is_null() {
        return Ok(None);
    }
    Ok(Some(mask(row, who, facts)?))
}
fn plan(row: &Setup, who: &str) -> Result<Value> {
    Ok(json!({"kind":"privacy","pairs":pairs(row,who)?}))
}
fn version(row: &Setup, who: &str) -> Result<u16> {
    let value = row
        .loaded
        .get(who)
        .ok_or(Error(409, "Native protocol is not loaded"))?;
    let (_, tail) = value
        .split_once(':')
        .ok_or(Error(400, "Invalid native protocol admission"))?;
    u16::from_str_radix(tail, 16).map_err(|_| Error(400, "Invalid native protocol admission"))
}
fn voice_message(id: &str, epoch: u32, recipient: u8, body: &[u8]) -> Vec<u8> {
    let mut out = b"H2VOICE2\0".to_vec();
    out.extend_from_slice(id.as_bytes());
    out.extend_from_slice(&epoch.to_le_bytes());
    out.push(recipient);
    out.extend_from_slice(body);
    out
}
fn voice(
    row: &Setup,
    id: &str,
    who: &str,
    source: usize,
    target: usize,
    epoch: u32,
    native: &[u8],
) -> Result<(usize, bool)> {
    m::check(native.len() >= 11, "Invalid voice packet")?;
    let subtype = native[7];
    m::check(subtype <= 2, "Invalid voice control")?;
    let speaker = if subtype == 0 {
        m::check(
            native.len() > 86 && native.len() <= 342,
            "Voice origin proof required",
        )?;
        let speaker = native[8] as usize;
        m::check(speaker < row.players.len(), "Invalid voice identity")?;
        let owner = &row.players[speaker].owner;
        reject(
            row.life.active.contains(owner),
            400,
            "Departed voice identity",
        )?;
        m::check(owner == who, "Invalid voice identity")?;
        let body = &native[..native.len() - 64];
        let signature = Signature::from_slice(&native[native.len() - 64..])
            .map_err(|_| Error(400, "Invalid voice origin proof"))?;
        let key = row
            .packets
            .keys
            .get(owner)
            .ok_or(Error(400, "Voice key retired"))?;
        key.verifying
            .verify_strict(&voice_message(id, epoch, target as u8, body), &signature)
            .map_err(|_| Error(400, "Invalid voice origin proof"))?;
        speaker
    } else {
        m::check(
            native.len() == if subtype == 1 { 11 } else { 12 },
            "Invalid voice preferences",
        )?;
        m::check(native[8] <= 1, "Invalid voice preferences")?;
        if subtype == 2 {
            m::check(
                Some(source) == row.authority() && (native[11] as usize) < row.players.len(),
                "Invalid voice membership",
            )?;
            native[11] as usize
        } else {
            source
        }
    };
    Ok((speaker, subtype == 0))
}
fn peers(r: &Value, who: &str) -> bool {
    r["peers"]
        .as_array()
        .is_some_and(|p| p.iter().any(|p| p["player"] == who))
}
fn full(r: &Value, who: &str) -> bool {
    r["full"]
        .as_array()
        .is_some_and(|p| p.iter().any(|p| p == who))
}
pub(super) fn auth(row: &Setup, who: &str) -> Result<Value> {
    row.actor(who)?;
    reject(
        row.life.active.contains(who),
        401,
        "Match voice authorization retired",
    )?;
    let signing = row
        .packets
        .keys
        .get(who)
        .and_then(|k| k.signing.as_ref())
        .ok_or(Error(401, "Voice key retired"))?;
    Ok(
        json!({"voice_auth":{"version":1,"seed":hex(&signing.to_bytes()),"keys":row.players.iter().map(|p|hex(&row.packets.keys[&p.owner].verifying.to_bytes())).collect::<Vec<_>>()}}),
    )
}
pub(super) fn execute(row: &mut Setup, core: &mut Coordinator, r: &Value) -> Result<Value> {
    row.life.observe(r, core.clock)?;
    let op = m::text(&r["op"], 64)?;
    let who = wire::current(row, r)?;
    let source = row.actor(&who)?;
    if op == "room_packets_voice_auth" {
        return auth(row, &who);
    }
    if op == "room_packets_privacy_plan" {
        return Ok(json!(pairs(row, &who)?));
    }
    if op == "room_packets_mask" {
        return Ok(json!(mask(row, &who, &r["facts"])?));
    }
    if op == "room_packets_control" {
        budget(row, r, &who, Stream::Control)?;
        return Ok(Value::Null);
    }
    m::check(
        ["room_packets_route", "room_packets_delivery"].contains(&op),
        "Unknown native packet operation",
    )?;
    let packet = packet(r)?;
    let offset = if row.continuity { 5 } else { 1 };
    m::check(
        packet.len() >= offset + 7 && packet.len() <= offset + 1400,
        "Invalid game packet",
    )?;
    let epoch = if row.continuity {
        u32::from_le_bytes(packet[1..5].try_into().unwrap())
    } else {
        0
    };
    let native = &packet[offset..];
    let incoming = op == "room_packets_route";
    if incoming {
        reject(
            !row.direct,
            400,
            "Gameplay and voice require a direct player connection",
        )?;
        if row.continuity
            && (row
                .handoff
                .as_ref()
                .is_none_or(|h| !h.active(source) || h.epoch() != epoch)
                || ["migrating", "awaiting_world", "rebinding"].contains(&row.phase()))
        {
            budget(row, r, &who, Stream::Control)?;
            return Ok(drop());
        }
    } else if row.continuity && row.handoff.as_ref().is_none_or(|h| h.epoch() != epoch) {
        return Ok(drop());
    }
    m::check(
        ["running", "reviewing", "postgame"].contains(&row.phase()),
        "Invalid game packet phase",
    )?;
    let target = if incoming {
        packet[0] as usize
    } else {
        m::integer(&r["target"], 0, (row.players.len() - 1) as i64)? as usize
    };
    let sender = if incoming { source } else { packet[0] as usize };
    m::check(
        sender < row.players.len() && target < row.players.len(),
        "Invalid packet route",
    )?;
    let origin = &row.players[sender].owner;
    if !incoming {
        m::check(
            row.players[target].owner == who,
            "Packet recipient differs from native route",
        )?;
        if !row.life.active.contains(origin) {
            return Ok(drop());
        }
        if r["origin_connection"]
            .as_u64()
            .is_none_or(|generation| row.wire.connection(origin) != Some(generation))
            || r["recipient_connection"]
                .as_u64()
                .is_none_or(|generation| row.wire.connection(&who) != Some(generation))
        {
            return Ok(drop());
        }
    }
    m::check(&native[..4] == b"H2N2", "Invalid native packet")?;
    m::check(
        u16::from_le_bytes(native[4..6].try_into().unwrap()) == version(row, origin)?,
        "Native packet differs from the admitted gameplay protocol",
    )?;
    let kind = native[6];
    if matches!(row.stage, Stage::Reviewing | Stage::Postgame) && kind != 6 {
        if incoming {
            budget(row, r, &who, Stream::Control)?;
        }
        return Ok(drop());
    }
    let audio = if kind == 6 {
        Some(voice(
            row,
            m::text(&r["id"], 64)?,
            origin,
            sender,
            target,
            epoch,
            native,
        )?)
    } else {
        None
    };
    if incoming {
        if kind == 6 {
            m::check(target != source, "Invalid voice route")?;
        } else if Some(source) == row.authority() {
            m::check(
                target != source && [3, 5, 14].contains(&kind),
                "Invalid host route",
            )?;
        } else {
            m::check(
                Some(target) == row.authority() && [1, 2, 4].contains(&kind),
                "Invalid client route",
            )?;
        }
    }
    if let Some((speaker, true)) = audio {
        // Host delivery retains the existing native relay exception. Incoming
        // privacy still applies to every logical recipient.
        if incoming || Some(source) != row.authority() {
            let recipient = &row.players[target].owner;
            let Some(mask) = privacy(row, recipient, &r["facts"])? else {
                return plan(row, recipient);
            };
            if mask & (1 << speaker) != 0 {
                if incoming {
                    budget(row, r, &who, Stream::Audio(speaker as u8, target as u8))?;
                }
                return Ok(drop());
            }
        }
    }
    if !incoming {
        return Ok(json!({"kind":"send"}));
    }
    if let Some((speaker, _)) = audio {
        budget(row, r, &who, Stream::Audio(speaker as u8, target as u8))?;
    } else {
        budget(row, r, &who, Stream::Game(target as u8))?;
    }
    let destination = row.players[target].owner.clone();
    if !peers(r, &destination) && (row.stage == Stage::Postgame || row.continuity) {
        return Ok(drop());
    }
    if peers(r, &destination) && full(r, &destination) && row.continuity {
        return Ok(json!({"kind":"drop","deliver":null,"close_peer":destination}));
    }
    reject(
        peers(r, &destination)
            && row.wire.connection(&destination).is_some()
            && !full(r, &destination),
        503,
        "Match reader stalled",
    )?;
    let mut output = vec![source as u8];
    if row.continuity {
        output.extend_from_slice(&epoch.to_le_bytes());
    }
    output.extend_from_slice(native);
    Ok(
        json!({"kind":"route","deliver":destination,"packet":hex(&output),"target":target,"close_peer":null,
            "origin_connection":row.wire.connection(&who),"recipient_connection":row.wire.connection(&destination)}),
    )
}
