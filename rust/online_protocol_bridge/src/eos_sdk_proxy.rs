//! The game owns policy; one adjacent Rust worker's process main owns the SDK.
//!
//! Parent pipe I/O runs on one non-SDK thread. Every synchronous exchange has
//! a caller deadline; timeout invalidates the connection and kills/reaps only
//! its owned child. Nothing pumps the SDK except an explicit forwarded call.
use crate::{eos_sdk_ipc as wire, ffi};
use std::{
    collections::{BTreeMap, BTreeSet},
    io::{self, Read, Write},
    path::{Path, PathBuf},
    process::{Child, Command, Stdio},
    sync::{
        Mutex, MutexGuard, OnceLock, TryLockError,
        atomic::{AtomicBool, Ordering},
        mpsc,
    },
    thread::{self, JoinHandle},
    time::{Duration, Instant},
};

pub(crate) type Error = Box<dyn std::error::Error>;
type Outcome<T> = Result<T, Fault>;
const CREATE_TIMEOUT: Duration = Duration::from_secs(15);
const CALL_TIMEOUT: Duration = Duration::from_secs(5);
// The actual RawEos destructor's Session destroy pump is bounded by2s.
const DESTROY_TIMEOUT: Duration = Duration::from_secs(4);
const REAP_TIMEOUT: Duration = Duration::from_secs(2);
#[cfg(windows)]
const WORKER_FILE_NAME: &str = "eos_sdk_worker.exe";
#[cfg(not(windows))]
const WORKER_FILE_NAME: &str = "eos_sdk_worker";

#[derive(Clone, Copy, Debug)]
struct Fault;
impl std::fmt::Display for Fault {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str("EOS SDK worker operation failed")
    }
}
impl std::error::Error for Fault {}

fn copy_text(value: &str, maximum: usize) -> Result<String, Error> {
    if value.len() > maximum || value.contains('\0') {
        return Err(Fault.into());
    }
    Ok(value.to_owned())
}

struct Job {
    bytes: Vec<u8>,
    answer: mpsc::SyncSender<Outcome<wire::Envelope<wire::Reply>>>,
}

fn io_loop(mut input: impl Write, mut output: impl Read, jobs: mpsc::Receiver<Job>) {
    let mut synchronized = false;
    for job in jobs {
        let result = wire::write_frame(&mut input, &job.bytes)
            .and_then(|()| {
                if !synchronized {
                    wire::read_startup(&mut output)?;
                    synchronized = true;
                }
                Ok(())
            })
            .and_then(|()| wire::read_frame(&mut output))
            .map_err(|_| Fault)
            .and_then(|value| value.ok_or(Fault));
        let failed = result.is_err();
        if job.answer.send(result).is_err() || failed {
            break;
        }
    }
}

struct Worker {
    child: Child,
    jobs: Option<mpsc::SyncSender<Job>>,
    io: Option<JoinHandle<()>>,
    sequence: u64,
    handles: BTreeSet<u64>,
}

fn reap_child(child: &mut Child, graceful: bool) -> bool {
    if graceful {
        let deadline = Instant::now() + REAP_TIMEOUT;
        while Instant::now() < deadline {
            if matches!(child.try_wait(), Ok(Some(_))) {
                return true;
            }
            thread::sleep(Duration::from_millis(2));
        }
    }
    let _ = child.kill();
    let deadline = Instant::now() + REAP_TIMEOUT;
    while Instant::now() < deadline {
        match child.try_wait() {
            Ok(Some(_)) => return true,
            Ok(None) => thread::sleep(Duration::from_millis(2)),
            Err(_) => return false,
        }
    }
    false
}

impl Worker {
    fn start(executable: &Path) -> Outcome<Self> {
        let child = Command::new(executable)
            .arg("--eos-sdk-worker")
            // Configuration is supplied privately in the Create message.
            .env_remove("H2_EOS_CLIENT_SECRET")
            .env_remove("H2_EOS_CONFIG")
            .stdin(Stdio::piped())
            .stdout(Stdio::piped())
            // SDK/vendor output is neither protocol nor a credential-safe log.
            .stderr(Stdio::null())
            .spawn()
            .map_err(|_| Fault)?;
        Ok(Self {
            child,
            jobs: None,
            io: None,
            sequence: 0,
            handles: BTreeSet::new(),
        })
    }

    fn start_io(&mut self) -> Outcome<()> {
        let (Some(input), Some(output)) = (self.child.stdin.take(), self.child.stdout.take())
        else {
            return Err(Fault);
        };
        let (sender, receiver) = mpsc::sync_channel(1);
        let io = match thread::Builder::new()
            .name("eos-sdk-pipe".into())
            .spawn(move || io_loop(input, output, receiver))
        {
            Ok(io) => io,
            Err(_) => return Err(Fault),
        };
        self.jobs = Some(sender);
        self.io = Some(io);
        Ok(())
    }

    fn exchange(&mut self, request: wire::Request, timeout: Duration) -> Outcome<wire::Reply> {
        if !request.valid() {
            return Err(Fault);
        }
        self.sequence = self.sequence.checked_add(1).ok_or(Fault)?;
        let expected = self.sequence;
        let message = wire::Envelope {
            version: wire::VERSION,
            sequence: expected,
            body: request,
        };
        let bytes = wire::encode(&message).map_err(|_| Fault)?;
        let (sender, receiver) = mpsc::sync_channel(1);
        self.jobs
            .as_ref()
            .ok_or(Fault)?
            .try_send(Job {
                bytes,
                answer: sender,
            })
            .map_err(|_| Fault)?;
        let reply = receiver.recv_timeout(timeout).map_err(|_| Fault)??;
        validate_reply(expected, &message.body, &reply)?;
        Ok(reply.body)
    }

