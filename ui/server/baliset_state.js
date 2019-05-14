
/* Currently, the baliset audio application doesn't provide any visibility into its state.
 * For now, just storing it here in the ui's server, optimistically assuming there's no dropped packets and everything just works.
 */

let last_node_id = 0;
nodes = {};

exports.getNodes = () => nodes;

exports.addNode = function(n) {
  let node_id = last_node_id++;
  nodes[node_id.toString()] = {"type": n.type, "x": n.x, "y": n.y};
  return node_id.toString();
}

exports.moveNode = function(msg) {
  let n = nodes[msg.node_id]
  if(nodes[msg.node_id] !== undefined) {
    nodes[msg.node_id].x = msg.x;
    nodes[msg.node_id].y = msg.y;
  }
  return {"route": "/node/move", "node_id": msg.node_id, "x": msg.x, "y": msg.y};
}
