"""Parse every-update native PPO JSON diagnostics without inventing missing values."""
import argparse
import json
import math
import pathlib
from ppo_config import configuration,validate
def parse(text, geometry):
    output=[]; previous=0
    for line in text.splitlines():
        if not line.startswith('SPACE_TRAIN_JSON '): continue
        row=json.loads(line.split(' ',1)[1])
        if any(not isinstance(v,(int,float)) or not math.isfinite(v) for v in row.values()): raise ValueError('Non-finite native metric')
        update=row['epoch']; slots=row['agent_steps']
        if int(update)!=update or update!=previous+1: raise ValueError('Missing, duplicate or out-of-order PPO update')
        if slots!=update*geometry['agent_slots_per_update']: raise ValueError('Native slot counter disagrees with geometry')
        worlds=row.get('space/world_decisions')
        if worlds is not None and worlds!=update*geometry['world_decisions_per_update']: raise ValueError('World counter disagrees with geometry')
        output.append({'ppo_updates':int(update),'optimizer_steps':int(update*geometry['optimizer_steps_per_update']),
                       'agent_slots':int(slots),'world_decisions':worlds,
                       'active_decisions':row.get('space/active_decisions'),
                       'physics_ticks':row.get('space/physics_ticks'),'wall_seconds':row.get('uptime'),'native':row})
        previous=update
    if not output: raise ValueError('No native PPO metrics found')
    return output

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('log',type=pathlib.Path)
    parser.add_argument('--ships',type=int,default=3)
    parser.add_argument('--output',type=pathlib.Path,required=True)
    args=parser.parse_args()
    args.output.write_text(json.dumps(parse(args.log.read_text(),validate(configuration(args.ships))),indent=2)+'\n')
