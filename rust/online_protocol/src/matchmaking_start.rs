//! TU1.5 76481/79690/796B0 timer and 764B4 signed ceiling conversion.
//! Readiness deferral, platform transactions and mode publication are callers.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Timer {
    pub clock: u32,
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Publication {
    pub remaining: i32,
    pub seconds: i32,
    pub ready: bool,
}
impl Timer {
    pub fn clear(&mut self) {
        self.clock = 0;
    }
    pub fn normal(&mut self, now: u32) -> Publication {
        if self.clock == 0 {
            self.clock = now;
        }
        let remaining = (10000u32.wrapping_sub(now.wrapping_sub(self.clock)) as i32).max(0);
        Publication {
            remaining,
            seconds: remaining.wrapping_add(999) / 1000,
            ready: remaining == 0,
        }
    }
}