    fn stop(&mut self, graceful: bool) -> bool {
        self.jobs.take();
        // Retain this owned Child in Owner if the OS has not reaped it yet.
        let reaped = reap_child(&mut self.child, graceful);
        if reaped {
            let deadline = Instant::now() + REAP_TIMEOUT;
            while self.io.as_ref().is_some_and(|io| !io.is_finished()) && Instant::now() < deadline
            {
                thread::sleep(Duration::from_millis(2));
            }
            if self.io.as_ref().is_some_and(JoinHandle::is_finished)
                && let Some(io) = self.io.take()
            {
                let _ = io.join();
            }
        }
        reaped && self.io.is_none()
    }
}

fn validate_reply(
    expected: u64,
    request: &wire::Request,
    reply: &wire::Envelope<wire::Reply>,
) -> Outcome<()> {
    if reply.version != wire::VERSION
        || reply.sequence != expected
        || !reply.body.valid()
        || !request.accepts(&reply.body)
    {
        return Err(Fault);
    }
    Ok(())
}

#[derive(Default)]
struct Owner {
    executable: Option<PathBuf>,
    worker: Option<Worker>,
    broken: bool,
    finalized: bool,
}

impl Owner {
    fn register_adjacent_worker(&mut self, host: &Path) -> Outcome<()> {
        if !host.is_absolute() {
            return Err(Fault);
        }
        let host_metadata = std::fs::metadata(host).map_err(|_| Fault)?;
        if !host_metadata.is_file() {
            return Err(Fault);
        }
        let executable = host.parent().ok_or(Fault)?.join(WORKER_FILE_NAME);
        // The package owns this exact sibling. Do not follow a redirected
        // worker, search PATH, accept an env override or execute the game.
        let metadata = std::fs::symlink_metadata(&executable).map_err(|_| Fault)?;
        if !metadata.file_type().is_file() || executable == host {
            return Err(Fault);
        }
        #[cfg(unix)]
        {
            use std::os::unix::fs::{MetadataExt, PermissionsExt};
            if metadata.permissions().mode() & 0o111 == 0
                || (metadata.dev(), metadata.ino()) == (host_metadata.dev(), host_metadata.ino())
            {
                return Err(Fault);
            }
        }
        self.register(executable)
    }

    fn flush_retirements(&mut self, deadline: Instant) -> Outcome<()> {
        if retirements().overflow.load(Ordering::Acquire) {
            self.invalidate();
            return Err(Fault);
        }
        while let Some(handle) = retirements().pop().inspect_err(|_| self.invalidate())? {
            if self
                .worker
                .as_ref()
                .is_none_or(|worker| !worker.handles.contains(&handle))
            {
                continue;
            }
            if Instant::now() >= deadline {
                retirements()
                    .insert(handle)
                    .inspect_err(|_| self.invalidate())?;
                return Err(Fault);
            }
            if self
                .request(wire::Request::Destroy { handle }, deadline)
                .is_err()
            {
                if !self.broken {
                    retirements()
                        .insert(handle)
                        .inspect_err(|_| self.invalidate())?;
                }
                return Err(Fault);
            }
        }
        Ok(())
    }

    fn register(&mut self, executable: PathBuf) -> Outcome<()> {
        if self.finalized || self.broken || !executable.is_absolute() || !executable.is_file() {
            return Err(Fault);
        }
        match &self.executable {
            Some(previous) if previous != &executable => Err(Fault),
            Some(_) => Ok(()),
            None => {
                self.executable = Some(executable);
                Ok(())
            }
        }
    }

    fn invalidate(&mut self) {
        self.broken = true;
        if let Some(mut worker) = self.worker.take()
            && !worker.stop(false)
        {
            self.worker = Some(worker);
        }
    }

    fn request(&mut self, request: wire::Request, deadline: Instant) -> Outcome<wire::Reply> {
        if self.broken || self.finalized || !request.valid() {
            return Err(Fault);
        }
        if self.worker.is_none() {
            if !matches!(request, wire::Request::Create(_)) {
                return Err(Fault);
            }
            let executable = self.executable.as_ref().ok_or(Fault)?;
            self.worker = Some(Worker::start(executable)?);
            let worker = self.worker.as_mut().ok_or(Fault)?;
            let timeout = CREATE_TIMEOUT.min(deadline.saturating_duration_since(Instant::now()));
            if worker.start_io().is_err()
                || !matches!(
                    worker.exchange(wire::Request::Hello, timeout),
                    Ok(wire::Reply::Hello)
                )
            {
                self.invalidate();
                return Err(Fault);
            }
        }
        let worker = self.worker.as_mut().ok_or(Fault)?;
        match &request {
            wire::Request::Create(_) if worker.handles.len() == wire::MAX_HANDLES => {
                return Err(Fault);
            }
            wire::Request::Call { handle, .. } | wire::Request::Destroy { handle }
                if !worker.handles.contains(handle) =>
            {
                return Err(Fault);
            }
            _ => {}
        }
        let timeout = match &request {
            wire::Request::Create(_) => CREATE_TIMEOUT,
            wire::Request::Destroy { .. } => DESTROY_TIMEOUT,
            wire::Request::Shutdown => Duration::from_secs(5 + 2 * worker.handles.len() as u64),
            _ => CALL_TIMEOUT,
        };
        let timeout = timeout.min(deadline.saturating_duration_since(Instant::now()));
        if timeout.is_zero() {
            self.invalidate();
            return Err(Fault);
        }
        let created = matches!(request, wire::Request::Create(_));
        let destroyed = match &request {
            wire::Request::Destroy { handle } => Some(*handle),
            _ => None,
        };
        let response = worker.exchange(request, timeout);
        match response {
            Ok(wire::Reply::Failed) => Err(Fault),
            Ok(wire::Reply::Handle(handle)) if created => {
                if !worker.handles.insert(handle) {
                    self.invalidate();
                    return Err(Fault);
                }
                Ok(wire::Reply::Handle(handle))
            }
            Ok(reply) => {
                if let Some(handle) = destroyed {
                    worker.handles.remove(&handle);
                }
                Ok(reply)
            }
            Err(_) => {
                self.invalidate();
                Err(Fault)
            }
        }
    }
}

