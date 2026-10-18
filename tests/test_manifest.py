import pathlib
import sys
import tempfile
import unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'scripts'))
from run_manifest import manifest
from ppo_config import configuration

class ManifestTests(unittest.TestCase):
    def test_identity_units_and_checkpoint(self):
        with tempfile.TemporaryDirectory() as directory:
            checkpoint=pathlib.Path(directory)/'policy.bin'; checkpoint.write_bytes(b'weights')
            counters={'physics_ticks':8,'world_decisions':2,'agent_slots':6,'active_decisions':5,
                      'ppo_updates':0,'optimizer_steps':0,'wall_seconds':.1}
            a=manifest(configuration(),checkpoint,'untrained',counters)
            b=manifest(configuration(),checkpoint,'untrained',counters)
            self.assertEqual(a['source'],b['source'])
            self.assertEqual(a['configuration_sha256'],b['configuration_sha256'])
            self.assertEqual(len(a['checkpoint']['sha256']),64)
            self.assertEqual(a['counters'],counters)
            self.assertEqual(a['checkpoint']['kind'],'untrained')
            self.assertEqual(a['upstream']['external_only'],True)
            changed=configuration(ships=2)
            self.assertNotEqual(a['configuration_sha256'],manifest(changed)['configuration_sha256'])
    def test_missing_counters_are_unknown(self):
        a=manifest(configuration())
        self.assertIsNone(a['counters']['active_decisions'])
        self.assertIsNone(a['checkpoint'])
        with self.assertRaises(ValueError): manifest(configuration(),counters={'ppo_updates':-1})

if __name__=='__main__': unittest.main()
