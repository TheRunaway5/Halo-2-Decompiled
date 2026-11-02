//! Canonical EOS identity binding. Only privately verified worker receipts and
//! still-current account/room capabilities reach the shared Rust SQL policy.
use super::*;
use crate::eos_identity::{Identity as Issuer, Verified};
use halo2_online_protocol::matchmaking_model as m;

#[derive(Clone)]
enum Kind {
    Account(Value),
    Room { id: String, admission: Value },
}
#[derive(Clone)]
pub struct EosOwner {
    kind: Kind,
    player: String,
    name: String,
}
impl EosOwner {
    pub fn player(&self) -> &str {
        &self.player
    }
    pub fn is_room(&self) -> bool {
        matches!(self.kind, Kind::Room { .. })
    }
}
fn recheck(
    owners: &mut Owners,
    accounts: u64,
    source: Option<u64>,
    sockets: &room_io::Sockets,
    clock: &Clock,
    header: &str,
    owner: &EosOwner,
) -> Result<()> {
    let current = match &owner.kind {
        Kind::Account(previous) => {
            let actual=owners.request(accounts,&json!({"op":"auth","now":clock.monotonic,"rate_now":clock.monotonic,"header":header,"rate":false}))?;
            actual == *previous
        }
        Kind::Room { id, admission } => {
            let source = source.ok_or(Error(503, "Room source unavailable"))?;
            if rooms::request(
                owners,
                source,
                sockets,
                clock,
                json!({"op":"room_service_exists","id":id}),
            )? != true
            {
                return Err(Error(404, "Match no longer exists"));
            }
            let actual = rooms::request(
                owners,
                source,
                sockets,
                clock,
                json!({"op":"room_wire_authorize","id":id,"route":"eos","authorization":header,"prior":admission}),
            )?;
            actual == *admission
        }
    };
    if !current {
        return Err(Error(401, "Account or room capability changed"));
    }
    Ok(())
}
impl Runtime {
    pub fn eos_recheck(&mut self, header: &str, owner: &EosOwner) -> Result<()> {
        self.room_maintenance()?;
        let clock = self.clock()?;
        recheck(
            &mut self.owners,
            self.accounts,
            self.queue,
            &self.room_sockets,
            &clock,
            header,
            owner,
        )
    }
    pub fn eos_owner(&mut self, header: &str, room: Option<&str>) -> Result<EosOwner> {
        if let Some(id) = room {
            let admission = self.room_authorize(id, "eos", header)?;
            Ok(EosOwner {
                player: m::text(&admission["player"], 64)?.into(),
                name: m::text(&admission["name"], 64)?.into(),
                kind: Kind::Room {
                    id: id.into(),
                    admission,
                },
            })
        } else {
            let auth = self.session(header)?;
            let player = m::text(&auth["player"], 64)?.to_owned();
            let name = self
                .db
                .query_row(
                    "SELECT screen_name FROM player_profiles WHERE player=?",
                    [&player],
                    |r| r.get::<_, String>(0),
                )
                .map_err(database::failure)?;
            Ok(EosOwner {
                kind: Kind::Account(auth),
                player,
                name,
            })
        }
    }
    pub fn eos_token(&mut self, header: &str, owner: &EosOwner, issuer: &Issuer) -> Result<Value> {
        self.room_maintenance()?;
        let clock = self.clock()?;
        recheck(
            &mut self.owners,
            self.accounts,
            self.queue,
            &self.room_sockets,
            &clock,
            header,
            owner,
        )?;
        issuer.token(&owner.player, &owner.name, clock.wall)
    }
    pub fn eos_bind(
        &mut self,
        header: &str,
        owner: &EosOwner,
        verified: &Verified,
    ) -> Result<Value> {
        self.room_maintenance()?;
        let clock = self.clock()?;
        recheck(
            &mut self.owners,
            self.accounts,
            self.queue,
            &self.room_sockets,
            &clock,
            header,
            owner,
        )?;
        if verified.account() != owner.player {
            return Err(Error(403, "EOS identity belongs to another account"));
        }
        let account_key = match &owner.kind {
            Kind::Account(auth) => auth["key"].as_str(),
            Kind::Room { .. } => None,
        };
        let mut setup = self.config(
            "profile",
            "eos_bind",
            Some(&owner.player),
            account_key,
            json!({"account":verified.account(),"puid":verified.puid()}),
            Value::Null,
            &clock,
        );
        let accounts = self.accounts;
        let source = self.queue;
        let sockets = &self.room_sockets;
        let started = self.started;
        let observations = sockets.observations(started);
        let sources = self.sources(&observations);
        let mut pending = None;
        let mut cloud_pending = None;
        let mut cloud_setup = self.config("friends", "cloud_enqueue", Some(&owner.player), None, Value::Null, Value::Null, &clock);
        let cloud_enabled = self.friend_cloud_enabled;
        let result = database::transaction(&mut self.owners, &mut self.db, source, |owners, db| {
            // BEGIN IMMEDIATE may have waited on another connection. Re-read
            // time and the actual capability after acquiring SQL ownership.
            let fresh = Clock {
                monotonic: started.elapsed().as_secs_f64(),
                wall: database::wall_now()?,
            };
            recheck(owners, accounts, source, sockets, &fresh, header, owner)?;
            setup["party_directory"]["clock"] = json!(fresh.monotonic);
            setup["party_directory"]["wall"] = json!(fresh.wall);
            if account_key.is_some() {
                setup["party_directory"]["account_now"] = json!(fresh.monotonic);
            }
            let proof = |op: &str| match &owner.kind {
                Kind::Room { id, admission } => Some(
                    json!({"op":op,"id":id,"route":"eos","authorization":header,"prior":admission,"account":verified.account(),"puid":verified.puid()}),
                ),
                Kind::Account(_) => None,
            };
            if let Some(proof) = proof("room_wire_proof_validate") {
                rooms::request(owners, source.unwrap(), sockets, &fresh, proof)?;
            }
            let (id, value) = database::drive(owners, db, setup.clone(), &fresh, sources)?;
            pending = Some(id);
            if cloud_enabled {
                let (id,_) = database::drive(owners,db,
                    json!({"database":{"mode":"history_identity","player":owner.player}}),&fresh,sources)?;
                owners.retire(id);
                cloud_setup["party_directory"]["clock"]=json!(fresh.monotonic);
                cloud_setup["party_directory"]["wall"]=json!(fresh.wall);
                cloud_setup["party_directory"]["friend_now"]=json!(fresh.monotonic);
                let (id,_) = database::drive(owners,db,cloud_setup.clone(),&fresh,sources)?;
                cloud_pending=Some(id);
            }
            if let Some(proof) = proof("room_wire_proof_commit") {
                let current = Clock {
                    monotonic: started.elapsed().as_secs_f64(),
                    wall: database::wall_now()?,
                };
                rooms::request(owners, source.unwrap(), sockets, &current, proof)?;
            }
            Ok(value)
        });
        let committed_clock = Clock {
            monotonic: self.started.elapsed().as_secs_f64(),
            wall: clock.wall,
        };
        let acknowledgement = (|| {
            if result.is_ok() {
                database::committed(&mut self.owners, pending.unwrap(), &setup, &committed_clock)?;
                if let Some(id)=cloud_pending {database::committed(&mut self.owners,id,&cloud_setup,&committed_clock)?;}
            }
            Ok(())
        })();
        if let Some(id) = pending {
            self.owners.retire(id);
        }
        if let Some(id)=cloud_pending {self.owners.retire(id);}
        let value = result?;
        acknowledgement?;
        if cloud_enabled {self.friend_cloud_wake.notify_one();}
        Ok(if matches!(owner.kind, Kind::Room { .. }) {
            json!({"eos_product_user_id":verified.puid()})
        } else {
            value
        })
    }
}
