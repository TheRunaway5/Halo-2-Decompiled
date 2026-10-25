//! EOS state and admission policy, with process-main ownership at the SDK ABI.
use crate::{
    eos_config::configuration,
    eos_sdk_proxy::{SdkProxy, new_sdk_proxy},
    ffi,
};
use halo2_online_protocol::eos_account_auth::{
    AccountAuth, Command, LoginResult, RequestKind, Response, SdkAction,
};
use halo2_online_protocol::eos_outgoing::{Outgoing, SendResult};
use halo2_online_protocol::eos_wire::{self, Reassembly};
use sha2::{Digest, Sha256};
use std::{
    collections::{BTreeMap, BTreeSet, VecDeque},
    io::Write,
    time::Instant,
};

type Error = Box<dyn std::error::Error>;
// The real relay burst control requires pacing below the SDK's accepted-send
// burst rate; the core separately enforces its32-attempt safety ceiling.
const SDK_FRAGMENTS_PER_TICK: usize = 4;
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
struct QueueSample {
    elapsed_ms: u64,
    result: i32,
    incoming_bytes: u64,
    incoming_packets: u64,
    outgoing_bytes: u64,
    outgoing_packets: u64,
}
impl Default for QueueSample {
    fn default() -> Self {
        Self {
            elapsed_ms: 0,
            result: -1,
            incoming_bytes: 0,
            incoming_packets: 0,
            outgoing_bytes: 0,
            outgoing_packets: 0,
        }
    }
}
impl QueueSample {
    fn new(now: u64, info: &ffi::EosPacketQueueInfo) -> Self {
        let mut sample = Self {
            elapsed_ms: now,
            result: info.result,
            ..Self::default()
        };
        if info.result == 0 {
            sample.incoming_bytes = info.incoming_bytes;
            sample.incoming_packets = info.incoming_packets;
            sample.outgoing_bytes = info.outgoing_bytes;
            sample.outgoing_packets = info.outgoing_packets;
        }
        sample
    }
}
#[derive(Default)]
struct PacketDiagnostics {
    enabled: bool,
    last_report: u64,
    last_tick: Option<u64>,
    tick_gap_max_ms: u64,
    burst_bytes: u64,
    burst_bytes_max: u64,
    not_ready_refused: u64,
    fragments_sent: u64,
    fragments_failed: u64,
    hello_failed: u64,
    last_send_result: i32,
    receive_dropped: u64,
    receive_depth_max: usize,
    incoming_bytes_max: u64,
    outgoing_bytes_max: u64,
    queue_result: i32,
    reassembly_expired: u64,
    reassembly_capacity_dropped: u64,
    pre_poll: QueueSample,
    post_poll: QueueSample,
    received_frames: u64,
    received_bytes: u64,
    completed_datagrams: u64,
    last_data_send_ms: Option<u64>,
}
impl PacketDiagnostics {
    fn tick(&mut self, now: u64) {
        if !self.enabled {
            return;
        }
        if let Some(previous) = self.last_tick {
            self.tick_gap_max_ms = self.tick_gap_max_ms.max(now.saturating_sub(previous));
        }
        self.last_tick = Some(now);
        self.burst_bytes = 0;
    }
    fn fragment(&mut self, bytes: usize, result: i32, hello: bool, now: u64) {
        if !self.enabled {
            return;
        }
        self.burst_bytes = self.burst_bytes.saturating_add(bytes as u64);
        self.burst_bytes_max = self.burst_bytes_max.max(self.burst_bytes);
        if result == 0 {
            if !hello {
                self.fragments_sent = self.fragments_sent.saturating_add(1);
                self.last_data_send_ms = Some(now);
            }
        } else {
            self.last_send_result = result;
            if hello {
                self.hello_failed = self.hello_failed.saturating_add(1);
            } else {
                self.fragments_failed = self.fragments_failed.saturating_add(1);
            }
        }
    }
    fn receive(&mut self, depth: usize) {
        if !self.enabled {
            return;
        }
        self.completed_datagrams = self.completed_datagrams.saturating_add(1);
        if depth >= 512 {
            self.receive_dropped = self.receive_dropped.saturating_add(1);
        } else {
            self.receive_depth_max = self.receive_depth_max.max(depth + 1);
        }
    }
    fn received_frame(&mut self, bytes: usize) {
        if self.enabled {
            self.received_frames = self.received_frames.saturating_add(1);
            self.received_bytes = self.received_bytes.saturating_add(bytes as u64);
        }
    }
    fn poll_queue(&mut self, now: u64, info: &ffi::EosPacketQueueInfo, before: bool) {
        if !self.enabled {
            return;
        }
        self.queue(info);
        if before {
            self.pre_poll = QueueSample::new(now, info);
        } else {
            self.post_poll = QueueSample::new(now, info);
        }
    }
    fn data_send_age_ms(&self, now: u64) -> u64 {
        self.last_data_send_ms
            .map_or(0, |sent| now.saturating_sub(sent))
    }
    fn queue(&mut self, info: &ffi::EosPacketQueueInfo) {
        if !self.enabled {
            return;
        }
        self.queue_result = info.result;
        if info.result == 0 {
            self.incoming_bytes_max = self.incoming_bytes_max.max(info.incoming_bytes);
            self.outgoing_bytes_max = self.outgoing_bytes_max.max(info.outgoing_bytes);
        }
    }
    fn report(&mut self, now: u64, final_report: bool, outgoing: &Outgoing) {
        if !self.enabled || (!final_report && now.saturating_sub(self.last_report) < 1000) {
            return;
        }
        self.last_report = now;
        let sending = outgoing.statistics();
        let _ = writeln!(
            std::io::stderr().lock(),
            "EOS packets final={} elapsed_ms={} sent={} failed={} fragments_sent={} fragments_failed={} hello_failed={} last_send_failure_result={} rust_receive_dropped={} rust_receive_depth_max={} burst_bytes_max={} tick_gap_max_ms={} sdk_incoming_bytes_max={} sdk_outgoing_bytes_max={} queue_result={} reassembly_expired={} reassembly_capacity_dropped={} admitted={} admission_refused_capacity={} admission_refused_invalid={} admission_refused_not_ready={} outgoing_depth={} outgoing_bytes={} outgoing_depth_max={} outgoing_bytes_max={} outgoing_expired={} outgoing_cancelled={} outgoing_backpressure={} sdk_pre_poll_elapsed_ms={} sdk_pre_poll_result={} sdk_pre_poll_incoming_bytes={} sdk_pre_poll_incoming_packets={} sdk_pre_poll_outgoing_bytes={} sdk_pre_poll_outgoing_packets={} sdk_post_poll_elapsed_ms={} sdk_post_poll_result={} sdk_post_poll_incoming_bytes={} sdk_post_poll_incoming_packets={} sdk_post_poll_outgoing_bytes={} sdk_post_poll_outgoing_packets={} received_frames={} received_bytes={} completed_datagrams={} data_send_seen={} data_send_age_ms={}",
            u8::from(final_report),
            now,
            sending.sent,
            sending.permanent_failed,
            self.fragments_sent,
            self.fragments_failed,
            self.hello_failed,
            self.last_send_result,
            self.receive_dropped,
            self.receive_depth_max,
            self.burst_bytes_max,
            self.tick_gap_max_ms,
            self.incoming_bytes_max,
            self.outgoing_bytes_max,
            self.queue_result,
            self.reassembly_expired,
            self.reassembly_capacity_dropped,
            sending.admitted,
            sending.capacity_refused,
            sending.invalid_refused,
            self.not_ready_refused,
            outgoing.depth(),
            outgoing.bytes(),
            sending.depth_max,
            sending.bytes_max,
            sending.expired,
            sending.cancelled,
            sending.backpressure,
            self.pre_poll.elapsed_ms,
            self.pre_poll.result,
            self.pre_poll.incoming_bytes,
            self.pre_poll.incoming_packets,
            self.pre_poll.outgoing_bytes,
            self.pre_poll.outgoing_packets,
            self.post_poll.elapsed_ms,
            self.post_poll.result,
            self.post_poll.incoming_bytes,
            self.post_poll.incoming_packets,
            self.post_poll.outgoing_bytes,
            self.post_poll.outgoing_packets,
            self.received_frames,
            self.received_bytes,
            self.completed_datagrams,
            u8::from(self.last_data_send_ms.is_some()),
            self.data_send_age_ms(now)
        );
    }
}
fn enqueue_received(
    packets: &mut VecDeque<ffi::EosPacket>,
    diagnostics: &mut PacketDiagnostics,
    peer: u8,
    data: Vec<u8>,
) {
    diagnostics.receive(packets.len());
    if packets.len() < 512 {
        packets.push_back(ffi::EosPacket {
            present: true,
            peer,
            data,
        });
    }
}
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
    raw: SdkProxy,
    self_slot: u8,
    peers: BTreeMap<u8, Peer>,
    socket: String,
    force: bool,
    local: String,
    signals: Vec<ffi::EosSignal>,
    packets: VecDeque<ffi::EosPacket>,
    pending_requests: BTreeMap<String, u64>,
    outgoing: Outgoing,
    started: Instant,
    last_hello: u64,
    session: ffi::EosSession,
    session_owner: u8,
    session_started: bool,
    session_admission_started: Option<u64>,
    session_ready: bool,
    session_joining: bool,
    invited: BTreeSet<u8>,
    invitations: VecDeque<(String, String)>,
    last_invite_query: Option<u64>,
    account: Option<AccountAuth>,
    diagnostics: PacketDiagnostics,
}
impl Drop for EosTransport {
    fn drop(&mut self) {
        self.outgoing.clear();
        self.diagnostics.report(
            self.started.elapsed().as_millis().min(u128::from(u64::MAX)) as u64,
            true,
            &self.outgoing,
        );
    }
}

