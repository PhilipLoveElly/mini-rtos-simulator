from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd


INPUT_FILE = Path("benchmark.csv")
SUMMARY_FILE = Path("benchmark_summary.csv")
OUTPUT_DIR = Path("benchmark_figures")


def percentile_95(series: pd.Series) -> float:
    return series.quantile(0.95)


def percentile_99(series: pd.Series) -> float:
    return series.quantile(0.99)


def validate_dataframe(df: pd.DataFrame) -> None:
    required_columns = {
        "tasks",
        "run",
        "elapsed_ms",
        "avg_ns",
        "throughput",
    }

    missing_columns = required_columns - set(df.columns)

    if missing_columns:
        missing = ", ".join(sorted(missing_columns))

        raise ValueError(
            f"benchmark.csv 缺少必要欄位：{missing}"
        )

    if df.empty:
        raise ValueError(
            "benchmark.csv 沒有任何資料"
        )


def create_summary(
    df: pd.DataFrame,
) -> pd.DataFrame:
    summary = (
        df.groupby("tasks")
        .agg(
            runs=("run", "count"),

            elapsed_mean_ms=(
                "elapsed_ms",
                "mean",
            ),
            elapsed_std_ms=(
                "elapsed_ms",
                "std",
            ),
            elapsed_min_ms=(
                "elapsed_ms",
                "min",
            ),
            elapsed_max_ms=(
                "elapsed_ms",
                "max",
            ),
            elapsed_p95_ms=(
                "elapsed_ms",
                percentile_95,
            ),
            elapsed_p99_ms=(
                "elapsed_ms",
                percentile_99,
            ),

            yield_mean_ns=(
                "avg_ns",
                "mean",
            ),
            yield_std_ns=(
                "avg_ns",
                "std",
            ),
            yield_min_ns=(
                "avg_ns",
                "min",
            ),
            yield_max_ns=(
                "avg_ns",
                "max",
            ),
            yield_p95_ns=(
                "avg_ns",
                percentile_95,
            ),
            yield_p99_ns=(
                "avg_ns",
                percentile_99,
            ),

            throughput_mean=(
                "throughput",
                "mean",
            ),
            throughput_std=(
                "throughput",
                "std",
            ),
            throughput_min=(
                "throughput",
                "min",
            ),
            throughput_max=(
                "throughput",
                "max",
            ),
            throughput_p95=(
                "throughput",
                percentile_95,
            ),
            throughput_p99=(
                "throughput",
                percentile_99,
            ),
        )
        .reset_index()
        .sort_values("tasks")
    )

    return summary


def save_yield_latency_chart(
    summary: pd.DataFrame,
) -> None:
    plt.figure(figsize=(9, 6))

    plt.plot(
        summary["tasks"],
        summary["yield_mean_ns"],
        marker="o",
        markersize=7,
        linewidth=2,
    )

    plt.xlabel("Number of Ready Tasks")
    plt.ylabel("Mean Yield Latency (ns)")
    plt.title(
        "Mean Scheduler Yield Latency "
        "vs Number of Ready Tasks"
    )

    plt.xticks(summary["tasks"])
    plt.grid(True)
    plt.tight_layout()

    # 保留原本檔名，避免 README 路徑需要修改。
    output_file = (
        OUTPUT_DIR / "yield_cost.png"
    )

    plt.savefig(
        output_file,
        dpi=200,
    )
    plt.close()


def save_yield_errorbar_chart(
    summary: pd.DataFrame,
) -> None:
    plt.figure(figsize=(9, 6))

    plt.errorbar(
        summary["tasks"],
        summary["yield_mean_ns"],
        yerr=summary["yield_std_ns"],
        marker="o",
        markersize=7,
        linewidth=2,
        capsize=5,
    )

    plt.xlabel("Number of Ready Tasks")
    plt.ylabel("Mean Yield Latency (ns)")
    plt.title(
        "Scheduler Yield Latency "
        "with Standard Deviation"
    )

    plt.xticks(summary["tasks"])
    plt.grid(True)
    plt.tight_layout()

    output_file = (
        OUTPUT_DIR /
        "yield_cost_errorbar.png"
    )

    plt.savefig(
        output_file,
        dpi=200,
    )
    plt.close()


def save_throughput_chart(
    summary: pd.DataFrame,
) -> None:
    plt.figure(figsize=(9, 6))

    throughput_millions = (
        summary["throughput_mean"]
        / 1_000_000
    )

    plt.plot(
        summary["tasks"],
        throughput_millions,
        marker="o",
        markersize=7,
        linewidth=2,
    )

    plt.xlabel("Number of Ready Tasks")
    plt.ylabel(
        "Mean Throughput "
        "(Million Yields/s)"
    )
    plt.title(
        "Scheduler Yield Throughput "
        "vs Number of Ready Tasks"
    )

    plt.xticks(summary["tasks"])
    plt.grid(True)
    plt.tight_layout()

    output_file = (
        OUTPUT_DIR /
        "yield_throughput.png"
    )

    plt.savefig(
        output_file,
        dpi=200,
    )
    plt.close()


