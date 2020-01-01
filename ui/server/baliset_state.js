/* Currently, the baliset audio application doesn't provide any visibility into its state.
 * For now, just storing it here in the ui's server, optimistically assuming there's no dropped packets and everything just works.
 */

let patch = {};
patch.last_node_id = 0;
patch.nodes = {};
patch.connections = [];

clearPatch = function() {
  patch.last_node_id = 0;
  patch.nodes = {};
  patch.connections = [];
}

exports.getPatch = () => patch;
exports.getNodes = () => patch.nodes;
exports.getConnections = () => patch.connections;

exports.addNode = function(n) {
  let node_id = patch.last_node_id++;
  patch.nodes[node_id] = {"type": n.type, "x": n.x, "y": n.y, "controls": {}};
  return node_id;
}

exports.deleteNode = function(msg) {
  delete patch.nodes[msg.node_id];

  let outdatedConnections = [];

  patch.connections.filter(conn => (conn[0] != msg.node_id != conn[2] != msg.node_id));

  patch.connections.forEach((conn, idx) => {
    if(conn[0] == msg.node_id || conn[2] == msg.node_id) {
      outdatedConnections.push(idx);
    }
  });

  for(let i = outdatedConnections.length - 1; i >= 0; i--) {
    patch.connections.splice(outdatedConnections[i], 1);
  }
}

exports.moveNode = function(msg) {
  let n = patch.nodes[msg.node_id]
  if(n !== undefined) {
    n.x = msg.x;
    n.y = msg.y;
  }
  return {"route": "/node/moved", "node_id": msg.node_id, "x": msg.x, "y": msg.y};
}

exports.controlNode = function(msg) {
  let n = patch.nodes[msg.node_id]
  console.log(`node: ${JSON.stringify(n)}`);
  if(n !== undefined) {
    n.controls[msg.control_id] = msg.value;
  }
}

function arrayEquals(arr1, arr2) {
  if(arr1.length != arr2.length) return false;
  for(let i = 0; i < arr1.length; i++) {
    if(arr1[i] != arr2[i]) return false;
  }
  return true;
}

//TODO this is horribly inefficient, linear search over connections for every insert or delete
exports.connectNode = function(msg) {
  const conn = [msg.out_node_id, msg.outlet_idx, msg.in_node_id, msg.inlet_idx];
  for (c of patch.connections) {
    if(arrayEquals(c, conn)) {console.log('found a match'); return;}
  }
  patch.connections.push(conn);
}

exports.disconnectNode = function(msg) {
  const conn = [msg.out_node_id, msg.outlet_idx, msg.in_node_id, msg.inlet_idx];
  for(let i = 0; i < patch.connections.length; i++) {
    if(arrayEquals(patch.connections[i], conn)) {
      patch.connections.splice(i, 1);
      return;
    }
  }
}

exports.loadPatch = function(p) {
  clearPatch();
  let nodeIdMap = {};
  for (const id in p.nodes) {
    //it's maybe not guaranteed that for ... in will follow same order as it did in osc_client.loadPatch - but I mean I dunno, hopefully it does.
    const n = p.nodes[id];
    const node_id = patch.last_node_id++;
    nodeIdMap[id] = node_id;
    patch.nodes[node_id] = n;
  }

  for (const conn of p.connections) {
    let out_node_id, outlet_idx, in_node_id, inlet_idx;
    [out_node_id, outlet_idx, in_node_id, inlet_idx] = conn;
    patch.connections.push([nodeIdMap[out_node_id], outlet_idx,
                            nodeIdMap[in_node_id], inlet_idx]);
  }
}
