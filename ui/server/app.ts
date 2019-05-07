import express = require("express");
import udp = require("dgram");
import fs = require("fs");
import cors = require("cors");

const app = express();
const port = 3001;

app.use(cors());
app.use(express.static("../client/resources/public"));

app.get("/thing", (req, res) => {
    console.log("got thing"); 
    res.send(JSON.stringify({"value": "hi"}));
})

app.get("/node_metadata", (req, res) => {
    let arr = [];
    fs.readdirSync("../../node-metadata")
      .forEach(filename => {
        let buf = fs.readFileSync("../../node-metadata/" + filename);
        arr.push(JSON.parse(buf.toString()));
      });
    //res.send(JSON.stringify({"definitions": arr}));
    res.send(arr);
})

app.listen(port, () => console.log(`listening on ${port}`));
