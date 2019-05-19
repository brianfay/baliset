/* Currently, the baliset audio application doesn't provide any visibility into its state.
 * For now, just storing it here in the ui's server, optimistically assuming there's no dropped packets and everything just works.
 */

let last_node_id = 0;
let nodes = {};

let connections = [];

exports.getNodes = () => nodes;
exports.getConnections = () => connections;

exports.addNode = function(n) {
  let node_id = last_node_id++;
  nodes[node_id] = {"type": n.type, "x": n.x, "y": n.y};
  return node_id;
}

exports.moveNode = function(msg) {
  console.log(`moving node: ${JSON.stringify(msg)}`);
  let n = nodes[msg.node_id]
  if(nodes[msg.node_id] !== undefined) {
    nodes[msg.node_id].x = msg.x;
    nodes[msg.node_id].y = msg.y;
  }
  return {"route": "/node/moved", "node_id": msg.node_id, "x": msg.x, "y": msg.y};
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
  connections.forEach(function(c) {
    if(arrayEquals(c, conn)) return;
  });
  connections.push(conn);
}

exports.disconnectNode = function(msg) {
  const conn = [msg.out_node_id, msg.outlet_idx, msg.in_node_id, msg.inlet_idx];
  for(let i = 0; i < connections.length; i++) {
    if(arrayEquals(connections[i], conn)) {
      connections.splice(i, 1);
      return;
    }
  }
}
