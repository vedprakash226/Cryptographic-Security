#!/bin/bash
set -e
NUM_USERS=${1:-100}
NUM_ITEMS=${2:-200}
NUM_FEATURES=${3:-2}
NUM_QUERIES=${4:-6}

echo "Running MPC with: $NUM_USERS users, $NUM_ITEMS items, $NUM_FEATURES features, $NUM_QUERIES queries"
export NUM_USERS NUM_ITEMS NUM_FEATURES NUM_QUERIES

docker-compose down -v
docker-compose build
docker-compose up gen_data p2 p1 p0

# Do NOT append args here; compose already injects them via ${...}
docker-compose run --rm verify