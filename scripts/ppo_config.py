"""Generate and validate bounded native cooperative PPO configuration."""
import argparse
import configparser
import pathlib
import io

def configuration(ships=3, updates=4, seed=11):
    if not 1 <= ships <= 8 or updates <= 0 or not 0 <= seed < 2**31:
        raise ValueError('Require 1–8 ships, positive updates, and a nonnegative int32 seed')
    config = {
        'base': {'env_name': 'spaceship', 'async': 0, 'reset_every_horizon': 0,
                 'cudagraphs': -1, 'seed': seed, 'checkpoint_interval': 1},
        'vec': {'total_agents': ships * 64, 'num_buffers': 1, 'num_threads': 4,
                'num_policies': 1, 'hist_policy_percent': 0},
        'selfplay': {'enabled': 0},
        'env': {'num_agents': ships, 'seed': seed, 'wave_size': 200,
                'spawn_interval_ticks': 108, 'drain_ticks': 1800, 'frame_skip': 4,
                'reward_kill': 1, 'reward_escape': -2, 'reward_damage': -.02, 'reward_death': -1},
        'policy': {'hidden_size': 64, 'num_layers': 2},
        'train': {'gpus': 1, 'total_timesteps': ships * 64 * 64 * updates,
                  'learning_rate': .001, 'gamma': .99, 'gae_lambda': .95,
                  'replay_ratio': 2, 'clip_coef': .2, 'ent_coef': .01,
                  'minibatch_size': 4096, 'horizon': 64, 'vtrace': 0}}
    validate(config)
    return config

def validate(config):
    c = config
    for section, key, expected in [('selfplay','enabled',0), ('vec','num_policies',1),
            ('vec','num_buffers',1), ('vec','hist_policy_percent',0), ('base','async',0),
            ('base','reset_every_horizon',0), ('train','vtrace',0), ('train','gpus',1)]:
        if c[section][key] != expected: raise ValueError(f'{section}.{key} must be {expected}')
    ships, agents = c['env']['num_agents'], c['vec']['total_agents']
    horizon, mb = c['train']['horizon'], c['train']['minibatch_size']
    steps = c['train']['total_timesteps']
    if not 1 <= ships <= 8 or agents <= 0 or agents % ships:
        raise ValueError('Each arena must contain its full allied team')
    if horizon != 64 or mb <= 0 or mb % horizon or (agents * horizon) % mb:
        raise ValueError('Invalid native rollout/minibatch geometry')
    if steps <= 0 or steps % (agents * horizon): raise ValueError('Budget must contain complete finite rollouts')
    h, layers = c['policy']['hidden_size'], c['policy']['num_layers']
    if h != 64 or layers != 2: raise ValueError('Initial supported model is hidden=64, layers=2')
    # Float32 allocator has 16-byte / 4-element alignment. Every tensor is aligned.
    shapes = [2554*h, h, h*12, 12] + [h*3*h, 3*h]*layers
    if any(size % 4 for size in shapes): raise ValueError('Unsupported padded optimizer layout')
    return {'arenas': agents // ships, 'world_decisions_per_update': agents // ships * horizon,
            'agent_slots_per_update': agents * horizon, 'updates': steps // (agents * horizon),
            'optimizer_steps_per_update': c['train']['replay_ratio'] * agents * horizon // mb,
            'precision': 'float32', 'self_play': False}

def write_config(path, config):
    validate(config)
    ini = configparser.ConfigParser()
    ini.read_dict(config)
    output = io.StringIO()
    ini.write(output)
    pathlib.Path(path).write_text(output.getvalue().rstrip() + '\n')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ships', type=int, default=3)
    parser.add_argument('--updates', type=int, default=4)
    parser.add_argument('--seed', type=int, default=11)
    parser.add_argument('--output', type=pathlib.Path, required=True)
    args = parser.parse_args()
    config = configuration(args.ships, args.updates, args.seed)
    write_config(args.output, config)
    print(validate(config))
