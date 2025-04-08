#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

# Run the make commands sequentially
make -f libksocket.mk
make -f initksock.mk
make -f run.mk

echo "Build and execution completed successfully."