"""Build the raylib viewer against external dependencies only."""
import argparse
import pathlib
import platform
import subprocess
import plistlib
import shutil
from prepare_pufferlib import PROJECT,external_path,verify

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--raylib-root',required=True)
    parser.add_argument('--pufferlib-root')
    parser.add_argument('--test',action='store_true')
    args=parser.parse_args()
    raylib=external_path(args.raylib_root)
    sources=['native/spaceship_view.c','native/spaceship_core.c','native/spaceship_bots.c','native/spaceship_input.c',
             'tests/test_view.c' if args.test else 'native/viewer.c']
    command=['clang','-std=c11','-O2','-Wall','-Wextra','-I'+str(PROJECT/'native'),'-I'+str(raylib/'include')]
    command+=['-DSPACE_ASSET_ROOT="'+str(PROJECT)+'"']
    if args.pufferlib_root:
        puffer=verify(args.pufferlib_root);command+=['-DPUFFERCPU_SOURCE="'+str(puffer/'src/puffercpu.c')+'"']
    target=PROJECT/'build'/('test_view' if args.test else 'viewer')
    target.parent.mkdir(exist_ok=True)
    command += [str(PROJECT/s) for s in sources]+[str(raylib/'lib/libraylib.a'),'-lm']
    if platform.system()=='Darwin':
        for framework in ('Cocoa','IOKit','CoreVideo','OpenGL'):command+=['-framework',framework]
    else: command+=['-lGL','-lpthread','-ldl','-lrt','-lX11']
    subprocess.run(command+['-o',str(target)],check=True)
    print(target)
    if platform.system()=='Darwin' and not args.test:
        bundle=PROJECT/'build/Spaceship.app/Contents'; (bundle/'MacOS').mkdir(parents=True,exist_ok=True)
        shutil.copy2(target,bundle/'MacOS/Spaceship')
        with (bundle/'Info.plist').open('wb') as output:
            plistlib.dump({'CFBundleExecutable':'Spaceship','CFBundleName':'Spaceship',
                'CFBundleIdentifier':'dev.spaceship.cooperativeviewer','CFBundleVersion':'1',
                'CFBundlePackageType':'APPL','NSHighResolutionCapable':True},output)
        print(bundle.parent)
