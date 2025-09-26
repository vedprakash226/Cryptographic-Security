# MPC Communication – Secure Update with Verification

This project implements a 2-party MPC protocol (P0, P1) with a helper (P2) to update user vectors securely:
u_i' = u_i + v_j * (1 - <u_i, v_j>)

It also supports private item index selection:
- For each query, the item index j is hidden from both parties via a secret-shared one-hot vector s (length n) with s_j = 1.
- The parties obliviously select v_j by computing v_sel = V^T s using MPC dot products, without learning j.

## Prerequisites
- Docker and Docker Compose
- Bash (macOS/Linux)

## Run
Use the run.sh script (builds images, runs MPC services, then runs the verifier last)
- ./run.sh "users" "items" "features" "queries"

Example:
- ./run.sh 300 300 30 100

## Overview of the Protocol
1) gen_data generates:
   - U0.txt, U1.txt (user shares)
   - V0.txt, V1.txt (item shares)
   - queries.txt (full user,item pairs; used by verifier only)
   - queries_users.txt (only user indices; used by MPC parties)
   - S0.txt, S1.txt (additive shares of one-hot selector per query; length n per line)
2) p2 provides Beaver triples when requested by P0/P1
3) p0 and p1 run the MPC protocol over all queries:
   - Obliviously select v_j using s shares (no party learns j)
   - Update u_i using v_sel
   - Reconstruct u_i' (for verification only), then re-share fresh random shares
4) p0 writes:
   - mpc_results.txt (final reconstructed user vectors that were updated)
   - mpc_results.done (flag to signal completion)
5) verify runs last, recomputes the final U directly from U0+U1, V0+V1 and queries.txt, compares per user, and prints all “Matched/Not matched” lines at the end.

## Outputs (in ./data)
- U0.txt, U1.txt, V0.txt, V1.txt, queries.txt, queries_users.txt
- S0.txt, S1.txt (selector shares)
- mpc_results.txt: per line “user_idx val0 ... val{k-1}”
- mpc_results.done: completion flag for verifier

## Terminal output
- Logs from gen_data, p2, p1, p0 stream first.
- After MPC finishes, verify prints:
  - Verifying MPC results against direct update for updated users
  - User X: Matched / Not matched
  - All compared users matched between MPC and direct update (if all matched)

## Re-sharing
After reconstructing an updated user vector, p0 re-shares it:
- p0 samples a fresh random share, computes the complementary share for p1, sends it to p1, and both overwrite their local shares. This keeps shares randomized after each update.

## Notes
- Oblivious selection cost is O(n·k) per query (k dot products over n items).
- All operations are performed modulo mod in shares.hpp and mpc.hpp.
