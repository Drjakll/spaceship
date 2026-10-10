"""Prepare the checksum-pinned official macOS raylib release outside this project."""
import argparse
import hashlib
import pathlib
import tarfile
import tempfile
import urllib.request
from prepare_pufferlib import external_path
URL='https://github.com/raysan5/raylib/releases/download/5.5/raylib-5.5_macos.tar.gz'
SHA256='930c67b676963c6cffbd965814664523081ecbf3d30fc9df4211d0064aa6ba39'

def prepare(destination,archive=None):
    destination=external_path(destination)
    if destination.exists(): raise ValueError('Use a new external dependency destination')
    with tempfile.TemporaryDirectory() as temporary:
        source=pathlib.Path(archive) if archive else pathlib.Path(temporary)/'raylib.tar.gz'
        if archive is None: urllib.request.urlretrieve(URL,source)
        if hashlib.sha256(source.read_bytes()).hexdigest()!=SHA256: raise ValueError('raylib archive checksum mismatch')
        with tarfile.open(source,'r:gz') as package:
            # Only regular files required by this project; never extract archive links.
            for name in ('include/raylib.h','lib/libraylib.a','LICENSE'):
                member=package.getmember('raylib-5.5_macos/'+name)
                if not member.isfile(): raise ValueError('Unexpected raylib archive member')
                target=destination/name;target.parent.mkdir(parents=True,exist_ok=True)
                with package.extractfile(member) as data: target.write_bytes(data.read())
    return destination

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--destination',required=True)
    parser.add_argument('--archive',type=pathlib.Path)
    args=parser.parse_args();print(prepare(args.destination,args.archive))
