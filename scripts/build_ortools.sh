#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ORTOOLS_ROOT="${ORTOOLS_ROOT:-$ROOT_DIR/.local/or-tools}"
ORTOOLS_SRC="${ORTOOLS_SRC:-$ROOT_DIR/.local/or-tools-src}"
ORTOOLS_GIT_REF="${ORTOOLS_GIT_REF:-main}"

echo "Building OR-Tools (ref: ${ORTOOLS_GIT_REF})"
echo "Install prefix: ${ORTOOLS_ROOT}"
echo "Source dir: ${ORTOOLS_SRC}"

mkdir -p "${ORTOOLS_ROOT}"

if [[ ! -d "${ORTOOLS_SRC}/.git" ]]; then
  rm -rf "${ORTOOLS_SRC}"
  git clone --depth 1 --branch "${ORTOOLS_GIT_REF}" https://github.com/google/or-tools.git "${ORTOOLS_SRC}"
else
  git -C "${ORTOOLS_SRC}" fetch --depth 1 origin "${ORTOOLS_GIT_REF}"
  git -C "${ORTOOLS_SRC}" checkout -f "${ORTOOLS_GIT_REF}"
fi

cmake -S "${ORTOOLS_SRC}" -B "${ORTOOLS_SRC}/build" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="${ORTOOLS_ROOT}" \
  -DBUILD_DEPS=ON \
  -DBUILD_SHARED_LIBS=ON

cmake --build "${ORTOOLS_SRC}/build" -j"$(nproc)"
cmake --install "${ORTOOLS_SRC}/build"

echo "OR-Tools installed to ${ORTOOLS_ROOT}"