fn valid_id(value: &str) -> bool {
    value.len() == 32
        && value
            .bytes()
            .all(|b| b.is_ascii_hexdigit() && !b.is_ascii_uppercase())
}

// Buckets partition compatible playlist revisions; the original attributes
// remain authoritative and are checked independently when joining an invite.
// A fixed short key avoids service rejection of concatenated catalog strings.
fn session_bucket(playlist: &str, revision: &str) -> String {
    let mut digest = Sha256::new();
    digest.update(b"H2Forever/EOS/session-bucket/v1\0");
    for value in [playlist, revision] {
        digest.update((value.len() as u64).to_be_bytes());
        digest.update(value.as_bytes());
    }
    let digest = digest.finalize();
    let mut bucket = String::with_capacity(38);
    bucket.push_str("h2q13:");
    for byte in &digest[..16] {
        bucket.push(char::from(b"0123456789abcdef"[usize::from(byte >> 4)]));
        bucket.push(char::from(b"0123456789abcdef"[usize::from(byte & 15)]));
    }
    bucket
}

fn session_admission_expired(t: &mut EosTransport, now: u64) -> bool {
    if !t.session.enabled
        || t.local.is_empty()
        || t.account
            .as_ref()
            .is_some_and(|account| !account.admitted())
    {
        return false;
    }
    let started = *t.session_admission_started.get_or_insert(now);
    !t.session_ready && now.saturating_sub(started) > 60000
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
/// Profile lookup uses a short-lived platform; account-bound match transport
/// separately authenticates this account and proves its identity to the room.
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
    let mut raw = new_sdk_proxy(&config)?;
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
        raw: SdkProxy::null(),
        self_slot,
        peers,
        socket: socket.to_owned(),
        force: force_relay,
        local: String::new(),
        signals: vec![],
        packets: VecDeque::new(),
        pending_requests: BTreeMap::new(),
        outgoing: Outgoing::default(),
        started: Instant::now(),
        last_hello: 0,
        session_ready: !session.enabled,
        session,
        session_owner,
        session_started: false,
        session_admission_started: None,
        session_joining: false,
        invited: BTreeSet::new(),
        invitations: VecDeque::new(),
        last_invite_query: None,
        account: None,
        diagnostics: PacketDiagnostics {
            enabled: std::env::var_os("H2_EOS_DIAGNOSTICS").is_some(),
            ..Default::default()
        },
    }))
}

