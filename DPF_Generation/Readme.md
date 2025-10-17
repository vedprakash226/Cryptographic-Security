# Distributed Point Function (DPF) Implementation in C++

## Overview
Implementation of **Distributed Point Function (DPF)** in **C++**, which allows two parties to share keys that encode a *point function* — i.e., a function that outputs a non-zero value at a single location and zero elsewhere — without revealing the location or value to either party individually.



## Implementations
- Implements **DPF key generation** for a given index and value.  
- Verifies correctness by ensuring the XOR of both evaluations reconstructs the desired point function.  
- Uses **pseudo-random number generators (PRNG)** for deterministic expansion.  
- If the advice bit of the current node is 1 I will do xor of its child seed values with the correction word.


## Summary

A **Distributed Point Function (DPF)** encodes a point function:

$$
f_{x^*, v}(x) =
\begin{cases}
v & \text{if } x = x^* \\
0 & \text{otherwise}
\end{cases}
$$

into two keys $(k_0, k_1)$ such that:

$$
f_{x^*, v}(x) = \text{Eval}(k_0, x) \oplus \text{Eval}(k_1, x)
$$

Each key individually reveals nothing about the special index \(x*\) or value \(v\).


## Components

### `struct child`
Represents the output of expanding a seed at a given level:
- `leftSeed`, `rightSeed` → child seeds  
- `leftFlag`, `rightFlag` → Boolean advice bits  

### `struct correctionWord`
Stores correction information at each level:
- `cw` → Correction word (XOR mask)  
- `leftAdviceBit`, `rightAdviceBit` → Correction advice bits  

### `class DPFKey`
Encapsulates:
- Initial `seed`  
- Starting flag `t0`  
- Vector of correction words (`cw_s`)  
- Final correction word (`final_cw`)  

## Function Used

| Function | Description |
|-----------|--------------|
| `Expand(u64 seed, u64 ctr_base)` | Expands a seed into two child seeds and flags using PRNG. |
| `generateDPF(u64 location, u64 value, u64 N)` | Generates a pair of DPF keys corresponding to a specific point function. |
| `evalDPF(DPFKey& key, u64 location, u64 N)` | Evaluates a DPF key at a specific index. |
| `EvalFull(DPFKey &k0, DPFKey &k1, u64 N, u64 value, u64 index)` | Verifies the correctness of generated keys by checking all indices. |

---

## To run the code
~~~
./main <DPF domain size> <number of DPF instances>

