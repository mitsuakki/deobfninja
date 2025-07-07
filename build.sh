#!/bin/bash

set -e  # stop script if any command fails

BUILD_DIR=build
BUILD_TYPE=RelWithDebInfo
GENERATOR=Ninja
PLUGIN_NAME=deobfninja
PLUGIN_OUTPUT="${BUILD_DIR}/lib${PLUGIN_NAME}.so"
LOCAL_PLUGIN_DIR="$HOME/.binaryninja/plugins"

function check_cmake() {
  if [ -f cmake ]; then
    echo "A local file named 'cmake' is shadowing the actual CMake command. Please remove it."
    exit 1
  fi

  if ! command -v cmake &> /dev/null; then
    echo "Cmake not found in PATH"
    exit 1
  fi
}

function build_all() {
  check_cmake
  cmake -B "$BUILD_DIR" -G "$GENERATOR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
  cmake --build "$BUILD_DIR" -- -j
}

function run_tests() {
  cmake --build "$BUILD_DIR"
  "$BUILD_DIR/${PLUGIN_NAME}_gtest"
}

function install_plugin() {
  if [ -z "$BN_INSTALL_DIR" ]; then
    echo "BN_INSTALL_DIR not set. Copying plugin to $LOCAL_PLUGIN_DIR"
    mkdir -p "$LOCAL_PLUGIN_DIR"
    cp "$PLUGIN_OUTPUT" "$LOCAL_PLUGIN_DIR"
    echo "Plugin copied to $LOCAL_PLUGIN_DIR. Launch Binary Ninja manually if path is correct."
  else
    cmake --install "$BUILD_DIR"
    "$BN_INSTALL_DIR/binaryninja"
  fi
}

function clean_build() {
  rm -rf "$BUILD_DIR"
}

function rebuild() {
  clean_build
  build_all
}

case "$1" in
  all)
    build_all
    ;;
  run)
    run_tests
    install_plugin
    ;;
  test)
    run_tests
    ;;
  clean)
    clean_build
    ;;
  rebuild)
    rebuild
    ;;
  *)
    echo "Usage: $0 {all|run|test|clean|rebuild}"
    exit 1
    ;;
esac
