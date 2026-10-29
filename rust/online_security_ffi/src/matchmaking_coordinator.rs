//! Trusted IO bridge to the Rust matchmaking owner. No network pointers.
use halo2_online_protocol::matchmaking_continuity::Continuity;
use halo2_online_protocol::matchmaking_coordinator::{self as core, Coordinator};
use halo2_online_protocol::matchmaking_model::{Error, Result};
use halo2_online_protocol::matchmaking_packet_budget::Budget;
use halo2_online_protocol::matchmaking_party_directory::Directory;
use halo2_online_protocol::matchmaking_persistence::{Database, DatabaseBank};
use std::collections::HashMap;
use std::sync::{
    LazyLock, Mutex,
    atomic::{AtomicUsize, Ordering},
};

struct Owners {
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
        }
    }
}
fn open(request: &core::WireValue) -> Result<u64> {
    let mut owners = OWNERS
        .lock()
        .map_err(|_| Error(503, "Rust matchmaking owner unavailable"))?;
    if owners.values.len() >= 4096 {
        return Err(Error(503, "Native owner capacity reached"));
    }
    let owner = if let Some(directory) = request.get("party_directory") {
        let directory = Directory::new(directory)?;
        if let Some(source) = directory.source() {
            if !matches!(owners.values.get(&source), Some(Owner::Queue(_))) {
                return Err(Error(503, "Party source owner retired"));
            }
        }
        Owner::PartyDirectory(Box::new(directory))
    } else if let Some(database) = request.get("database") {
        if database["mode"] == "snapshot_bank" {
            Owner::Bank(Box::new(DatabaseBank::new(database)?))
        } else if database["mode"] == "result_commit" {
            use halo2_online_protocol::matchmaking_model as m;
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
            use halo2_online_protocol::matchmaking_model as m;
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
        halo2_online_protocol::matchmaking_model::fields(request, &["packet_budget"])?;
        halo2_online_protocol::matchmaking_model::fields(budget, &[])?;
        Owner::Budget(Box::default())
    } else if let Some(continuity) = request.get("continuity") {
        halo2_online_protocol::matchmaking_model::fields(request, &["continuity"])?;
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
static OWNERS: LazyLock<Mutex<Owners>> = LazyLock::new(|| {
    Mutex::new(Owners {
        next: 0,
        values: HashMap::new(),
    })
});
static BUFFERS: AtomicUsize = AtomicUsize::new(0);

fn execute(owners: &mut Owners, id: u64, request: &core::WireValue) -> Result<core::WireValue> {
    let source = match owners.values.get(&id) {
        Some(Owner::PartyDirectory(directory)) => directory.source(),
        Some(_) => return owners.values.get_mut(&id).unwrap().execute(request),
        None => return Err(Error(503, "Rust matchmaking owner retired")),
    };
    if let Some(source) = source {
        if !matches!(owners.values.get(&source), Some(Owner::Queue(_))) {
            return Err(Error(503, "Party source owner retired"));
        }
    }
    let mut event = owners.values.get_mut(&id).unwrap().execute(request)?;
    for _ in 0..1024 {
        if event["kind"] != "core" {
            return Ok(event);
        }
        let source = source.ok_or(Error(503, "Party source owner required"))?;
        let Some(Owner::Queue(queue)) = owners.values.get_mut(&source) else {
            return Err(Error(503, "Party source owner retired"));
        };
        let result = queue.execute(&event["request"])?;
        event =
            owners.values.get_mut(&id).unwrap().execute(
                &serde_json::json!({"op":"reply","frame":event["frame"],"value":result}),
            )?;
    }
    Err(Error(503, "Party owner continuation exceeds bounds"))
}

unsafe fn carrier(input: *const u8, length: usize) -> Option<Vec<u8>> {
    if input.is_null() || length > core::REQUEST_LIMIT {
        return None;
    }
    Some(unsafe { std::slice::from_raw_parts(input, length) }.to_vec())
}
unsafe fn reply(result: Result<core::WireValue>, length: *mut usize) -> *mut u8 {
    if length.is_null() {
        return std::ptr::null_mut();
    }
    let response = core::envelope(result).to_string();
    if response.len() > core::RESPONSE_LIMIT {
        return std::ptr::null_mut();
    }
    let data = response.into_bytes().into_boxed_slice();
    unsafe {
        length.write(data.len());
    }
    BUFFERS.fetch_add(1, Ordering::Relaxed);
    Box::into_raw(data) as *mut u8
}

/// # Safety
/// Input has exactly length live service-owned bytes. Zero means rejected
/// setup. Identity facts and profiles must be admitted by the native service.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn h2_matchmaking_owner_new(input: *const u8, length: usize) -> u64 {
    let Some(data) = (unsafe { carrier(input, length) }) else {
        return 0;
    };
    let Ok(request) = core::decode(&data) else {
        return 0;
    };
    open(&request).unwrap_or(0)
}

/// # Safety
/// The bounded constructor and returned buffer follow the owner request ABI.
/// Database result sources must be existing private Rust queue handles.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn h2_matchmaking_object_open(
    input: *const u8,
    length: usize,
    output_length: *mut usize,
) -> *mut u8 {
    let Some(data) = (unsafe { carrier(input, length) }) else {
        return std::ptr::null_mut();
    };
    let result = core::decode(&data)
        .and_then(|r| open(&r))
        .map(|id| core::WireValue::from(id));
    unsafe { reply(result, output_length) }
}
#[unsafe(no_mangle)]
pub extern "C" fn h2_matchmaking_owner_drop(id: u64) {
    if let Ok(mut owners) = OWNERS.lock() {
        owners.values.remove(&id);
    }
}

/// # Safety
/// Input is a bounded owned carrier, output_length is one live usize. A
/// successful pointer owns exactly the reported bytes; copy it then release
/// through h2_matchmaking_buffer_free. No pointer/length is accepted from HTTP.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn h2_matchmaking_owner_request(
    id: u64,
    input: *const u8,
    length: usize,
    output_length: *mut usize,
) -> *mut u8 {
    if output_length.is_null() {
        return std::ptr::null_mut();
    }
    let Some(data) = (unsafe { carrier(input, length) }) else {
        return std::ptr::null_mut();
    };
    let result = core::decode(&data).and_then(|request| {
        let mut owners = OWNERS
            .lock()
            .map_err(|_| Error(503, "Rust matchmaking owner unavailable"))?;
        execute(&mut owners, id, &request)
    });
    unsafe { reply(result, output_length) }
}

