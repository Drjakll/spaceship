"""Capture source, configuration, checkpoint and explicitly named work units."""
import argparse
import datetime
import hashlib
import json
import math
import pathlib
import subprocess
from ppo_config import configuration, validate
ROOT=pathlib.Path(__file__).resolve().parents[1]
COUNTERS=('physics_ticks','world_decisions','agent_slots','active_decisions','ppo_updates','optimizer_steps','wall_seconds')
def sha(path): return hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest()

def manifest(config, checkpoint=None, checkpoint_kind='unknown', counters=None):
    geometry=validate(config)
    if checkpoint_kind not in ('unknown','untrained','trained'): raise ValueError('Unknown checkpoint kind')
    units={key:None for key in COUNTERS}
    for key,value in (counters or {}).items():
        if key not in units or value is not None and (not math.isfinite(value) or value<0):
            raise ValueError('Invalid counter '+key)
        if value is not None and key!='wall_seconds' and int(value)!=value: raise ValueError('Fractional step count')
        units[key]=value
    if units['agent_slots'] is not None and units['world_decisions'] is not None:
        if units['agent_slots']!=units['world_decisions']*config['env']['num_agents']: raise ValueError('Inconsistent step units')
    if units['active_decisions'] is not None and units['agent_slots'] is not None:
        if units['active_decisions']>units['agent_slots']: raise ValueError('Active decisions exceed allocated slots')
    paths=sorted(p for directory in ('native','scripts','patches','config') for p in (ROOT/directory).rglob('*')
                 if p.is_file() and '__pycache__' not in p.parts)
    sources={str(p.relative_to(ROOT)):sha(p) for p in paths}
    git=lambda *args: subprocess.check_output(['git',*args],cwd=ROOT,text=True).strip()
    return {'schema_version':1,'created_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'source':{'commit':git('rev-parse','HEAD'),'dirty':bool(git('status','--porcelain')),'files':sources},
        'upstream':json.loads((ROOT/'pufferlib-lock.json').read_text()),
        'configuration':config,'configuration_sha256':hashlib.sha256(json.dumps(config,sort_keys=True).encode()).hexdigest(),
        'geometry':geometry,'checkpoint':None if checkpoint is None else {
            'path':str(pathlib.Path(checkpoint).resolve()),'sha256':sha(checkpoint),'kind':checkpoint_kind,
            'bytes':pathlib.Path(checkpoint).stat().st_size,'format':'native-fp32-weight-snapshot',
            'exact_optimizer_resume':False},'counters':units}

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ships',type=int,default=3)
    parser.add_argument('--updates',type=int,default=4)
    parser.add_argument('--checkpoint',type=pathlib.Path)
    parser.add_argument('--checkpoint-kind',choices=['unknown','untrained','trained'],default='unknown')
    parser.add_argument('--output',type=pathlib.Path,required=True)
    args=parser.parse_args()
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(manifest(configuration(args.ships,args.updates),args.checkpoint,args.checkpoint_kind),indent=2)+'\n')
