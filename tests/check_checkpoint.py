"""Real-upstream integration: native weights, strict load checks and CPU episodes."""
import json
import pathlib
import subprocess
import tempfile

def check(binary):
    with tempfile.TemporaryDirectory() as directory:
        checkpoint=pathlib.Path(directory)/'untrained.bin'
        subprocess.run([str(binary),'--init-untrained',str(checkpoint),'19'],check=True)
        assert checkpoint.stat().st_size==188800*4
        command=[str(binary),str(checkpoint),'3','1000','7']
        a=json.loads(subprocess.check_output(command,text=True))
        b=json.loads(subprocess.check_output(command,text=True))
        assert a==b
        assert a['scheduled']==sum(a[x] for x in ['killed','escaped','unresolved','unspawned'])
        checkpoint.write_bytes(checkpoint.read_bytes()[:-4])
        bad=subprocess.run(command,capture_output=True,text=True)
        assert bad.returncode!=0 and 'checkpoint size' in bad.stderr
        print('PASS real PufferNet checkpoint load, recurrent evaluation and truncated-file rejection')

if __name__=='__main__':
    import sys
    check(pathlib.Path(sys.argv[1]))
