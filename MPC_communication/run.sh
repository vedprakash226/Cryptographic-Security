#!/bin/bash
set -e
NUM_USERS=${1:-100}
NUM_ITEMS=${2:-200}
NUM_FEATURES=${3:-2}
NUM_QUERIES=${4:-6}

echo "Running MPC with: $NUM_USERS users, $NUM_ITEMS items, $NUM_FEATURES features, $NUM_QUERIES queries"
export NUM_USERS NUM_ITEMS NUM_FEATURES NUM_QUERIES

# This is to remove the chached data (sometimes it creates issues)
if [ -d "data" ]; then
    echo "Removing existing data folder..."
    rm -rf data
fi

docker-compose down -v
docker-compose build

# data generation
docker-compose run --rm gen_data

# run MPC (p2, p1, p0) to completion
docker-compose up p2 p1 p0

# run verifier last
docker-compose run --rm verify