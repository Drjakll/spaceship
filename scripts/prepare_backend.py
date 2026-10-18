"""Build a separate external integration tree from verified upstream source."""
import argparse
import json
import pathlib
import shutil
import subprocess
from prepare_pufferlib import PROJECT, verify, external_path
from run_manifest import sha

def prepare(root,destination):
    root=verify(root); destination=external_path(destination)
    if destination.exists(): raise ValueError('Use a new external destination; existing trees are not overwritten')
    destination.mkdir(parents=True)
    for name in ('src','vendor'):
        shutil.copytree(root/name,destination/name)
    (destination/'config').mkdir()
    shutil.copy2(root/'config/default.ini',destination/'config/default.ini')
    shutil.copy2(root/'build.sh',destination/'build.sh')
    shutil.copy2(root/'LICENSE',destination/'PUFFERLIB-LICENSE')
    subprocess.run(['patch','-p1','-i',str(PROJECT/'patches/pufferl-spaceship.patch')],cwd=destination,check=True)
    shutil.copytree(PROJECT/'native',destination/'src/space_native')
    env=destination/'ocean/spaceship';env.mkdir(parents=True)
    (env/'spaceship.h').write_text('typedef float obs_t;\n#include "../../src/space_native/spaceship_native.h"\n')
    shutil.copy2(PROJECT/'config/spaceship.ini',destination/'config/spaceship.ini')
    identity={'upstream':str(root),'trainer_sha256':sha(destination/'src/pufferl.cu'),
              'patch_sha256':sha(PROJECT/'patches/pufferl-spaceship.patch'),'precision':'float32'}
    (destination/'spaceship-build.json').write_text(json.dumps(identity,indent=2)+'\n')
    return destination

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pufferlib-root',required=True)
    parser.add_argument('--destination',required=True)
    parser.add_argument('--build-cuda',action='store_true')
    args=parser.parse_args()
    destination=prepare(args.pufferlib_root,args.destination)
    print(destination)
    if args.build_cuda:
        subprocess.run(['bash','build.sh','spaceship','puffer-spaceship','--float'],cwd=destination,check=True)
