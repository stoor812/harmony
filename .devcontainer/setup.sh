#!/bin/bash
set -e

echo "Installing harmony dependencies..."

# Update package list
sudo apt-get update

# Install core build tools
sudo apt-get install -y \
  cmake \
  build-essential \
  libssl-dev \
  libcurl4-openssl-dev \
  git \
  curl

# Install cpr (C++ HTTP client)
cd /tmp
git clone https://github.com/libcpr/cpr.git
cd cpr
cmake -B build -DCPR_USE_SYSTEM_CURL=ON
cmake --build build --parallel
sudo cmake --install build

echo "All dependencies installed."