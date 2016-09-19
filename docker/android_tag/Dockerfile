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

FROM chromium_android_build_base:latest
MAINTAINER snibug "snibug@linecorp.com"

ARG BRANCH
ARG CHROMIUM_TAG

# Sync Chromium tag
WORKDIR /root/chromium/src

RUN git fetch --tags
RUN git checkout -b $BRANCH $CHROMIUM_TAG
RUN gclient sync --with_branch_heads --jobs 16