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
