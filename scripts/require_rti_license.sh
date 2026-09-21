#!/usr/bin/env bash
set -euo pipefail

license_file="${RTI_LICENSE_FILE:-}"
if [[ -z "${license_file}" || ! -f "${license_file}" || ! -s "${license_file}" ]]; then
  echo "RTI Connext license not found." >&2
  echo "Provide a valid rti_license.dat and run the application again." >&2
  echo "If you do not have one, see the repository README for instructions to obtain an evaluation license." >&2
  exit 2
fi
