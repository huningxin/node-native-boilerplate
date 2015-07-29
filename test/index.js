
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

var depthMapSize = 76800; // QVGA
var depthMap = new Int16Array(depthMapSize); 

dc.on('newDepthMap', function() {
    dc.getDepthMap(depthMap, function(error) {
      if (typeof error == 'undefined') {
        console.log('Got depth map #' + samples++);
      } else {
        console.log('Got depth map failed: ' + error);
      }
    });
});

dc.start(function(error) {
  if (typeof error === 'undefined') {
    console.log('start successfully.');
    setTimeout(stop, 3000);  
  } else {
    console.log('start failed: ' + error);
  }
});

function stop() {
  dc.stop(function(error) {
    if (typeof error === 'undefined') {
      console.log('stop successfully.');
    } else {
      console.log('stop failed: ' + error);
    }
  });
}
