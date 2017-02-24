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

var stellite = require('./stellite');

stellite.setMinLogLevel(3);

var server = stellite.createQuicServer({
    cert: '',
    key: ''
  },
  function(req, res) {
    req.on('headers', function onHeaders(headers, fin) {
      console.log('on headers');
      console.log(headers);

      res.writeHeaders(200, headers, fin);
    });

    req.on('data', function onData(data, fin) {
      console.log('on data()');
      console.log(`received: ${data.toString('utf8')}, fin: ${fin}`);
      res.write(data, fin);
    });
  }
);

server.listen('::', 443);
