//! Bandwidth-bounded snapshot replication for the native multiplayer port.
//!
//! Targets the retail Halo 2 per-client channel budget (30720 bits/s start,
//! 71680 bits/s probe ceiling) instead of sending a complete world every
//! tick. See [`stream`] for the protocol and [`bandwidth`] for the budget.

pub mod bandwidth;
pub mod generic;
pub mod pose;
pub mod range;
pub mod stream;
