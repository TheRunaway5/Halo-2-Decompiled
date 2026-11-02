"""Stable private identities shared by queue and social sessions."""
from dataclasses import dataclass

from aiohttp import web
from .native_identity import database

VERIFIED_ACCOUNT_NAME = web.RequestKey("verified_account_name", str)
VERIFIED_DISCORD_USERNAME = VERIFIED_ACCOUNT_NAME  # Existing callers retain their API.


@dataclass(frozen=True)
class VerifiedOwner:
    provider: str
    subject: str
    username: str


def initialize(db):
    return database(db,'schema')


def player(db, owner, name, username="", *, now=None):
    return database(db,'resolve',body=dict(owner=owner,screen_name=name,username=username),now=now)


def lookup(db, owner):return database(db,'lookup',body=owner)
def admission_identities(db, owners):return database(db,'admission_identities',body=owners)
def resolve_seats(db, seats):return database(db,'resolve_seats',body=seats)


def allocate(db, owner):
    """Allocate inside the caller's transaction; the trigger serializes the quota."""
    return database(db,'allocate',body=owner)
