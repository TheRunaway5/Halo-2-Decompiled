//! Ordinary connections retain30s. Typed Steam work may use the native75s
//! lookup deadline, within a95s absolute connection bound and5s parser limits.
use std::{
    sync::Mutex,
    time::{Duration, Instant},
};
use tokio::sync::Notify;
pub(super) struct Budget {
    deadline: Mutex<Instant>,
    ceiling: Instant,
    changed: Notify,
}
impl Budget {
    pub fn new() -> Self {
        let now = Instant::now();
        Self {
            deadline: Mutex::new(now + Duration::from_secs(30)),
            ceiling: now + Duration::from_secs(95),
            changed: Notify::new(),
        }
    }
    pub fn steam(&self) {
        if let Ok(mut deadline) = self.deadline.lock() {
            *deadline =
                (*deadline).max((Instant::now() + Duration::from_secs(90)).min(self.ceiling));
            self.changed.notify_one();
        }
    }
    pub async fn expired(&self) {
        loop {
            let changed = self.changed.notified();
            let deadline = self
                .deadline
                .lock()
                .map(|d| *d)
                .unwrap_or_else(|_| Instant::now());
            tokio::select! {
                _=changed=>{},
                _=tokio::time::sleep_until(deadline.into())=>{
                    if self.deadline.lock().is_ok_and(|d|*d>Instant::now()) {continue;}
                    return;
                }
            }
        }
    }
}
