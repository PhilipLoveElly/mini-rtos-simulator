#!/usr/bin/env python3

import sys
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd


def main() -> int:
    if len(sys.argv) != 2:
        print(
            "Usage: python3 plot_latency.py "
            "<latency_csv>"
        )
        return 1

    input_file = Path(sys.argv[1])

    if not input_file.exists():
        print(f"File not found: {input_file}")
        return 1

    data = pd.read_csv(input_file)

    if "latency_ns" not in data.columns:
        print(
            "CSV must contain a latency_ns column"
        )
        return 1

    latency = data["latency_ns"].dropna()

    if latency.empty:
        print("No latency samples found")
        return 1

    output_dir = (
        input_file.parent
        / f"{input_file.stem}_figures"
    )

    output_dir.mkdir(
        parents=True,
        exist_ok=True,
    )

    median = latency.median()
    p95 = latency.quantile(0.95)
    p99 = latency.quantile(0.99)
    maximum = latency.max()

    print("Latency Summary")
    print("=" * 50)
    print(f"Samples : {len(latency)}")
    print(f"Mean    : {latency.mean():.3f} ns")
    print(f"Median  : {median:.3f} ns")
    print(f"P95     : {p95:.3f} ns")
    print(f"P99     : {p99:.3f} ns")
    print(f"Maximum : {maximum:.3f} ns")

    # Histogram
    histogram_limit = p99 * 1.15

    histogram_data = latency[
        latency <= histogram_limit
    ]

    plt.figure(figsize=(10, 6))

    plt.hist(
        histogram_data,
        bins=100,
    )

    plt.axvline(
        median,
        linestyle="--",
        label=f"Median = {median:.1f} ns",
    )

    plt.axvline(
        p95,
        linestyle="--",
        label=f"P95 = {p95:.1f} ns",
    )

    plt.axvline(
        p99,
        linestyle="--",
        label=f"P99 = {p99:.1f} ns",
    )

    plt.xlabel("Yield Cycle Latency (ns)")
    plt.ylabel("Sample Count")
    plt.title(
        "Scheduler Yield-Cycle Latency Histogram"
    )

    plt.xlim(
        latency.min(),
        histogram_limit,
    )

    plt.legend()
    plt.tight_layout()

    histogram_file = (
        output_dir
        / "yield_latency_histogram.png"
    )

    plt.savefig(
        histogram_file,
        dpi=200,
    )

    plt.close()

    # CDF
    sorted_latency = np.sort(
        latency.to_numpy()
    )

    cumulative_probability = (
        np.arange(
            1,
            len(sorted_latency) + 1,
        )
        / len(sorted_latency)
    )

    plt.figure(figsize=(10, 6))

    plt.plot(
        sorted_latency,
        cumulative_probability,
    )

    plt.axvline(
        median,
        linestyle="--",
        label=f"Median = {median:.1f} ns",
    )

    plt.axvline(
        p95,
        linestyle="--",
        label=f"P95 = {p95:.1f} ns",
    )

    plt.axvline(
        p99,
        linestyle="--",
        label=f"P99 = {p99:.1f} ns",
    )

    plt.xlabel("Yield Cycle Latency (ns)")
    plt.ylabel("Cumulative Probability")

    plt.title(
        "Scheduler Yield-Cycle Latency Distribution (CDF)"
    )

    cdf_limit = p99 * 1.15

    plt.xlim(
        latency.min(),
        cdf_limit,
    )

    plt.ylim(0.0, 1.0)

    plt.legend()
    plt.tight_layout()

    cdf_file = (
        output_dir
        / "yield_latency_cdf.png"
    )

    plt.savefig(
        cdf_file,
        dpi=200,
    )

    plt.close()

    print()
    print("Generated files:")
    print(f"  - {histogram_file}")
    print(f"  - {cdf_file}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())