def save_elapsed_chart(
    summary: pd.DataFrame,
) -> None:
    plt.figure(figsize=(9, 6))

    plt.plot(
        summary["tasks"],
        summary["elapsed_mean_ms"],
        marker="o",
        markersize=7,
        linewidth=2,
    )

    plt.xlabel("Number of Ready Tasks")
    plt.ylabel("Mean Elapsed Time (ms)")
    plt.title(
        "Benchmark Elapsed Time "
        "vs Number of Ready Tasks"
    )

    plt.xticks(summary["tasks"])
    plt.grid(True)
    plt.tight_layout()

    output_file = (
        OUTPUT_DIR / "elapsed_time.png"
    )

    plt.savefig(
        output_file,
        dpi=200,
    )
    plt.close()


def save_yield_histogram(
    df: pd.DataFrame,
) -> None:
    """
    顯示所有 benchmark run 的平均 yield latency 分布。

    可以用來觀察：
    - 資料主要集中在哪個區間
    - 是否存在長尾
    - 是否有離群值
    """

    plt.figure(figsize=(9, 6))

    plt.hist(
        df["avg_ns"],
        bins=30,
        edgecolor="black",
    )

    plt.xlabel(
        "Mean Yield Latency per Run (ns)"
    )
    plt.ylabel("Number of Benchmark Runs")
    plt.title(
        "Distribution of Mean "
        "Scheduler Yield Latency"
    )

    plt.grid(
        True,
        axis="y",
    )
    plt.tight_layout()

    output_file = (
        OUTPUT_DIR /
        "yield_cost_histogram.png"
    )

    plt.savefig(
        output_file,
        dpi=200,
    )
    plt.close()


def save_yield_run_sequence(
    df: pd.DataFrame,
) -> None:
    """
    顯示每個 task count 在每一次 run 的平均 yield latency。

    可以用來觀察：
    - 偶發 latency spike
    - 週期性波動
    - benchmark 是否隨時間變慢
    """

    plt.figure(figsize=(11, 6))

    for task_count, group in df.groupby(
        "tasks",
        sort=True,
    ):
        ordered_group = group.sort_values(
            "run"
        )

        plt.plot(
            ordered_group["run"],
            ordered_group["avg_ns"],
            marker=".",
            markersize=4,
            linewidth=1,
            label=f"{task_count} tasks",
        )

    plt.xlabel("Run Number")
    plt.ylabel(
        "Mean Yield Latency per Run (ns)"
    )
    plt.title(
        "Scheduler Yield Latency "
        "Across Benchmark Runs"
    )

    plt.grid(True)
    plt.legend(
        title="Ready Tasks",
        ncol=2,
    )
    plt.tight_layout()

    output_file = (
        OUTPUT_DIR /
        "yield_cost_run_sequence.png"
    )

    plt.savefig(
        output_file,
        dpi=200,
    )
    plt.close()


def main() -> None:
    if not INPUT_FILE.exists():
        raise FileNotFoundError(
            f"找不到 {INPUT_FILE}，"
            "請先執行 benchmark.sh"
        )

    df = pd.read_csv(INPUT_FILE)

    validate_dataframe(df)

    numeric_columns = [
        "tasks",
        "run",
        "elapsed_ms",
        "avg_ns",
        "throughput",
    ]

    for column in numeric_columns:
        df[column] = pd.to_numeric(
            df[column],
            errors="raise",
        )

    summary = create_summary(df)

    OUTPUT_DIR.mkdir(
        parents=True,
        exist_ok=True,
    )

    summary.to_csv(
        SUMMARY_FILE,
        index=False,
        float_format="%.3f",
    )

    save_yield_latency_chart(summary)
    save_yield_errorbar_chart(summary)
    save_throughput_chart(summary)
    save_elapsed_chart(summary)

    # 使用原始 benchmark 資料繪圖，
    # 而不是使用已聚合的 summary。
    save_yield_histogram(df)
    save_yield_run_sequence(df)

    display_columns = [
        "tasks",
        "runs",
        "yield_mean_ns",
        "yield_std_ns",
        "yield_min_ns",
        "yield_max_ns",
        "yield_p95_ns",
        "yield_p99_ns",
        "throughput_mean",
    ]

    print()
    print("Scheduler Benchmark Summary")
    print("=" * 120)

    print(
        summary[display_columns].to_string(
            index=False,
            float_format=lambda value: (
                f"{value:.3f}"
            ),
        )
    )

    print()
    print(
        f"Summary CSV : {SUMMARY_FILE}"
    )
    print(
        f"Figures     : {OUTPUT_DIR}/"
    )
    print()
    print("Generated files:")

    for file_path in sorted(
        OUTPUT_DIR.iterdir()
    ):
        print(f"  - {file_path}")


if __name__ == "__main__":
    main()