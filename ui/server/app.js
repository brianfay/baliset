"use strict";
const osc_client = require("./osc_client");
const express = require("express");
const fs = require("fs");
const cors = require("cors");
const WebSocket = require("ws");
const baliset_state = require("./baliset_state");
const app = express();
const web_port = 3001;
const websocket_port = 3002;

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

//websockets
const wss = new WebSocket.Server({port: websocket_port});

wss.broadcast = function broadcast(data) {
  //console.log(`broadcasting: ${JSON.stringify(data)}`);
  wss.clients.forEach(function each(client) {
    if (client.readyState === WebSocket.OPEN) {
      client.send(JSON.stringify(data));
    }
  });
};

wss.notifyOtherClients = function(ws, data) {
  wss.clients.forEach(function(client) {
    //console.log(`sending move node: ${JSON.stringify(data)}`);
    if (client !== ws && client.readyState == WebSocket.OPEN) {
      client.send(JSON.stringify(data));
    }
  });
}

function messageHandler(ws) {
  const handler = function(msg) {
    msg = JSON.parse(msg);
    console.log(`got message: ${msg}`);
    if(msg.route === undefined) {
      console.log(`expected message to be a json array containing a route parameter, got ${msg}`);
      return;
    }
    switch(msg.route) {
      case "/node/add": {
        const clientMsg = osc_client.addNode(msg);
        wss.broadcast(clientMsg);
        break;
      }
      case "/node/connect": {
        const clientMsg = osc_client.connectNode(msg);
        wss.broadcast(clientMsg);
        break;
      }
      case "/node/control": {
        const clientMsg = osc_client.controlNode(msg);
        //wss.notifyOtherClients(clientMsg);
        break;
      }
      case "/node/disconnect": {
        const clientMsg = osc_client.disconnectNode(msg);
        wss.broadcast(clientMsg);
        break;
      }
      case "/node/delete": {
        const clientMsg = osc_client.deleteNode(msg);
        wss.broadcast(clientMsg);
        break;
      }
      case "/node/move": {
        const clientMsg = baliset_state.moveNode(msg);
        wss.notifyOtherClients(ws, clientMsg);
        break;
      }
      default:
        console.log(`unrecognized websocket msg: ${msg.route}`);
      }
  }
 return handler;
}


wss.on("connection", function connection(ws) {
  console.log("got a connection.");
  //console.log(`clients: ${wss.clients}`);
  //wss.clients.forEach(function(client){console.log(`client: ${client}`)})
  ws.on("message", messageHandler(ws));
  ws.on("close", function close(){
    console.log("disconnected.");
  })
  ws.send(JSON.stringify({"route": "/app_state",
                          "nodes": baliset_state.getNodes(),
                          "connections": baliset_state.getConnections()}));
});