/// The authenticated match path never creates or logs in an anonymous DeviceID.
pub fn new_eos_account_transport(
    self_slot: u8,
    rows: Vec<ffi::EosPeer>,
    socket: &str,
    force_relay: bool,
    session: ffi::EosSession,
) -> Result<Box<EosTransport>, Error> {
    let mut transport = new_eos_transport(self_slot, rows, socket, force_relay, session)?;
    transport.account = Some(AccountAuth::new(0));
    Ok(transport)
}

fn elapsed(t: &EosTransport) -> u64 {
    t.started.elapsed().as_millis().min(u64::MAX as u128) as u64
}

fn platform(t: &mut EosTransport) -> Result<(), Error> {
    if t.raw.is_null() {
        t.raw = new_sdk_proxy(&t.config)?;
        if t.account.is_none() {
            t.raw.pin_mut().sdk_device()?;
        }
    }
    Ok(())
}

fn admit_local(t: &mut EosTransport, user: &str) -> Result<(), Error> {
    if !valid_id(user) || t.peers.values().any(|peer| peer.user == user) {
        return Err("Invalid local EOS identity".into());
    }
    if t.local.is_empty() {
        t.raw.pin_mut().sdk_socket(&t.socket, t.force)?;
        t.local = user.to_owned();
        t.signals = t
            .peers
            .keys()
            .map(|&peer| ffi::EosSignal {
                peer,
                user: t.local.clone(),
            })
            .collect();
    } else if t.local != user {
        return Err("EOS identity changed on refresh".into());
    }
    Ok(())
}

pub fn eos_auth_request(t: &mut EosTransport) -> Result<ffi::EosAuthRequest, Error> {
    let result = eos_auth_request_inner(t);
    finish_operation(t, result)
}
fn eos_auth_request_inner(t: &mut EosTransport) -> Result<ffi::EosAuthRequest, Error> {
    let now = elapsed(t);
    let request = match &mut t.account {
        Some(account) => account.request(now)?,
        None => None,
    };
    Ok(match request {
        Some(request) => ffi::EosAuthRequest {
            kind: request.kind as u8,
            ticket: request.ticket,
            token: request.token,
            user: request.user,
        },
        None => ffi::EosAuthRequest {
            kind: 0,
            ticket: 0,
            token: String::new(),
            user: String::new(),
        },
    })
}

pub fn eos_auth_bound(t: &EosTransport) -> bool {
    t.account.as_ref().is_some_and(AccountAuth::admitted)
}

pub fn eos_auth_complete(
    t: &mut EosTransport,
    response: &ffi::EosAuthResponse,
) -> Result<bool, Error> {
    let result = eos_auth_complete_inner(t, response);
    finish_operation(t, result)
}
fn eos_auth_complete_inner(
    t: &mut EosTransport,
    response: &ffi::EosAuthResponse,
) -> Result<bool, Error> {
    let kind = match response.kind {
        1 => RequestKind::IssuerToken,
        2 => RequestKind::IdentityProof,
        _ => return Ok(false),
    };
    let now = elapsed(t);
    let Some(account) = &mut t.account else {
        return Ok(false);
    };
    let completion = account.complete(
        Response {
            kind,
            ticket: response.ticket,
            status: response.status,
            ok: response.ok,
            token: &response.token,
            user: &response.user,
            expires_in: response.expires_in,
        },
        now,
    )?;
    match completion.command {
        Command::None => {}
        Command::ExternalLogin(token) => {
            platform(t)?;
            t.raw.pin_mut().sdk_external_login(&token)?;
        }
        Command::Admitted => {
            let user = account.user().to_owned();
            admit_local(t, &user)?;
        }
    }
    Ok(completion.accepted)
}

