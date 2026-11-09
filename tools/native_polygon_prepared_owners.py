"""Recognize exact output owners without broadening raw polygon installers.

Each triple was derived by one specialization of a complete raw owner after
its unchanged body/resolver/dispatcher pins authenticated. These are output
witnesses only: replace_polygon_support/features still reject prepared input.
"""
import hashlib,re

PREPARED={
    'support':{
        'ObservedWorldState':('207092075ad2a3e1a837d4943583b21412cd336c9867b3cf13fa8023b74de551','20de37b96fc56e19b3ec67f5a2e7aedebe84b001436654380330b4704430f0f2','9035cefe73aef945554d109aa7a2256ee0a3f52c1fc0499a416d8c4b9c52ab00'),
        'State':('171098f13a4096d7ed80aeb6e861853f574e44dcf1c9bc4d6afa50a5660983d1','d25f254c2747dfe3b0d2c335ab77c11611a8f55bfb8c540fa69d3b5226a2d74e','232d5a082ea70455fc96d412dffb550cbb789b532b7e28b429fb4fedc8f63577'),
        'WorldState':('acb11deefa1132c51672d82eeabe621b4096579619051d3f1b2e8fc541f7f33c','20de37b96fc56e19b3ec67f5a2e7aedebe84b001436654380330b4704430f0f2','65651152984258f6302a2c8d07a75d4f07d0ecd329a8a66347b82f502b3a6d12'),
    },
    'features':{
        'ObservedWorldState':('9123a413f743a838e220e6a664168b6cd847d8e684c7f1ea9a319851b6b3c7f5','20de37b96fc56e19b3ec67f5a2e7aedebe84b001436654380330b4704430f0f2','9035cefe73aef945554d109aa7a2256ee0a3f52c1fc0499a416d8c4b9c52ab00'),
        'State':('2db24ff300af1f07f1d6ee93c247bae9871a8d607bbbb65566b3e8b4ced4c817','d25f254c2747dfe3b0d2c335ab77c11611a8f55bfb8c540fa69d3b5226a2d74e','232d5a082ea70455fc96d412dffb550cbb789b532b7e28b429fb4fedc8f63577'),
        'WorldState':('f0a0349bb98834beea4c17ee4295813d01cb3083f2144181aceb3f56790bbff6','20de37b96fc56e19b3ec67f5a2e7aedebe84b001436654380330b4704430f0f2','65651152984258f6302a2c8d07a75d4f07d0ecd329a8a66347b82f502b3a6d12'),
    },
}
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def installed(source,binding,family):
    """Validate either an installed strict raw owner or its exact known output."""
    replace=getattr(binding,'replace_polygon_'+family)
    try:
        owner=binding.authenticate(source)
        if not owner or replace(source)!=source:raise ValueError('Incomplete authenticated polygon-'+family+' source prefix')
        return True
    except ValueError as raw_error:
        matches=list(binding.BODY.finditer(source))
        if len(matches)!=1:raise ValueError('Ambiguous prepared polygon-'+family+' owner')from raw_error
        match=matches[0];state=match[1]
        resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
        dispatch=re.search(r'static void dispatch\('+state+r' &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
        if source.count(binding.INCLUDE)!=1 or not resolver or not dispatch or (digest(match[0]),digest(resolver[0]),digest(dispatch[0]))!=PREPARED[family].get(state):
            raise ValueError('Complete prepared polygon-'+family+' body/resolver/dispatcher drift')from raw_error
        return True
