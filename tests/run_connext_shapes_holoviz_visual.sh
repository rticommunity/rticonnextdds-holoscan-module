#!/usr/bin/env bash
set -euo pipefail

image=${1:?image argument required}
artifact_relative=${2:?artifact relative path required}
host_workspace=${HOST_WORKSPACE:?HOST_WORKSPACE must be the host path to this repository}
host_sdk_root=${HOST_SDK_ROOT:?HOST_SDK_ROOT must be the host path to the EA2 SDK}
workspace=/workspace/rticonnextdds_holoscan_module
sdk=/workspace/holoscan-sdk
artifact_dir="${workspace}/${artifact_relative}"
host_artifact_dir="${host_workspace}/${artifact_relative}"
local_name="connext-holoviz-local-${$}"
external_name="connext-holoviz-external-${$}"
domain_id=$((100 + $$ % 100))
mkdir -p "${artifact_dir}"
rm -f "${artifact_dir}/local.png" "${artifact_dir}/external.png"
common=(--runtime=nvidia --net host --ipc host -u "$(id -u):$(id -g)"
  -v "${host_workspace}:${workspace}"
  -v "${host_sdk_root}:${sdk}:ro"
  -v "${host_workspace}/rti_license.dat:/opt/rti.com/rti_connext_dds-7.7.0/rti_license.dat:ro"
  -v "${host_artifact_dir}:/artifacts"
  -w "${workspace}/build/rticonnextdds-holoscan-module/applications/connext_shapes_holoviz"
  -e HOME="${workspace}" -e HOLOSCAN_LIB_PATH="${sdk}/lib"
  -e LD_LIBRARY_PATH="${sdk}/lib")
cleanup() { docker rm -f "${local_name}" "${external_name}" >/dev/null 2>&1 || true; }
trap cleanup EXIT

docker run -d --name "${local_name}" "${common[@]}" "${image}" bash -lc \
  "export DISPLAY=:99 XDG_RUNTIME_DIR=/tmp; Xvfb :99 -screen 0 800x800x24 >/tmp/xvfb.log 2>&1 & exec ./connext_shapes_holoviz --domain-id ${domain_id}" >/dev/null
docker run -d --name "${external_name}" "${common[@]}" "${image}" bash -lc \
  "export DISPLAY=:100 XDG_RUNTIME_DIR=/tmp; Xvfb :100 -screen 0 800x800x24 >/tmp/xvfb.log 2>&1 & exec ./connext_shapes_holoviz --domain-id ${domain_id} --publish-topic Circle --publish-color RED" >/dev/null
sleep 8
test "$(docker inspect --format "{{.State.Running}}" "${local_name}")" = true
test "$(docker inspect --format "{{.State.Running}}" "${external_name}")" = true
docker exec -e DISPLAY=:99 "${local_name}" import -window "Connext Shapes + Holoviz" /artifacts/local.png
docker exec -e DISPLAY=:100 "${external_name}" import -window "Connext Shapes + Holoviz" /artifacts/external.png
for capture in /artifacts/local.png /artifacts/external.png; do
  colors=$(docker exec "${local_name}" convert "${capture}" -format "%c" histogram:info:)
  printf "%s\n" "${colors}" | grep -q "#000000"
  printf "%s\n" "${colors}" | grep -q "#FF0000"
  printf "%s\n" "${colors}" | grep -q "#000080"
done
! docker logs "${local_name}" 2>&1 | grep -q "RUNTIME_OPERATOR_CALLBACK_FAILED"
! docker logs "${external_name}" 2>&1 | grep -q "RUNTIME_OPERATOR_CALLBACK_FAILED"
echo "connext_shapes_holoviz visual DDS test passed; captures: ${host_artifact_dir}/local.png ${host_artifact_dir}/external.png"
