#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Grouped bar charts for 4 apps (bfs, cc, dc, dfs) and 3 configs (baseline, cont-pmd, ideal).
Figure 1: absolute values
Figure 2: relative to baseline (baseline normalized to 1.0)

Usage:
  - Edit the DATA dict below with your absolute numbers, OR
  - Provide a CSV via --csv path with columns:
        app,baseline,cont_pmd,ideal
    Example row:
        bfs,120.5,110.2,95.0
"""

from collections import OrderedDict
import argparse
import csv
import numpy as np
import matplotlib.pyplot as plt

# --- 1) Configure your data here if not using CSV ---
# Replace the placeholder numbers with your measurements (absolute units).
DATA = OrderedDict({
    "bfs":  [2.88, 2.59, 2.54],   # [baseline, cont-pmd, ideal]
    "cc":   [3.08, 2.76, 2.74],
    "dc":   [5.68,  5.03,  4.98],
    "dfs":  [2.93, 2.63, 2.56],
})
DATA = OrderedDict({
    "bfs":  [2.88, 2.55, 2.54],   # [baseline, cont-pmd, ideal]
    "cc":   [3.08, 2.75, 2.74],
    "dc":   [5.68,  4.99,  4.98],
    "dfs":  [2.93, 2.57, 2.56],
})

APPS = ["bfs", "cc", "dc", "dfs"]
CONFIGS = ["baseline", "cont-pmd", "ideal"]  # fixed order

def load_from_csv(path):
    # Expect columns: app,baseline,cont_pmd,ideal
    temp = {}
    with open(path, newline="") as f:
        reader = csv.DictReader(f)
        required = {"app", "baseline", "cont_pmd", "ideal"}
        missing = required - set(h.strip().lower() for h in reader.fieldnames or [])
        if missing:
            raise ValueError(f"CSV missing columns: {missing}")
        for row in reader:
            app = row["app"].strip()
            # store as float list in CONFIGS order
            vals = [
                float(row["baseline"]),
                float(row["cont_pmd"]),
                float(row["ideal"]),
            ]
            temp[app] = vals

    # Keep canonical app order; include only those present
    ordered = OrderedDict()
    for app in APPS:
        if app in temp:
            ordered[app] = temp[app]
    if not ordered:
        raise ValueError("No known apps found in CSV. Expected one of: " + ", ".join(APPS))
    return ordered

def normalize_to_baseline(ordered_data):
    rel = OrderedDict()
    for app, vals in ordered_data.items():
        baseline = vals[0]
        if baseline == 0:
            raise ValueError(f"Baseline is zero for app '{app}', cannot normalize.")
        rel[app] = [v / baseline for v in vals]
    return rel

def annotate_bars(ax, rects, fmt="{:.2f}", y_offset=0.01):
    for r in rects:
        height = r.get_height()
        ax.annotate(fmt.format(height),
                    xy=(r.get_x() + r.get_width()/2, height),
                    xytext=(0, max(y_offset*height, 0.01)),
                    textcoords="offset points",
                    ha='center', va='bottom', fontsize=9, rotation=0)

def plot_grouped_bars(ordered_data, title, ylabel, rel=False, save_path=None):
    apps = list(ordered_data.keys())
    values = np.array(list(ordered_data.values()))  # shape: (n_apps, 3)

    n_apps = len(apps)
    n_cfgs = len(CONFIGS)
    x = np.arange(n_apps)
    width = 0.22  # bar width

    fig, ax = plt.subplots(figsize=(9, 4.8))
    bars = []
    for i in range(n_cfgs):
        # center the groups around each x
        offset = (i - (n_cfgs - 1)/2) * width
        rects = ax.bar(x + offset, values[:, i], width, label=CONFIGS[i])
        bars.append(rects)

    ax.set_title(title)
    ax.set_xticks(x, apps)
    ax.set_ylabel(ylabel)
    ax.legend(ncols=3, loc="upper center", bbox_to_anchor=(0.5, 1.15))
    ax.grid(axis='y', linestyle=':', alpha=0.6)

    # Annotate values on bars
    if rel:
        for rects in bars:
            annotate_bars(ax, rects, fmt="{:.2f}×")
        # Reference line at 1.0 (baseline)
        ax.axhline(1.0, linestyle='--', linewidth=1)
        ax.set_ylim(bottom=0)
    else:
        for rects in bars:
            annotate_bars(ax, rects, fmt="{:.2f}")
        ax.set_ylim(bottom=0)

    fig.tight_layout()
    if save_path:
        fig.savefig(save_path, dpi=200, bbox_inches="tight")
    return fig, ax

def main():
    parser = argparse.ArgumentParser(description="Plot absolute and relative grouped bars for apps/configs.")
    parser.add_argument("--csv", type=str, default=None, help="CSV file with columns: app,baseline,cont_pmd,ideal")
    parser.add_argument("--abs-out", type=str, default="bars_absolute.png", help="Output PNG for absolute plot")
    parser.add_argument("--rel-out", type=str, default="bars_relative.png", help="Output PNG for relative plot")
    parser.add_argument("--ylabel", type=str, default="Runtime (ms)", help="Y-axis label for absolute plot")
    args = parser.parse_args()

    if args.csv:
        data = load_from_csv(args.csv)
    else:
        data = DATA  # from the dict at the top

    # Absolute
    plot_grouped_bars(
        data,
        title="Absolute Results by Application and Configuration",
        ylabel=args.ylabel,
        rel=False,
        save_path=args.abs_out
    )

    # Relative (normalized to baseline per app)
    rel_data = normalize_to_baseline(data)
    plot_grouped_bars(
        rel_data,
        title="Relative to Baseline (Baseline = 1.0)",
        ylabel="Relative (× baseline)",
        rel=True,
        save_path=args.rel_out
    )

    plt.savefig("plot-res.png")

if __name__ == "__main__":
    main()
