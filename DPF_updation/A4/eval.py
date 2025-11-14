#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import os
import shutil
import subprocess
import time
from pathlib import Path
from typing import Dict, List, Sequence

import matplotlib.pyplot as plt

REPO_ROOT = Path(__file__).resolve().parent
OUT_DIR_DEFAULT = REPO_ROOT / "data"


def run_cmd(args: Sequence[str], env: Dict[str, str], cwd: Path) -> None:
    print(f"[CMD] {' '.join(args)}")
    subprocess.run(args, check=True, env=env, cwd=cwd)


def clean_data_dir(outdir: Path) -> None:
    if outdir.exists():
        shutil.rmtree(outdir)
    outdir.mkdir(parents=True, exist_ok=True)


def compose_env(m: int, n: int, k: int, q: int) -> Dict[str, str]:
    env = os.environ.copy()
    env.update(
        {
            "NUM_USERS": str(m),
            "NUM_ITEMS": str(n),
            "NUM_FEATURES": str(k),
            "NUM_QUERIES": str(q),
        }
    )
    return env


def compose_down(env: Dict[str, str]) -> None:
    try:
        run_cmd(["docker-compose", "down", "-v"], env=env, cwd=REPO_ROOT)
    except subprocess.CalledProcessError:
        print("[WARN] docker-compose down failed; continuing.")


def compose_build(env: Dict[str, str]) -> None:
    run_cmd(["docker-compose", "build"], env=env, cwd=REPO_ROOT)


def compose_run_gen_data(env: Dict[str, str]) -> None:
    run_cmd(["docker-compose", "run", "--rm", "gen_data"], env=env, cwd=REPO_ROOT)


def compose_up_protocol(env: Dict[str, str]) -> None:
    run_cmd(
        ["docker-compose", "up", "--abort-on-container-exit", "p2", "p1", "p0"],
        env=env,
        cwd=REPO_ROOT,
    )


def compose_run_verify(env: Dict[str, str]) -> None:
    run_cmd(["docker-compose", "run", "--rm", "verify"], env=env, cwd=REPO_ROOT)


def run_pipeline(m: int, n: int, k: int, q: int, prebuilt: bool) -> float:
    env = compose_env(m, n, k, q)
    compose_down(env)
    if not prebuilt:
        compose_build(env)
    clean_data_dir(OUT_DIR_DEFAULT)
    t0 = time.perf_counter()
    compose_run_gen_data(env)
    compose_up_protocol(env)
    compose_run_verify(env)
    duration = time.perf_counter() - t0
    compose_down(env)
    return duration


def parse_list(arg: str) -> List[int]:
    if not arg:
        return []
    return [int(x) for x in arg.split(",") if x.strip()]


