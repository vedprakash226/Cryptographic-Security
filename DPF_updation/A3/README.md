<div align="center">
<h1>DPF‑Augmented 2‑Party MPC Protocol</h1>
<em>Private item index j via Distributed Point Function (DPF) for silent item & user updates</em>
</div>

## What’s new (DPF vs. One‑Hot)
- We remove explicit one‑hot selector shares (S0/S1) for selecting the secret item index j.
- We use the DPF material (`DPF0.txt`, `DPF1.txt`, `DPF_NEG.txt`) to: (a) select \(v_j\) and (b) inject the item update at the hidden index j via a linear combination.
- `S0.txt`/`S1.txt` may still be generated for backwards compatibility but are ignored by this DPF‑based flow.

## 1) Goal and Setting
We implement the coupled updates over the prime field \(\mathbb{Z}_p\) with \(p=10^9+7\):

\[\begin{aligned}
u_i' &= u_i + v_j\,(1 - \langle u_i, v_j \rangle),\\
v_j' &= v_j + u_i\,(1 - \langle u_i, v_j \rangle),
\end{aligned}\]

with the item index \(j\) hidden from both computing parties P0 and P1. A helper P2 supplies Beaver triples only (no data leakage of inputs). The user update uses secure dot and scalar–vector product. The item update is applied at the secret index using a DPF‑derived sign vector and a linear combination across items (no one‑hot shares).

### Roles
- P0 and P1: compute over additive shares, communicate with each other and with P2.
- P2: provides Beaver triples upon request; learns nothing about data.
- User (offline generation step): prepares queries and DPF keys.

## 2) Protocol at a Glance (per query)
1. Data generation (`gen_data.cpp`): writes additive shares of U and V, the user indices (`queries_users.txt`), public pairs for verifier (`queries.txt`), DPF keys (`DPF0.txt`, `DPF1.txt`) and a negate hint (`DPF_NEG.txt`).
2. DPF‑based selection of \(v_j\): evaluate a pseudo‑random sign vector \(s\in\{\pm1\}^n\) from each key, convert to coefficients \(c = s/2\) (mod p), form a linear combination of item rows. Across parties, coefficients sum to 1 at j and to 0 elsewhere, so shares add to \(v_j\).
3. Compute \(t = \langle u_i, v_j \rangle\) securely; let \(\delta = 1 - t\).
4. Compute the shared vector \(M = u_i\,\delta\) (for the item update) and the shared vector \(v_j\,\delta\) (for the user update).
5. Apply item update at j using the same DPF sign vector mechanism to inject \(M\) at the hidden position.
6. Finalize: write reconstructed results for verification.

Notes on masking: the current implementation repurposes the final correction word (FCW) shares from the DPF keys as additive masks; the sum of FCW shares is 0 (zero‑payload construction). This keeps arithmetic consistent while not revealing \(M\).

## 3) Code Map
- DPF core (`DPF.cpp`, `DPF.hpp`): `generateDPF`, `evalFlagAt`, `evalSigns`, `writeKey`, `readKey`.
- Beaver triples server (`p2.cpp`): accepts two sockets (P0, P1), serves triples upon request from P0.
- Party logic (`pB.cpp` + `mpc.hpp`):
	- Connections: peer link (P0↔P1) and P2 link with role handshake.
	- MPC ops: `MPC_DOTPRODUCT`, `scalarVecProd` (with triples), `select_item_oblivious`, `DPF_select_item`, `updateProtocol`, `itemUpdateShare`.
	- I/O: reads U/V shares, users list, DPF keys and negate bits; writes `mpc_results.txt`, `mpc_V_results.txt`, `mpc_results.done`.
- Verification (`verify.cpp`): reconstructs initial U,V, replays each query directly with the same formula, compares per‑index vectors against MPC outputs.

## 4) Files Produced in `data/`
| File | Purpose |
|------|---------|
| `U0.txt`, `U1.txt` | Additive shares of user vectors (dimension k). |
| `V0.txt`, `V1.txt` | Additive shares of item vectors (dimension k). |
| `S0.txt`, `S1.txt` | Legacy one‑hot selector shares; ignored by DPF flow. |
| `queries.txt` | Verifier‑only (clear) pairs `(user,item)`. |
| `queries_users.txt` | User indices only (consumed by MPC parties). |
| `DPF0.txt`, `DPF1.txt` | Per‑query DPF keys (text serialization). |
| `DPF_NEG.txt` | Per‑query bit: which party negates its sign vector (alignment). |
| `mpc_results.txt` | Reconstructed updated users after all queries. |
| `mpc_V_results.txt` | Reconstructed updated items after all queries. |
| `mpc_results.done` | Completion flag for the verifier. |

