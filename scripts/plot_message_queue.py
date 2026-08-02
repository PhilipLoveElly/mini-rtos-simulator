#!/usr/bin/env python3

from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd


RESULT_ROOT = Path(
    "benchmark_results/message_queue"
)

OUTPUT_DIR = Path(
    "docs/images"
)

THROUGHPUT_OUTPUT = (
    OUTPUT_DIR / "message_queue_throughput.png"
)

BLOCK_OUTPUT = (
    OUTPUT_DIR / "message_queue_block_count.png"
)


def find_latest_csv() -> Path:
    csv_files = list(
        RESULT_ROOT.glob(
            "*/message_queue.csv"
        )
    )

    if not csv_files:
        raise FileNotFoundError(
            "No message_queue.csv found under "
            f"{RESULT_ROOT}"
        )

    return max(
        csv_files,
        key=lambda path: path.stat().st_mtime,
    )


def main() -> None:
    csv_path = find_latest_csv()

    print(f"Reading: {csv_path}")

    OUTPUT_DIR.mkdir(
        parents=True,
        exist_ok=True,
    )

    df = pd.read_csv(
        csv_path
    )

    required_columns = {
        "capacity",
        "messages",
        "throughput",
        "producer_blocks",
        "consumer_blocks",
        "checksum",
    }

    missing_columns = (
        required_columns - set(df.columns)
    )

    if missing_columns:
        raise ValueError(
            "Missing CSV columns: "
            + ", ".join(
                sorted(missing_columns)
            )
        )

    if df.empty:
        raise ValueError(
            "CSV contains no benchmark data"
        )

    message_counts = (
        df["messages"]
        .drop_duplicates()
        .tolist()
    )

    if len(message_counts) != 1:
        raise ValueError(
            "CSV contains multiple message counts: "
            f"{message_counts}"
        )

    message_count = int(
        message_counts[0]
    )

    expected_checksum = (
        message_count
        * (message_count - 1)
        // 2
    )

    invalid_checksum_rows = df[
        df["checksum"] != expected_checksum
    ]

    if not invalid_checksum_rows.empty:
        raise ValueError(
            "Checksum validation failed for "
            f"{len(invalid_checksum_rows)} row(s)"
        )

    summary = (
        df.groupby(
            "capacity",
            as_index=False,
        )
        .agg(
            mean_throughput=(
                "throughput",
                "mean",
            ),
            std_throughput=(
                "throughput",
                "std",
            ),
            mean_producer_blocks=(
                "producer_blocks",
                "mean",
            ),
            mean_consumer_blocks=(
                "consumer_blocks",
                "mean",
            ),
        )
        .sort_values(
            "capacity"
        )
    )

    summary["std_throughput"] = (
        summary["std_throughput"]
        .fillna(0.0)
    )

    summary["mean_throughput_mps"] = (
        summary["mean_throughput"]
        / 1_000_000.0
    )

    summary["std_throughput_mps"] = (
        summary["std_throughput"]
        / 1_000_000.0
    )

    block_counts_match = (
        summary["mean_producer_blocks"]
        .equals(
            summary["mean_consumer_blocks"]
        )
    )

    if not block_counts_match:
        raise ValueError(
            "Producer and consumer block counts differ. "
            "Update the blocking plot to show separate lines."
        )

    display_columns = [
        "capacity",
        "mean_throughput_mps",
        "std_throughput_mps",
        "mean_producer_blocks",
        "mean_consumer_blocks",
    ]

    print()
    print(
        summary[
            display_columns
        ].to_string(
            index=False
        )
    )

    # Throughput vs capacity
    plt.figure(
        figsize=(8, 5)
    )

    plt.errorbar(
        summary["capacity"],
        summary["mean_throughput_mps"],
        yerr=summary["std_throughput_mps"],
        marker="o",
        capsize=4,
        linewidth=2,
    )

    plt.xscale(
        "log",
        base=2,
    )

    plt.xticks(
        summary["capacity"],
        summary["capacity"],
    )

    plt.xlabel(
        "Message Queue Capacity"
    )

    plt.ylabel(
        "Throughput (million messages/s)"
    )

    plt.title(
        "Message Queue Throughput vs Capacity"
    )

    plt.grid(
        True,
        alpha=0.5,
    )

    plt.tight_layout()

    plt.savefig(
        THROUGHPUT_OUTPUT,
        dpi=300,
    )

    plt.close()

    # Blocking count vs capacity
    plt.figure(
        figsize=(8, 5)
    )

    plt.plot(
        summary["capacity"],
        summary["mean_producer_blocks"],
        marker="o",
        linewidth=2,
        label="Producer / Consumer Blocks",
    )

    plt.xscale(
        "log",
        base=2,
    )

    plt.xticks(
        summary["capacity"],
        summary["capacity"],
    )

    plt.xlabel(
        "Message Queue Capacity"
    )

    plt.ylabel(
        "Mean Block Count"
    )

    plt.title(
        "Message Queue Blocking vs Capacity"
    )

    plt.grid(
        True,
        alpha=0.5,
    )

    plt.legend()

    plt.tight_layout()

    plt.savefig(
        BLOCK_OUTPUT,
        dpi=300,
    )

    plt.close()

    print()
    print(
        f"Saved: {THROUGHPUT_OUTPUT}"
    )

    print(
        f"Saved: {BLOCK_OUTPUT}"
    )


if __name__ == "__main__":
    main()