//! Private service-owned candidate state. Handles are never network identities.
use halo2_online_protocol::session_candidates::{self as core, Candidates, Configuration};
use std::collections::HashMap;
use std::sync::{LazyLock, Mutex};

struct Handles {
    next: u64,
    values: HashMap<u64, Candidates>,
}

static HANDLES: LazyLock<Mutex<Handles>> = LazyLock::new(|| {
    Mutex::new(Handles {
        next: 0,
        values: HashMap::new(),
    })
});

// Bounded private allocation, above the service's 1000-ticket capacity. This
// is a modern resource limit, not an original service population rule.
const MAX_HANDLES: usize = 4096;

unsafe fn carrier(input: *const u8, length: usize) -> Option<Vec<u8>> {
    if input.is_null() || length > core::REQUEST_LIMIT {
        return None;
    }
    Some(unsafe { std::slice::from_raw_parts(input, length) }.to_vec())
}

/// # Safety
/// Output is live disjoint storage for capacity bytes, at most REQUEST_LIMIT.
/// Returns the actual JSON length; failure leaves all output unchanged.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn h2_candidates_defaults(output: *mut u8, capacity: usize) -> usize {
    if output.is_null() || capacity > core::REQUEST_LIMIT {
        return 0;
    }
    let data = Configuration::default().as_json().to_string();
    if data.len() > capacity {
        return 0;
    }
    unsafe {
        std::ptr::copy_nonoverlapping(data.as_ptr(), output, data.len());
    }
    data.len()
}

/// # Safety
/// Input is a service-owned JSON carrier with length live bytes, bounded by
/// REQUEST_LIMIT. It contains no network pointers or external authority.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn h2_candidates_configuration_valid(
    input: *const u8,
    length: usize,
) -> bool {
    (unsafe { carrier(input, length) }).is_some_and(|v| core::decode_configuration(&v).is_ok())
}

/// # Safety
/// Input obeys the same bounded owned-carrier contract as above.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn h2_candidates_search_valid(input: *const u8, length: usize) -> bool {
    (unsafe { carrier(input, length) }).is_some_and(|v| core::decode_search(&v).is_ok())
}

/// # Safety
/// The trusted JSON request contains complete search/configuration operands;
/// length is bounded by REQUEST_LIMIT. Zero denotes rejected allocation/input.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn h2_candidates_new(input: *const u8, length: usize) -> u64 {
    if input.is_null() || length > core::REQUEST_LIMIT {
        return 0;
    }
    let Ok((search, config)) = core::request(unsafe { std::slice::from_raw_parts(input, length) })
    else {
        return 0;
    };
    let Ok(value) = Candidates::new(search, config) else {
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
pub extern "C" fn h2_candidates_drop(id: u64) {
    if let Ok(mut handles) = HANDLES.lock() {
        handles.values.remove(&id);
    }
}

/// # Safety
/// The request follows h2_candidates_new's owned-carrier contract. A config
/// change intentionally retains the byte cache; invalid requests change none.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn h2_candidates_configure(id: u64, input: *const u8, length: usize) -> bool {
    if input.is_null() || length > core::REQUEST_LIMIT {
        return false;
    }
    let Ok((search, config)) = core::request(unsafe { std::slice::from_raw_parts(input, length) })
    else {
        return false;
    };
    let Ok(mut handles) = HANDLES.lock() else {
        return false;
    };
    handles
        .values
        .get_mut(&id)
        .is_some_and(|value| value.configure(search, config).is_ok())
}

/// # Safety
/// Output contains exactly BANK_SIZE live owned bytes. No state pointer leaves
/// Rust; unknown/retired handles leave output unchanged.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn h2_candidates_bank(id: u64, output: *mut u8, length: usize) -> bool {
    if output.is_null() || length != core::BANK_SIZE {
        return false;
    }
    let Ok(handles) = HANDLES.lock() else {
        return false;
    };
    let Some(value) = handles.values.get(&id) else {
        return false;
    };
    unsafe {
        std::ptr::copy_nonoverlapping(value.bank().as_ptr(), output, length);
    }
    true
}

/// # Safety
/// Input is exactly BANK_SIZE trusted cache bytes. This restores internal
/// state for a retained original cache projection; it grants no admission.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn h2_candidates_restore_cache(
    id: u64,
    input: *const u8,
    length: usize,
) -> bool {
    if input.is_null() || length != core::BANK_SIZE {
        return false;
    }
    let input = unsafe { std::slice::from_raw_parts(input, length) }.to_vec();
    let Ok(mut handles) = HANDLES.lock() else {
        return false;
    };
    handles
        .values
        .get_mut(&id)
        .is_some_and(|value| value.restore_cache(&input).is_ok())
}

/// # Safety
/// Input and output each contain exactly BANK_SIZE live service-owned bytes.
/// Inputs are copied before state/output mutation. Invalid input changes none.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn h2_candidates_update(
    id: u64,
    input: *const u8,
    length: usize,
    output: *mut u8,
    output_length: usize,
) -> bool {
    if input.is_null()
        || output.is_null()
        || length != core::BANK_SIZE
        || output_length != core::BANK_SIZE
    {
        return false;
    }
    let input = unsafe { std::slice::from_raw_parts(input, length) }.to_vec();
    let Ok(mut handles) = HANDLES.lock() else {
        return false;
    };
    let Some(value) = handles.values.get_mut(&id) else {
        return false;
    };
    let Ok(bank) = value.update(&input) else {
        return false;
    };
    unsafe {
        std::ptr::copy_nonoverlapping(bank.as_ptr(), output, output_length);
    }
    true
}

/// # Safety
/// Output contains STRIDE disjoint live bytes. Returns 0 for a full selected
/// record, 1 for no selection, 2 for invalid input/handle. Failure changes none.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn h2_candidates_select(
    id: u64,
    minimum: i32,
    elapsed: i32,
    active: bool,
    output: *mut u8,
    length: usize,
) -> u32 {
    if output.is_null() || length != core::STRIDE {
        return 2;
    }
    let Ok(handles) = HANDLES.lock() else {
        return 2;
    };
    let Some(value) = handles.values.get(&id) else {
        return 2;
    };
    let Some(row) = value.select(minimum, elapsed, active) else {
        return 1;
    };
    unsafe {
        std::ptr::copy_nonoverlapping(row.as_ptr(), output, length);
    }
    0
}

/// # Safety
/// Configuration is a bounded owned JSON carrier. Output is one live i32;
/// invalid configuration/radius leaves it unchanged.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn h2_candidates_threshold(
    radius: i32,
    maximum: i32,
    input: *const u8,
    length: usize,
    output: *mut i32,
) -> bool {
    if output.is_null() {
        return false;
    }
    let Some(value) = (unsafe { carrier(input, length) }) else {
        return false;
    };
    let Ok(config) = core::decode_configuration(&value) else {
        return false;
    };
    let Ok(result) = core::threshold(radius, maximum, &config) else {
        return false;
    };
    unsafe {
        output.write(result);
    }
    true
}