pub fn eos_signal(transport: &mut EosTransport, slot: u8, user: &str) -> Result<(), Error> {
    let result = eos_signal_inner(transport, slot, user);
    finish_operation(transport, result)
}
fn eos_signal_inner(transport: &mut EosTransport, slot: u8, user: &str) -> Result<(), Error> {
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
    let result = eos_tick_inner(t);
    let result = finish_operation(t, result);
    if t.diagnostics.enabled {
        let now = elapsed(t);
        t.diagnostics.report(now, false, &t.outgoing);
    }
    result
}
fn finish_operation<T>(t: &mut EosTransport, result: Result<T, Error>) -> Result<T, Error> {
    if result.is_err() {
        t.outgoing.clear();
    }
    result
}
fn eos_tick_inner(t: &mut EosTransport) -> Result<(), Error> {
    platform(t)?;
    let now = elapsed(t);
    t.diagnostics.tick(now);
    if let Some(account) = &mut t.account {
        account.tick(now)?;
    } else if t.local.is_empty() && now > 30000 {
        return Err("EOS sign-in timed out".into());
    }
    if t.diagnostics.enabled {
        let info = t.raw.sdk_packet_queue_info();
        let observed = elapsed(t);
        t.diagnostics.poll_queue(observed, &info, true);
    }
    let events = t.raw.pin_mut().sdk_poll();
    if t.diagnostics.enabled {
        let info = t.raw.sdk_packet_queue_info();
        let observed = elapsed(t);
        t.diagnostics.poll_queue(observed, &info, false);
        if let Ok(events) = &events {
            // Count the complete SDK result before dispatch can reject an
            // event. Include hello and unadmitted frames, not just data.
            for event in events {
                if event.kind == 8 {
                    t.diagnostics.received_frame(event.data.len());
                }
            }
        }
    }
    for event in events? {
        if event.kind != 8 && std::env::var_os("H2_EOS_DIAGNOSTICS").is_some() {
            eprintln!("EOS event kind={} result={}", event.kind, event.code);
        }
        if let Some(account) = &mut t.account {
            match event.kind {
                1 => return Err("Unexpected anonymous EOS login event".into()),
                2 | 3 => {
                    let result = match event.code {
                        0 => LoginResult::Success,
                        2 => LoginResult::InvalidUser,
                        _ => LoginResult::Failed,
                    };
                    match account.sdk_login_result(event.kind == 3, result, &event.user, now)? {
                        SdkAction::CreateInitialUser => t.raw.pin_mut().sdk_create_user()?,
                        SdkAction::CopyIdentityProof => {
                            let proof = t.raw.sdk_id_token()?;
                            account.identity_proof(&proof, now)?;
                        }
                    }
                    continue;
                }
                7 => {
                    account.auth_expiration(now)?;
                    continue;
                }
                _ => {}
            }
            if !account.admitted() {
                continue;
            }
        }
        match event.kind {
            1 if event.code == 0 || event.code == 1 => t.raw.pin_mut().sdk_login()?,
            2 if event.code == 2 => t.raw.pin_mut().sdk_create_user()?,
            2 | 3 if event.code == 0 => {
                admit_local(t, &event.user)?;
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
                        if t.pending_requests.len() >= 32
                            && let Some(oldest) = t
                                .pending_requests
                                .iter()
                                .min_by_key(|(_, at)| *at)
                                .map(|(user, _)| user.clone())
                        {
                            t.pending_requests.remove(&oldest);
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
                        t.outgoing.cancel_peer(slot);
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
                        let before = peer.reader.statistics();
                        let received = peer.reader.receive(&event.data, now);
                        if t.diagnostics.enabled {
                            let after = peer.reader.statistics();
                            t.diagnostics.reassembly_expired = t
                                .diagnostics
                                .reassembly_expired
                                .saturating_add(after.expired.saturating_sub(before.expired));
                            t.diagnostics.reassembly_capacity_dropped =
                                t.diagnostics.reassembly_capacity_dropped.saturating_add(
                                    after
                                        .capacity_dropped
                                        .saturating_sub(before.capacity_dropped),
                                );
                        }
                        if let Some(data) = received {
                            enqueue_received(&mut t.packets, &mut t.diagnostics, slot, data);
                        }
                    }
                    _ => {}
                }
            }
            _ => {}
        }
    }
    if t.account
        .as_ref()
        .is_some_and(|account| !account.admitted())
    {
        return Ok(());
    }
    t.pending_requests
        .retain(|_, at| now.saturating_sub(*at) <= 30000);
    for peer in t.peers.values_mut() {
        if !peer.user.is_empty() && t.pending_requests.remove(&peer.user).is_some() {
            peer.accepted = t.raw.pin_mut().sdk_accept(&peer.user);
        }
    }
    let session_expired = session_admission_expired(t, now);
    if t.session.enabled && !t.local.is_empty() {
        if t.self_slot == t.session_owner {
            if !t.session_started {
                let bucket = session_bucket(&t.session.playlist, &t.session.revision);
                t.raw.pin_mut().sdk_session_create(
                    &t.session,
                    &bucket,
                    (t.peers.len() + 1) as u32,
                )?;
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
        if session_expired {
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
                let result = t.raw.pin_mut().sdk_send(&peer.user, &hello, true);
                t.diagnostics.fragment(hello.len(), result, true, now);
            }
        }
    }
    drain_outgoing(t);
    Ok(())
}
fn drain_outgoing(t: &mut EosTransport) {
    let started = t.started;
    let peers = &t.peers;
    let raw = &mut t.raw;
    let diagnostics = &mut t.diagnostics;
    for _ in 0..SDK_FRAGMENTS_PER_TICK {
        let mut blocked = false;
        // Refresh the monotonic clock before each SDK attempt: a slow SDK/log
        // call cannot extend a queued packet's fixed admission/partial deadline.
        let now = started.elapsed().as_millis().min(u128::from(u64::MAX)) as u64;
        let attempted = t.outgoing.poll(now, 1, |slot, bytes| {
            let Some(peer) = peers.get(&slot) else {
                return SendResult::Failed;
            };
            let result = raw.pin_mut().sdk_send(&peer.user, bytes, false);
            let observed = if diagnostics.enabled {
                started.elapsed().as_millis().min(u128::from(u64::MAX)) as u64
            } else {
                0
            };
            diagnostics.fragment(bytes.len(), result, false, observed);
            if result != 0 && diagnostics.enabled {
                diagnostics.queue(&raw.sdk_packet_queue_info());
            }
            match result {
                0 => SendResult::Sent,
                22 => {
                    blocked = true;
                    SendResult::Backpressure
                }
                _ => SendResult::Failed,
            }
        });
        if blocked || attempted == 0 {
            break;
        }
    }
}
pub fn eos_signals(t: &mut EosTransport) -> Vec<ffi::EosSignal> {
    std::mem::take(&mut t.signals)
}
pub fn eos_ready(t: &EosTransport, peer: u8) -> bool {
    t.account.as_ref().is_none_or(AccountAuth::admitted)
        && t.session_ready
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
        if t.diagnostics.enabled {
            t.diagnostics.not_ready_refused = t.diagnostics.not_ready_refused.saturating_add(1);
        }
        return false;
    }
    t.outgoing.admit(slot, bytes, elapsed(t))
}
pub fn eos_receive(t: &mut EosTransport) -> ffi::EosPacket {
    t.packets.pop_front().unwrap_or(ffi::EosPacket {
        present: false,
        peer: 0,
        data: vec![],
    })
}

