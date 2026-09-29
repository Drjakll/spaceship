import copy
import sys
import pathlib
import unittest
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1] / 'scripts'))
from ppo_config import configuration, validate

class ConfigTests(unittest.TestCase):
    def test_every_team_size(self):
        for ships in range(1, 9):
            c = configuration(ships=ships, updates=4)
            validate(c)
            agents, horizon = c['vec']['total_agents'], c['train']['horizon']
            self.assertEqual(agents % ships, 0)
            self.assertEqual(c['train']['total_timesteps'], agents * horizon * 4)
            self.assertEqual(c['vec']['num_policies'], 1)
            self.assertEqual(c['selfplay']['enabled'], 0)
            self.assertEqual(c['base']['async'], 0)
    def test_reject_incompatible_or_unbounded(self):
        c = configuration()
        for section, key, value in [('selfplay','enabled',1), ('vec','num_policies',2),
            ('vec','total_agents',193), ('train','total_timesteps',0),
            ('base','async',1), ('train','vtrace',1), ('base','reset_every_horizon',1)]:
            bad = copy.deepcopy(c); bad[section][key] = value
            with self.assertRaises(ValueError): validate(bad)
        for ships in [0, 9]:
            with self.assertRaises(ValueError): configuration(ships)

if __name__ == '__main__': unittest.main()
