
/*********************************************************************
 * NAN - Native Abstractions for Node.js
 *
 * Copyright (c) 2015 NAN contributors
 *
 * MIT License <https://github.com/nodejs/nan/blob/master/LICENSE.md>
 ********************************************************************/

const testRoot = require('path').resolve(__dirname, '..')
    , DepthCamera = require('bindings')({ module_root: testRoot, bindings: 'DepthCamera' }).DepthCamera
    , EventEmitter = require('events').EventEmitter;

// extend prototype
function inherits(target, source) {
    for (var k in source.prototype) {
        target.prototype[k] = source.prototype[k];
    }
}

inherits(DepthCamera, EventEmitter);

var dc = new DepthCamera();

var samples = 0;

dc.on('newDepthSample', function() {
    console.log('Depth Sample #' + samples++);
});

dc.start(function(e) {
  console.log('start: ' + e);
  setTimeout(stop, 3000);
});

function stop() {
  dc.stop(function(e) {
    console.log('stop: ' + e);
  });
}
