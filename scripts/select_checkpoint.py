"""Archive immutable weights and select using development scenarios only."""
import argparse
import hashlib
import json
import math
import pathlib

def rank(report):
    failure=report['aggregate']['failure_fraction']['mean']
    survivors=report['aggregate']['survivors']['mean']
    update=report['checkpoint_update']
    if not all(math.isfinite(x) for x in (failure,survivors,update)) or not 0<=failure<=1 or survivors<0 or update<=0:
        raise ValueError('Invalid checkpoint selection metrics')
    return failure,-survivors,update

def choose(reports, incumbent=None):
    pool=([incumbent] if incumbent else [])+list(reports)
    if not pool: raise ValueError('No checkpoints supplied')
    fingerprint=pool[0]['rules_sha256']
    for report in pool:
        if report['scenario_split']!='dev' or report['scenario_seeds']!=list(range(1000,1020)):
            raise ValueError('Selection uses only the complete development scenario set')
        if report['rules_sha256']!=fingerprint: raise ValueError('Cannot compare different rules')
        checkpoint=report['manifest']['checkpoint']
        if report.get('trained') is not True or not checkpoint or checkpoint['kind']!='trained':
            raise ValueError('Selection requires explicitly trained checkpoint reports')
    return min(pool,key=rank)  # stable: incumbent wins exact ties

def archive(report, directory):
    directory=pathlib.Path(directory);directory.mkdir(parents=True,exist_ok=True)
    checkpoint=report['manifest']['checkpoint']
    data=pathlib.Path(checkpoint['path']).read_bytes()
    digest=hashlib.sha256(data).hexdigest()
    if digest!=checkpoint['sha256']: raise ValueError('Checkpoint changed after evaluation')
    target=directory/(digest+'.bin')
    if target.exists():
        if target.read_bytes()!=data: raise ValueError('Archive hash collision or corruption')
    else:
        with target.open('xb') as output: output.write(data)
    return {'sha256':digest,'path':str(target.resolve()),'update':report['checkpoint_update']}

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reports',type=pathlib.Path,nargs='+',required=True)
    parser.add_argument('--incumbent-report',type=pathlib.Path)
    parser.add_argument('--archive',type=pathlib.Path,required=True)
    args=parser.parse_args()
    reports=[json.loads(p.read_text()) for p in args.reports]
    incumbent=json.loads(args.incumbent_report.read_text()) if args.incumbent_report else None
    winner=choose(reports,incumbent)
    for report in reports: archive(report,args.archive)
    selected=archive(winner,args.archive)
    latest=archive(max(reports+([incumbent] if incumbent else []),key=lambda r:r['checkpoint_update']),args.archive)
    (args.archive/'labels.json').write_text(json.dumps({'selected':selected,'latest':latest},indent=2)+'\n')
    (args.archive/'selected-report.json').write_text(json.dumps(winner,indent=2)+'\n')
    print(json.dumps({'selected':selected,'latest':latest},indent=2))