fn owner() -> &'static Mutex<Owner> {
    static OWNER: OnceLock<Mutex<Owner>> = OnceLock::new();
    OWNER.get_or_init(|| Mutex::new(Owner::default()))
}

#[derive(Default)]
struct Retirements {
    pending: Mutex<BTreeSet<u64>>,
    overflow: AtomicBool,
}
impl Retirements {
    fn insert(&self, handle: u64) -> Outcome<()> {
        let Ok(mut pending) = self.pending.lock() else {
            self.overflow.store(true, Ordering::Release);
            return Err(Fault);
        };
        if pending.contains(&handle) {
            return Ok(());
        }
        if pending.len() >= wire::MAX_HANDLES {
            // No live handle is silently abandoned: latch terminal failure and
            // the next owner acquisition kills/reaps the entire owned child.
            self.overflow.store(true, Ordering::Release);
            return Err(Fault);
        }
        pending.insert(handle);
        Ok(())
    }
    fn pop(&self) -> Outcome<Option<u64>> {
        match self.pending.lock() {
            Ok(mut pending) => Ok(pending.pop_first()),
            Err(_) => {
                self.overflow.store(true, Ordering::Release);
                Err(Fault)
            }
        }
    }
}
fn retirements() -> &'static Retirements {
    static RETIREMENTS: OnceLock<Retirements> = OnceLock::new();
    RETIREMENTS.get_or_init(Retirements::default)
}

fn lock_until(deadline: Instant) -> Outcome<MutexGuard<'static, Owner>> {
    loop {
        if Instant::now() >= deadline {
            return Err(Fault);
        }
        match owner().try_lock() {
            Ok(guard) => return Ok(guard),
            Err(TryLockError::Poisoned(_)) => return Err(Fault),
            Err(TryLockError::WouldBlock) => thread::sleep(Duration::from_millis(2)),
        }
    }
}

fn request(value: wire::Request) -> Result<wire::Reply, Error> {
    // Lock admission consumes the same caller budget. Timing out before owning
    // the lock must not kill another caller's active SDK request.
    let timeout = match &value {
        wire::Request::Create(_) => CREATE_TIMEOUT * 2,
        wire::Request::Destroy { .. } => DESTROY_TIMEOUT,
        _ => CALL_TIMEOUT,
    };
    let deadline = Instant::now() + timeout;
    let mut owner = lock_until(deadline)?;
    owner.flush_retirements(deadline)?;
    let result = owner.request(value, deadline);
    if retirements().overflow.load(Ordering::Acquire) {
        owner.invalidate();
        return Err(Fault.into());
    }
    result.map_err(Into::into)
}

pub(crate) fn register_adjacent_worker() -> Result<(), Error> {
    let host = std::env::current_exe().map_err(|_| Fault)?;
    lock_until(Instant::now() + CALL_TIMEOUT)?
        .register_adjacent_worker(&host)
        .map_err(Into::into)
}

/// Explicit private-fixture opt-in; production never accepts an env/PATH override.
#[cfg(test)]
#[allow(dead_code)] // Used by explicit owned SDK component harnesses.
pub(crate) fn register_test_worker_executable(executable: &Path) -> Result<(), Error> {
    let executable = executable.canonicalize().map_err(|_| Fault)?;
    lock_until(Instant::now() + CALL_TIMEOUT)?
        .register(executable)
        .map_err(Into::into)
}

pub(crate) fn process_shutdown() -> bool {
    let deadline = Instant::now() + Duration::from_secs(5 + 2 * wire::MAX_HANDLES as u64);
    let Ok(mut owner) = lock_until(deadline) else {
        return false;
    };
    if owner.flush_retirements(deadline).is_err() && !owner.broken {
        return false;
    }
    if owner.broken {
        if let Some(mut worker) = owner.worker.take()
            && !worker.stop(false)
        {
            owner.worker = Some(worker);
        }
        return false;
    }
    if owner.finalized {
        return owner.worker.is_none();
    }
    let had_live_handles = owner
        .worker
        .as_ref()
        .is_some_and(|worker| !worker.handles.is_empty());
    // Match the SDK lifetime contract: refusal while a platform is retained
    // must not retire it or prevent subsequent profile/match creation.
    if had_live_handles {
        return false;
    }
    if owner.worker.is_none() {
        return !owner.broken;
    }
    let replied = matches!(
        owner.request(wire::Request::Shutdown, deadline),
        Ok(wire::Reply::Bool(true))
    );
    owner.finalized = true;
    let reaped = if let Some(mut worker) = owner.worker.take() {
        if worker.stop(replied) {
            true
        } else {
            owner.worker = Some(worker);
            false
        }
    } else {
        true
    };
    replied && reaped
}

pub(crate) struct SdkProxy {
    handle: u64,
}
impl SdkProxy {
    pub fn null() -> Self {
        Self { handle: 0 }
    }
    pub fn is_null(&self) -> bool {
        self.handle == 0
    }
    pub fn as_ref(&self) -> Option<&Self> {
        (!self.is_null()).then_some(self)
    }
    pub fn pin_mut(&mut self) -> &mut Self {
        self
    }

