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

const util = require('util');
const events = require('events');


// IncomingRequest
function IncomingRequest(stream) {
  if (!(this instanceof IncomingRequest)) {
    throw new Error('invalid constructor call of IncomingRequest');
  }

  events.EventEmitter.call(this);

  this.stream = stream;

  var self = this;
  stream.setHeadersAvailableCallback(function(headers, fin) {
    self.onHeaders(headers);
    self.emit('headers', headers, fin);

    if (fin) {
      self.removeAllListeners();
    }
  });

  stream.setDataAvailableCallback(function(data, fin) {
    self.emit('data', data, fin);

    if (fin) {
      self.removeAllListeners();
    }
  });
}
util.inherits(IncomingRequest, events.EventEmitter);


IncomingRequest.prototype.onHeaders = function onHeaders(headers) {
  this.method = this.checkHeader(headers[':method']);
  this.scheme = this.checkHeader(headers[':scheme']);
  this.host = this.checkHeader(headers[':authority']);
  this.url = this.checkHeader(headers[':path']);
};


IncomingRequest.prototype.checkHeader = function checkHeader(value) {
  if ((typeof value !== 'string') || value.length === 0) {
    throw new Error(`invalid header value: ${value}`);
  }
  return value;
};


module.exports = IncomingRequest;
