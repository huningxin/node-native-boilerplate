
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

var obj = new DepthCamera();
obj.on('event', function() {
    console.log('event');
});

obj.call_emit();
