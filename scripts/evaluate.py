"""Run fixed development or reserved test scenarios and write a JSON report."""
import argparse
import hashlib
import json
import pathlib
import platform
import statistics
import subprocess
import time
from run_manifest import manifest
from ppo_config import configuration
ROOT = pathlib.Path(__file__).resolve().parents[1]

def evaluation_identity(binary):
    return {'binary_sha256':hashlib.sha256(pathlib.Path(binary).read_bytes()).hexdigest(),
            'source_files':{name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()
                            for name in ('native/spaceship_core.c','native/spaceship_core.h')}}

def report(policy='lanes', ships=3, split='dev', binary=None, checkpoint=None, wave_size=200, checkpoint_kind='unknown'):
    seeds = list(range(1000,1020)) if split=='dev' else list(range(10000,10100))
    if split not in ('dev','test'): raise ValueError('Unknown scenario split')
    binary=pathlib.Path(binary or ROOT/'build/evaluate').resolve()
    build_identity=evaluation_identity(binary)
    episodes=[]; start=time.perf_counter()
    for seed in seeds:
        command = [str(binary), str(checkpoint or policy), str(ships), str(seed), str(wave_size)]
        episodes.append(json.loads(subprocess.check_output(command,text=True)))
    elapsed=time.perf_counter()-start
    keys=['failure_fraction','escape_fraction','killed','survivors','duration_seconds','episode_return','score']
    aggregate={k:{'mean':statistics.mean(e[k] for e in episodes),
                  'std':statistics.pstdev(e[k] for e in episodes)} for k in keys}
    rules={'ships':ships,'wave_size':wave_size,'spawn_interval_ticks':108,'drain_ticks':1800,
           'frame_skip':4,'reward':[1,-2,-.02,-1], 'schema':'spaceship-v1-2554'}
    fingerprint=hashlib.sha256(json.dumps({'rules':rules,'build':build_identity},sort_keys=True).encode()).hexdigest()
    counters={key:sum(e[key] for e in episodes) for key in ('physics_ticks','world_decisions','agent_slots','active_decisions')}
    counters.update(ppo_updates=0,optimizer_steps=0,wall_seconds=elapsed)
    config=configuration(ships); config['env']['wave_size']=wave_size
    identity=manifest(config,checkpoint,checkpoint_kind,counters)
    return {'schema_version':1,'policy':policy,'trained':False if not checkpoint or checkpoint_kind=='untrained' else True if checkpoint_kind=='trained' else None,
            'manifest':identity,'evaluation_build':build_identity,
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
    parser.add_argument('--checkpoint-kind',choices=['unknown','untrained','trained'],default='unknown')
    parser.add_argument('--checkpoint-update',type=int,default=0)
    parser.add_argument('--output',type=pathlib.Path,required=True)
    args=parser.parse_args()
    if (args.policy=='checkpoint') != bool(args.checkpoint): parser.error('Checkpoint policy requires --checkpoint')
    result=report(args.policy,args.ships,args.split,args.binary,args.checkpoint,checkpoint_kind=args.checkpoint_kind)
    result['checkpoint_update']=args.checkpoint_update
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({'output':str(args.output),'aggregate':result['aggregate'],'targets':result['targets']},indent=2))
