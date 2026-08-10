#
# Copyright 2026 AUTOMATOUS.IO
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

#
#   docker build -t automatous-io/shelly-gen4-builder:idf-v5.5.2-matter-2cb668c9 \
#       .devcontainer
#
# No published espressif/esp-matter tag matches this pin: 2cb668c9 is a commit
# on esp-matter's main branch, and every published tag tracks a release branch.
FROM ubuntu:24.04@sha256:561618e2c15bf2397621dd04f96926663a3b5616c189cf7e38db7e82f5c538ea

ARG IDF_CHECKOUT_REF=v5.5.2
ARG IDF_CLONE_URL=https://github.com/espressif/esp-idf.git
ARG IDF_INSTALL_TARGETS=esp32c6

ARG ESP_MATTER_REF=2cb668c95de4f24786d20b7cb03c171d6e27b79e
ARG ESP_MATTER_CLONE_URL=https://github.com/espressif/esp-matter.git

ENV DEBIAN_FRONTEND=noninteractive
ENV IDF_PATH=/opt/esp/idf
ENV IDF_TOOLS_PATH=/opt/esp
ENV ESP_MATTER_PATH=/opt/esp/esp-matter
ENV IDF_PYTHON_CHECK_CONSTRAINTS=no
ENV IDF_CCACHE_ENABLE=1

RUN set -x \
    && apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential cmake ninja-build ccache \
        autoconf automake libtool pkg-config \
        bison flex gperf make \
        python3 python3-venv python3-dev python3-pip python-is-python3 \
        git git-lfs curl wget ca-certificates unzip xz-utils bzip2 zip \
        libffi-dev libssl-dev libncurses-dev libusb-1.0-0-dev libbsd-dev \
        libglib2.0-dev libdbus-1-dev \
        libavahi-client-dev libavahi-common-dev \
        libgirepository1.0-dev libcairo2-dev \
        libreadline-dev libmbedtls-dev libevent-dev \
        nodejs \
    && git lfs install \
    && apt-get clean && rm -rf /var/lib/apt/lists/* \
    && rm -rf /usr/share/doc/* /usr/share/man/*

# --shallow-submodules is what keeps this small; IDF's submodule history is the
# bulk of the espressif/idf published image.
RUN set -x \
    && git clone --depth=1 --branch=${IDF_CHECKOUT_REF} \
        --recurse-submodules --shallow-submodules \
        ${IDF_CLONE_URL} ${IDF_PATH} \
    && git config --system --add safe.directory ${IDF_PATH}

RUN set -x \
    && ${IDF_PATH}/tools/idf_tools.py --non-interactive install required --targets=${IDF_INSTALL_TARGETS} \
    && ${IDF_PATH}/tools/idf_tools.py --non-interactive install cmake \
    && ${IDF_PATH}/tools/idf_tools.py --non-interactive install-python-env \
    && rm -rf ${IDF_TOOLS_PATH}/dist /root/.cache/pip

RUN set -x \
    && mkdir -p ${ESP_MATTER_PATH} && cd ${ESP_MATTER_PATH} \
    && git init -q \
    && git remote add origin ${ESP_MATTER_CLONE_URL} \
    && git fetch origin --depth=1 ${ESP_MATTER_REF} \
    && git checkout -q FETCH_HEAD \
    && git submodule update --init --depth=1 \
    && cd ${ESP_MATTER_PATH}/connectedhomeip/connectedhomeip \
    && ./scripts/checkout_submodules.py --platform esp32 linux --shallow

# install.sh runs connectedhomeip's bootstrap, which generates
# build_overrides/pigweed_environment.gni and installs zap.
#
# --no-host-tool skips building chip-cert and chip-tool; idf.py build does not
# use them. IDF_PATH_FORCE=1 is required because export.sh otherwise tries to
# detect IDF_PATH from the working directory, which is ESP_MATTER_PATH here.
RUN set -x \
    && cd ${ESP_MATTER_PATH} \
    && export IDF_PATH_FORCE=1 \
    && . ${IDF_PATH}/export.sh \
    && ./install.sh --no-host-tool \
    && rm -rf /root/.cache/pip /tmp/*

RUN cd ${ESP_MATTER_PATH} && git rev-parse HEAD > /opt/esp/esp-matter-commit.txt \
    && cd ${ESP_MATTER_PATH}/connectedhomeip/connectedhomeip \
    && git rev-parse HEAD > /opt/esp/connectedhomeip-commit.txt \
    && cd ${IDF_PATH} && git describe --tags --always > /opt/esp/esp-idf-version.txt

# The workspace is bind-mounted from the host user while this runs as root, and
# IDF shells out to git for the app version.
RUN git config --system --add safe.directory '*'

# VS Code terminals are non-login interactive shells, which read bash.bashrc.
RUN printf '%s\n' \
    'if [ -z "${IDF_EXPORT_DONE:-}" ]; then' \
    '  . "$IDF_PATH/export.sh" >/dev/null 2>&1 || true' \
    '  . "$ESP_MATTER_PATH/export.sh" >/dev/null 2>&1 || true' \
    '  export IDF_EXPORT_DONE=1' \
    'fi' >> /etc/bash.bashrc

WORKDIR /workspaces
CMD ["/bin/bash"]
