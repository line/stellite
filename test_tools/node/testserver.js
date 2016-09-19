// Copyright 2016 LINE Corporation
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

var http = require('http'),
    https = require('https'),
    spdy = require('spdy'),
    http2 = require('http2'),
    fs = require('fs'),
    argv = require('minimist')(process.argv.slice(2)),
    express = require('express'),
    proxy = require('http-proxy'),
    ALTERNATIVE_SERVICES = 'Alt-Svc',
    ALTERNATIVE_PROTOCOL = 'alternative-protocol';

var ssl_options = {
  key: fs.readFileSync(__dirname + '/../res/example.com.cert.key'),
  cert: fs.readFileSync(__dirname + '/../res/example.com.cert.pem'),
};

var app = express();

app.get('/gzip', function (req, res) {
  res.setHeader('Content-Encoding', 'gzip');
  res.sendfile(__dirname + '/res/data.gzip');
});

app.get('/deflate', function(req, res) {
  res.setHeader('Content-Encoding', 'deflate');
  res.sendfile(__dirname + '/res/data.deflate');
});

app.get('/', function(req, res) {
  if (argv.alternative_services) {
    res.setHeader(ALTERNATIVE_SERVICES, argv.alternative_services);
    res.setHeader(ALTERNATIVE_PROTOCOL, '4430:quic,p=1');
  }

  res.send('get');
});

app.post('/', function(req, res) {
  if (argv.alternative_services) {
    res.setHeader(ALTERNATIVE_SERVICES, argv.alternative_services);
  }
  res.send("post");
});

app.put('/', function(req, res) {
  if (argv.alternative_services) {
    res.setHeader(ALTERNATIVE_SERVICES, argv.alternative_services);
  }
  res.send('put');
});

app.delete('/', function(req, res) {
  if (argv.alternative_services) {
    res.setHeader(ALTERNATIVE_SERVICES, argv.alternative_services);
  }
  res.send('delete');
});

// Pending response
app.get('/pending', function(req, res) {
  if (argv.alternative_services) {
    res.setHeader(ALTERNATIVE_SERVICES, argv.alternative_services);
  }

  if (req.query.delay === undefined) {
    res.send('direct response');
    return;
  }

  setTimeout(function() {
    res.send('pending response');
  }, req.query.delay);
});

//  Send and wait response
app.get('/send_and_wait', function(req, res) {
  if (argv.alternative_service) {
    res.setHeader(ALTERNATIVE_SERVICES, argv.alternative_services);
  }

  if (req.query.delay === undefined) {
    res.send('direct response');
  }

  res.write('first response ');
  setTimeout(function() {
     res.write('lazy response');
     res.end();
  }, req.query.delay);
});

// Chunked response
app.get('/chunked', function(req, res) {
  res.setHeader('Transfer-Encoding', 'chunked');
  res.setHeader('Content-Type', 'text/html');

  var count = 0;
  var chunked_response = function() {
    if (100 < count) {
      res.end();
      return;
    }

    count = count + 1;
    res.write('<p>message: ' + count + '</p>');
    setTimeout(chunked_response, 10);
  };

  chunked_response();
});

if (argv.port === undefined) {
  argv.port = 18000;
}

function notifyStartup(fd) {
  fs.write(fd, '\0');
}

function onServerStartComplete() {
  if (argv.startup_pipe === undefined) {
    return;
  }
  notifyStartup(argv.startup_pipe);
}

function runSpdy() {
  spdy.createServer(ssl_options, app).listen(argv.port, onServerStartComplete);
}

function runHttps() {
  https.createServer(ssl_options, app).listen(argv.port, onServerStartComplete);
}

function runHttp2() {
  http2.createServer(ssl_options, function (req, res) {
    res.end('hello world');
  }).listen(argv.port, onServerStartComplete);
}

function runHttp() {
  http.createServer(app).listen(argv.port, onServerStartComplete);
}

function runProxy() {
  var proxy_server = proxy.createProxyServer();
  http.createServer(function (req, res) {
    proxy_server.web(req, res, {
      target: 'http://' + req.headers.host,
    });
  }).listen(argv.port, onServerStartComplete);
}

if (argv.spdy) {
  runSpdy();
} else if (argv.https) {
  runHttps();
} else if (argv.http2) {
  runHttp2();
} else if (argv.http) {
  runHttp();
} else if (argv.proxy) {
  runProxy();
} else {
  throw new Error('unknown server type error');
}
