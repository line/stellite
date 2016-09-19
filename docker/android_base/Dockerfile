# Copyright 2016 LINE Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http:#www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

FROM ubuntu:14.04
MAINTAINER snibug "snibug@linecorp.com"

# Install primary dependencies.
RUN echo "deb http://archive.ubuntu.com/ubuntu trusty multiverse" >> /etc/apt/sources.list
RUN apt-get update -q && apt-get upgrade -qy && apt-get install -qy curl git

# Install Chromium build dependencies.
RUN curl -SL https://chromium.googlesource.com/chromium/src/+/72334afec5c1869f45fafaa654e70b92b686d508/build/install-build-deps.sh?format=TEXT | base64 -d > /tmp/install-build-deps.sh \
 && chmod +x /tmp/install-build-deps.sh \
 && /tmp/install-build-deps.sh --no-prompt --no-arm --no-chromeos-fonts --no-nacl \
 && rm /tmp/install-build-deps.sh

# Install Chromium's depot_tools.
ENV DEPOT_TOOLS /root/depot_tools
RUN git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git $DEPOT_TOOLS
ENV PATH $PATH:$DEPOT_TOOLS
RUN echo "\n# Add Chromium's depot_tools to the PATH." >> .bashrc \
 && echo "export PATH=\"\$PATH:$DEPOT_TOOLS\"" >> .bashrc

# Create a Chromium directory.
RUN mkdir /root/chromium
WORKDIR /root/chromium

# Download Chromium's source code.
RUN fetch --nohooks chromium

RUN echo "{ 'GYP_DEFINES': 'OS=android' }" > /root/chromium/chromium.gyp_env
RUN echo "\ntarget_os = [\"android\"]" >> /root/chromium/.gclient

# Sync gclient (sync third-party and tool-chain)
RUN echo "y" | gclient sync

# Install Android dependency
RUN /root/chromium/src/build/install-build-deps-android.sh
