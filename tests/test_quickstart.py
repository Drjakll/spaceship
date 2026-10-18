import pathlib
import re
import subprocess
import unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]

class QuickstartTests(unittest.TestCase):
    def test_guide_commands_and_links(self):
        path=ROOT/'README.md'
        self.assertTrue(path.is_file(),'Local/GPU quick-start guide is missing')
        content=path.read_text()
        for command in ['make test','scripts/check_external.py','scripts/build_viewer.py',
                        'scripts/evaluate.py','scripts/gpu_diagnostic.py check','scripts/gpu_diagnostic.py train']:
            self.assertIn(command,content)
        for link in re.findall(r'\]\(([^)#]+)(?:#[^)]*)?\)',content):
            if '://' not in link: self.assertTrue((ROOT/link).exists(),link)
        for block in re.findall(r'```sh\n(.*?)```',content,re.S):
            result=subprocess.run(['sh','-n'],input=block,text=True,capture_output=True)
            self.assertEqual(result.returncode,0,result.stderr)

if __name__=='__main__': unittest.main()
