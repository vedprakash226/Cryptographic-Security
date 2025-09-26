# MPC Communication – Secure Update with Verification

This project implements a 2-party MPC protocol (P0, P1) with a helper (P2) to update user vectors securely:
u_i' = u_i + v_j * (1 - <u_i, v_j>)

It supports private item index selection in two ways:
1) MPC one-hot selection (default): j is hidden by using a secret-shared one-hot vector s and computing v_sel = V^T s with MPC dot products.
2) OT-extension based private index (optional): j is hidden using 1-out-of-N Oblivious Transfer Extension, reducing per-query cost.

## Prerequisites
- Docker and Docker Compose
- Bash (macOS/Linux)

## Run
Use the run.sh script (builds images, runs MPC services, then runs the verifier last)
- ./run.sh "users" "items" "features" "queries"

Example:
- ./run.sh 300 300 30 1000


## Overview of the Protocol
1) gen_data generates:
   - U0.txt, U1.txt (user shares)
   - V0.txt, V1.txt (item shares)
   - queries.txt (full user,item pairs; used by verifier only)
   - queries_users.txt (only user indices; used by MPC parties)
   - S0.txt, S1.txt (additive shares of one-hot selector per query; length n per line) [only for MPC one-hot]
2) p2 provides Beaver triples when requested by P0/P1
3) p0 and p1 run the MPC protocol over all queries:
   - Private item selection:
     - MPC one-hot: obliviously select v_j using s shares (no party learns j)
     - OT-based: use 1-out-of-N OT Extension to obtain a share of v_j without revealing j (no S0/S1 needed)
   - Update u_i using the selected v_j
   - Reconstruct u_i' (for verification only), then re-share fresh random shares
4) p0 writes:
   - mpc_results.txt (final reconstructed user vectors that were updated)
   - mpc_results.done (flag to signal completion)
5) verify runs last, recomputes the final U directly from U0+U1, V0+V1 and queries.txt, compares per user, and prints all “Matched/Not matched” lines at the end.

## Private index j via OT Extension 
- Goal: reduce the O(n·k) per-query cost of MPC one-hot selection.
- Idea: For each feature dimension d in [0..k-1], treat the server holding V as the OT sender with N=n messages (the d-th coordinate of each item’s vector). The chooser (holding j) runs a 1-out-of-N OT to obtain the d-th coordinate of v_j, without revealing j or other coordinates.
- Implementation sketch:
  - Base OT: run λ (~128) base OTs once per session to seed OT extension.
  - OT Extension: use KKRT/IKNP to expand to many correlated OTs; derive 1-out-of-N via bit-decomposition of j (log2 n) or direct 1-of-N if supported.
  - Batch across k: run k parallel selections to get all coordinates of v_j.
- Complexity (per query):
  - Time/Comm: ~O(k·log n) vs O(k·n) for one-hot MPC (exact constants depend on the chosen OT scheme).
  - No S0/S1 files needed; queries_users.txt still used.


## Outputs (in ./data)
- U0.txt, U1.txt, V0.txt, V1.txt, queries.txt, queries_users.txt
- S0.txt, S1.txt (selector shares; only for MPC one-hot)
- mpc_results.txt: per line “user_idx val0 ... val{k-1}”
- mpc_results.done: completion flag for verifier

## Terminal output
- Logs from gen_data, p2, p1, p0 stream first.
- After MPC finishes, verify prints:
  - Verifying MPC results against direct update for updated users
  - User X: Matched / Not matched
  - All compared users matched between MPC and direct update (if all matched)

## Time complexity
Let m users, n items, k features, Q queries.
- MPC one-hot:
  - Per query: O(k·n) time and comm (k dot products across n items), O(k·n) triples.
  - Total: O(Q·k·n).
- OT extension (This can also be implemented to reduce the time complexity):
  - Per query: ~O(k·log n) time/comm (depends on OT variant and implementation).
  - Total: ~O(Q·k·log n).
- Update step (both): O(k) extra per query.
- Memory per party: O((m + n)·k).