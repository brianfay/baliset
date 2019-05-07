const osc = require('osc-min')
const udp = require('dgram')

const sock = udp.createSocket('udp4', function(msg, rinfo){
  try {
    return console.log(osc.fromBuffer(msg))
  } catch (error) {
    return console.log('invalid osc packet')
  }
});

sock.bind(7563);

function add(node_type){
  buf = osc.toBuffer({address: '/node/add',
                      args: [node_type]})

  sock.send(buf, 0, buf.length, 9000, "localhost")
}

function connect(out_node_id, outlet_id, in_node_id, inlet_id){
  let buf = osc.toBuffer({address: '/node/connect', args: [{type: "integer", value: out_node_id},
                                                           {type: "integer", value: outlet_id},
                                                           {type: "integer", value: in_node_id},
                                                           {type: "integer", value: inlet_id}]});
  sock.send(buf, 0, buf.length, 9000, "localhost");
}

function disconnect(out_node_id, outlet_id, in_node_id, inlet_id){
  let buf = osc.toBuffer({address: '/node/disconnect', args: [{type: "integer", value: out_node_id},
                                                              {type: "integer", value: outlet_id},
                                                              {type: "integer", value: in_node_id},
                                                              {type: "integer", value: inlet_id}]});
  sock.send(buf, 0, buf.length, 9000, "localhost");
}

function delete_node(node_id){
  let buf = osc.toBuffer({address: '/node/delete', args: [{type: "integer", value: node_id}]});
  sock.send(buf, 0, buf.length, 9000, "localhost");
}

function control(node_id, control_id, value){
  let buf = osc.toBuffer({address: '/node/control', args: [{type: "integer", value: node_id},
                                                           {type: "integer", value: control_id},
                                                           {type: "float", value: value}]});
  sock.send(buf, 0, buf.length, 9000, "localhost");
}

add('sin');
add('sin');
add('dac');

connect(0,0,2,0);
connect(1,0,2,1);
control(0,0,175);
control(0,0,110);

setInterval(function(){
  let freq1 = Math.ceil(Math.random() * 7) * 110;
  console.log("freq 1: ", freq1);
  let freq2 = Math.ceil(Math.random() * 7) * 110;
  console.log("freq 2: ", freq2);
  control(0, 0, freq1);
  control(1, 0, freq2);
}, 1000)
