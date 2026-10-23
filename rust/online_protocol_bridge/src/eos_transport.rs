//! EOS state and admission policy, with generated CXX ownership at the SDK ABI.
use crate::{ffi, eos_config::configuration};
use halo2_online_protocol::eos_wire::{self, Reassembly};
use std::{
    collections::{BTreeMap, BTreeSet, VecDeque},
    time::Instant,
};

type Error = Box<dyn std::error::Error>;
struct Peer {
    key: [u8; 32],
    user: String,
    accepted: bool,
    ready: bool,
    failed: bool,
    relayed: bool,
    session_joined: bool,
    registration_requested: bool,
    registered: bool,
    reader: Reassembly,
}
pub struct EosTransport {
    config: ffi::EosConfig,
    raw: cxx::UniquePtr<ffi::RawEos>,
    self_slot: u8,
    peers: BTreeMap<u8, Peer>,
    socket: String,
    force: bool,
    local: String,
    signals: Vec<ffi::EosSignal>,
    packets: VecDeque<ffi::EosPacket>,
    pending_requests: BTreeMap<String, u64>,
    sequence: u32,
    started: Instant,
    last_hello: u64,
    session: ffi::EosSession,
    session_owner: u8,
    session_started: bool,
    session_ready: bool,
    session_joining: bool,
    invited: BTreeSet<u8>,
    invitations: VecDeque<(String, String)>,
    last_invite_query: Option<u64>,
}

fn valid_id(value: &str) -> bool {
    value.len() == 32
        && value
            .bytes()
            .all(|b| b.is_ascii_hexdigit() && !b.is_ascii_uppercase())
}
fn key(value: &str) -> Result<[u8; 32], Error> {
    if value.len() != 64 || !value.bytes().all(|b| b.is_ascii_hexdigit()) {
        return Err("Invalid EOS room key".into());
    }
    let mut result = [0; 32];
    for (index, byte) in result.iter_mut().enumerate() {
        *byte = u8::from_str_radix(&value[index * 2..index * 2 + 2], 16)?;
    }
    Ok(result)
}
/// Short-lived proprietary OpenID credential from the authenticated master.
/// This identity is separate from each PC's anonymous P2P transport identity.
pub fn eos_account_identity(token: &str) -> Result<ffi::EosIdentity, Error> {
    if token.is_empty() || token.len() > 8192 || token.bytes().any(|b| b <= 32 || b == 127) {
        return Err("Invalid EOS account credential".into());
    }
    let mut config = configuration()?;
    config.cache = std::path::Path::new(&config.cache)
        .join("profiles")
        .to_string_lossy()
        .into_owned();
    std::fs::create_dir_all(&config.cache)?;
    let mut raw = ffi::new_raw_eos(&config)?;
    raw.pin_mut().sdk_external_login(token)?;
    let deadline = Instant::now() + std::time::Duration::from_secs(20);
    let mut creating = false;
    while Instant::now() < deadline {
        for event in raw.pin_mut().sdk_poll()? {
            match (event.kind, event.code) {
                (2, 2) if !creating => {
                    creating = true;
                    raw.pin_mut().sdk_create_user()?;
                }
                (2 | 3, 0) if valid_id(&event.user) => {
                    let token = raw.sdk_id_token()?;
                    if token.is_empty() || token.len() > 8192 {
                        return Err("Invalid EOS identity proof".into());
                    }
                    return Ok(ffi::EosIdentity {
                        user: event.user,
                        token,
                    });
                }
                (2 | 3, _) => return Err("EOS account sign-in failed".into()),
                _ => {}
            }
        }
        std::thread::sleep(std::time::Duration::from_millis(2));
    }
    Err("EOS account sign-in timed out".into())
}

#[cfg(test)]
mod lifetime_tests {
    use super::*;

    // Reaches the actual SDK adapter used by profile login followed by a match.
    // Ordinary offline tests must not need credentials or the proprietary SDK.
    #[test]
    #[ignore = "requires the licensed EOS SDK and a private untrusted client configuration"]
    fn sdk_platforms_survive_release_and_recreation() {
        assert!(ffi::eos_sdk_available(), "The official EOS SDK is required");
        let mut config = configuration().expect("Private client configuration");
        let cache = std::path::PathBuf::from(&config.cache);
        let mut platform = |name: &str| {
            let path = cache.join(name);
            std::fs::create_dir_all(&path).expect("Owned cache directory");
            config.cache = path.to_string_lossy().into_owned();
            ffi::new_raw_eos(&config).expect("EOS platform recreation must remain available")
        };
        drop(platform("profile"));
        drop(platform("first-match"));
        let active_match = platform("active-match");
        assert!(!ffi::sdk_process_shutdown(), "An active match must retain its SDK");
        drop(platform("concurrent-profile"));
        drop(platform("replacement-profile"));
        drop(active_match);
        drop(platform("next-match"));
        assert!(ffi::sdk_process_shutdown(), "Finalize before static destruction");
        assert!(ffi::sdk_process_shutdown(), "Shutdown is idempotent");
    }
}

