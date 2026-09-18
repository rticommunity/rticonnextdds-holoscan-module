#!/usr/bin/env bash
set -euo pipefail

subscriber=$1
publisher=$2
expected=$3
test_directory=$(mktemp -d)
subscriber_pid=""
cleanup() {
  if [[ -n "${subscriber_pid}" ]] && kill -0 "${subscriber_pid}" 2>/dev/null; then
    kill "${subscriber_pid}" 2>/dev/null || true
  fi
  wait "${subscriber_pid}" 2>/dev/null || true
  rm -rf "${test_directory}"
}
trap cleanup EXIT

"${subscriber}" >"${test_directory}/subscriber.log" 2>&1 &
subscriber_pid=$!
if ! "${publisher}" >"${test_directory}/publisher.log" 2>&1; then
  cat "${test_directory}/publisher.log" "${test_directory}/subscriber.log"
  exit 1
fi
if ! wait "${subscriber_pid}"; then
  cat "${test_directory}/publisher.log" "${test_directory}/subscriber.log"
  exit 1
fi
grep -Fq "${expected}" "${test_directory}/subscriber.log"
cat "${test_directory}/publisher.log" "${test_directory}/subscriber.log"
