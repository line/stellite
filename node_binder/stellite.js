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

"use strict";

const stellite = require('./stellite.node');
const IncomingRequest = require('./_incoming_request');
const OutgoingResponse = require('./_outgoing_response');
const IncomingResponse = require('./_incoming_response');

const events = require('events');
const fs = require('fs');
const util = require('util');

// HttpFetcher -----------------------------------------------------------------
function HttpFetcher() {
  if (!(this instanceof HttpFetcher)) {
    throw new Error('invalid constrcutor call HttpFetcher');
  }

  var fetcher = stellite.createHttpFetcher(IncomingResponse);
  this.fetcher = fetcher;
}


HttpFetcher.prototype.defaultOptions = function defaultOptions(options) {
  if ((typeof options) === 'string') {
    return {
      url: options,
      is_stop_on_redirect: false,
      is_chunked_upload: false,
      is_stream_response: false,
      max_retries_on_5xx: 0,
      max_retries_on_network_change: 0,
      method: 'GET',
      timeout: 60 * 1000,
      payload: '',
    };
  }

  if (!options instanceof Object) {
    throw new Error('invalid fetcher argument error');
  }

  if (options.url === undefined) {
    throw new Error('url option are required');
  }

  if (options.payload && options.is_chunked_upload) {
    throw new Error('payload and is_chunked_upload are exclusive');
  }

  if ((options.method === 'POST' || options.method === 'PUT') &&
      !(options.payload || options.is_chunked_upload)) {
    throw new Error('payload param are required');
  }

  return {
    url: options.url,
    method: options.method || 'GET',
    is_stop_on_redirect: options.is_stop_on_redirect || false,
    is_chunked_upload: options.is_chunked_upload || false,
    is_stream_response: options.is_stream_response || false,
    max_retries_on_5xx: options.max_retries_on_5xx || 0,
    max_retries_on_network_change: options.max_retries_on_network_change || 0,
    timeout: options.timeout || 60 * 1000,
    payload: options.payload || '',
  };
};

HttpFetcher.prototype.request = function request(options, callback) {
  return this.fetcher.request(this.defaultOptions(options), callback);
};


HttpFetcher.prototype.get = function get(options, callback) {
  var requestOptions = this.defaultOptions(options);
  requestOptions.method = 'GET';
  return this.request(requestOptions, callback);
};


HttpFetcher.prototype.post = function post(options, callback) {
  var requestOptions = this.defaultOptions(options);
  requestOptions.method = 'POST';
  return this.request(requestOptions, callback);
};


HttpFetcher.prototype.put = function put(options, callback) {
  var requestOptions = this.defaultOptions(options);
  requestOptions.method = 'PUT';
  return this.request(requestOptions, callback);
};


HttpFetcher.prototype.patch = function patch(options, callback) {
  var requestOptions = this.defaultOptions(options);
  requestOptions.method = 'PATCH';
  return this.request(requestOptions, callback);
};


HttpFetcher.prototype.head = function head(options, callback) {
  var requestOptions = this.defaultOptions(options);
  requestOptions.method = 'HEAD';
  return this.request(requestOptions, callback);
};


HttpFetcher.prototype.delete = function del(options, callback) {
  var requestOptions = this.defaultOptions(options);
  requestOptions.method = 'DELETE';
  return this.request(requestOptions, callback);
};


// QuicServer ------------------------------------------------------------------
function QuicServer(options, requestListener) {
  if (!(this instanceof QuicServer)) {
    throw new Error('invalid constructor call for QuicServer');
  }

  // call parent constructor
  events.EventEmitter.call(this);

  if (!fs.existsSync(options.cert)) {
    throw new Error(`invalid certificate file: ${options.cert}`);
  }

  if (!fs.existsSync(options.key)) {
    throw new Error(`invalid key file: ${options.key}`);
  }

  this.requestListener = requestListener;

  var server = stellite.createQuicServer(
    options.cert,
    options.key,
    this);

  this.server = server;

  server.initialize();
}
util.inherits(QuicServer, events.EventEmitter);


QuicServer.prototype.listen = function listen(bind_address, port) {
  var self = this;
  this.on('stream', function callback(sessionId, stream) {
    var request = new IncomingRequest(stream);
    var response = new OutgoingResponse(stream);
    self.requestListener(request, response);
  });

  this.server.listen(bind_address, port);
};


QuicServer.prototype.shutdown = function shutdown() {
  this.server.shutdown();
};


QuicServer.prototype.onSessionCreated = function onSessionCreated(sessionId) {
  this.emit('session', sessionId);
};


QuicServer.prototype.onSessionClosed = function onSessionClosed(
  sessionId,
  errorCode,
  errorDetails) {
  this.emit('close', sessionId);
};


QuicServer.prototype.onStreamCreated = function onStramCreated(
  sessionId,
  stream) {
  this.emit('stream', sessionId, stream);
};


QuicServer.prototype.onStreamClosed = function onStreamClosed(
  sessionId,
  streamId) {
  this.emit('close', sessionId, streamId);
};


function createQuicServer(options, requestListener) {
  if (typeof options === 'function') {
    throw new Error('options are required.');
  }

  if (!options.key || !options.cert) {
    throw new Error('option.key and options.cert are required.');
  }

  return new QuicServer(options, requestListener);
}


function createHttpFetcher() {
  return new HttpFetcher();
}

module.exports.createQuicServer = createQuicServer;
module.exports.createHttpFetcher = createHttpFetcher;
module.exports.setMinLogLevel = stellite.setMinLogLevel;
