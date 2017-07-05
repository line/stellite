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

const chai = require('chai');
const express = require('express');
const http = require('http');
const stellite = require('./stellite');

const fetcher = stellite.createHttpFetcher();
const expect = chai.expect;

describe('stellite unittest', () => {
  let app, server;

  before(() => {
    app = express();
    app.get('/', (req, res) => {
      res.status(200).end('ok');
    });
    server = http.createServer(app);
    server.listen(8080);
  });

  after(() => {
    const res = server.close();
  });

  describe('stellite client test case', () => {
    it('should return 200', (done) => {
      const res = fetcher.request({
        url: 'http://localhost:8080',
        method: 'GET',
        is_chunked_upload: false,
        is_stream_response: false,
        is_stop_on_redirect: false,
      }).on('response', (status_code, headers, data) => {
        expect(status_code).to.equal(200);
        done();
      });
    });
  });
});