    fn call(&self, call: wire::Call) -> Result<wire::Reply, Error> {
        request(wire::Request::Call {
            handle: self.handle,
            call,
        })
    }
    fn unit(&self, call: wire::Call) -> Result<(), Error> {
        match self.call(call)? {
            wire::Reply::Unit => Ok(()),
            _ => Err(Fault.into()),
        }
    }
    pub fn sdk_device(&mut self) -> Result<(), Error> {
        self.unit(wire::Call::Device)
    }
    pub fn sdk_login(&mut self) -> Result<(), Error> {
        self.unit(wire::Call::Login)
    }
    pub fn sdk_external_login(&mut self, token: &str) -> Result<(), Error> {
        self.unit(wire::Call::ExternalLogin {
            token: copy_text(token, 8192)?,
        })
    }
    pub fn sdk_create_user(&mut self) -> Result<(), Error> {
        self.unit(wire::Call::CreateUser)
    }
    pub fn sdk_socket(&mut self, name: &str, force: bool) -> Result<(), Error> {
        self.unit(wire::Call::Socket {
            name: copy_text(name, 32)?,
            force,
        })
    }
    pub fn sdk_id_token(&self) -> Result<String, Error> {
        match self.call(wire::Call::IdToken)? {
            wire::Reply::Text(text) => Ok(text),
            _ => Err(Fault.into()),
        }
    }
    pub fn sdk_poll(&mut self) -> Result<Vec<ffi::EosEvent>, Error> {
        match self.call(wire::Call::Poll)? {
            wire::Reply::Events(events) => Ok(events
                .into_iter()
                .map(|event| ffi::EosEvent {
                    kind: event.kind,
                    code: event.code,
                    user: event.user,
                    socket: event.socket,
                    data: event.data,
                })
                .collect()),
            _ => Err(Fault.into()),
        }
    }
    pub fn sdk_accept(&mut self, user: &str) -> bool {
        let Ok(user) = copy_text(user, 64) else {
            return false;
        };
        matches!(
            self.call(wire::Call::Accept { user }),
            Ok(wire::Reply::Bool(true))
        )
    }
    pub fn sdk_send(&mut self, user: &str, bytes: &[u8], delayed: bool) -> i32 {
        if bytes.is_empty() || bytes.len() > wire::MAX_PACKET_BYTES {
            return -1;
        }
        let Ok(user) = copy_text(user, 64) else {
            return -1;
        };
        match self.call(wire::Call::Send {
            user,
            bytes: bytes.to_vec(),
            delayed,
        }) {
            Ok(wire::Reply::Result(result)) => result,
            _ => -1,
        }
    }
    pub fn sdk_packet_queue_info(&self) -> ffi::EosPacketQueueInfo {
        match self.call(wire::Call::QueueInfo) {
            Ok(wire::Reply::Queue(info)) => ffi::EosPacketQueueInfo {
                result: info.result,
                incoming_bytes: info.incoming_bytes,
                incoming_packets: info.incoming_packets,
                outgoing_bytes: info.outgoing_bytes,
                outgoing_packets: info.outgoing_packets,
            },
            _ => ffi::EosPacketQueueInfo {
                result: -1,
                incoming_bytes: 0,
                incoming_packets: 0,
                outgoing_bytes: 0,
                outgoing_packets: 0,
            },
        }
    }
    pub fn sdk_session_create(
        &mut self,
        session: &ffi::EosSession,
        bucket: &str,
        capacity: u32,
    ) -> Result<(), Error> {
        self.unit(wire::Call::SessionCreate {
            session: wire::Session {
                enabled: session.enabled,
                playlist: copy_text(&session.playlist, 64)?,
                revision: copy_text(&session.revision, 64)?,
                map: copy_text(&session.map, 64)?,
                variant: copy_text(&session.variant, 64)?,
            },
            bucket: copy_text(bucket, 64)?,
            capacity,
        })
    }
    pub fn sdk_session_invite(&mut self, user: &str) -> Result<(), Error> {
        self.unit(wire::Call::SessionInvite {
            user: copy_text(user, 64)?,
        })
    }
    pub fn sdk_session_query(&mut self) -> Result<(), Error> {
        self.unit(wire::Call::SessionQuery)
    }
    pub fn sdk_session_register(&mut self, user: &str) -> Result<(), Error> {
        self.unit(wire::Call::SessionRegister {
            user: copy_text(user, 64)?,
        })
    }
    pub fn sdk_session_info(&self, invite: &str) -> Result<ffi::EosInviteInfo, Error> {
        match self.call(wire::Call::SessionInfo {
            invite: copy_text(invite, 256)?,
        })? {
            wire::Reply::Invite(info) => Ok(ffi::EosInviteInfo {
                session: info.session,
                owner: info.owner,
                playlist: info.playlist,
                revision: info.revision,
                map: info.map,
                variant: info.variant,
            }),
            _ => Err(Fault.into()),
        }
    }
    pub fn sdk_session_join(&mut self, invite: &str) -> Result<(), Error> {
        self.unit(wire::Call::SessionJoin {
            invite: copy_text(invite, 256)?,
        })
    }
}

impl Drop for SdkProxy {
    fn drop(&mut self) {
        if self.handle != 0 {
            // Do not wait behind another platform's call or lose this handle
            // when that wait would expire. All later calls/shutdown drain it.
            let _ = retirements().insert(self.handle);
            if let Ok(mut owner) = owner().try_lock() {
                let _ = owner.flush_retirements(Instant::now() + DESTROY_TIMEOUT);
            }
            self.handle = 0;
        }
    }
}

pub(crate) fn new_sdk_proxy(config: &ffi::EosConfig) -> Result<SdkProxy, Error> {
    let value = wire::Config {
        product: copy_text(&config.product, 32)?,
        sandbox: copy_text(&config.sandbox, 32)?,
        deployment: copy_text(&config.deployment, 32)?,
        client: copy_text(&config.client, 4096)?,
        secret: copy_text(&config.secret, 4096)?,
        cache: copy_text(&config.cache, 16384)?,
    };
    match request(wire::Request::Create(value))? {
        wire::Reply::Handle(handle) => Ok(SdkProxy { handle }),
        _ => Err(Fault.into()),
    }
}