## 5) DPF Key Format (one line per key)
```
depth seed t0 final_cw  [cw_0 leftAdv_0 rightAdv_0  cw_1 leftAdv_1 rightAdv_1 ...]
```
Notes:
- `final_cw` is an additive share (mod p) with shares summing to 0 (zero payload); reused as a mask in the protocol.
- Advice bits and correction words reconstruct the traversal; flags induce the \{±1\} sign vector.

## 6) XOR → Additive Conversion (current, insecure simplification)
Native DPFs are XOR‑based. We convert to additive by globally negating exactly one party’s sign vector so that at j the two signs add to +2 and elsewhere cancel to 0. We record the negating party in `DPF_NEG.txt`. This leaks one bit if generated adversarially and is acceptable for coursework; future work: private alignment via OT/MPC.

After alignment:
\[
	ext{share}_0(j) + \text{share}_1(j) = M,\quad \text{share}_0(l)+\text{share}_1(l)=0\; (l \neq j).
\]

### Selection of \(v_j\) without one‑hot
Let `signs = evalSigns(key, n, negate)` with entries in \{±1\}. Define coefficients \(c_t = \tfrac{\text{signs}[t]}{2}\; (\bmod\,p)\). Each party computes the linear combination
\(\;v\_{\text{sel},b} = \sum_{t=0}^{n-1} c_t \cdot V_b[t].\)
Across parties this sums to \(v_j\).

## 7) Secure Primitives Recap
All multiplications rely on Beaver triples from P2:
- Dot product \( \langle x, y \rangle \).
- Scalar–vector product \( s\cdot v \).

Dot product share for party \(b\in\{0,1\}\):
\[ z_b = c_b + e\,b_b + f\,a_b + [b=0] \, e f \pmod p, \]
with openings \(e = x - a\), \(f = y - b\).

## 8) How to Run
The repository is containerized. Use the helper script which builds images, generates data, runs the parties, then runs the verifier.

```bash
./run.sh <num_users> <num_items> <num_features> <num_queries>
```

Example:
```bash
./run.sh 100 200 2 6
```

The script:
1. Rebuilds containers (gen_data, p2, p1, p0, verify).
2. Generates inputs and DPF keys into `./data`.
3. Runs MPC services (p2 → p1/p0) to completion.
4. Runs verifier; prints whether items and users match the direct computation.

Outputs appear under `./data` as listed above.

## 9) Complexity (current implementation)
Let m users, n items, k features, Q queries.
- DPF selection and injection: linear combination across all items ⇒ \(O(n\,k)\) ops per query.
- User update: one dot + one scalar–vector ⇒ \(O(k)\).
- Total per query dominated by the selection linear combination: \(O(n\,k)\).
Future optimization: sublinear selection via PIR/DPF table techniques or packing; out of scope here.

## 10) Verification
`verify` waits for `mpc_results.done`, reconstructs initial U and V, replays each `(u_i, v_j)` from `queries.txt` with the same equations, and compares to `mpc_results.txt` (users) and `mpc_V_results.txt` (items). It reports mismatches with indices and coordinates.

## 11) Security Notes & Hardening Ideas
| Aspect | Current | Improvement |
|--------|---------|-------------|
| Global negation bit | Insecure (user chooses) | MPC/OT‑based private alignment |
| DPF payload | Zero; reuse FCW as mask | Non‑zero payload DPF for richer updates |
| Triples source (P2) | Honest‑but‑curious helper | OT‑based preprocessing, MACs (SPDZ) |
| Reconstruction | P0 reconstructs results | Batch/final‑only reconstruction |

## 12) Troubleshooting
| Issue | Likely cause | Fix |
|------|--------------|-----|
| Verifier mismatch | Wrong negate bits | Regenerate data; ensure `DPF_NEG.txt` aligns with keys |
| Parties hang | P2 not accepting or role handshake failed | Ensure `p2` service is up before `p1/p0` |
| No outputs | Containers rebuilt but old `data/` lingering | The script removes `data/` before run; rerun `./run.sh` |

## 13) License / Academic Use
For academic experimentation with DPF‑assisted MPC. Not production‑hardened.

—
Contributions (secure alignment, packed selection, testing) are welcome.
