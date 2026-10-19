# syntax=docker/dockerfile:1

# *******************************************************************************
#  (c) 2026 Copyright, Real-Time Innovations, Inc. All rights reserved.
#  RTI grants Licensee a license to use, modify, compile, and create derivative
#  works of the Software. Licensee has the right to distribute object form only
#  for use with RTI products. The Software is provided "as is", with no warranty
#  of any type, including any warranty for fitness for any purpose. RTI is under no
#  obligation to maintain or support the Software. RTI shall not be liable for any
#  incidental or consequential damages arising out of the use or inability to use
#  the software.
# *******************************************************************************/

# SDK development image built from NVIDIA Holoscan 5 EA3 with
# the official `./run build_dev_image` workflow.
ARG BASE_IMAGE=holoscan-sdk-dev-aarch64:v5.0.0-ea3
ARG TARGETARCH
FROM ${BASE_IMAGE} AS connext-base

ARG DEBIAN_FRONTEND=noninteractive
ARG RTI_CONNEXT_VERSION=7.7.0
ARG HOLOSCAN_CLI_VERSION=5.0.0rc3

RUN rm -f /etc/apt/sources.list.d/kitware.list \
    && apt-get update \
    && apt-get install --no-install-recommends -y \
        ca-certificates \
        curl \
        openjdk-21-jre-headless \
        xvfb \
        imagemagick \
    && curl -fsSL -o /usr/share/keyrings/rti-official-archive.gpg \
        https://packages.rti.com/deb/official/repo.key \
    && printf -- "deb [arch=%s signed-by=%s] %s %s main\n" \
        "$(dpkg --print-architecture)" \
        /usr/share/keyrings/rti-official-archive.gpg \
        https://packages.rti.com/deb/official \
        "$(. /etc/os-release && echo "${VERSION_CODENAME}")" \
        > /etc/apt/sources.list.d/rti.list \
    && RTI_LICENSE_AGREEMENT_ACCEPTED=accepted apt-get update \
    && RTI_LICENSE_AGREEMENT_ACCEPTED=accepted apt-get install -y \
        "rti-connext-dds-${RTI_CONNEXT_VERSION}" \
    && rm -rf /var/lib/apt/lists/*

# Keep the container self-contained for the Holoscan CLI workflow. The host
# CLI creates this image and invokes `holoscan` again inside it. Pin the
# Holoscan 5 pre-release CLI used to validate this workflow.
RUN python3 -m pip install --no-cache-dir --pre \
        --extra-index-url https://pypi.nvidia.com \
        "holoscan-cli==${HOLOSCAN_CLI_VERSION}"

ENV NDDSHOME=/opt/rti.com/rti_connext_dds-${RTI_CONNEXT_VERSION}
ENV RTI_CONNEXT_DDS_DIR=${NDDSHOME}
ENV RTI_LICENSE_FILE=${NDDSHOME}/rti_license.dat
ENV HOME=/opt/rti-home

RUN mkdir -p ${HOME}/.rti \
    && printf 'copy_workspace=false\n' > ${HOME}/.rti/rticommon_config.sh \
    && chmod -R a+rwX ${HOME}

# Docker's target architecture selects the matching Connext libraries.
FROM connext-base AS connext-amd64
ENV CONNEXTDDS_ARCH=x64Linux4gcc8.5.0

FROM connext-base AS connext-arm64
ENV CONNEXTDDS_ARCH=armv8Linux4gcc8.5.0

FROM connext-${TARGETARCH} AS development
ARG RTI_CONNEXT_ARCH
ENV CONNEXTDDS_ARCH=${RTI_CONNEXT_ARCH:-${CONNEXTDDS_ARCH}}

WORKDIR /workspace/rticonnextdds-holoscan-module
CMD ["/bin/bash"]
