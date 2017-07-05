// Copyright 2017 LINE Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

'use strict';

const stellite = require('./stellite');

var fetcher = stellite.createHttpFetcher();

var options = {
  url:'http://example.com',
  method: 'GET',
  is_chunked_upload: false,
  is_stream_response: false,
  is_stop_on_redirect: false,
};

for (let i = 0; i < 10; ++i) {
  var response = fetcher.request(options)
    .on('response', function(status_code, headers, data) {
      console.log('on response:');
      console.log(`status code: ${status_code}`);
      console.log(headers);
    })
    .on('headers', function(status_code, headers) {
      console.log(`on header: ${headers}`);
      console.log(`status code: ${status_code}`);
    })
    .on('data', function(data) {
      console.log(`on data: ${data.toString('utf8')}`);
    })
    .on('error', function(error_code) {
      console.log(`error: ${error_code}`);
    });
}

module.exports.fetcher = fetcher;
module.exports.response = response;
