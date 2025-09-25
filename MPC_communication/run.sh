#!/bin/bash

NUM_USERS=${1:-100}
NUM_ITEMS=${2:-200}
NUM_FEATURES=${3:-2}
NUM_QUERIES=${4:-6}

echo "Running MPC with: $NUM_USERS users, $NUM_ITEMS items, $NUM_FEATURES features, $NUM_QUERIES queries"

export NUM_USERS
export NUM_ITEMS
export NUM_FEATURES
export NUM_QUERIES

# Build and run
docker-compose down
docker-compose build
docker-compose up