trait Backend {
    fn execute(&mut self, request: wire::Request) -> wire::Reply;
    fn shutdown(&mut self) -> bool;
}

#[derive(Default)]
struct SdkBackend {
    handles: BTreeMap<u64, cxx::UniquePtr<ffi::RawEos>>,
    next_handle: u64,
    finalized: bool,
    shutdown_result: bool,
}

impl SdkBackend {
    fn execute_checked(&mut self, request: wire::Request) -> Result<wire::Reply, Error> {
        match request {
            wire::Request::Create(config) => {
                if self.finalized || self.handles.len() == wire::MAX_HANDLES {
                    return Err(Fault.into());
                }
                let handle = self.next_handle.checked_add(1).ok_or(Fault)?;
                let config = ffi::EosConfig {
                    product: config.product,
                    sandbox: config.sandbox,
                    deployment: config.deployment,
                    client: config.client,
                    secret: config.secret,
                    cache: config.cache,
                };
                let raw = ffi::new_raw_eos(&config)?;
                if raw.is_null() {
                    return Err(Fault.into());
                }
                self.next_handle = handle;
                self.handles.insert(handle, raw);
                Ok(wire::Reply::Handle(handle))
            }
            wire::Request::Destroy { handle } => {
                self.handles.remove(&handle).ok_or(Fault)?;
                Ok(wire::Reply::Unit)
            }
            wire::Request::Call { handle, call } => {
                let raw = self.handles.get_mut(&handle).ok_or(Fault)?;
                match call {
                    wire::Call::Device => raw.pin_mut().sdk_device()?,
                    wire::Call::Login => raw.pin_mut().sdk_login()?,
                    wire::Call::ExternalLogin { token } => {
                        raw.pin_mut().sdk_external_login(&token)?
                    }
                    wire::Call::CreateUser => raw.pin_mut().sdk_create_user()?,
                    wire::Call::Socket { name, force } => raw.pin_mut().sdk_socket(&name, force)?,
                    wire::Call::IdToken => return Ok(wire::Reply::Text(raw.sdk_id_token()?)),
                    wire::Call::Poll => {
                        return Ok(wire::Reply::Events(
                            raw.pin_mut()
                                .sdk_poll()?
                                .into_iter()
                                .map(|event| wire::Event {
                                    kind: event.kind,
                                    code: event.code,
                                    user: event.user,
                                    socket: event.socket,
                                    data: event.data,
                                })
                                .collect(),
                        ));
                    }
                    wire::Call::Accept { user } => {
                        return Ok(wire::Reply::Bool(raw.pin_mut().sdk_accept(&user)));
                    }
                    wire::Call::Send {
                        user,
                        bytes,
                        delayed,
                    } => {
                        return Ok(wire::Reply::Result(
                            raw.pin_mut().sdk_send(&user, &bytes, delayed),
                        ));
                    }
                    wire::Call::QueueInfo => {
                        let info = raw.sdk_packet_queue_info();
                        return Ok(wire::Reply::Queue(wire::QueueInfo {
                            result: info.result,
                            incoming_bytes: info.incoming_bytes,
                            incoming_packets: info.incoming_packets,
                            outgoing_bytes: info.outgoing_bytes,
                            outgoing_packets: info.outgoing_packets,
                        }));
                    }
                    wire::Call::SessionCreate {
                        session,
                        bucket,
                        capacity,
                    } => {
                        let session = ffi::EosSession {
                            enabled: session.enabled,
                            playlist: session.playlist,
                            revision: session.revision,
                            map: session.map,
                            variant: session.variant,
                        };
                        raw.pin_mut()
                            .sdk_session_create(&session, &bucket, capacity)?;
                    }
                    wire::Call::SessionInvite { user } => {
                        raw.pin_mut().sdk_session_invite(&user)?
                    }
                    wire::Call::SessionQuery => raw.pin_mut().sdk_session_query()?,
                    wire::Call::SessionRegister { user } => {
                        raw.pin_mut().sdk_session_register(&user)?
                    }
                    wire::Call::SessionInfo { invite } => {
                        let info = raw.sdk_session_info(&invite)?;
                        return Ok(wire::Reply::Invite(wire::InviteInfo {
                            session: info.session,
                            owner: info.owner,
                            playlist: info.playlist,
                            revision: info.revision,
                            map: info.map,
                            variant: info.variant,
                        }));
                    }
                    wire::Call::SessionJoin { invite } => {
                        raw.pin_mut().sdk_session_join(&invite)?
                    }
                }
                Ok(wire::Reply::Unit)
            }
            _ => Err(Fault.into()),
        }
    }
}

impl Backend for SdkBackend {
    fn execute(&mut self, request: wire::Request) -> wire::Reply {
        self.execute_checked(request).unwrap_or(wire::Reply::Failed)
    }
    fn shutdown(&mut self) -> bool {
        if self.finalized {
            return self.shutdown_result;
        }
        self.handles.clear();
        self.finalized = true;
        self.shutdown_result = ffi::sdk_process_shutdown();
        self.shutdown_result
    }
}

fn run_backend(reader: &mut impl Read, writer: &mut impl Write, backend: &mut impl Backend) -> i32 {
    let result = serve(reader, writer, backend);
    let clean = backend.shutdown();
    if result.is_ok() && clean { 0 } else { 2 }
}

struct WorkerOutput(cxx::UniquePtr<ffi::SdkWorkerOutput>);
impl Write for WorkerOutput {
    fn write(&mut self, bytes: &[u8]) -> io::Result<usize> {
        self.0
            .pin_mut()
            .sdk_worker_write(bytes)
            .map_err(|_| io::Error::other("EOS worker output failed"))?;
        Ok(bytes.len())
    }
    fn flush(&mut self) -> io::Result<()> {
        self.0
            .pin_mut()
            .sdk_worker_flush()
            .map_err(|_| io::Error::other("EOS worker output failed"))
    }
}
impl Drop for SdkBackend {
    fn drop(&mut self) {
        self.shutdown();
    }
}

