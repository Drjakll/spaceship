"""Bounded NVIDIA verification plan and training gate."""
import argparse
import hashlib
import json
import pathlib
import platform
import subprocess
from run_manifest import ROOT,sha
from ppo_config import configuration,validate

def source_identity():
    paths=sorted(p for directory in ('native','scripts','patches','config') for p in (ROOT/directory).rglob('*')
                 if p.is_file() and '__pycache__' not in p.parts)
    return hashlib.sha256(json.dumps({str(p.relative_to(ROOT)):sha(p) for p in paths},sort_keys=True).encode()).hexdigest()

def diagnostic_plan():
    return [{'ships':ships,'period':period} for ships in range(1,9) for period in (1,63,64,65)]
def verify_receipt(receipt, binary_sha256, source_sha256):
    if receipt.get('binary_sha256')!=binary_sha256 or receipt.get('source_sha256')!=source_sha256:
        raise ValueError('Diagnostic identities do not match current binary/source')
    if receipt.get('executed_on')!='NVIDIA CUDA': raise ValueError('Actual CUDA diagnostic required')
    cases=receipt.get('cases',[])
    if len(cases)!=32 or {(c['ships'],c['period']) for c in cases}!={(c['ships'],c['period']) for c in diagnostic_plan()}:
        raise ValueError('Incomplete team/boundary diagnostic matrix')
    for case in cases:
        if any(case.get(key) is not True for key in ('passed','bootstrap_preserved','action_logprob_checked','optimizer_changed','checkpoint_roundtrip')) or case.get('checked_targets',0)<=0:
            raise ValueError('A required CUDA check did not pass')

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode',choices=['plan','check','train'])
    parser.add_argument('--backend',type=pathlib.Path)
    parser.add_argument('--receipt',type=pathlib.Path,default=ROOT/'artifacts/gpu-diagnostic.json')
    parser.add_argument('--ships',type=int,default=3)
    parser.add_argument('--updates',type=int,default=4)
    parser.add_argument('--seed',type=int,default=11)
    parser.add_argument('--max-seconds',type=int,default=900)
    args=parser.parse_args()
    if args.mode=='plan':
        print(json.dumps({'cases':diagnostic_plan(),'executed':False,'updates_per_case':2,'max_seconds':args.max_seconds},indent=2))
    else:
        if args.backend is None or platform.system()!='Linux': parser.error('A prepared Linux/NVIDIA backend is required')
        backend=args.backend.resolve();binary=backend/'puffer-spaceship'
        identity=source_identity(); binary_hash=sha(binary)
        # Refuse a stale copied adapter even when its binary path is unchanged.
        for path in (ROOT/'native').glob('*'):
            if path.is_file() and sha(path)!=sha(backend/'src/space_native'/path.name): parser.error('Backend source is stale; prepare and rebuild it')
        build=json.loads((backend/'spaceship-build.json').read_text())
        if build['patch_sha256']!=sha(ROOT/'patches/pufferl-spaceship.patch'): parser.error('Backend patch is stale')
        if args.max_seconds<=0: parser.error('A positive wall-time cap is required')
        if args.mode=='check':
            import time
            started=time.monotonic(); cases=[]
            for case in diagnostic_plan():
                remaining=args.max_seconds-(time.monotonic()-started)
                if remaining<=0: raise TimeoutError('Diagnostic wall-time budget exhausted')
                command=[str(binary),'space-check','--headless',f"--env.num_agents={case['ships']}",
                    f"--vec.total_agents={64*case['ships']}",f"--env.diagnostic_period={case['period']}",
                    '--base.async=0','--base.cudagraphs=-1','--base.reset_every_horizon=0',
                    '--vec.num_policies=1','--vec.num_buffers=1','--selfplay.enabled=0','--train.vtrace=0']
                result=subprocess.run(command,cwd=backend,capture_output=True,text=True,check=True,timeout=remaining)
                matches=[json.loads(line.split(' ',1)[1]) for line in result.stdout.splitlines() if line.startswith('SPACE_CHECK_JSON ')]
                if len(matches)!=1: raise ValueError('Missing native diagnostic result')
                cases.append(matches[0]);print(f"Passed ships={case['ships']} period={case['period']}",flush=True)
            receipt={'binary_sha256':binary_hash,'source_sha256':identity,'executed_on':'NVIDIA CUDA','cases':cases}
            verify_receipt(receipt,binary_hash,identity)
            args.receipt.parent.mkdir(parents=True,exist_ok=True);args.receipt.write_text(json.dumps(receipt,indent=2)+'\n')
        else:
            verify_receipt(json.loads(args.receipt.read_text()),binary_hash,identity)
            config=configuration(args.ships,args.updates,args.seed);validate(config)
            command=[str(binary),'train','--headless']+[f'--{section}.{key}={value}' for section,items in config.items() for key,value in items.items()]
            command+=['--env.diagnostic_period=0']
            subprocess.run(command,cwd=backend,check=True,timeout=args.max_seconds)
