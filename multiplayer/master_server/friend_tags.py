"""Typed SQLite IO for Rust public tags; account ownership uses private IDs."""
from .native_friends import database
from .native_matchmaking import value

DIGITS=8


def initialize(db):return database(db,'tag_schema')
def assign(db,who):return database(db,'assign_tag',who)
def tag(db,who):return database(db,'tag',who)
def resolve(db,tag):return database(db,'resolve_tag',body=tag)
def fold(text):return value('friend_fold',value=text)
