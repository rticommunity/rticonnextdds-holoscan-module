#!/usr/bin/env bash
set -euo pipefail
if [[ -z "${RTI_LICENSE_FILE:-}" || ! -s "${RTI_LICENSE_FILE}" ]]; then
  echo "A valid RTI_LICENSE_FILE is required." >&2
  exit 2
fi
