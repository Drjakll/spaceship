"""Run fixed development or reserved test scenarios and write a JSON report."""
import argparse
import hashlib
import json
import pathlib
import platform
import statistics
import subprocess
import time
ROOT = pathlib.Path(__file__).resolve().parents[1]

def report(policy='lanes', ships=3, split='dev', binary=None, checkpoint=None, wave_size=200):
    seeds = list(range(1000,1020)) if split=='dev' else list(range(10000,10100))
    if split not in ('dev','test'): raise ValueError('Unknown scenario split')
    episodes=[]; start=time.perf_counter()
    for seed in seeds:
        command = [str(binary or ROOT/'build/evaluate'), str(checkpoint or policy), str(ships), str(seed), str(wave_size)]
        episodes.append(json.loads(subprocess.check_output(command,text=True)))
    elapsed=time.perf_counter()-start
    keys=['failure_fraction','escape_fraction','killed','survivors','duration_seconds','episode_return','score']
    aggregate={k:{'mean':statistics.mean(e[k] for e in episodes),
                  'std':statistics.pstdev(e[k] for e in episodes)} for k in keys}
    rules={'ships':ships,'wave_size':wave_size,'spawn_interval_ticks':108,'drain_ticks':1800,
           'frame_skip':4,'reward':[1,-2,-.02,-1], 'schema':'spaceship-v1-2554'}
    fingerprint=hashlib.sha256(json.dumps(rules,sort_keys=True).encode()+(ROOT/'native/spaceship_core.c').read_bytes()).hexdigest()
    return {'schema_version':1,'policy':policy,'trained':False if not checkpoint else None,
            'scenario_split':split,'scenario_seeds':seeds,'rules':rules,'rules_sha256':fingerprint,
            'action_mode':'seeded_sampling' if checkpoint or policy=='random' else 'deterministic_script',
            'episodes':episodes,'aggregate':aggregate,
            'targets':{'raw_escape_pass':aggregate['escape_fraction']['mean']<=.1,
                       'successful_defense':aggregate['failure_fraction']['mean']<=.1},
            'performance':{'wall_seconds':elapsed,'world_decisions_per_second':sum(e['world_decisions'] for e in episodes)/elapsed,
                'agent_slots_per_second':sum(e['agent_slots'] for e in episodes)/elapsed,
                'platform':platform.platform(),'machine':platform.machine(),'rendering':False}}

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--policy',choices=['random','greedy','lanes','checkpoint'],default='lanes')
    parser.add_argument('--ships',type=int,choices=range(1,9),default=3)
    parser.add_argument('--split',choices=['dev','test'],default='dev')
    parser.add_argument('--binary',type=pathlib.Path)
    parser.add_argument('--checkpoint',type=pathlib.Path)
    parser.add_argument('--output',type=pathlib.Path,required=True)
    args=parser.parse_args()
    if (args.policy=='checkpoint') != bool(args.checkpoint): parser.error('Checkpoint policy requires --checkpoint')
    result=report(args.policy,args.ships,args.split,args.binary,args.checkpoint)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({'output':str(args.output),'aggregate':result['aggregate'],'targets':result['targets']},indent=2))
