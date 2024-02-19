#!/usr/bin/env bash
set -e
SCRIPT_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
ROOT_DIR=$SCRIPT_DIR/..
BUILD_DIR="${BUILD_DIR:-$ROOT_DIR/build/linux-make-Debug}"

cmake --install $BUILD_DIR