#[cfg(test)]
mod account_tests {
    use super::*;

    #[test]
    fn packet_diagnostics_observe_actual_bounded_queue_and_failed_fragment() {
        let mut diagnostics = PacketDiagnostics {
            enabled: true,
            ..Default::default()
        };
        let mut queue = VecDeque::new();
        for _ in 0..513 {
            enqueue_received(&mut queue, &mut diagnostics, 0, vec![9]);
        }
        assert_eq!(queue.len(), 512);
        assert_eq!(diagnostics.receive_dropped, 1);
        assert_eq!(diagnostics.receive_depth_max, 512);
        assert_eq!(diagnostics.completed_datagrams, 513);
        diagnostics.tick(10);
        diagnostics.fragment(1170, 0, false, 10);
        diagnostics.fragment(258, 7, false, 11);
        assert_eq!(
            (diagnostics.fragments_sent, diagnostics.fragments_failed),
            (1, 1)
        );
        assert_eq!(diagnostics.last_send_result, 7);
        assert_eq!(diagnostics.burst_bytes_max, 1428);
        diagnostics.tick(1010);
        assert_eq!(diagnostics.tick_gap_max_ms, 1000);
        assert_eq!(diagnostics.burst_bytes, 0);
        diagnostics.fragments_failed = u64::MAX;
        diagnostics.fragment(258, 7, false, 1010);
        assert_eq!(diagnostics.fragments_failed, u64::MAX);
        let mut disabled = PacketDiagnostics::default();
        disabled.fragment(1170, 7, false, 1010);
        assert_eq!(disabled.fragments_failed, 0);
    }

    #[test]
    fn packet_diagnostics_distinguish_current_poll_gauges_from_high_water() {
        let mut diagnostics = PacketDiagnostics {
            enabled: true,
            ..Default::default()
        };
        assert_eq!(diagnostics.pre_poll.result, -1);
        assert_eq!(diagnostics.post_poll.result, -1);
        let mut info = ffi::EosPacketQueueInfo {
            result: 0,
            incoming_bytes: 1170,
            incoming_packets: 1,
            outgoing_bytes: 65000,
            outgoing_packets: 56,
        };
        diagnostics.poll_queue(100, &info, true);
        info.incoming_bytes = 0;
        info.incoming_packets = 0;
        info.outgoing_bytes = 258;
        info.outgoing_packets = 1;
        diagnostics.poll_queue(110, &info, false);
        assert_eq!(diagnostics.pre_poll.outgoing_bytes, 65000);
        assert_eq!(diagnostics.pre_poll.outgoing_packets, 56);
        assert_eq!(diagnostics.post_poll.outgoing_bytes, 258);
        assert_eq!(diagnostics.post_poll.outgoing_packets, 1);
        assert_eq!(diagnostics.post_poll.incoming_bytes, 0);
        assert_eq!(diagnostics.post_poll.elapsed_ms, 110);
        assert_eq!(diagnostics.outgoing_bytes_max, 65000);
        info.result = 22;
        diagnostics.poll_queue(120, &info, false);
        assert_eq!(diagnostics.post_poll.result, 22);
        assert_eq!(diagnostics.post_poll.elapsed_ms, 120);
        assert_eq!(diagnostics.post_poll.incoming_bytes, 0);
        assert_eq!(diagnostics.post_poll.incoming_packets, 0);
        assert_eq!(diagnostics.post_poll.outgoing_bytes, 0);
        assert_eq!(diagnostics.post_poll.outgoing_packets, 0);
        assert_eq!(diagnostics.pre_poll.outgoing_bytes, 65000);
        assert_eq!(diagnostics.outgoing_bytes_max, 65000);
        assert_eq!(diagnostics.queue_result, 22);

        let mut disabled = PacketDiagnostics::default();
        disabled.poll_queue(120, &info, true);
        assert_eq!(disabled.pre_poll, QueueSample::default());
    }