fn serve(
    reader: &mut impl Read,
    writer: &mut impl Write,
    backend: &mut impl Backend,
) -> io::Result<()> {
    let mut sequence = 0u64;
    let mut admitted = false;
    while let Some(request) = wire::read_frame::<wire::Envelope<wire::Request>>(reader)? {
        let expected = sequence
            .checked_add(1)
            .ok_or_else(|| io::Error::other("EOS worker sequence exhausted"))?;
        if request.version != wire::VERSION
            || request.sequence != expected
            || !request.body.valid()
            || (!admitted && !matches!(request.body, wire::Request::Hello))
            || (admitted && matches!(request.body, wire::Request::Hello))
        {
            return Err(io::Error::other("Invalid EOS worker request"));
        }
        sequence = expected;
        let finish = matches!(request.body, wire::Request::Shutdown);
        let reply = match request.body {
            wire::Request::Hello => {
                admitted = true;
                wire::Reply::Hello
            }
            wire::Request::Shutdown => wire::Reply::Bool(backend.shutdown()),
            operation => backend.execute(operation),
        };
        if !reply.valid() {
            return Err(io::Error::other("Invalid EOS worker reply"));
        }
        let bytes = wire::encode(&wire::Envelope {
            version: wire::VERSION,
            sequence,
            body: reply,
        })?;
        wire::write_frame(writer, &bytes)?;
        if finish {
            return Ok(());
        }
    }
    Ok(())
}

