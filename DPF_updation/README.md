# MPC Communication – Secure Update with Verification [Private j(item index) via One‑Hot Selection]

This project implements a 2‑party MPC protocol (P0, P1) with a helper (P2) to update user vectors securely:
u_i' = u_i + v_j · (1 − <u_i, v_j>)  over Z_p.

The item index j is kept private using a secret‑shared one‑hot selector s (length n) with s_j = 1. The parties obliviously select v_j by computing v_sel = V^T s via MPC dot products; no party learns j.

## Prerequisites
- Docker and Docker Compose
- Bash (macOS/Linux)

## Run
Use the run.sh script (builds images, runs MPC services, then runs the verifier last):
- ./run.sh "users" "items" "features" "queries"

Example:
- ./run.sh 300 300 30 1000

## Overview of the Protocol
1) gen_data generates:
   - U0.txt, U1.txt (user shares)
   - V0.txt, V1.txt (item shares)
   - queries.txt (full user,item pairs; used by verifier only)
   - queries_users.txt (only user indices; used by MPC parties)
   - S0.txt, S1.txt (additive shares of one‑hot selector per query; each line has n numbers)
2) p2 provides Beaver triples when requested by P0/P1 (role handshake ensures P0=0, P1=1; only P0 sends the triple count).
3) p0 and p1 (for each query):
   - Oblivious selection: compute v_sel = V^T s using k secure dot products over n (j remains hidden).
   - Secure update: compute u_i' = u_i + v_sel · (1 − <u_i, v_sel>) using secure dot and secure scalar–vector product.
   - Reconstruct u_i' (for verification only), then re‑share fresh random shares.
4) p0 writes:
   - mpc_results.txt (final reconstructed user vectors for touched users)
   - mpc_results.done (flag to signal completion)
5) verify runs last, recomputes the final U directly from U0+U1, V0+V1 and queries.txt, compares per user, and prints all “Matched/Not matched” lines at the end.

## Secret‑sharing implementation
- Ring: additive shares over Z_p with p = 1,000,000,007 (prime).
- Share of x: P0 holds x0, P1 holds x1 with x0 + x1 = x (mod p).
- Vectors are shared element‑wise; every +, −, · reduces mod p.
- Reconstruction: x = (x0 + x1) mod p (only used by P0 to persist verification outputs).
- Re‑sharing after reconstruction: P0 samples r ∈ Z_p^k, sets P1’s share to (x − r), sends it; both overwrite local shares.

## How inner products and updates are computed securely
All multiplications use Beaver triples from P2.

- Beaver triple: (a, b, c) with c = a·b (mod p), additively shared between P0 and P1.
- Secure dot product z = <x, y> (length L):
  - Each party has x_b, y_b and triple shares a_b, b_b, c_b.
  - Open masked differences e = (x − a) and f = (y − b) (1 round trip).
  - Output share: z_b = c_b + e·b_b + f·a_b + (b==P0 ? e·f : 0)  (all mod p).
- Secure scalar–vector product w = s·v:
  - Same as above, reusing one scalar a across coordinates; per coordinate use (a, b_i, c_i).
- Secure update u' = u + v · (1 − <u, v>):
  - t = <u, v> via secure dot.
  - δ = 1 − t (shares: P0 adds 1; both subtract t).
  - w = δ·v via secure scalar–vector product.
  - u' = u + w.

## Communication rounds and efficiency considerations
Per query (one‑hot selection):
- Oblivious selection (v_sel = V^T s):
  - k secure dot products of length n.
  - Each dot product: 1 round trip to open e,f; messages O(n) field elements per party.
- Update step:
  - 1 secure dot product (length k): 1 round, O(k) elems.
  - 1 secure scalar–vector product (length k): 1 round, O(k) elems.
- Reconstruction and re‑share:
  - 1 send (P0→P1) with O(k) elems.
- P2 triples per query:
  - O(k·n) for selection + O(k) for update.
- Pipelining/batching:
  - The k column dot‑products for selection can be pipelined to reduce latency.

## Outputs (in ./data)
- U0.txt, U1.txt, V0.txt, V1.txt, queries.txt, queries_users.txt
- S0.txt, S1.txt (selector shares)
- mpc_results.txt: per line “user_idx val0 ... val{k‑1}”
- mpc_results.done: completion flag for verifier

## Terminal output
- Logs from gen_data, p2, p1, p0 stream first.
- After MPC finishes, verify prints:
  - Verifying MPC results against direct update for updated users
  - User X: Matched / Not matched
  - All compared users matched between MPC and direct update (if all matched)

## Time complexity
Let m users, n items, k features, Q queries.
- Per query: O(k·n) time/comm (k dot products across n items) for selection + O(k) for update ⇒ O(k·n).
- Over Q queries: O(Q·k·n) time and communication, O(Q·k·n) Beaver triples.
- Memory per party: O((m + n)·k).
