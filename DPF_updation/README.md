<div align="center">
<h1>DPF‑Augmented 2‑Party MPC Protocol</h1>
<em>Private item index selection via Distributed Point Functions (DPFs) for user & item profile updates</em>
</div>

---

## 1. High‑Level Goal

We have a recommendation‑style setting:

- `m` users, `n` items.
- Each user and item is represented as a length‑`k` vector over `ℤ_mod` (`mod = 1 000 000 007`).
- A client issues `Q` queries, each specifying:
  - a user index `i`,
  - a (secret) item index `j`,
  - and an implicit rating signal.

Two semi‑honest servers `P0` and `P1` hold additive shares of all user and item vectors and want to:

1. Select item `j` *without revealing `j`* to any party.
2. Update the chosen **user profile** and corresponding **item profile**.
3. Let an offline verifier check correctness.

The secret item index `j` is encoded via a **Distributed Point Function (DPF)**, instead of a one‑hot vector.

---

## 2. Roles and Executables

- **P2 (Triple Server)** – provides Beaver triples to `P0` and `P1`.
  - Code: `p2.cpp`
- **P0, P1 (MPC Parties)** – hold additive shares of users/items and run the DPF‑based MPC update.
  - Code: `pB.cpp`, `mpc.hpp`, `shares.hpp`, `utility.hpp`, `DPF.hpp`, `DPF.cpp`
- **Data Generator** – creates initial random data and DPF keys for all queries.
  - Code: `gen_data.cpp`
- **Verifier** – reconstructs outputs, runs a cleartext “direct” update, and compares.
  - Code: `verify.cpp`

All are built and run inside Docker (see `Dockerfile` and `docker-compose.yml`).

---

## 3. DPF Key Generation Protocol

### 3.1 DPF Functionality

We use a 2‑party DPF for a point function:

\[
f_{j,v}: \{0, \dots, n-1\} \rightarrow \mathbb{Z}_{\text{mod}}
\]

such that:

- \( f_{j,v}(j) = v \) (non‑zero value at index `j`)
- \( f_{j,v}(x) = 0 \) for all \( x \ne j \)

A DPF key pair `(k0, k1)` is generated so that:

- Party `P0` holds key `k0`
- Party `P1` holds key `k1`
- For every index `x`, the evaluations satisfy:

\[
f_0(x) + f_1(x) \equiv f_{j,v}(x) \pmod{\text{mod}}
\]

Therefore, the **additive reconstruction** of the two evaluations yields the point function.

### 3.2 Key Generation

Key generation is implemented in `DPF.cpp` / `DPF.hpp` and invoked from `gen_data.cpp`.

- A binary tree over the domain `{0,…,n-1}` is built conceptually.
- Starting from a common root seed, we expand down the tree to depth `⌈log₂ n⌉`.
- At each level, we:
  - derive left/right child seeds via a PRG,
  - add a **correction word** (`correctionWord` in `DPF.hpp`) so that:
    - at index `j`, the two parties’ evaluations add to `v`,
    - elsewhere they add to `0`.

The size of a DPF key is proportional to the tree depth:

\[
|\text{key}| = O(\log n)
\]

and key generation cost is:

\[
T_{\text{genKey}}(n) = O(\log n)
\]

This generation happens **once per query** during data generation (`gen_data`), not during the MPC online phase.

### 3.3 DPF Evaluation

Each party locally evaluates its DPF key at all item indices in the MPC protocol.

For index `x`:

1. Start from the key’s root seed.
2. Walk down the binary tree according to the bits of `x`.
3. Apply the stored correction words and advice bits.
4. Recover the share value `f_b(x)` for party `b ∈ {0,1}`.

The per‑index evaluation cost is:

\[
T_{\text{eval}}(n) = O(\log n)
\]

Thus, generating a DPF key and evaluating it at one specific index `x` are both `O(log n)`.

---

## 4. MPC Protocol with DPF

The MPC protocol uses the DPF keys as follows (high level):

