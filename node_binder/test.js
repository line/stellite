
const stellite = require('../build_mac/src/out_mac/stellite.node');

function TestObject() {
}

function createTestObject() {
  return new TestObject();
}

module.exports.createTestObject = createTestObject;
