import copy
import pathlib
import sys
import unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'scripts'))
from gpu_diagnostic import diagnostic_plan, verify_receipt

class GPUPlanTests(unittest.TestCase):
    def test_all_teams_and_boundary_cases_required(self):
        plan=diagnostic_plan()
        self.assertEqual(len(plan),32)
        self.assertEqual({c['ships'] for c in plan},set(range(1,9)))
        self.assertEqual({c['period'] for c in plan},{1,63,64,65})
        receipt={'binary_sha256':'binary','source_sha256':'source','executed_on':'NVIDIA CUDA',
                 'cases':[dict(c,passed=True,bootstrap_preserved=True,action_logprob_checked=True,
                    optimizer_changed=True,checkpoint_roundtrip=True,checked_targets=100) for c in plan]}
        verify_receipt(receipt,'binary','source')
        for mutate in [lambda r:r['cases'].pop(), lambda r:r['cases'][0].update(passed=False),
                       lambda r:r.update(binary_sha256='old'),lambda r:r.update(executed_on='CPU')]:
            bad=copy.deepcopy(receipt);mutate(bad)
            with self.assertRaises(ValueError):verify_receipt(bad,'binary','source')

if __name__=='__main__': unittest.main()