    #[test]
    fn packet_diagnostics_count_sdk_frames_and_actual_reassembly_separately() {
        let mut diagnostics = PacketDiagnostics {
            enabled: true,
            ..Default::default()
        };
        let data = vec![9; eos_wire::MAX_DATAGRAM];
        let frames = eos_wire::fragments(4, &data);
        let mut reader = Reassembly::default();
        let mut packets = VecDeque::new();
        // An invalid SDK-delivered frame and a duplicated fragment are still
        // received frames; only the one real completed datagram is enqueued.
        let received: [&[u8]; 4] = [&b"bad"[..], &frames[1], &frames[1], &frames[0]];
        let expected_bytes: u64 = received.iter().map(|frame| frame.len() as u64).sum();
        for frame in received {
            diagnostics.received_frame(frame.len());
            if let Some(data) = reader.receive(frame, 0) {
                enqueue_received(&mut packets, &mut diagnostics, 1, data);
            }
        }
        assert_eq!(diagnostics.received_frames, 4);
        assert_eq!(diagnostics.received_bytes, expected_bytes);
        assert_eq!(diagnostics.completed_datagrams, 1);
        assert_eq!(packets.pop_front().unwrap().data, data);
        diagnostics.received_frames = u64::MAX;
        diagnostics.received_bytes = u64::MAX;
        diagnostics.completed_datagrams = u64::MAX;
        diagnostics.received_frame(1);
        diagnostics.receive(512);
        assert_eq!(diagnostics.received_frames, u64::MAX);
        assert_eq!(diagnostics.received_bytes, u64::MAX);
        assert_eq!(diagnostics.completed_datagrams, u64::MAX);

        let mut disabled = PacketDiagnostics::default();
        disabled.received_frame(1170);
        disabled.receive(0);
        assert_eq!(disabled.received_frames, 0);
        assert_eq!(disabled.received_bytes, 0);
        assert_eq!(disabled.completed_datagrams, 0);
    }

    #[test]
    fn packet_diagnostics_last_data_send_ignores_hello_and_failed_attempts() {
        let mut diagnostics = PacketDiagnostics {
            enabled: true,
            ..Default::default()
        };
        assert_eq!(diagnostics.last_data_send_ms, None);
        assert_eq!(diagnostics.data_send_age_ms(100), 0);
        diagnostics.fragment(38, 0, true, 5);
        assert_eq!(diagnostics.last_data_send_ms, None);
        diagnostics.fragment(1170, 0, false, 10);
        diagnostics.fragment(258, 22, false, 30);
        diagnostics.fragment(38, 0, true, 40);
        assert_eq!(diagnostics.last_data_send_ms, Some(10));
        assert_eq!(diagnostics.data_send_age_ms(100), 90);
        assert_eq!(diagnostics.data_send_age_ms(0), 0);
        diagnostics.fragment(258, 0, false, 110);
        assert_eq!(diagnostics.data_send_age_ms(111), 1);
        let mut disabled = PacketDiagnostics::default();
        disabled.fragment(1170, 0, false, 10);
        assert_eq!(disabled.last_data_send_ms, None);
    }

    #[test]
    fn session_bucket_known_vector_is_short_stable_lowercase_hex() {
        let bucket = session_bucket("casual-party", "casual-party-lab-3-gathering-1");
        assert_eq!(bucket, "h2q13:345d2459208ab97dd79563d884f326ff");
        assert_eq!(bucket.len(), 38);
        assert!(
            bucket[6..]
                .bytes()
                .all(|b| b.is_ascii_hexdigit() && !b.is_ascii_uppercase())
        );
        assert_eq!(
            bucket,
            session_bucket("casual-party", "casual-party-lab-3-gathering-1")
        );
    }

    #[test]
    fn session_bucket_partitions_playlist_revision_with_unambiguous_framing() {
        let baseline = session_bucket("casual-party", "revision-1");
        assert_ne!(baseline, session_bucket("ranked-party", "revision-1"));
        assert_ne!(baseline, session_bucket("casual-party", "revision-2"));
        assert_ne!(session_bucket("a:b", "c"), session_bucket("a", "b:c"));
    }

