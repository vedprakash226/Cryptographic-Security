# MPC Communication – Secure Update with Verification

This project implements a 2-party MPC protocol (P0, P1) with a helper (P2) to update user vectors securely:
u_i' = u_i + v_j * (1 - <u_i, v_j>)

It also includes a direct (non-MPC) verifier that recomputes the final user vectors and checks if MPC results match.

## Prerequisites
- Docker and Docker Compose
- Bash (macOS/Linux)

## Run
Use the run.sh script (builds images, runs MPC services, then runs the verifier last)
- ./run.sh "users" "items" "features" "queries"

Example:
- ./run.sh 60 100 20 100


## Overview of the Protocol
1) gen_data generates:
   - U0.txt, U1.txt (user shares)
   - V0.txt, V1.txt (item shares)
   - queries.txt
2) p2 provides Beaver triples when requested by P0/P1
3) p0 and p1 run the MPC protocol over all queries.
4) p0 reconstructs each updated user’s final vector (just for verification), then re-shares it with p1.
5) p0 writes:
   - mpc_results.txt (final reconstructed user vectors that were updated)
   - mpc_results.done (flag to signal completion)
6) verify runs last, recomputes the final U directly from U0+U1, V0+V1 and queries.txt, compares per user, and prints all “Matched/Not matched” lines at the end.

## Outputs (in ./data)
- U0.txt, U1.txt, V0.txt, V1.txt, queries.txt
- mpc_results.txt: per line “user_idx val0 ... val{k-1}”
- mpc_results.done: completion flag for verifier

## Terminal output
- Logs from gen_data, p2, p1, p0 stream first.
- After MPC finishes, verify prints:
  - Verifying MPC results against direct update for updated users
  - User X: Matched / Not matched
  - All compared users matched between MPC and direct update. (if all matched)

Note: Results are printed at the end because verify runs after p0/p1/p2 complete.

## Re-sharing
After reconstructing an updated user vector, p0 re-shares it:
- p0 samples a fresh random share, computes the complementary share for p1, sends it to p1, and both overwrite their local shares. This keeps shares randomized after each update.
