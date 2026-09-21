# syntax=docker/dockerfile:1

# *******************************************************************************
#  (c) 2026 Copyright, Real-Time Innovations, Inc. All rights reserved.
#  RTI Examples License.
# *******************************************************************************/

ARG BASE_IMAGE=nvcr.io/nvidia/clara-holoscan/holoscan:v4.6.0-cuda12-dgpu
FROM ${BASE_IMAGE}

ARG RTI_CONNEXT_VERSION=7.7.0
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install --no-install-recommends -y \
      ca-certificates curl openjdk-21-jre xauth xvfb \
    && curl -fsSL -o /usr/share/keyrings/rti-official-archive.gpg \
      https://packages.rti.com/deb/official/repo.key \
    && printf 'deb [signed-by=/usr/share/keyrings/rti-official-archive.gpg] https://packages.rti.com/deb/official %s main\n' \
      "$(. /etc/os-release && echo "$VERSION_CODENAME")" \
      > /etc/apt/sources.list.d/rti.list \
    && RTI_LICENSE_AGREEMENT_ACCEPTED=accepted apt-get update \
    && RTI_LICENSE_AGREEMENT_ACCEPTED=accepted apt-get install --no-install-recommends -y \
      "rti-connext-dds-${RTI_CONNEXT_VERSION}" \
    && rm -rf /var/lib/apt/lists/*

ENV NDDSHOME=/opt/rti.com/rti_connext_dds-${RTI_CONNEXT_VERSION}
ENV RTI_CONNEXT_DDS_DIR=${NDDSHOME}
ENV RTI_LICENSE_FILE=${NDDSHOME}/rti_license.dat
COPY scripts/connext_env.sh /opt/rti.com/connext_env.sh
ENV BASH_ENV=/opt/rti.com/connext_env.sh
COPY scripts/require_rti_license.sh /usr/local/bin/require_rti_license
RUN chmod 0755 /usr/local/bin/require_rti_license
WORKDIR /workspace/rticonnextdds-holoscan-module
CMD ["bash"]