pub fn new_eos_transport(
    self_slot: u8,
    rows: Vec<ffi::EosPeer>,
    socket: &str,
    force_relay: bool,
    session: ffi::EosSession,
) -> Result<Box<EosTransport>, Error> {
    if !ffi::eos_sdk_available() {
        return Err("This build does not include the EOS SDK".into());
    }
    if self_slot >= 16 || rows.len() > 15 || !valid_id(socket) {
        return Err("Invalid EOS room".into());
    }
    if session.enabled
        && [
            &session.playlist,
            &session.revision,
            &session.map,
            &session.variant,
        ]
        .iter()
        .any(|value| {
            value.is_empty() || value.len() > 64 || value.bytes().any(|b| b < 32 || b == 127)
        })
    {
        return Err("Invalid EOS playlist session metadata".into());
    }
    let session_owner = rows
        .iter()
        .map(|row| row.slot)
        .chain([self_slot])
        .min()
        .unwrap_or(self_slot);
    let mut peers = BTreeMap::new();
    for row in rows {
        if row.slot >= 16 || row.slot == self_slot || peers.contains_key(&row.slot) {
            return Err("Invalid EOS physical roster".into());
        }
        peers.insert(
            row.slot,
            Peer {
                key: key(&row.key)?,
                user: String::new(),
                accepted: false,
                ready: false,
                failed: false,
                relayed: false,
                session_joined: false,
                registration_requested: false,
                registered: false,
                reader: Reassembly::default(),
            },
        );
    }
    Ok(Box::new(EosTransport {
        config: configuration()?,
        raw: cxx::UniquePtr::null(),
        self_slot,
        peers,
        socket: socket.to_owned(),
        force: force_relay,
        local: String::new(),
        signals: vec![],
        packets: VecDeque::new(),
        pending_requests: BTreeMap::new(),
        sequence: 0,
        started: Instant::now(),
        last_hello: 0,
        session_ready: !session.enabled,
        session,
        session_owner,
        session_started: false,
        session_joining: false,
        invited: BTreeSet::new(),
        invitations: VecDeque::new(),
        last_invite_query: None,
    }))
}

