# Initial scripted baseline measurements

Native C, macOS arm64, clang `-O2`; 20 development seeds 1000–1019, 200 scheduled enemies per seed. These are scripted policies, not trained PPO results. Default rules were frozen for this measurement; no reserved test scenarios were used for tuning.

| Policy | Ships | Mean raw escapes | Mean defensive failures | Mean kills | Mean survivors |
|---|---:|---:|---:|---:|---:|
| Seeded random | 3 | 20.725% | 46.725% | 106.55 | 0.25 |
| Independent greedy | 3 | 0% | 0% | 200 | 3 |
| Lane coverage | 3 | 0% | 0% | 200 | 3 |
| Lane coverage | 1 | 1.15% | 66.375% | 67.25 | 0 |

The one-ship result illustrates why raw escapes alone are insufficient: every team died early, leaving most scheduled enemies unresolved or unspawned. Both three-ship aiming scripts solve these development scenarios, so this benchmark establishes a reachable defense strategy but does not establish learned coordination or a unique advantage for lane allocation.

Reproduce with `make build/evaluate` then `python3 scripts/evaluate.py --policy lanes --output artifacts/dev-lanes.json`; substitute `random` or `greedy`. Add `--ships 1` for the single-ship reference. Reports contain seed/action-seed identities, rules fingerprint, individual contributions, reward components, mean/population standard deviation, and measured process-inclusive throughput. Rendering is disabled. Timing is excluded from semantic reproducibility comparisons.
