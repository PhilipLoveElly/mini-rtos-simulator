#!/usr/bin/env python3

import pandas as pd
import matplotlib.pyplot as plt

CSV = "benchmark_results/priority_inheritance/priority_inheritance.csv"
OUTPUT = "benchmark_results/priority_inheritance_figures/high_task_wait_ticks.png"

# Read benchmark data
df = pd.read_csv(CSV)

# Mean wait ticks for each workload
summary = (
    df.groupby(["pi_enabled", "medium_iterations"])["wait_ticks"]
      .mean()
      .reset_index()
)

# Split enabled / disabled
enabled = summary[summary["pi_enabled"] == 1]
disabled = summary[summary["pi_enabled"] == 0]

plt.figure(figsize=(8, 5))

plt.plot(
    enabled["medium_iterations"],
    enabled["wait_ticks"],
    marker="o",
    linewidth=2,
    label="Priority Inheritance Enabled"
)

plt.plot(
    disabled["medium_iterations"],
    disabled["wait_ticks"],
    marker="s",
    linewidth=2,
    label="Priority Inheritance Disabled"
)

plt.xlabel("MediumTask Iterations")
plt.ylabel("HighTask Wait Time (ticks)")
plt.title("Priority Inheritance Benchmark")

plt.grid(True)
plt.legend()

plt.tight_layout()
plt.savefig(OUTPUT, dpi=300)

print(f"Saved to {OUTPUT}")