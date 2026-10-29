//! Original party-A press/timer leaves and complete parameter publication.
//! Caller authorization/transport are separate native service boundaries.

/// 19A78E / TU1.5 194FBD, after status/host/getter gates.
pub fn press(seconds: i32) -> i32 {
    if seconds == -1 {
        10
    } else if seconds > 3 {
        seconds - 1
    } else {
        seconds
    }
}

/// 6E360 / TU1.5 71D70: signed elapsed, one decrement, no catch-up.
pub fn tick(seconds: i32, elapsed_ms: u32) -> i32 {
    if seconds > 0 && (elapsed_ms as i32) >= 1000 {
        seconds - 1
    } else {
        seconds
    }
}

/// Complete 5DF30 / TU1.5 5EC20 data owner. The generic requester preserves
/// the original opaque twelve bytes in replays and uses a private account
/// reference in the native transport adapter; neither interpretation is guessed.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Parameter<R> {
    pub active: u8,
    pub seconds: i32,
    pub mode: i32,
    pub watermark: i32,
    pub updates: u32,
    pub requester: R,
}
impl<R: Default + Clone> Default for Parameter<R> {
    fn default() -> Self {
        Self {
            active: 0,
            seconds: 0,
            mode: 0,
            watermark: -1,
            updates: 0,
            requester: R::default(),
        }
    }
}
impl<R: Default + Clone> Parameter<R> {
    fn clear_mode(&mut self) {
        self.mode = 0;
        self.watermark = -1;
        self.requester = R::default();
    }
    pub fn publish(
        &mut self,
        leader: bool,
        start: u8,
        seconds: i32,
        mode: i32,
        requester: &R,
    ) -> bool {
        let active = if leader { start } else { self.active };
        let mut changed = false;
        if self.mode != 0 && self.watermark > seconds {
            self.clear_mode();
            self.updates = self.updates.wrapping_add(1);
            changed = true;
        }
        let preserve_mode = mode != 0 && active != 0 && self.active != 0;
        // The original equality test deliberately does not compare requester.
        if self.active != active || self.seconds != seconds || self.mode != mode {
            self.active = active;
            self.seconds = seconds;
            if preserve_mode {
                self.mode = mode;
                self.watermark = seconds;
                self.requester = if mode == 1 {
                    requester.clone()
                } else {
                    R::default()
                };
            } else {
                self.clear_mode();
            }
            self.updates = self.updates.wrapping_add(1);
            changed = true;
        }
        changed
    }
}
