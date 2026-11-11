#include "enemy_ppo.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(void) {
    fputs("usage:\n"
          "  enemy_ppo train --run-dir DIR --ship-checkpoint FILE [--updates N] [--envs N] [--steps N]\n"
          "                  [--hidden N] [--seed N]\n"
          "  enemy_ppo ship-train --run-dir DIR --enemy-checkpoint FILE [--ship-checkpoint FILE]\n"
          "                       [--total-decisions N | --updates N] [--max-seconds S]\n"
          "                       [--checkpoint-every N] [--envs N] [--steps N] [--hidden N] [--seed N]\n"
          "  enemy_ppo eval --ship-checkpoint FILE [--enemy-checkpoint FILE | --random]\n"
          "                 [--episodes N] [--seed N] [--output FILE]\n",
          stderr);
}

static int train(int argc, char **argv) {
    EnemyTrainConfig cfg = {.seed = 1, .updates = 10000, .envs = 16, .steps = 128, .hidden = 64};
    for (int i = 0; i < argc; i++) {
        const char *flag = argv[i];
        if (i + 1 >= argc) {
            fprintf(stderr, "missing value for %s\n", flag);
            return 2;
        }
        const char *value = argv[++i];
        if (!strcmp(flag, "--run-dir")) {
            cfg.run_dir = value;
        } else if (!strcmp(flag, "--ship-checkpoint")) {
            cfg.ship_checkpoint = value;
        } else if (!strcmp(flag, "--updates")) {
            cfg.updates = atoi(value);
        } else if (!strcmp(flag, "--envs")) {
            cfg.envs = atoi(value);
        } else if (!strcmp(flag, "--steps")) {
            cfg.steps = atoi(value);
        } else if (!strcmp(flag, "--hidden")) {
            cfg.hidden = atoi(value);
        } else if (!strcmp(flag, "--seed")) {
            cfg.seed = strtoull(value, NULL, 10);
        } else {
            fprintf(stderr, "unknown option %s\n", flag);
            return 2;
        }
    }
    if (!cfg.run_dir || !cfg.ship_checkpoint) {
        fputs("train needs --run-dir and --ship-checkpoint\n", stderr);
        return 2;
    }
    return enemy_ppo_train(&cfg) == 0 ? 0 : 1;
}

static int train_ships(int argc, char **argv) {
    RunConfig cfg = run_config_default();
    const char *enemy_checkpoint = NULL;
    long long updates = 0;
    int updates_set = 0;
    for (int i = 0; i < argc; i++) {
        const char *flag = argv[i];
        if (i + 1 >= argc) {
            fprintf(stderr, "missing value for %s\n", flag);
            return 2;
        }
        const char *value = argv[++i];
        if (!strcmp(flag, "--run-dir")) {
            cfg.run_dir = value;
        } else if (!strcmp(flag, "--enemy-checkpoint")) {
            enemy_checkpoint = value;
        } else if (!strcmp(flag, "--ship-checkpoint")) {
            cfg.initial_checkpoint = value;
        } else if (!strcmp(flag, "--total-decisions")) {
            cfg.total_decisions = atoll(value);
        } else if (!strcmp(flag, "--updates")) {
            updates = atoll(value);
            updates_set = 1;
        } else if (!strcmp(flag, "--max-seconds")) {
            cfg.max_seconds = atof(value);
        } else if (!strcmp(flag, "--checkpoint-every")) {
            cfg.checkpoint_every = atoi(value);
        } else if (!strcmp(flag, "--envs")) {
            cfg.train.envs = atoi(value);
        } else if (!strcmp(flag, "--steps")) {
            cfg.train.steps = atoi(value);
        } else if (!strcmp(flag, "--hidden")) {
            cfg.train.hidden = atoi(value);
        } else if (!strcmp(flag, "--seed")) {
            cfg.train.seed = strtoull(value, NULL, 10);
        } else {
            fprintf(stderr, "unknown option %s\n", flag);
            return 2;
        }
    }
    if (!cfg.run_dir || !enemy_checkpoint || cfg.train.envs <= 0 || cfg.train.steps <= 0 ||
        cfg.train.hidden <= 0 || cfg.train.hidden > 4096 || cfg.train.ppo.minibatches <= 0 ||
        cfg.total_decisions <= 0 || cfg.max_seconds <= 0.0 || cfg.checkpoint_every < 0 ||
        (updates_set && updates <= 0)) {
        fputs("ship-train needs --run-dir, --enemy-checkpoint, and positive training sizes\n", stderr);
        return 2;
    }
    if (updates_set) {
        long long decisions_per_update = (long long)cfg.train.envs * cfg.train.steps;
        if (updates > LLONG_MAX / decisions_per_update) {
            fputs("ship-train: --updates exceeds the supported training budget\n", stderr);
            return 2;
        }
        cfg.total_decisions = updates * decisions_per_update;
    }
    cfg.opponent_checkpoint = enemy_checkpoint;
    return enemy_ppo_train_ships(&cfg, enemy_checkpoint) == 0 ? 0 : 1;
}

static int evaluate(int argc, char **argv) {
    const char *ship_checkpoint = NULL, *enemy_checkpoint = NULL, *output = NULL;
    int episodes = 100, random_policy = 0;
    uint64_t seed = 10000;
    for (int i = 0; i < argc; i++) {
        const char *flag = argv[i];
        if (!strcmp(flag, "--random")) {
            random_policy = 1;
            continue;
        }
        if (i + 1 >= argc) {
            fprintf(stderr, "missing value for %s\n", flag);
            return 2;
        }
        const char *value = argv[++i];
        if (!strcmp(flag, "--ship-checkpoint")) {
            ship_checkpoint = value;
        } else if (!strcmp(flag, "--enemy-checkpoint")) {
            enemy_checkpoint = value;
        } else if (!strcmp(flag, "--episodes")) {
            episodes = atoi(value);
        } else if (!strcmp(flag, "--seed")) {
            seed = strtoull(value, NULL, 10);
        } else if (!strcmp(flag, "--output")) {
            output = value;
        } else {
            fprintf(stderr, "unknown option %s\n", flag);
            return 2;
        }
    }
    if (!ship_checkpoint || (!random_policy && !enemy_checkpoint) || (random_policy && enemy_checkpoint)) {
        fputs("eval needs --ship-checkpoint and exactly one of --enemy-checkpoint or --random\n", stderr);
        return 2;
    }
    return enemy_ppo_evaluate(enemy_checkpoint, ship_checkpoint, episodes, seed, output, random_policy) == 0 ? 0 : 1;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        usage();
        return 2;
    }
    if (!strcmp(argv[1], "train")) {
        return train(argc - 2, argv + 2);
    }
    if (!strcmp(argv[1], "ship-train")) {
        return train_ships(argc - 2, argv + 2);
    }
    if (!strcmp(argv[1], "eval")) {
        return evaluate(argc - 2, argv + 2);
    }
    usage();
    return 2;
}
