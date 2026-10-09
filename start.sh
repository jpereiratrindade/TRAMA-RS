#!/usr/bin/env bash
set -Eeuo pipefail

project_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
build_dir="${TRAMA_BUILD_DIR:-$project_dir/build}"
database="${TRAMA_DB:-$project_dir/data/trama.sqlite}"
host="${TRAMA_HOST:-0.0.0.0}"
port="${TRAMA_PORT:-8080}"

cmake -S "$project_dir" -B "$build_dir" \
  -DCMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE:-Release}" \
  -DTRAMA_CXX23_FALLBACK="${TRAMA_CXX23_FALLBACK:-OFF}"
cmake --build "$build_dir" --parallel "${TRAMA_BUILD_JOBS:-4}"

if [[ ! -s "$database" ]]; then
  mkdir -p -- "$(dirname -- "$database")"
  "$build_dir/bin/trama" seed --db "$database" --data-dir "$project_dir/data"
fi

echo "TRAMA-RS: http://$host:$port"
exec "$build_dir/bin/trama-rsd" --db "$database" --host "$host" --port "$port"
