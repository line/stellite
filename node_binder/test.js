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
  let app, server, chunked_upload_size;

  before(() => {
    app = express();
    app.get('/', (req, res) => res.status(200).end('ok'));
    app.post('/', (req, res) => res.status(200).end('ok'));
    app.put('/', (req, res) => res.status(200).end('ok'));
    app.delete('/', (req, res) => res.status(200).end('ok'));

    app.get('/stream', (req, res) => {
      for (let i = 0; i < 100; ++i) {
        res.write(`stream data: ${i}\n`);
      }
      res.status(200).end();
    });

    app.post('/upload', (req, res) => {
      req.on('data', (data) => {
        res.write(data);
      });
      req.on('end', () => {
        res.status(200).end();
      });
    });

    server = http.createServer(app);
    server.listen(8002);
  });

  after(() => {
    const res = server.close();
  });

  it('should GET return 200', (done) => {
    fetcher.request({
      url: 'http://localhost:8002',
      method: 'GET',
    }).on('response', (statusCode, headers, data) => {
      expect(statusCode).to.equal(200);
      expect(data.toString('utf8')).to.equal('ok');
      done();
    });
  });

  it('should POST return 200', (done) => {
    fetcher.request({
      url: 'http://localhost:8002',
      method: 'POST',
      payload: 'body',
    }).on('response', (statusCode, headers, data) => {
      expect(statusCode).to.equal(200);
      expect(data.toString('utf8')).to.equal('ok');
      done();
    });
  });

  it('should PUT return 200', (done) => {
    fetcher.request({
      url: 'http://localhost:8002',
      method: 'PUT',
      payload: 'body',
    }).on('response', (statusCode, headers, data) => {
      expect(statusCode).to.equal(200);
      expect(data.toString('utf8')).to.equal('ok');
      done();
    });
  });

  it('should DELETE return 200', (done) => {
    fetcher.request({
      url: 'http://localhost:8002',
      method: 'DELETE',
    }).on('response', (statusCode, headers, data) => {
      expect(statusCode).to.equal(200);
      expect(data.toString('utf8')).to.equal('ok');
      done();
    });
  });

  it('should get response using stream callback', (done) => {
    let received = 0;
    const req = fetcher.request({
      url: 'http://localhost:8002/stream',
      is_stream_response: true,
      method: 'GET',
    }).on('data', (data, fin) => {
      received += data.length;
      if (fin) {
        expect(received).to.be.above(1);
        done();
      }
    });
  });

  it('should request using chunked upload', (done) => {
    const request = fetcher.request({
      url: 'http://localhost:8002/upload',
      is_chunked_upload: true,
      method: 'POST',
    });

    request.write('hello', false);
    request.write('world', false);
    request.write('end', true);

    request.on('response', (statusCode, headers, data) => {
      expect(statusCode).to.be.equal(200);
      done();
    });
  });

  it('should chunked request cannot send after last chunk', () => {
    const request = fetcher.request({
      url: 'http://localhost:8002/upload',
      is_chunked_upload: true,
      method: 'POST',
    });

    expect(() => {
      request.write('hello', true);
      // make exception after fin write
      request.write('world', false);
    }).to.throw(Error);
  });

  it('should POST request require payload or is_chunked_upload param', () => {
    // payload, chunked_upload are missing
    expect(() => {
      const request = fetcher.request({
        url: 'http://localhost:8002',
        method: 'POST',
      });
    }).to.throw(Error);
  });

  it('should PUT request require payload or is_chunked_upload param', () => {
    expect(() => {
      const request = fetcher.request({
        url: 'http://localhost:8002',
        method: 'PUT',
      });
    }).to.throw(Error);
  });

  it('should can chunked request and stream response', (done) => {
    const req = fetcher.request({
      url: 'http://localhost:8002/upload',
      is_chunked_upload: true,
      is_stream_response: true,
      method: 'POST',
    });

    const message = 'hello';

    req.on('header', (statusCode, headers) => {
    });

    req.on('data', (data, fin) => {
      expect(data.toString('utf8')).to.equal(message + message);
      done();
    });

    req.write(message, false);
    req.write(message, true);
  });

  it('should generate quic server config', (done) => {
    const config = stellite.generateQuicServerConfig();
    expect(config).to.be.a('string');
    done();
  });
});
