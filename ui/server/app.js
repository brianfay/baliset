"use strict";
const osc = require('osc-min')
const udp = require('dgram')
const express = require("express");
const fs = require("fs");
const cors = require("cors");
const WebSocket = require("ws");
const app = express();
const web_port = 3001;
const websocket_port = 3002;
const baliset_port = 7563;
const baliset_remote_port = 9000;
const baliset_host = "localhost";

//express
app.use(cors());

app.use(express.static("../client/resources/public"));

app.get("/node_metadata", function (req, res) {
    var arr = [];
    fs.readdirSync("../../node-metadata")
        .forEach(function (filename) {
        var buf = fs.readFileSync("../../node-metadata/" + filename);
        arr.push(JSON.parse(buf.toString()));
    });
    res.send(arr);
});

app.listen(web_port, function () {return console.log("express listening on " + web_port)});

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

//websockets
const wss = new WebSocket.Server({port: websocket_port});

function handleMessage(msg) {
  msg = JSON.parse(msg);
  if(msg.route === undefined) {
    console.log(`expected message to be a json array containing a route parameter, got ${msg}`);
    return;
  }
  switch(msg.route){
  case "/node/add":
    addNode(msg);
    break;
  case "/node/connect":
    connectNode(msg);
    break;
  case "/node/disconnect":
    disconnectNode(msg);
    break;
  case "/node/delete":
    deleteNode(msg);
    break;
  default:
    console.log(`unrecognized websocket msg: ${route}`);
  }
}

function addNode(msg) {
  console.log(`adding node: ${msg.type}`);
  let buf = osc.toBuffer({address: '/node/add',
                          args: [msg.type]});

  baliset_sock.send(buf, 0, buf.length, 9000, baliset_host, (err) => console.log(`callback: ${err}`));
}

function connectNode(msg) {
  let buf = osc.toBuffer({address: '/node/connect', args: [{type: "integer", value: msg.out_node_id},
                                                           {type: "integer", value: msg.outlet_id},
                                                           {type: "integer", value: msg.in_node_id},
                                                           {type: "integer", value: msg.inlet_id}]});
  baliset_sock.send(buf, 0, buf.length, 9000, baliset_host);
}

function disconnectNode(msg) {
  let buf = osc.toBuffer({address: '/node/disconnect', args: [{type: "integer", value: msg.out_node_id},
                                                              {type: "integer", value: msg.outlet_id},
                                                              {type: "integer", value: msg.in_node_id},
                                                              {type: "integer", value: msg.inlet_id}]});
  baliset_sock.send(buf, 0, buf.length, 9000, baliset_host);
}

function deleteNode(msg) {
  let buf = osc.toBuffer({address: '/node/delete', args: [{type: "integer", value: msg.node_id}]});
  baliset_sock.send(buf, 0, buf.length, 9000, baliset_host);
}

function controlNode(msg) {
  let buf = osc.toBuffer({address: '/node/control', args: [{type: "integer", value: msg.node_id},
                                                           {type: "integer", value: msg.control_id},
                                                           {type: "float",   value: msg.value}]});
  baliset_sock.send(buf, 0, buf.length, 9000, baliset_host);
}

wss.on("connection", function connection(ws) {
  console.log("got a connection.");
  console.log(`clients: ${wss.clients}`);
  wss.clients.forEach(function(client){console.log(`client: ${client}`)})
  ws.on("message", handleMessage);
  ws.on("close", function close(){
    console.log("disconnected.");
  })
  ws.send("something");
});
