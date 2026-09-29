import json
import pathlib
import subprocess
import unittest
ROOT = pathlib.Path(__file__).resolve().parents[1]

class EvaluationTests(unittest.TestCase):
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