/// Invoked only by the executable's exact internal-flag branch on process main.
pub(crate) fn worker_main() -> i32 {
    std::panic::catch_unwind(|| {
        // Preserve the private protocol pipe before SDK initialization can emit
        // a vendor banner; ordinary stdout is redirected by this native owner.
        let Ok(output) = ffi::new_sdk_worker_output() else {
            return 2;
        };
        if output.is_null() {
            return 2;
        }
        let mut output = WorkerOutput(output);
        // Emit after stdout isolation and before any SDK initialization. The
        // parent consumes this marker only for its credential-free Hello.
        if wire::write_startup(&mut output).is_err() {
            return 2;
        }
        let mut backend = SdkBackend::default();
        run_backend(&mut io::stdin().lock(), &mut output, &mut backend)
    })
    .unwrap_or(3)
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::io::Cursor;

    struct RegistrationFixture(PathBuf);
    impl RegistrationFixture {
        fn new() -> Self {
            static NEXT: std::sync::atomic::AtomicUsize = std::sync::atomic::AtomicUsize::new(0);
            let parent = PathBuf::from(
                std::env::var_os("HALO2_TEST_SCRATCH")
                    .expect("Run filesystem controls through tools/run_test.py"),
            );
            assert!(parent.is_absolute() && parent.is_dir());
            for _ in 0..64 {
                let path = parent.join(format!(
                    "sdk-adjacent-registration-{}-{}",
                    std::process::id(),
                    NEXT.fetch_add(1, Ordering::Relaxed)
                ));
                match std::fs::create_dir(&path) {
                    Ok(()) => return Self(path),
                    Err(error) if error.kind() == io::ErrorKind::AlreadyExists => {}
                    Err(error) => panic!("Owned registration fixture failed: {error}"),
                }
            }
            panic!("Owned registration fixture exhausted");
        }
        fn host(&self, name: &str) -> PathBuf {
            let directory = self.0.join(name);
            std::fs::create_dir(&directory).unwrap();
            let host = directory.join("halo2_native");
            std::fs::write(&host, b"registration host; never executed").unwrap();
            host
        }
        fn worker(host: &Path) -> PathBuf {
            let worker = host.parent().unwrap().join(WORKER_FILE_NAME);
            std::fs::write(&worker, b"registration worker; never executed").unwrap();
            #[cfg(unix)]
            {
                use std::os::unix::fs::PermissionsExt;
                std::fs::set_permissions(&worker, std::fs::Permissions::from_mode(0o700)).unwrap();
            }
            worker
        }
    }
    impl Drop for RegistrationFixture {
        fn drop(&mut self) {
            std::fs::remove_dir_all(&self.0).unwrap();
        }
    }

    #[test]
    fn host_registration_requires_the_fixed_adjacent_worker_without_fallback() {
        let fixture = RegistrationFixture::new();
        let host = fixture.host("package");
        let elsewhere = fixture.host("elsewhere");
        RegistrationFixture::worker(&elsewhere);
        let mut owner = Owner::default();
        assert!(owner.register_adjacent_worker(&host).is_err());
        assert!(owner.executable.is_none() && owner.worker.is_none());
        assert!(
            owner
                .request(
                    wire::Request::Create(config()),
                    Instant::now() + CREATE_TIMEOUT
                )
                .is_err()
        );
        assert!(owner.worker.is_none());
        std::fs::create_dir(host.parent().unwrap().join(WORKER_FILE_NAME)).unwrap();
        assert!(owner.register_adjacent_worker(&host).is_err());
        assert!(owner.executable.is_none());
        assert!(
            owner
                .register_adjacent_worker(Path::new("halo2_native"))
                .is_err()
        );
    }

    #[test]
    fn host_registration_is_idempotent_and_cannot_switch_package_workers() {
        let fixture = RegistrationFixture::new();
        let host = fixture.host("package");
        let worker = RegistrationFixture::worker(&host);
        let other_host = fixture.host("other-package");
        RegistrationFixture::worker(&other_host);
        let mut owner = Owner::default();
        owner.register_adjacent_worker(&host).unwrap();
        owner.register_adjacent_worker(&host).unwrap();
        assert_eq!(owner.executable.as_ref(), Some(&worker));
        assert!(owner.register_adjacent_worker(&other_host).is_err());
        assert_eq!(owner.executable.as_ref(), Some(&worker));
        assert!(owner.worker.is_none());
    }

    #[cfg(unix)]
    #[test]
    fn host_registration_refuses_redirected_or_non_executable_workers() {
        use std::os::unix::fs::{PermissionsExt, symlink};
        let fixture = RegistrationFixture::new();
        let host = fixture.host("package");
        let worker = host.parent().unwrap().join(WORKER_FILE_NAME);
        let other_host = fixture.host("other-package");
        let other_worker = RegistrationFixture::worker(&other_host);
        let mut owner = Owner::default();
        for target in [&host, &other_worker] {
            symlink(target, &worker).unwrap();
            assert!(owner.register_adjacent_worker(&host).is_err());
            assert!(owner.executable.is_none());
            std::fs::remove_file(&worker).unwrap();
        }
        std::fs::set_permissions(&host, std::fs::Permissions::from_mode(0o700)).unwrap();
        std::fs::hard_link(&host, &worker).unwrap();
        assert!(owner.register_adjacent_worker(&host).is_err());
        assert!(owner.executable.is_none());
        std::fs::remove_file(&worker).unwrap();
        let worker = RegistrationFixture::worker(&host);
        std::fs::set_permissions(&worker, std::fs::Permissions::from_mode(0o600)).unwrap();
        assert!(owner.register_adjacent_worker(&host).is_err());
        assert!(owner.executable.is_none() && owner.worker.is_none());
    }

    #[derive(Default)]
    struct FakeBackend {
        handles: BTreeSet<u64>,
        destroyed: usize,
        calls: usize,
        shutdowns: usize,
    }
    impl Backend for FakeBackend {
        fn execute(&mut self, request: wire::Request) -> wire::Reply {
            self.calls += 1;
            match request {
                wire::Request::Create(_) => {
                    self.handles.insert(1);
                    wire::Reply::Handle(1)
                }
                wire::Request::Call {
                    handle: 1,
                    call: wire::Call::Send { .. },
                } if self.handles.contains(&1) => wire::Reply::Result(22),
                wire::Request::Destroy { handle } if self.handles.remove(&handle) => {
                    self.destroyed += 1;
                    wire::Reply::Unit
                }
                _ => wire::Reply::Failed,
            }
        }
        fn shutdown(&mut self) -> bool {
            self.shutdowns += 1;
            self.destroyed += self.handles.len();
            self.handles.clear();
            true
        }
    }
    fn config() -> wire::Config {
        wire::Config {
            product: "a".repeat(32),
            sandbox: "b".repeat(32),
            deployment: "c".repeat(32),
            client: "client".into(),
            secret: "not-a-real-credential".into(),
            cache: std::env::current_dir()
                .unwrap()
                .to_string_lossy()
                .into_owned(),
        }
    }
    fn append(bytes: &mut Vec<u8>, sequence: u64, body: wire::Request) {
        wire::write_frame(
            bytes,
            &wire::encode(&wire::Envelope {
                version: wire::VERSION,
                sequence,
                body,
            })
            .unwrap(),
        )
        .unwrap();
    }

    #[test]
    fn server_preserves_raw22_and_retires_live_handles_on_eof() {
        let mut bytes = vec![];
        append(&mut bytes, 1, wire::Request::Hello);
        append(&mut bytes, 2, wire::Request::Create(config()));
        append(
            &mut bytes,
            3,
            wire::Request::Call {
                handle: 1,
                call: wire::Call::Send {
                    user: "d".repeat(32),
                    bytes: vec![7; 1170],
                    delayed: false,
                },
            },
        );
        let mut backend = FakeBackend::default();
        let mut output = vec![];
        assert_eq!(
            run_backend(&mut Cursor::new(bytes), &mut output, &mut backend),
            0
        );
        assert_eq!(
            (backend.destroyed, backend.shutdowns, backend.calls),
            (1, 1, 2)
        );
        let mut output = Cursor::new(output);
        for sequence in 1..=3 {
            let reply: wire::Envelope<wire::Reply> =
                wire::read_frame(&mut output).unwrap().unwrap();
            assert_eq!(reply.sequence, sequence);
            if sequence == 3 {
                assert!(matches!(reply.body, wire::Reply::Result(22)));
            }
        }
    }

    #[test]
    fn server_refuses_work_before_hello_and_wrong_sequences_then_cleans_up() {
        for (sequence, first) in [
            (1, wire::Request::Create(config())),
            (2, wire::Request::Hello),
        ] {
            let mut bytes = vec![];
            append(&mut bytes, sequence, first);
            let mut backend = FakeBackend::default();
            let mut output = vec![];
            assert_eq!(
                run_backend(&mut Cursor::new(bytes), &mut output, &mut backend),
                2
            );
            assert_eq!((backend.calls, backend.shutdowns), (0, 1));
            assert!(output.is_empty());
        }
        let mut bytes = vec![];
        append(&mut bytes, 1, wire::Request::Hello);
        append(&mut bytes, 2, wire::Request::Create(config()));
        append(&mut bytes, 2, wire::Request::Hello);
        let mut backend = FakeBackend::default();
        assert_eq!(
            run_backend(&mut Cursor::new(bytes), &mut vec![], &mut backend),
            2
        );
        assert_eq!(backend.destroyed, 1);
    }

    #[test]
    fn parent_rejects_wrong_version_sequence_and_reply_type() {
        let request = wire::Request::Call {
            handle: 1,
            call: wire::Call::Poll,
        };
        let mut reply = wire::Envelope {
            version: wire::VERSION,
            sequence: 9,
            body: wire::Reply::Events(vec![]),
        };
        assert!(validate_reply(9, &request, &reply).is_ok());
        reply.version += 1;
        assert!(validate_reply(9, &request, &reply).is_err());
        reply.version = wire::VERSION;
        reply.sequence = 8;
        assert!(validate_reply(9, &request, &reply).is_err());
        reply.sequence = 9;
        reply.body = wire::Reply::Bool(true);
        assert!(validate_reply(9, &request, &reply).is_err());
    }

    #[test]
    fn parent_syncs_once_then_rejects_a_second_startup_marker() {
        let mut bytes = vec![b'x'; 30];
        wire::write_startup(&mut bytes).unwrap();
        wire::write_frame(
            &mut bytes,
            &wire::encode(&wire::Envelope {
                version: wire::VERSION,
                sequence: 1,
                body: wire::Reply::Hello,
            })
            .unwrap(),
        )
        .unwrap();
        wire::write_startup(&mut bytes).unwrap();
        let (jobs, receiver) = mpsc::sync_channel(2);
        let (first_answer, first_reply) = mpsc::sync_channel(1);
        let (next_answer, next_reply) = mpsc::sync_channel(1);
        for (sequence, answer) in [(1, first_answer), (2, next_answer)] {
            jobs.send(Job {
                bytes: wire::encode(&wire::Envelope {
                    version: wire::VERSION,
                    sequence,
                    body: wire::Request::Hello,
                })
                .unwrap(),
                answer,
            })
            .unwrap();
        }
        drop(jobs);
        io_loop(vec![], Cursor::new(bytes), receiver);
        assert!(matches!(
            first_reply.recv().unwrap().unwrap().body,
            wire::Reply::Hello
        ));
        assert!(next_reply.recv().unwrap().is_err());
    }

    #[test]
    fn broken_or_unregistered_owner_never_spawns_or_changes_hosts() {
        let mut owner = Owner::default();
        assert!(
            owner
                .request(
                    wire::Request::Create(config()),
                    Instant::now() + CREATE_TIMEOUT
                )
                .is_err()
        );
        assert!(owner.worker.is_none());
        owner.register(std::env::current_exe().unwrap()).unwrap();
        assert!(owner.register(std::env::current_dir().unwrap()).is_err());
        owner.broken = true;
        assert!(
            owner
                .request(
                    wire::Request::Create(config()),
                    Instant::now() + CREATE_TIMEOUT
                )
                .is_err()
        );
        assert!(owner.worker.is_none());
    }

    #[test]
    fn blocking_pipe_read_does_not_block_the_caller_deadline() {
        struct BlockingRead(mpsc::Receiver<()>);
        impl Read for BlockingRead {
            fn read(&mut self, _: &mut [u8]) -> io::Result<usize> {
                let _ = self.0.recv();
                Ok(0)
            }
        }
        let (release, blocked) = mpsc::sync_channel(0);
        let (jobs, receiver) = mpsc::sync_channel(1);
        let io = thread::spawn(move || io_loop(Vec::<u8>::new(), BlockingRead(blocked), receiver));
        let (answer, reply) = mpsc::sync_channel(1);
        jobs.send(Job {
            bytes: wire::encode(&wire::Envelope {
                version: wire::VERSION,
                sequence: 1,
                body: wire::Request::Hello,
            })
            .unwrap(),
            answer,
        })
        .unwrap();
        assert!(matches!(
            reply.recv_timeout(Duration::from_millis(10)),
            Err(mpsc::RecvTimeoutError::Timeout)
        ));
        drop(reply);
        drop(release);
        drop(jobs);
        io.join().unwrap();
    }

    #[test]
    fn blocking_pipe_write_does_not_block_the_caller_deadline() {
        struct BlockingWrite(Option<mpsc::Receiver<()>>);
        impl Write for BlockingWrite {
            fn write(&mut self, bytes: &[u8]) -> io::Result<usize> {
                if let Some(wait) = self.0.take() {
                    let _ = wait.recv();
                }
                Ok(bytes.len())
            }
            fn flush(&mut self) -> io::Result<()> {
                Ok(())
            }
        }
        let (release, blocked) = mpsc::sync_channel(0);
        let (jobs, receiver) = mpsc::sync_channel(1);
        let io = thread::spawn(move || {
            io_loop(BlockingWrite(Some(blocked)), Cursor::new(vec![]), receiver)
        });
        let (answer, reply) = mpsc::sync_channel(1);
        jobs.send(Job {
            bytes: wire::encode(&wire::Envelope {
                version: wire::VERSION,
                sequence: 1,
                body: wire::Request::Hello,
            })
            .unwrap(),
            answer,
        })
        .unwrap();
        assert!(matches!(
            reply.recv_timeout(Duration::from_millis(10)),
            Err(mpsc::RecvTimeoutError::Timeout)
        ));
        drop(reply);
        drop(release);
        drop(jobs);
        io.join().unwrap();
    }

    #[test]
    fn deferred_destroy_is_bounded_deduplicated_and_overflow_is_terminal() {
        let queue = Retirements::default();
        for handle in 1..=wire::MAX_HANDLES as u64 {
            queue.insert(handle).unwrap();
        }
        queue.insert(1).unwrap();
        assert!(!queue.overflow.load(Ordering::Acquire));
        assert!(queue.insert(wire::MAX_HANDLES as u64 + 1).is_err());
        assert!(queue.overflow.load(Ordering::Acquire));
        for handle in 1..=wire::MAX_HANDLES as u64 {
            assert_eq!(queue.pop().unwrap(), Some(handle));
        }
        assert_eq!(queue.pop().unwrap(), None);
        // Draining cannot clear the terminal overflow latch.
        assert!(queue.overflow.load(Ordering::Acquire));
    }
}
