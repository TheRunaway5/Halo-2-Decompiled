//! Bounded C ABI; the shared safe Rust registry owns all policy and state.
use halo2_online_protocol::matchmaking_coordinator as core;
use halo2_online_protocol::matchmaking_model::{Error, Result};
use halo2_online_protocol::matchmaking_owners::Owners;
use std::sync::{
    LazyLock, Mutex,
    atomic::{AtomicUsize, Ordering},
};
static OWNERS: LazyLock<Mutex<Owners>> = LazyLock::new(|| Mutex::new(Owners::new()));
static BUFFERS: AtomicUsize = AtomicUsize::new(0);
fn open(request: &core::WireValue) -> Result<u64> {
    OWNERS
        .lock()
        .map_err(|_| Error(503, "Rust matchmaking owner unavailable"))?
        .open(request)
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
        owners.retire(id);
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
        owners.request(id, &request)
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