1. **Offline / Data Generation** (`gen_data.cpp`)
   - Generate random initial user vectors `U[i]` and item vectors `V[j]`.
   - For each of the `Q` queries with secret item index `j` and update value `v`:
     - Generate a DPF key pair `(k0, k1)` for `f_{j,v}`.
     - Save keys and query metadata to disk under `data/`.

2. **Online / Protocol Execution** (`pB.cpp`, `mpc.hpp`)
   - `P0` and `P1` read their corresponding DPF keys and shares of `U` and `V`.
   - For each query:
     1. Each party evaluates its DPF key on the item index domain to obtain:
        - A *share* of “selection coefficients” indicating the chosen item.
        - Evaluation cost per index `O(log n)`; over all `n` items it is effectively `O(n log n)`.
     2. Using these shared coefficients plus Beaver triples from `P2`, they:
        - Update the **user profile** share for user `i`.
        - Update the **selected item profile** share for item `j`.
     3. Re‑share / refresh user shares when needed (to avoid leakage).

3. **Verification** (`verify.cpp`)
   - Reconstruct outputs from `P0` and `P1`.
   - Replay all queries in the clear (without DPF) and perform direct updates.
   - Compare only **updated users and items**, printing “Matched/Not matched” for those indices.

---

## 5. Time Complexity
Let:
- `n` = number of items,
- `k` = profile dimension,
- `Q` = number of queries.

### 5.1 DPF Key Generation
Performed once per query in `gen_data`:

- **Per key (per query):** `O(log n)`
- **For all `Q` queries:** `O(Q log n)`

This is purely offline and does not involve communication between `P0`/`P1` (keys are generated centrally and written to disk).

### 5.2 MPC Online Phase (Per Query)

In your current code:

- Each party evaluates the DPF key on all `n` items to construct the selection coefficients.
- Per item DPF evaluation cost is `O(log n)`, so the selection phase is `O(n log n)` per query.
- The subsequent secure arithmetic (Beaver triple‑based scalar/vector operations) is `O(n·k)` and dominates when `k` is reasonably large.

So:

- **DPF part (as a function of `n`):** `O(n log n)` per query.
- **Overall MPC update per query (including secure arithmetic):** `O(n·k)` field ops and communication, with an extra `log n` factor in the DPF evaluation.

The key point you care about for the DPF itself:

> **DPF key size and generation / single‑point evaluation are `O(log n)` in the domain size.**

---

## 6. How to Build and Run

All commands below are from the `A3/` directory.

### 6.1 Simple local run (helper script)

```bash
./run.sh 100 200 2 6
# NUM_USERS=100, NUM_ITEMS=200, NUM_FEATURES=2, NUM_QUERIES=6
```

This script just echoes the configuration; the actual MPC protocol is run inside Docker.

### 6.2 Docker‑based workflow

```bash
# 1) Build images
docker-compose build

# 2) Generate data (users/items + DPF keys)
NUM_USERS=100 NUM_ITEMS=200 NUM_FEATURES=2 NUM_QUERIES=6 \
docker-compose run --rm gen_data

# 3) Run the MPC parties and triple server
docker-compose up p2 p1 p0

# 4) Run verifier (reconstruct & check updated users/items)
docker-compose run --rm verify

# 5) Tear down
docker-compose down -v
```

Outputs are written under `A3/data/`.

---

## 7. Benchmark Scripts

Run the benchmark driver in `A4/eval.py`:

```bash
cd A4
python3 eval.py --vary <queries|items|users> [options]
```

Key flags:
- `--users / --items / --features / --queries`: baseline sizes.
- `--queries-list`, `--items-list`, `--users-list`: comma-separated sweep values.
- `--prebuilt`: skip rebuilding Docker images.
- `--outdir`: where CSV/plots are written (default `../A3/data`).

Example invocations:

```bash
python3 eval.py --vary queries --queries-list 50,100,150,200 --users 100 --items 200 --features 40 --prebuilt
python3 eval.py --vary items --items-list 50,100,150,200 --users 100 --features 40 --queries 50
python3 eval.py --vary users --users-list 50,100,150,200 --items 200 --features 40 --queries 50
```

Each run performs `gen_data`, `(p2,p1,p0)` execution, `verify`, then produces CSV + PNG plots under the chosen output directory.

---