pub fn eos_signal(transport: &mut EosTransport, slot: u8, user: &str) -> Result<(), Error> {
    if !valid_id(user)
        || user == transport.local
        || transport
            .peers
            .iter()
            .any(|(&id, p)| id != slot && p.user == user)
    {
        return Err("Invalid EOS peer identity".into());
    }
    let peer = transport
        .peers
        .get_mut(&slot)
        .ok_or("Unadmitted EOS slot")?;
    if !peer.user.is_empty() && peer.user != user {
        return Err("EOS peer identity changed".into());
    }
    peer.user = user.to_owned();
    Ok(())
}
pub fn eos_tick(t: &mut EosTransport) -> Result<(), Error> {
    if t.raw.is_null() {
        t.raw = ffi::new_raw_eos(&t.config)?;
        t.raw.pin_mut().sdk_device()?;
    }
    let now = t.started.elapsed().as_millis().min(u64::MAX as u128) as u64;
    if t.local.is_empty() && now > 30000 {
        return Err("EOS sign-in timed out".into());
    }
    for event in t.raw.pin_mut().sdk_poll()? {
        if event.kind != 8 && std::env::var_os("H2_EOS_DIAGNOSTICS").is_some() {
            eprintln!("EOS event kind={} result={}", event.kind, event.code);
        }
        match event.kind {
            1 if event.code == 0 || event.code == 1 => t.raw.pin_mut().sdk_login()?,
            2 if event.code == 2 => t.raw.pin_mut().sdk_create_user()?,
            2 | 3 if event.code == 0 => {
                if !valid_id(&event.user) || t.peers.values().any(|p| p.user == event.user) {
                    return Err("Invalid local EOS identity".into());
                }
                if t.local.is_empty() {
                    t.local = event.user;
                    t.raw.pin_mut().sdk_socket(&t.socket, t.force)?;
                    t.signals = t
                        .peers
                        .keys()
                        .map(|&peer| ffi::EosSignal {
                            peer,
                            user: t.local.clone(),
                        })
                        .collect();
                } else if t.local != event.user {
                    return Err("EOS identity changed on refresh".into());
                }
            }
            1..=3 => return Err(format!("EOS sign-in failed (SDK result {})", event.code).into()),
            7 => t.raw.pin_mut().sdk_login()?,
            9 if t.session.enabled && !t.session_ready && !t.session_joining => {
                if event.socket.len() <= 256 && t.invitations.len() < 16 {
                    t.invitations.push_back((event.user, event.socket));
                }
            }
            10 | 11 if t.session.enabled => {
                if event.code != 0 {
                    return Err(format!(
                        "EOS session admission failed (SDK result {})",
                        event.code
                    )
                    .into());
                }
                t.session_ready = true;
            }
            13 if t.session.enabled => {
                return Err(
                    format!("EOS session invitation failed (SDK result {})", event.code).into(),
                );
            }
            16 if t.session.enabled && t.self_slot == t.session_owner => {
                if let Some(peer) = t.peers.values_mut().find(|p| p.user == event.user) {
                    peer.registered = true;
                }
            }
            4..=6 | 8 => {
                if event.socket != t.socket {
                    continue;
                }
                let Some((&slot, peer)) = t
                    .peers
                    .iter_mut()
                    .find(|(_, p)| !p.user.is_empty() && p.user == event.user)
                else {
                    // SDK signaling can beat the authenticated master roster.
                    // Remember the request, but grant nothing until that PUID
                    // is bound to an admitted physical slot by eos_signal.
                    if event.kind == 4 && valid_id(&event.user) {
                        if t.pending_requests.len() >= 32 {
                            if let Some(oldest) = t.pending_requests.iter().min_by_key(|(_, at)| *at).map(|(user, _)| user.clone()) {
                                t.pending_requests.remove(&oldest);
                            }
                        }
                        t.pending_requests.insert(event.user, now);
                    }
                    continue;
                };
                match event.kind {
                    4 => {
                        peer.accepted = t.raw.pin_mut().sdk_accept(&peer.user);
                    }
                    5 => {
                        peer.relayed = event.code == 2;
                        peer.failed = false;
                    }
                    6 => {
                        peer.ready = false;
                        peer.failed = true;
                        peer.accepted = false;
                        peer.reader = Reassembly::default();
                    }
                    8 if eos_wire::authenticates(&event.data, slot, t.self_slot, &peer.key) => {
                        peer.ready = true;
                        peer.failed = false;
                        // A response is also sent by the periodic handshake; do
                        // not echo every hello and create an amplification loop.
                    }
                    8 if t.session.enabled
                        && eos_wire::authenticates_session(
                            &event.data,
                            slot,
                            t.self_slot,
                            &peer.key,
                        ) =>
                    {
                        peer.ready = true;
                        peer.failed = false;
                        peer.session_joined = true;
                    }
                    8 if peer.ready
                        && (!t.session.enabled
                            || t.session_ready
                                && peer.session_joined
                                && (t.self_slot != t.session_owner || peer.registered)) =>
                    {
                        if let Some(data) = peer.reader.receive(&event.data, now) {
                            if t.packets.len() < 512 {
                                t.packets.push_back(ffi::EosPacket {
                                    present: true,
                                    peer: slot,
                                    data,
                                });
                            }
                        }
                    }
                    _ => {}
                }
            }
            _ => {}
        }
    }
    t.pending_requests.retain(|_, at| now.saturating_sub(*at) <= 30000);
    for peer in t.peers.values_mut() {
        if !peer.user.is_empty() && t.pending_requests.remove(&peer.user).is_some() {
            peer.accepted = t.raw.pin_mut().sdk_accept(&peer.user);
        }
    }
    if t.session.enabled && !t.local.is_empty() {
        if t.self_slot == t.session_owner {
            if !t.session_started {
                t.raw
                    .pin_mut()
                    .sdk_session_create(&t.session, (t.peers.len() + 1) as u32)?;
                t.session_started = true;
            }
            if t.session_ready {
                for (&slot, peer) in &mut t.peers {
                    if !peer.user.is_empty() && t.invited.insert(slot) {
                        t.raw.pin_mut().sdk_session_invite(&peer.user)?;
                    }
                    if peer.session_joined && !peer.registration_requested {
                        t.raw.pin_mut().sdk_session_register(&peer.user)?;
                        peer.registration_requested = true;
                    }
                }
            }
        } else if !t.session_ready && !t.session_joining {
            // Recover invitations sent before the subscription became ready.
            // This is bounded setup traffic; established sessions do not poll.
            if t.last_invite_query
                .is_none_or(|last| now.saturating_sub(last) >= 5000)
            {
                t.raw.pin_mut().sdk_session_query()?;
                t.last_invite_query = Some(now);
            }
            if let Some(owner) = t
                .peers
                .get(&t.session_owner)
                .filter(|peer| !peer.user.is_empty())
            {
                while let Some((user, invite)) = t.invitations.pop_front() {
                    if !user.is_empty() && user != owner.user {
                        if std::env::var_os("H2_EOS_DIAGNOSTICS").is_some() {
                            eprintln!("EOS invitation sender rejected");
                        }
                        continue;
                    }
                    let Ok(info) = t
                        .raw
                        .as_ref()
                        .ok_or("Missing EOS platform")?
                        .sdk_session_info(&invite)
                    else {
                        if std::env::var_os("H2_EOS_DIAGNOSTICS").is_some() {
                            eprintln!("EOS invitation lookup unavailable");
                        }
                        continue;
                    };
                    if info.session != t.socket
                        || info.owner != owner.user
                        || info.playlist != t.session.playlist
                        || info.revision != t.session.revision
                        || info.map != t.session.map
                        || info.variant != t.session.variant
                    {
                        if std::env::var_os("H2_EOS_DIAGNOSTICS").is_some() {
                            eprintln!(
                                "EOS invitation metadata rejected id={} owner={} playlist={} revision={} map={} variant={}",
                                info.session == t.socket,
                                info.owner == owner.user,
                                info.playlist == t.session.playlist,
                                info.revision == t.session.revision,
                                info.map == t.session.map,
                                info.variant == t.session.variant
                            );
                        }
                        continue;
                    }
                    t.raw.pin_mut().sdk_session_join(&invite)?;
                    t.session_joining = true;
                    break;
                }
            }
        }
        if !t.session_ready && now > 60000 {
            return Err("EOS session admission timed out".into());
        }
    }
    if !t.local.is_empty() && now.saturating_sub(t.last_hello) >= 100 {
        t.last_hello = now;
        for (&slot, peer) in &mut t.peers {
            if peer.user.is_empty() || peer.failed {
                continue;
            }
            // One deterministic initiator avoids simultaneous SDK session
            // offers. The other endpoint accepts only its admitted request.
            if !peer.accepted && t.self_slot < slot {
                peer.accepted = t.raw.pin_mut().sdk_accept(&peer.user);
            }
            if peer.accepted {
                let hello = if t.session.enabled && t.session_ready {
                    eos_wire::session_hello(t.self_slot, slot, &peer.key)
                } else {
                    eos_wire::hello(t.self_slot, slot, &peer.key)
                };
                t.raw.pin_mut().sdk_send(&peer.user, &hello, true);
            }
        }
    }
    Ok(())
}
pub fn eos_signals(t: &mut EosTransport) -> Vec<ffi::EosSignal> {
    std::mem::take(&mut t.signals)
}
pub fn eos_ready(t: &EosTransport, peer: u8) -> bool {
    t.session_ready
        && t.peers.get(&peer).is_some_and(|p| {
            p.ready
                && (!t.session.enabled
                    || p.session_joined && (t.self_slot != t.session_owner || p.registered))
        })
}
pub fn eos_failed(t: &EosTransport, peer: u8) -> bool {
    t.peers.get(&peer).is_none_or(|p| p.failed)
}
pub fn eos_relayed(t: &EosTransport, peer: u8) -> bool {
    t.peers.get(&peer).is_some_and(|p| p.ready && p.relayed)
}
pub fn eos_send(t: &mut EosTransport, slot: u8, bytes: &[u8]) -> bool {
    if !eos_ready(t, slot) {
        return false;
    }
    let Some(peer) = t.peers.get(&slot).filter(|p| p.ready) else {
        return false;
    };
    let packets = eos_wire::fragments(t.sequence, bytes);
    t.sequence = t.sequence.wrapping_add(1);
    !packets.is_empty()
        && packets
            .iter()
            .all(|packet| t.raw.pin_mut().sdk_send(&peer.user, packet, false))
}
pub fn eos_receive(t: &mut EosTransport) -> ffi::EosPacket {
    t.packets.pop_front().unwrap_or(ffi::EosPacket {
        present: false,
        peer: 0,
        data: vec![],
    })
}