    #[test]
    fn session_bucket_maximum_metadata_keeps_short_bucket_and_exact_attributes() {
        let mut session = ffi::EosSession {
            enabled: true,
            playlist: "p".repeat(64),
            revision: "r".repeat(64),
            map: "lockout".to_owned(),
            variant: "Team Slayer".to_owned(),
        };
        let bucket = session_bucket(&session.playlist, &session.revision);
        assert_eq!(bucket.len(), 38);
        assert_eq!(session.playlist, "p".repeat(64));
        assert_eq!(session.revision, "r".repeat(64));
        assert_eq!(session.map, "lockout");
        assert_eq!(session.variant, "Team Slayer");
        session.map = "highplains2".to_owned();
        session.variant = "Slayer".to_owned();
        assert_eq!(bucket, session_bucket(&session.playlist, &session.revision));
    }

    const USER: &str = "0123456789abcdef0123456789abcdef";
    const OTHER: &str = "1123456789abcdef0123456789abcdef";

    fn offline_transport() -> EosTransport {
        EosTransport {
            config: ffi::EosConfig {
                product: String::new(),
                sandbox: String::new(),
                deployment: String::new(),
                client: String::new(),
                secret: String::new(),
                cache: String::new(),
            },
            raw: SdkProxy::null(),
            self_slot: 0,
            peers: BTreeMap::new(),
            socket: "a".repeat(32),
            force: false,
            local: String::new(),
            signals: vec![],
            packets: VecDeque::new(),
            pending_requests: BTreeMap::new(),
            outgoing: Outgoing::default(),
            started: Instant::now(),
            last_hello: 0,
            session: ffi::EosSession {
                enabled: false,
                playlist: String::new(),
                revision: String::new(),
                map: String::new(),
                variant: String::new(),
            },
            session_owner: 0,
            session_started: false,
            session_admission_started: None,
            session_ready: true,
            session_joining: false,
            invited: BTreeSet::new(),
            invitations: VecDeque::new(),
            last_invite_query: None,
            account: Some(AccountAuth::new(0)),
            diagnostics: PacketDiagnostics::default(),
        }
    }

    #[test]
    fn outgoing_lifetime_follows_actual_auth_and_signal_errors() {
        let mut t = offline_transport();
        let request = eos_auth_request(&mut t).unwrap();
        assert!(t.outgoing.admit(1, &[1; 1404], 0));
        let mut response = ffi::EosAuthResponse {
            kind: request.kind,
            ticket: request.ticket + 1,
            status: 401,
            ok: false,
            token: String::new(),
            user: String::new(),
            expires_in: 0,
        };
        assert!(!eos_auth_complete(&mut t, &response).unwrap());
        assert_eq!(t.outgoing.depth(), 1); // A stale reply does not cancel admitted work.
        response.ticket = request.ticket;
        assert!(eos_auth_complete(&mut t, &response).is_err());
        assert_eq!((t.outgoing.depth(), t.outgoing.bytes()), (0, 0));
        assert!(t.outgoing.admit(1, &[2; 1404], 0));
        assert!(eos_signal(&mut t, 1, "invalid").is_err());
        assert_eq!(t.outgoing.depth(), 0);
        assert!(t.outgoing.admit(1, &[3; 1404], 0));
        assert!(finish_operation::<()>(&mut t, Err("terminal tick".into())).is_err());
        assert_eq!(t.outgoing.depth(), 0);
        assert_eq!(t.outgoing.statistics().cancelled, 3);
        assert!(t.raw.is_null());
    }

    fn prepare_proof(t: &mut EosTransport) -> ffi::EosAuthRequest {
        let account = t.account.as_mut().unwrap();
        let request = account.request(0).unwrap().unwrap();
        account
            .complete(
                Response {
                    kind: request.kind,
                    ticket: request.ticket,
                    status: 200,
                    ok: true,
                    token: "issuer.jwt",
                    user: "",
                    expires_in: 120,
                },
                0,
            )
            .unwrap();
        account
            .sdk_login_result(false, LoginResult::Success, USER, 0)
            .unwrap();
        account.identity_proof("actual.sdk.proof", 0).unwrap();
        eos_auth_request(t).unwrap()
    }

    #[test]
    fn session_admission_clock_starts_after_real_account_admission_with_full_budget() {
        let mut t = offline_transport();
        t.session.enabled = true;
        t.session_ready = false;
        let account = t.account.as_mut().unwrap();
        let issuer = account.request(0).unwrap().unwrap();
        account
            .complete(
                Response {
                    kind: RequestKind::IssuerToken,
                    ticket: issuer.ticket,
                    status: 200,
                    ok: true,
                    token: "issuer.jwt",
                    user: "",
                    expires_in: 120,
                },
                1,
            )
            .unwrap();
        account
            .sdk_login_result(false, LoginResult::Success, USER, 45000)
            .unwrap();
        account.identity_proof("actual.sdk.proof", 45000).unwrap();
        let proof = account.request(45000).unwrap().unwrap();
        assert!(!session_admission_expired(&mut t, 59000));
        assert_eq!(t.session_admission_started, None);
        t.account
            .as_mut()
            .unwrap()
            .complete(
                Response {
                    kind: RequestKind::IdentityProof,
                    ticket: proof.ticket,
                    status: 200,
                    ok: true,
                    token: "",
                    user: USER,
                    expires_in: 0,
                },
                59000,
            )
            .unwrap();
        assert!(!session_admission_expired(&mut t, 59000));
        assert_eq!(t.session_admission_started, None); // The socket identity is still absent.
        t.local = USER.to_owned();
        assert!(!session_admission_expired(&mut t, 59000));
        assert_eq!(t.session_admission_started, Some(59000));
        assert!(!session_admission_expired(&mut t, 119000));
        assert!(session_admission_expired(&mut t, 119001));
    }

