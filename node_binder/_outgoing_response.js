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

// OutgoingResponse
function OutgoingResponse(stream) {
  if (!(this instanceof OutgoingResponse)) {
    throw new Error('invalid constructor call fo OutgoingResponse');
  }

  // call parents constructor
  events.EventEmitter.call(this);

  this.stream = stream;
  this.headersSent = false;
  this.statusCode = 200;
}
util.inherits(OutgoingResponse, events.EventEmitter);


OutgoingResponse.prototype.writeHeaders = function writeHeaders(
    statusCode,
    headers,
    fin) {

  if (this.headersSent) {
    return;
  }

  if (headers === null) {
    headers = {};
  }

  headers[':status'] = statusCode;

  this.stream.writeHeaders(headers, fin);
  this.headersSent = true;
};


OutgoingResponse.prototype.writeData = function writeData(data, fin) {
  if (!this.headersSent) {
    this.writeHeaders(this.statusCode, null, false);
  }
  this.stream.writeData(data, fin);
};


OutgoingResponse.prototype.writeTraileres = function writeTrailers(trailers) {
  this.stream.writeTrailers(trailers);
};


OutgoingResponse.prototype.write = function write(data, fin) {
  fin = fin || false;
  this.writeData(data, fin);
};


OutgoingResponse.prototype.end = function end() {
  this.writeData(null, true);
};


module.exports = OutgoingResponse;
