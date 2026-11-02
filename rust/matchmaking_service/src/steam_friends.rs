//! Typed IO for the existing Rust relationship coordinator and sealed proofs.
use super::*;
use std::sync::atomic::{AtomicBool, Ordering};

pub(crate) enum Begin {
    Done(Value),
    Work {
        id: u64,
        event: Value,
        features: Value,
    },
}
impl Runtime {
    pub(crate) fn steam_friends_begin(
        &mut self,
        header: &str,
        source: &str,
        body: Option<&Value>,
        features: &Value,
        credential: &str,
    ) -> Result<Begin> {
        if body.is_none() {
            let clock = self.clock()?;
            self.owners.request(
                self.steam_friends,
                &json!({"op":"rate","now":clock.monotonic,"source":source}),
            )?;
        }
        let auth = self.session(header)?;
        let clock = self.clock()?;
        let mut flags = Value::Null;
        if let Some(body) = body {
            let setup = self.config(
                "party",
                "command_probe",
                auth["player"].as_str(),
                auth["key"].as_str(),
                body.clone(),
                features.clone(),
                &clock,
            );
            let probe = self.plan(setup)?;
            flags = probe["features"].clone();
            if probe["route"] == "view" {
                return Ok(Begin::Done(self.party(
                    header,
                    "command",
                    body.clone(),
                    features.clone(),
                )?));
            }
            if probe["route"] != "steam" {
                return Err(Error(400, "Steam invitation command required"));
            }
        }
        let id=self.owners.open(&json!({"steam_request":{"cache_source":self.steam_friends,"account_source":self.accounts,
            "account_now":clock.monotonic,"player":auth["player"],"key":auth["key"],"mode":if body.is_some(){"invite"}else{"view"},
            "body":body,"now":clock.monotonic,"credential":credential,"transaction_owned":self.db.is_autocommit()}}))?;
        match self.steam_friends_step(id, json!({"op":"begin"}), credential) {
            Ok(event) => Ok(Begin::Work {
                id,
                event,
                features: flags,
            }),
            Err(error) => {
                self.owners.retire(id);
                Err(error)
            }
        }
    }
    pub(crate) fn steam_friends_step(
        &mut self,
        id: u64,
        mut reply: Value,
        credential: &str,
    ) -> Result<Value> {
        let clock = self.clock()?;
        reply["now"] = json!(clock.monotonic);
        reply["account_now"] = json!(clock.monotonic);
        reply["credential"] = json!(credential);
        self.owners.request(id, &reply)
    }
    pub(crate) fn steam_friends_sql(&mut self, event: &Value) -> Result<Value> {
        let tx = self
            .db
            .transaction_with_behavior(TransactionBehavior::Deferred)
            .map_err(database::failure)?;
        let statements = event["statements"]
            .as_array()
            .ok_or(Error(503, "Native Steam SQL required"))?;
        if statements.len() > 9 {
            return Err(Error(503, "Too many Steam SQL banks"));
        }
        let mut banks = Vec::with_capacity(statements.len());
        for statement in statements {
            let value = database::sql(
                &tx,
                &json!({"mode":"query","frame":event["frame"],"statements":[statement]}),
            )?;
            banks.push(value["rows"].clone());
        }
        tx.commit().map_err(database::failure)?;
        Ok(json!({"op":"reply","frame":event["frame"],"rows":banks,"committed":true}))
    }
    pub(crate) fn steam_friends_presence(&mut self) -> Result<Value> {
        let clock = self.clock()?;
        let rooms = self.room_sockets.observations(self.started);
        self.owners.request(self.accounts,&json!({"op":"presence","view":"observed","source":self.queue,
            "now":clock.monotonic,"queue_now":clock.monotonic,"wall":clock.wall,"rooms":rooms.facts()}))
    }
    pub(crate) fn steam_friends_retire(&mut self, id: u64) {
        self.owners.retire(id);
    }
    pub(crate) fn steam_friends_invite(
        &mut self,
        header: &str,
        body: Value,
        features: Value,
        claim: Value,
        credential: &str,
        alive: &AtomicBool,
    ) -> Result<Value> {
        if !alive.load(Ordering::Acquire) {
            return Err(Error(503, "Steam request cancelled"));
        }
        let auth = self.session(header)?;
        let clock = self.clock()?;
        let mut setup = self.config(
            "party",
            "command",
            auth["player"].as_str(),
            auth["key"].as_str(),
            body.clone(),
            features.clone(),
            &clock,
        );
        setup["party_directory"]["steam_source"] = json!(self.steam_friends);
        setup["party_directory"]["steam_now"] = json!(clock.monotonic);
        setup["party_directory"]["steam_credential"] = json!(credential);
        setup["party_directory"]["steam"] = claim;
        let view = self.config(
            "party",
            "view",
            auth["player"].as_str(),
            auth["key"].as_str(),
            Value::Null,
            features,
            &clock,
        );
        self.plan_reply(setup, Some(view), Some(alive))
    }
}
