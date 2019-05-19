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
  const oscMsg = {address: '/node/add',
                  args: [msg.type]};
  console.log(`sending ${JSON.stringify(oscMsg)}`);
  const buf = osc.toBuffer(oscMsg);
  baliset_sock.send(buf, 0, buf.length, 9000, baliset_host);
  const node_id = baliset_state.addNode(msg);
  return {"route": "/node/added", "type": msg.type, "node_id": node_id, "x": msg.x, "y": msg.y};
}

exports.connectNode = function(msg) {
  const oscMsg = {address: '/node/connect',
                  args: [{type: "integer", value: msg.out_node_id},
                         {type: "integer", value: msg.outlet_idx},
                         {type: "integer", value: msg.in_node_id},
                         {type: "integer", value: msg.inlet_idx}]};
  console.log(`sending ${JSON.stringify(oscMsg)}`);
  const buf = osc.toBuffer(oscMsg);
  baliset_sock.send(buf, 0, buf.length, 9000, baliset_host);
  baliset_state.connectNode(msg);
  return {"route": "/node/connected", "out_node_id": msg.out_node_id,
          "outlet_idx": msg.outlet_idx, "in_node_id": msg.in_node_id, "inlet_idx": msg.inlet_idx};
}

exports.disconnectNode = function(msg) {
  const oscMsg = {address: '/node/disconnect',
                  args: [{type: "integer", value: msg.out_node_id},
                         {type: "integer", value: msg.outlet_idx},
                         {type: "integer", value: msg.in_node_id},
                         {type: "integer", value: msg.inlet_idx}]};
  console.log(`sending ${JSON.stringify(oscMsg)}`);
  const buf = osc.toBuffer(oscMsg);
  baliset_sock.send(buf, 0, buf.length, 9000, baliset_host);
  baliset_state.disconnectNode(msg);
  return {"route": "/node/disconnected", "out_node_id": msg.out_node_id,
          "outlet_idx": msg.outlet_idx, "in_node_id": msg.in_node_id, "inlet_idx": msg.inlet_idx};
}

exports.deleteNode = function(msg) {
  const oscMsg = {address: '/node/delete', args: [{type: "integer", value: msg.node_id}]};
  console.log(`sending ${JSON.stringify(oscMsg)}`);
  const buf = osc.toBuffer(oscMsg);
  baliset_sock.send(buf, 0, buf.length, 9000, baliset_host);
}

exports.controlNode = function(msg) {
  const oscMsg = {address: '/node/control', args: [{type: "integer", value: msg.node_id},
                                                   {type: "integer", value: msg.control_id},
                                                   {type: "float",   value: msg.value}]};
  console.log(`sending ${JSON.stringify(oscMsg)}`);
  const buf = osc.toBuffer(oscMsg);
  baliset_sock.send(buf, 0, buf.length, 9000, baliset_host);
}
