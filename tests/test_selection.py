import copy
import pathlib
import sys
import unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'scripts'))
from select_checkpoint import choose

def candidate(name,failure,survivors,update):
    return {'scenario_split':'dev','scenario_seeds':list(range(1000,1020)),
            'rules_sha256':'same','checkpoint_update':update,'trained':True,
            'manifest':{'checkpoint':{'sha256':name,'kind':'trained'}},
            'aggregate':{'failure_fraction':{'mean':failure},'survivors':{'mean':survivors}}}

class SelectionTests(unittest.TestCase):
    def test_ranking_and_tie_retention(self):
        a=candidate('a',.1,2,4); b=candidate('b',.05,1,5); c=candidate('c',.05,2,6)
        d=candidate('d',.05,2,7)
        self.assertIs(choose([a,b,c,d]),c)
        twin=copy.deepcopy(c); twin['manifest']['checkpoint']['sha256']='twin'
        self.assertIs(choose([twin],c),c)
    def test_refuses_held_out_and_mixed_rules(self):
        a=candidate('a',.1,2,4); bad=copy.deepcopy(a); bad['scenario_split']='test'
        with self.assertRaises(ValueError): choose([bad])
        bad=copy.deepcopy(a); bad['rules_sha256']='different'
        with self.assertRaises(ValueError): choose([a,bad])
        bad=copy.deepcopy(a);bad['trained']=False
        with self.assertRaises(ValueError): choose([bad])

if __name__=='__main__': unittest.main()
