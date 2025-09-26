# MPC with Oblivious Transfer for Private Index Selection

This project implements a secure multiparty computation (MPC) protocol enhanced with an oblivious transfer mechanism to keep query indices confidential. The protocol allows two parties (P0 and P1) to collaboratively compute a function over their inputs while keeping those inputs private.

## Overview

The project consists of several components:

- **MPC Protocol**: The core functionality for secure computation, allowing P0 and P1 to update user vectors based on item vectors while maintaining privacy.
- **Oblivious Transfer**: A mechanism that ensures the confidentiality of the query index used in the MPC protocol, preventing either party from learning the other's input.
- **Data Generation**: Scripts to generate initial data for the protocol, including user and item shares, as well as random queries.

## Directory Structure

- `src/apps/`: Contains the application logic for each party in the MPC protocol.
  - `gen_data.cpp`: Generates initial data for the MPC protocol.
  - `p0.cpp`: Implements the functionality for the first party (P0).
  - `p1.cpp`: Implements the functionality for the second party (P1).
  - `p2.cpp`: Implements the functionality for the third party (P2).
  - `verify.cpp`: Verifies the results of the MPC protocol.

- `src/common/`: Contains common definitions and utilities.
  - `common.hpp`: Common utility functions and type definitions.
  - `shares.hpp`: Defines the Share structure and operations for secret sharing.
  - `types.hpp`: Common types and constants.

- `src/io/`: Contains utility functions for file operations.
  - `utility.cpp`: Functions for reading and writing data.
  - `utility.hpp`: Declarations for utility functions.

- `src/mpc/`: Contains the MPC protocol logic.
  - `beaver.hpp`: Defines the Beaver triple structure and related functions.
  - `mpc.hpp`: Defines the MPCProtocol class.

- `src/net/`: Contains networking functionalities.
  - `channel.cpp`: Implements connection handling.
  - `channel.hpp`: Declarations for networking functions.

- `src/oblivious_index/`: Contains the oblivious transfer implementation.
  - `private_index_select.cpp`: Implements the oblivious transfer protocol for private index selection.
  - `private_index_select.hpp`: Declarations for the oblivious index selection functions.

- `src/ot/`: Contains implementations of various oblivious transfer protocols.
  - `base_ot.cpp`, `base_ot.hpp`: Base oblivious transfer protocol.
  - `kkrt.cpp`, `kkrt.hpp`: KKRT oblivious transfer protocol.
  - `ot_extension.cpp`, `ot_extension.hpp`: Extension of the oblivious transfer protocol.

## Build and Run

To build and run the project, follow these steps:

1. **Install Dependencies**: Ensure you have Docker and Docker Compose installed on your machine.
2. **Build the Project**: Use the provided `Dockerfile` to build the Docker image.
3. **Run the Services**: Use `docker-compose.yml` to start the services for the MPC protocol and the oblivious transfer mechanism.
4. **Generate Data**: Run the `gen_data` service to generate the initial data required for the protocol.
5. **Execute the Protocol**: Start the P0, P1, and P2 services to execute the MPC protocol.
6. **Verify Results**: Use the `verify` service to check the correctness of the results.

## Outputs

The project generates several output files, including:

- User and item shares.
- Queries for the MPC protocol.
- Results of the MPC computation.
- Verification results comparing MPC outputs with direct computations.

## Conclusion

This project demonstrates the integration of oblivious transfer into an MPC protocol, enhancing privacy and security during the computation process. The modular structure allows for easy extension and modification of the protocol components.