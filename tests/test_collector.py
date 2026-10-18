import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]

class CollectorPatchTests(unittest.TestCase):
    def test_cooperative_patch_contract(self):
        path = ROOT / 'patches/pufferl-spaceship.patch'
        self.assertTrue(path.is_file(), 'Cooperative collector patch is missing')
        patch = path.read_text()
        additions = '\n'.join(line[1:] for line in patch.splitlines() if line.startswith('+') and not line.startswith('+++'))
        self.assertIn('space_prepare_bootstrap(pufferl, stream);', additions)
        self.assertIn('p->num_policies == 1', additions)
        self.assertIn('p->vec->policy_layout[1] == p->hypers.total_agents', additions)
        self.assertIn('pufferl->env.rewards.data + dest_off', additions)
        self.assertIn('pufferl->env.terminals.data + dest_off', additions)
        self.assertIn('space_advantage<<<', additions)
        bootstrap = additions.split('static void space_prepare_bootstrap')[1].split('// End Spaceship bootstrap')[0]
        self.assertIn('cudaMemcpyAsync(p->space_bootstrap_state.data, carry.data', bootstrap)
        for forbidden in ['sample_logits', 'puf_step', 'rng_states']:
            self.assertNotIn(forbidden, bootstrap)
        self.assertIn('-        rollouts->rewards.data, -1.0f, 1.0f', patch)

if __name__ == '__main__':
    unittest.main()
