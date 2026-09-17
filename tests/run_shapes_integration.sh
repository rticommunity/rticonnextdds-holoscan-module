#!/usr/bin/env bash

# SPDX-FileCopyrightText: Copyright (c) 2026 Real-Time Innovations, Inc.
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

subscriber_executable=$1
publisher_executable=$2
test_directory=$(mktemp -d)
subscriber_pid=

cleanup() {
    if [[ -n "${subscriber_pid}" ]] && kill -0 "${subscriber_pid}" 2>/dev/null; then
        kill "${subscriber_pid}" 2>/dev/null || true
        wait "${subscriber_pid}" 2>/dev/null || true
    fi
    rm -rf "${test_directory}"
}
trap cleanup EXIT

"${subscriber_executable}" >"${test_directory}/subscriber.log" 2>&1 &
subscriber_pid=$!
sleep 2

if ! "${publisher_executable}" >"${test_directory}/publisher.log" 2>&1; then
    cat "${test_directory}/publisher.log"
    cat "${test_directory}/subscriber.log"
    exit 1
fi

if ! wait "${subscriber_pid}"; then
    subscriber_pid=
    cat "${test_directory}/publisher.log"
    cat "${test_directory}/subscriber.log"
    exit 1
fi
subscriber_pid=

grep -Fq "DDS Shapes publisher sent 20 ShapeTypeExtended samples" \
    "${test_directory}/publisher.log"
grep -Fq "DDS Shapes subscriber received 20 ShapeTypeExtended samples" \
    "${test_directory}/subscriber.log"

cat "${test_directory}/publisher.log"
cat "${test_directory}/subscriber.log"
