# syntax=docker/dockerfile:1

# SPDX-FileCopyrightText: Copyright (c) 2026 Real-Time Innovations, Inc.
# SPDX-License-Identifier: Apache-2.0

# EA2 development image. The default base is built from NVIDIA's private
# Engineering Release source with its official `./run build` workflow.
ARG BASE_IMAGE=holoscan-sdk-build-aarch64:v5.0.0-ea2
FROM ${BASE_IMAGE}

ARG DEBIAN_FRONTEND=noninteractive
ARG RTI_CONNEXT_VERSION=7.7.0

RUN rm -f /etc/apt/sources.list.d/kitware.list \
    && apt-get update \
    && apt-get install --no-install-recommends -y \
        ca-certificates \
        curl \
        openjdk-21-jre-headless \
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

ARG RTI_CONNEXT_ARCH=armv8Linux4gcc8.5.0

ENV NDDSHOME=/opt/rti.com/rti_connext_dds-${RTI_CONNEXT_VERSION}
ENV RTI_CONNEXT_DDS_DIR=${NDDSHOME}
ENV CONNEXTDDS_ARCH=${RTI_CONNEXT_ARCH}
ENV RTI_LICENSE_FILE=${NDDSHOME}/rti_license.dat
ENV HOME=/opt/rti-home

RUN mkdir -p ${HOME}/.rti \
    && printf 'copy_workspace=false\n' > ${HOME}/.rti/rticommon_config.sh \
    && chmod -R a+rwX ${HOME}

WORKDIR /workspace/rticonnextdds-holoscan-module
CMD ["/bin/bash"]