/// # Safety
/// Bounded typed model/value request; output ownership follows the owner API.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn h2_matchmaking_value(
    input: *const u8,
    length: usize,
    output_length: *mut usize,
) -> *mut u8 {
    if output_length.is_null() {
        return std::ptr::null_mut();
    }
    let Some(data) = (unsafe { carrier(input, length) }) else {
        return std::ptr::null_mut();
    };
    unsafe {
        reply(
            core::decode(&data).and_then(|r| core::value(&r)),
            output_length,
        )
    }
}

/// # Safety
/// Pointer and length must be the exact, not-yet-freed pair returned by this
/// module. Neither value may originate in a network request or a Python client.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn h2_matchmaking_buffer_free(pointer: *mut u8, length: usize) {
    if pointer.is_null() || length > core::RESPONSE_LIMIT {
        return;
    }
    unsafe {
        drop(Box::from_raw(std::ptr::slice_from_raw_parts_mut(
            pointer, length,
        )));
    }
    BUFFERS.fetch_sub(1, Ordering::Relaxed);
}
#[unsafe(no_mangle)]
pub extern "C" fn h2_matchmaking_buffers_live() -> usize {
    BUFFERS.load(Ordering::Relaxed)
}

#[unsafe(no_mangle)]
pub extern "C" fn h2_matchmaking_test_inputs_enabled() -> bool {
    cfg!(feature = "matchmaking-test-inputs")
}
