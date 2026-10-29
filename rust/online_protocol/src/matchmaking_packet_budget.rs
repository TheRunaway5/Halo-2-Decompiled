//! Bounded native per-stream abuse policy, not recovered Xbox coefficients.
use crate::matchmaking_model::{self as m, Error, Result};
use serde_json::{Value, json};
use std::collections::BTreeMap;
#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
pub enum Stream {
    Control,
    Game(u8),
    Audio(u8, u8),
}
#[derive(Default)]
pub struct Budget {
    streams: BTreeMap<Stream, (f64, f64)>,
}
impl Budget {
    pub fn take(&mut self, key: Stream, now: f64) -> Result<()> {
        m::check(now.is_finite() && now >= 0., "Invalid packet budget clock")?;
        if let Stream::Game(target) = key {
            m::check(target < 16, "Invalid game budget stream")?;
        }
        if let Stream::Audio(a, b) = key {
            m::check(a < 16 && b < 16, "Invalid audio budget stream")?;
        }
        let (rate, burst) = if matches!(key, Stream::Audio(..)) {
            (75., 75.)
        } else {
            (1000., 512.)
        };
        let (credits, previous) = self.streams.get(&key).copied().unwrap_or((burst, now));
        let now = now.max(previous);
        let credits = burst.min(credits + (now - previous) * rate);
        if credits < 1. {
            return Err(Error(429, "Match packet rate exceeded"));
        }
        self.streams.insert(key, (credits - 1., now));
        Ok(())
    }
    pub fn execute(&mut self, r: &Value) -> Result<Value> {
        m::check(r["op"] == "take", "Unknown native budget operation")?;
        let key = if !r["audio"].is_null() {
            let v = r["audio"]
                .as_array()
                .ok_or(Error(400, "Invalid audio budget stream"))?;
            m::check(v.len() == 2, "Invalid audio budget stream")?;
            Stream::Audio(
                m::integer(&v[0], 0, 15)? as u8,
                m::integer(&v[1], 0, 15)? as u8,
            )
        } else if !r["game"].is_null() {
            Stream::Game(m::integer(&r["game"], 0, 15)? as u8)
        } else {
            Stream::Control
        };
        self.take(key, m::real(&r["now"])?)?;
        Ok(json!(null))
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn untrusted_stream_indices_and_clock_cannot_allocate_or_refill_a_budget() {
        let mut b = Budget::default();
        for now in [f64::NAN, f64::INFINITY, -1.] {
            assert!(b.take(Stream::Control, now).is_err());
        }
        for stream in [Stream::Game(16), Stream::Audio(16, 0), Stream::Audio(0, 16)] {
            assert!(b.take(stream, 0.).is_err());
        }
        assert!(b.streams.is_empty());
        for _ in 0..75 {
            b.take(Stream::Audio(0, 1), 100.).unwrap();
        }
        assert_eq!(b.take(Stream::Audio(0, 1), 99.).unwrap_err().0, 429);
        assert_eq!(b.take(Stream::Audio(0, 1), 100.).unwrap_err().0, 429);
        b.take(Stream::Audio(0, 1), 100.02).unwrap();
        assert_eq!(b.take(Stream::Audio(0, 1), 100.02).unwrap_err().0, 429);
        b.take(Stream::Audio(0, 2), 100.).unwrap();
        assert_eq!(b.streams.len(), 2);
    }
}
