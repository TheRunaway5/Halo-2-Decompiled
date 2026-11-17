from pathlib import Path
import sys
import unittest

ROOT=Path(__file__).resolve().parents[1];sys.path.insert(0,str(ROOT/'tools'))
import native_diagnostic_logger_world_replacements as binder

class GetterWorldBindings(unittest.TestCase):
    @classmethod
    def setUpClass(cls):cls.source=(ROOT/'src/physics/physics_live_world_tu15.cpp').read_text()
    def rejected(self,source):
        with self.assertRaises(ValueError):binder.replace_diagnostic_logger_world(source)
    def test_full_owner_reconstruction(self):
        bound=binder.replace_diagnostic_logger_world(self.source)
        self.assertEqual(bound,binder.replace_diagnostic_logger_world(bound))
        original=self.source.replace(binder.INCLUDE,'',1).replace(binder.PREFIX,'',1)
        self.assertEqual(bound.replace(binder.INCLUDE,'',1).replace(binder.PREFIX,'',1),original)
    def test_complete_original_method_pin(self):
        body=binder.function(self.source,binder.METHOD)
        self.rejected(self.source.replace(body,body.replace('0xc1270u','0xc1271u',1),1))
    def test_exact_d3950_sibling_and_legacy_method(self):
        self.assertEqual(self.source.count(binder.D3950_ROUTE),1)
        original=self.source.replace(binder.D3950_ROUTE,'',1)
        self.assertEqual(binder.digest(binder.canonical_method(binder.function(original,binder.METHOD))),
                         binder.METHOD_PIN)
        current=binder.replace_diagnostic_logger_world(self.source)
        legacy=binder.replace_diagnostic_logger_world(original)
        self.assertEqual(current.replace(binder.D3950_ROUTE,'',1),legacy)
        self.rejected(current.replace(binder.D3950_ROUTE,
            binder.D3950_ROUTE.replace('source_run_ai_frame_tu15','source_run_ai_trace_tu15'),1))
    def test_bank_gate_cannot_move(self):
        bound=binder.replace_diagnostic_logger_world(self.source)
        self.rejected(bound.replace(binder.ANCHOR+binder.PREFIX,binder.PREFIX+'\n'+binder.ANCHOR,1))
    def test_target_and_predicate_drift(self):
        bound=binder.replace_diagnostic_logger_world(self.source)
        self.rejected(bound.replace(binder.PREFIX,binder.PREFIX.replace('0x2e16d0u','0x2e16d1u'),1))
        self.rejected(bound.replace(binder.PREFIX,binder.PREFIX.replace('&&','||'),1))
    def test_duplicate_overlays(self):
        bound=binder.replace_diagnostic_logger_world(self.source)
        self.rejected(bound.replace(binder.PREFIX,binder.PREFIX*2,1))
        self.rejected(binder.INCLUDE+bound)
    def test_actual_owner_declaration_drift(self):
        self.rejected(self.source.replace('struct PhysicsLiveWorldTu15::Impl {','struct AnotherWorld::Impl {',1))
    def test_no_missing_full_method(self):
        self.rejected(self.source.replace(binder.function(self.source,binder.METHOD),'',1))

if __name__=='__main__':unittest.main()
