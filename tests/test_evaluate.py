import json
import pathlib
import subprocess
import unittest
import sys
import tempfile
from unittest import mock
ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'scripts'))
import evaluate

class EvaluationTests(unittest.TestCase):
    def test_rules_identify_header_and_executable(self):
        with tempfile.TemporaryDirectory() as directory:
            root=pathlib.Path(directory)
            (root/'native').mkdir()
            (root/'native/spaceship_core.c').write_text('physics')
            header=root/'native/spaceship_core.h'; header.write_text('capacity=64')
            binary=root/'evaluate'; binary.write_bytes(b'build-one')
            with mock.patch.object(evaluate,'ROOT',root):
                first=evaluate.evaluation_identity(binary)
                header.write_text('capacity=32')
                second=evaluate.evaluation_identity(binary)
                self.assertNotEqual(first['source_files'],second['source_files'])
                binary.write_bytes(b'build-two')
                third=evaluate.evaluation_identity(binary)
                self.assertNotEqual(second['binary_sha256'],third['binary_sha256'])
                self.assertEqual(evaluate.evaluation_identity(binary),third)
    def test_report_fingerprint_uses_build_identity(self):
        first=evaluate.report(wave_size=1)
        changed=dict(first['evaluation_build'],binary_sha256='different-build')
        with mock.patch.object(evaluate,'evaluation_identity',return_value=changed):
            second=evaluate.report(wave_size=1)
        self.assertNotEqual(first['rules_sha256'],second['rules_sha256'])
        self.assertEqual(first['episodes'],second['episodes'])
    def episode(self, policy):
        result = subprocess.run([str(ROOT/'build/evaluate'), policy, '3', '1000', '7'],
                                capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        return json.loads(result.stdout)
    def test_reproducible_conserved_metrics(self):
        for policy in ['random', 'greedy', 'lanes']:
            a, b = self.episode(policy), self.episode(policy)
            self.assertEqual(a,b)
            self.assertEqual(a['scheduled'], 7)
            self.assertEqual(sum(a[x] for x in ['killed','escaped','unresolved','unspawned']),7)
            self.assertEqual(a['agent_slots'], a['world_decisions']*3)
            self.assertLessEqual(a['active_decisions'],a['agent_slots'])
            self.assertAlmostEqual(a['failure_fraction'],1-a['killed']/7,places=6)
            r = a['reward_components']
            self.assertAlmostEqual(a['episode_return'],sum(r.values()),places=4)
            self.assertEqual(len(a['ships']),3)
            self.assertEqual(sum(s['kills'] for s in a['ships']), a['killed'])

if __name__ == '__main__': unittest.main()
