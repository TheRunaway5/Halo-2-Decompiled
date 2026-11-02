"""Temporary typed SQLite IO; career aggregates/schema/outbox belong to Rust."""
from .native_profiles import database


def totals(db,player):return database(db,'totals',player)
def initialize(db):return database(db,'schema')
def enqueue(db,player):return database(db,'enqueue',player)