def save_csv(path: Path, rows: List[Dict[str, float | int | str]], header: List[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=header)
        writer.writeheader()
        writer.writerows(rows)


def plot_lines(
    x: Sequence[int],
    y_map: Dict[str, Sequence[float]],
    xlabel: str,
    ylabel: str,
    title: str,
    out_path: Path,
) -> None:
    plt.figure(figsize=(7, 4))
    for label, ys in y_map.items():
        plt.plot(x, ys, marker="o", label=label)
    plt.xlabel(xlabel)
    plt.ylabel(ylabel)
    plt.title(title)
    plt.grid(True, linestyle="--", alpha=0.4)
    plt.legend()
    out_path.parent.mkdir(parents=True, exist_ok=True)
    plt.tight_layout()
    plt.savefig(out_path, dpi=150)
    plt.close()


def read_timings(path: Path) -> tuple[float, float, int]:
    if not path.exists():
        raise FileNotFoundError(f"{path} missing (ensure p0 wrote timings.txt).")
    item_total = user_total = 0.0
    count = 0
    with path.open() as f:
        next(f)
        for line in f:
            _, item_us, user_us = line.strip().split(",")
            item_total += float(item_us)
            user_total += float(user_us)
            count += 1
    return item_total / 1e6, user_total / 1e6, count


def vary_queries(m: int, n: int, k: int, queries_list: List[int], prebuilt: bool, outdir: Path) -> None:
    if not queries_list:
        raise ValueError("queries_list must contain at least one value.")
    rows: List[Dict[str, float | int]] = []
    per_user: List[float] = []
    per_item: List[float] = []

    for q in queries_list:
        print(f"[INFO] Running pipeline for queries={q}")
        total_time = run_pipeline(m, n, k, q, prebuilt)
        user_total = total_time 
        item_total = total_time 
        per_user_time = user_total / max(m, 1)
        per_item_time = item_total / max(n, 1)

        rows.append(
            {
                "queries": q,
                "users": m,
                "items": n,
                "features": k,
                "total_time_s": total_time,
                "user_total_time_s": user_total,
                "item_total_time_s": item_total,
                "user_time_per_user_s": per_user_time,
                "item_time_per_item_s": per_item_time,
            }
        )
        per_user.append(per_user_time)
        per_item.append(per_item_time)

    csv_path = outdir / "bench_queries.csv"
    save_csv(
        csv_path,
        rows,
        [
            "queries",
            "users",
            "items",
            "features",
            "total_time_s",
            "user_total_time_s",
            "item_total_time_s",
            "user_time_per_user_s",
            "item_time_per_item_s",
        ],
    )
    plot_lines(
        queries_list,
        {
            "Per-user update time (s)": per_user,
            "Per-item update time (s)": per_item,
        },
        xlabel="# Queries",
        ylabel="Seconds",
        title="Per-update time vs. #queries",
        out_path=outdir / "bench_queries.png",
    )
    print(f"[INFO] Saved CSV -> {csv_path}")
    print(f"[INFO] Saved plot -> {outdir / 'bench_queries.png'}")
    


def vary_items(m: int, items_list: List[int], k: int, q: int, prebuilt: bool, outdir: Path) -> None:
    if not items_list:
        raise ValueError("items_list must contain at least one value.")
    rows: List[Dict[str, float | int]] = []
    per_user: List[float] = []
    per_item: List[float] = []

    for n in items_list:
        print(f"[INFO] Running pipeline for items={n}")
        total_time = run_pipeline(m, n, k, q, prebuilt)
        item_total, user_total, updates = read_timings(REPO_ROOT / "data" / "timings.txt")
        per_user_time = user_total / max(updates, 1)
        per_item_time = item_total / max(updates, 1)

        rows.append(
            {
                "queries": q,
                "users": m,
                "items": n,
                "features": k,
                "total_time_s": total_time,
                "user_total_time_s": user_total,
                "item_total_time_s": item_total,
                "user_time_per_user_s": per_user_time,
                "item_time_per_item_s": per_item_time,
            }
        )
        per_user.append(per_user_time)
        per_item.append(per_item_time)

    csv_path = outdir / "bench_items.csv"
    save_csv(
        csv_path,
        rows,
        [
            "queries",
            "users",
            "items",
            "features",
            "total_time_s",
            "user_total_time_s",
            "item_total_time_s",
            "user_time_per_user_s",
            "item_time_per_item_s",
        ],
    )
    plot_lines(
        items_list,
        {
            "Per-user update time (s)": per_user,
            "Per-item update time (s)": per_item,
        },
        xlabel="# Items",
        ylabel="Seconds",
        title="Per-update time vs. #items",
        out_path=outdir / "bench_items.png",
    )
    print(f"[INFO] Saved CSV -> {csv_path}")
    print(f"[INFO] Saved plot -> {outdir / 'bench_items.png'}")


def vary_users(users_list: List[int], n: int, k: int, q: int, prebuilt: bool, outdir: Path) -> None:
    if not users_list:
        raise ValueError("users_list must contain at least one value.")
    rows: List[Dict[str, float | int]] = []
    per_user: List[float] = []
    per_item: List[float] = []

    for m in users_list:
        print(f"[INFO] Running pipeline for users={m}")
        total_time = run_pipeline(m, n, k, q, prebuilt)
        user_total = total_time
        item_total = total_time
        per_user_time = user_total / max(m, 1)
        per_item_time = item_total / max(n, 1)

        rows.append(
            {
                "queries": q,
                "users": m,
                "items": n,
                "features": k,
                "total_time_s": total_time,
                "user_total_time_s": user_total,
                "item_total_time_s": item_total,
                "user_time_per_user_s": per_user_time,
                "item_time_per_item_s": per_item_time,
            }
        )
        per_user.append(per_user_time)
        per_item.append(per_item_time)

    csv_path = outdir / "bench_users.csv"
    save_csv(
        csv_path,
        rows,
        [
            "queries",
            "users",
            "items",
            "features",
            "total_time_s",
            "user_total_time_s",
            "item_total_time_s",
            "user_time_per_user_s",
            "item_time_per_item_s",
        ],
    )
    plot_lines(
        users_list,
        {
            "Per-user update time (s)": per_user,
            "Per-item update time (s)": per_item,
        },
        xlabel="# Users",
        ylabel="Seconds",
        title="Per-update time vs. #users",
        out_path=outdir / "bench_users.png",
    )
    print(f"[INFO] Saved CSV -> {csv_path}")
    print(f"[INFO] Saved plot -> {outdir / 'bench_users.png'}")


def main() -> None:
    parser = argparse.ArgumentParser(description="Benchmark MPC pipeline.")
    parser.add_argument("--vary", choices=["queries", "items", "users"], required=True)
    parser.add_argument("--users", type=int, default=100)
    parser.add_argument("--items", type=int, default=200)
    parser.add_argument("--features", type=int, default=40)
    parser.add_argument("--queries", type=int, default=50)
    parser.add_argument("--queries-list", type=str, default="")
    parser.add_argument("--items-list", type=str, default="")
    parser.add_argument("--users-list", type=str, default="")
    parser.add_argument("--prebuilt", action="store_true", help="Skip docker-compose build step.")
    parser.add_argument("--outdir", type=Path, default=OUT_DIR_DEFAULT)
    args = parser.parse_args()

    if args.vary == "queries":
        vary_queries(
            args.users,
            args.items,
            args.features,
            parse_list(args.queries_list),
            args.prebuilt,
            args.outdir,
        )
    elif args.vary == "items":
        vary_items(
            args.users,
            parse_list(args.items_list),
            args.features,
            args.queries,
            args.prebuilt,
            args.outdir,
        )
    else:
        vary_users(
            parse_list(args.users_list),
            args.items,
            args.features,
            args.queries,
            args.prebuilt,
            args.outdir,
        )


if __name__ == "__main__":
    main()