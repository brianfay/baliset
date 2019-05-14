const osc = require("osc-min")
const udp = require("dgram")
const baliset_state = require("./baliset_state");
const baliset_port = 7563;
const baliset_host = "localhost";
const baliset_remote_port = 9000;

//baliset socket
const baliset_sock= udp.createSocket('udp4', function(msg, rinfo){
  try {
    return console.log(osc.fromBuffer(msg))
  } catch (error) {
    return console.log('invalid osc packet')
  }
});

baliset_sock.bind(baliset_port);

baliset_sock.on("error", function(err) {
  console.log(`error on baliset socket:\n ${err.stack}`);
});

exports.addNode = function(msg) {
  console.log(`adding node: ${msg.type}`);
  let oscMsg = {address: '/node/add',
                args: [msg.type]};
  let buf = osc.toBuffer(oscMsg);
  baliset_sock.send(buf, 0, buf.length, 9000, baliset_host);
  let node_id = baliset_state.addNode(msg);
  return {"route": "/node/added", "type": msg.type, "node_id": node_id, "x": msg.x, "y": msg.y};
}

exports.connectNode = function(msg) {
  let oscMsg = {address: '/node/connect', 
                args: [{type: "integer", value: msg.out_node_id},
                       {type: "integer", value: msg.outlet_id},
                       {type: "integer", value: msg.in_node_id},
                       {type: "integer", value: msg.inlet_id}]};
  let buf = osc.toBuffer(oscMsg);
  baliset_sock.send(buf, 0, buf.length, 9000, baliset_host);
}

exports.disconnectNode = function(msg) {
  let oscMsg = {address: '/node/disconnect', 
                args: [{type: "integer", value: msg.out_node_id},
                       {type: "integer", value: msg.outlet_id},
                       {type: "integer", value: msg.in_node_id},
                       {type: "integer", value: msg.inlet_id}]};
  let buf = osc.toBuffer(oscMsg);
  baliset_sock.send(buf, 0, buf.length, 9000, baliset_host);
}

exports.deleteNode = function(msg) {
  let oscMsg = {address: '/node/delete', args: [{type: "integer", value: msg.node_id}]};
  let buf = osc.toBuffer(oscMsg);
  baliset_sock.send(buf, 0, buf.length, 9000, baliset_host);
}

exports.controlNode = function(msg) {
  let oscMsg = {address: '/node/control', args: [{type: "integer", value: msg.node_id},
                                                 {type: "integer", value: msg.control_id},
                                                 {type: "float",   value: msg.value}]};
  let buf = osc.toBuffer(oscMsg);
  baliset_sock.send(buf, 0, buf.length, 9000, baliset_host);
}
