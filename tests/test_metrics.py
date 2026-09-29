import json
import pathlib
import sys
import unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'scripts'))
from ppo_metrics import parse
from ppo_config import configuration,validate

class MetricsTests(unittest.TestCase):
    def test_native_metrics_preserved_with_units(self):
        geometry=validate(configuration())
        rows=[{'epoch':i,'agent_steps':i*12288,'loss/policy_loss':-.1,
               'loss/entropy':1.5,'loss/approx_kl':.004,'loss/clipfrac':.1,
               'space/world_decisions':i*4096,'space/active_decisions':i*12000} for i in (1,2)]
        text='compiler output\n'+'\n'.join('SPACE_TRAIN_JSON '+json.dumps(r) for r in rows)
        result=parse(text,geometry)
        self.assertEqual(result[1]['ppo_updates'],2)
        self.assertEqual(result[1]['optimizer_steps'],12)
        self.assertEqual(result[1]['native'],rows[1])
        self.assertEqual(result[1]['world_decisions'],8192)
        self.assertEqual(result[1]['active_decisions'],24000)
        with self.assertRaises(ValueError):parse(text+'\nSPACE_TRAIN_JSON '+json.dumps(rows[1]),geometry)
        with self.assertRaises(ValueError):parse('SPACE_TRAIN_JSON {"epoch":1,"agent_steps":1}',geometry)

if __name__=='__main__': unittest.main()