    #[test]
    fn session_admission_clock_never_restarts_for_setup_retry_or_account_refresh() {
        let mut t = offline_transport();
        t.session.enabled = true;
        t.session_ready = false;
        let proof = prepare_proof(&mut t);
        t.account
            .as_mut()
            .unwrap()
            .complete(
                Response {
                    kind: RequestKind::IdentityProof,
                    ticket: proof.ticket,
                    status: 200,
                    ok: true,
                    token: "",
                    user: USER,
                    expires_in: 0,
                },
                1,
            )
            .unwrap();
        t.local = USER.to_owned();
        assert!(!session_admission_expired(&mut t, 59000));
        t.account.as_mut().unwrap().auth_expiration(60000).unwrap();
        assert!(t.account.as_ref().unwrap().admitted());
        assert_eq!(
            t.account
                .as_mut()
                .unwrap()
                .request(60000)
                .unwrap()
                .unwrap()
                .kind,
            RequestKind::IssuerToken
        );
        assert!(!session_admission_expired(&mut t, 60000));
        assert_eq!(t.session_admission_started, Some(59000));
        t.local.clear();
        assert!(!session_admission_expired(&mut t, 90000));
        t.local = USER.to_owned();
        assert!(session_admission_expired(&mut t, 119001));
        assert_eq!(t.session_admission_started, Some(59000));
        t.session_ready = true;
        assert!(!session_admission_expired(&mut t, u64::MAX));
        assert_eq!(t.session_admission_started, Some(59000));
    }

    #[test]
    fn account_http_request_boundary_is_single_dispatch_and_stale_safe_without_sdk() {
        let mut t = offline_transport();
        let request = eos_auth_request(&mut t).unwrap();
        assert_eq!(request.kind, 1);
        assert_ne!(request.ticket, 0);
        assert!(request.token.is_empty() && request.user.is_empty());
        assert_eq!(eos_auth_request(&mut t).unwrap().kind, 0);
        let wrong = ffi::EosAuthResponse {
            kind: 2,
            ticket: request.ticket,
            status: 200,
            ok: true,
            token: String::new(),
            user: USER.to_owned(),
            expires_in: 0,
        };
        assert!(!eos_auth_complete(&mut t, &wrong).unwrap());
        assert!(!eos_auth_bound(&t));
        assert!(t.raw.is_null() && eos_signals(&mut t).is_empty());
        let failed = ffi::EosAuthResponse {
            kind: 1,
            ticket: request.ticket,
            status: 200,
            ok: false,
            token: String::new(),
            user: String::new(),
            expires_in: 0,
        };
        assert!(eos_auth_complete(&mut t, &failed).is_err());
        assert!(!eos_auth_bound(&t) && t.raw.is_null());
    }

    #[test]
    fn proof_payload_and_wrong_server_ack_cannot_publish_identity_or_socket() {
        let mut t = offline_transport();
        let proof = prepare_proof(&mut t);
        assert_eq!(proof.kind, 2);
        assert_eq!(proof.token, "actual.sdk.proof");
        assert_eq!(proof.user, USER);
        assert!(!eos_auth_bound(&t));
        assert!(eos_signals(&mut t).is_empty());
        let ack = ffi::EosAuthResponse {
            kind: 2,
            ticket: proof.ticket,
            status: 200,
            ok: true,
            token: String::new(),
            user: OTHER.to_owned(),
            expires_in: 0,
        };
        assert!(eos_auth_complete(&mut t, &ack).is_err());
        assert!(!eos_auth_bound(&t));
        assert!(t.local.is_empty() && t.raw.is_null() && eos_signals(&mut t).is_empty());
    }

    #[test]
    fn authenticated_peer_identity_can_arrive_early_but_self_alias_refuses_before_sdk_socket() {
        let mut t = offline_transport();
        t.peers.insert(
            1,
            Peer {
                key: [0; 32],
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
        eos_signal(&mut t, 1, USER).unwrap();
        assert!(t.local.is_empty() && t.raw.is_null());
        let proof = prepare_proof(&mut t);
        let ack = ffi::EosAuthResponse {
            kind: 2,
            ticket: proof.ticket,
            status: 200,
            ok: true,
            token: String::new(),
            user: USER.to_owned(),
            expires_in: 0,
        };
        assert!(eos_auth_complete(&mut t, &ack).is_err());
        assert!(t.raw.is_null() && t.local.is_empty() && eos_signals(&mut t).is_empty());
    }
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
            new_sdk_proxy(&config).expect("EOS platform recreation must remain available")
        };
        drop(platform("profile"));
        drop(platform("first-match"));
        let active_match = platform("active-match");
        assert!(
            !crate::eos_process_shutdown(),
            "An active match must retain its SDK"
        );
        drop(platform("concurrent-profile"));
        drop(platform("replacement-profile"));
        drop(active_match);
        drop(platform("next-match"));
        assert!(
            crate::eos_process_shutdown(),
            "Finalize before static destruction"
        );
        assert!(crate::eos_process_shutdown(), "Shutdown is idempotent");
    }
}
