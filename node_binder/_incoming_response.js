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

const util = require('util');
const events = require('events');


function IncomingResponse() {
  if (!(this instanceof IncomingResponse)) {
    throw new Error('invalid constructor call IncomingResponse');
  }
  events.EventEmitter.call(this);

  if (arguments.length !== 2) {
    throw new Error('invalid arguments length: expect 2');
  }

  this._fetcher = arguments[0];
  this._requestId = arguments[1];
}
util.inherits(IncomingResponse, events.EventEmitter);


IncomingResponse.prototype.write = function write(data) {
  this._fetcher.appendChunkToUpload(this._requestId, data, false);
};


IncomingResponse.prototype.end = function end(data) {
  this._fetcher.appendChunkToUpload(this._requestId, data, true);
};


module.exports = IncomingResponse;
