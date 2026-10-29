//! Private Rust lifecycle objects. These handles are not client credentials.
use halo2_online_protocol::search_lifecycle::{self as core, Object};
use std::collections::HashMap;
use std::sync::{LazyLock, Mutex};

struct Handles {
    next: u64,
    values: HashMap<u64, Object>,
}
static HANDLES: LazyLock<Mutex<Handles>> = LazyLock::new(|| {
    Mutex::new(Handles {
        next: 0,
        values: HashMap::new(),
    })
});
// Explicit modern private resource bound, above one 1000-ticket service's
// party/clock/jitter population. It is not an original matchmaking limit.
const MAX_HANDLES: usize = 4096;

unsafe fn carrier(input: *const u8, length: usize) -> Option<Vec<u8>> {
    if input.is_null() || length > core::REQUEST_LIMIT {
        return None;
    }
    Some(unsafe { std::slice::from_raw_parts(input, length) }.to_vec())
}

/// # Safety
/// Input is a bounded live service-owned JSON carrier. No network pointer or
/// externally supplied handle is forwarded. Zero means rejected allocation.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn h2_lifecycle_new(input: *const u8, length: usize) -> u64 {
    let Some(data) = (unsafe { carrier(input, length) }) else {
        return 0;
    };
    let Ok(request) = core::decode(&data) else {
        return 0;
    };
    let Ok(value) = Object::new(&request) else {
        return 0;
    };
    let Ok(mut handles) = HANDLES.lock() else {
        return 0;
    };
    if handles.values.len() >= MAX_HANDLES {
        return 0;
    }
    let Some(id) = handles.next.checked_add(1) else {
        return 0;
    };
    handles.next = id;
    handles.values.insert(id, value);
    id
}

#[unsafe(no_mangle)]
pub extern "C" fn h2_lifecycle_drop(id: u64) {
    if let Ok(mut handles) = HANDLES.lock() {
        handles.values.remove(&id);
    }
}

/// # Safety
/// Input has length owned live bytes; output has capacity disjoint owned live
/// bytes. Both are bounded. Failure, including insufficient response capacity,
/// leaves state and output unchanged. Returns the exact successful JSON size.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn h2_lifecycle_command(
    id: u64,
    input: *const u8,
    length: usize,
    output: *mut u8,
    capacity: usize,
) -> usize {
    if output.is_null() || capacity > core::RESPONSE_LIMIT {
        return 0;
    }
    let Some(data) = (unsafe { carrier(input, length) }) else {
        return 0;
    };
    let Ok(request) = core::decode(&data) else {
        return 0;
    };
    let Ok(mut handles) = HANDLES.lock() else {
        return 0;
    };
    let Some(current) = handles.values.get_mut(&id) else {
        return 0;
    };
    let Ok((next, result)) = current.command(&request) else {
        return 0;
    };
    let response = result.to_string();
    if response.len() > capacity {
        return 0;
    }
    *current = next;
    unsafe {
        std::ptr::copy_nonoverlapping(response.as_ptr(), output, response.len());
    }
    response.len()
}

/// # Safety
/// This stateless typed carrier follows the same ownership and capacity
/// contract as h2_lifecycle_command. Failure leaves output unchanged.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn h2_lifecycle_value(
    input: *const u8,
    length: usize,
    output: *mut u8,
    capacity: usize,
) -> usize {
    if output.is_null() || capacity > core::RESPONSE_LIMIT {
        return 0;
    }
    let Some(data) = (unsafe { carrier(input, length) }) else {
        return 0;
    };
    let Ok(request) = core::decode(&data) else {
        return 0;
    };
    let Ok(value) = core::value(&request) else {
        return 0;
    };
    let response = value.to_string();
    if response.len() > capacity {
        return 0;
    }
    unsafe {
        std::ptr::copy_nonoverlapping(response.as_ptr(), output, response.len());
    }
    response.len()
}
