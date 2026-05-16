const express = require("express");
const cors = require("cors");
const bodyParser = require("body-parser");
const { spawn } = require("child_process"); // ✅ IMPORTANT
const path = require("path");
//const stations = require("./stations.json");
const fs = require("fs");

const stations = JSON.parse(
  fs.readFileSync(path.join(__dirname, "stations.json"), "utf-8")
);
console.log("🔥 Sample station:", stations[0]); 

console.log("🔥 SERVER FILE LOADED");

const app = express();
app.use(cors());
app.use(bodyParser.json());
app.get("/", (req,res)=>{
  res.send("SERVER WORKING");
});

console.log("Stations loaded:", stations.length);

//console.log("🔥 Sample station:", stations[0]);
/*
app.get("/api/stations", (req, res) => {
  console.log("Stations API HIT", req.query.q);  // 👈 ADD THIS

  const query = (req.query.q || "").toLowerCase();

  if (!query) return res.json([]);
*/
app.get("/api/stations", (req, res) => {

  const query = (req.query.q || "").toLowerCase().trim();

  console.log("🔥 Stations API HIT:", query);

  if (!query) return res.json([]);

  const results = stations
    .filter(s => {
      // ✅ ensure valid format
      if (!Array.isArray(s) || s.length < 2) return false;

      const code = String(s[0]).toLowerCase();
      const name = String(s[1]).toLowerCase();

      return (
        code.includes(query) ||     // match code
        name.includes(query) ||     // match name
        name.startsWith(query)      // better UX
      );
    })
    .slice(0, 10)
    .map(s => ({
      code: s[0],
      name: s[1]
    }));

  console.log("✅ Results:", results.length);

  res.json(results);
});



app.get("/", (req, res) => {
  console.log("ROOT HIT");
  res.send("SERVER WORKING ✅");
});


let isRunning = false; 
app.post("/api/run", (req, res) => {

  if (isRunning) {
    return res.json({ result: "⏳ Please wait, previous request still processing..." });
  }

  isRunning = true;
//app.post("/api/run", (req, res) => {
  const { mode, source, destination } = req.body;

  const exePath = path.join(__dirname, "./cpp_core/myProgram");

  const args = [
    mode === "direct" ? "1" :
    mode === "allRoutes" ? "2" :
    mode === "stations" ? "3" :
    mode === "tour" ? "4" :
    mode === "attractions" ? "5" :
    mode === "maps" ? "6" :
    mode === "planner" ? "7" :
    mode === "indirect" ? "8" : "1",

    (source || "").toUpperCase(),
    (destination || "").toUpperCase()
  ];

  console.log("🚀 Running:", exePath);
  console.log("ARGS:", args);

  const child = spawn(exePath, args, {
    cwd: path.join(__dirname, "./cpp_core")
  });

  let output = "";
  let errorOutput = "";

  // capture stdout
  child.stdout.on("data", (data) => {
    output += data.toString();
  });

  // capture stderr
  child.stderr.on("data", (data) => {
    errorOutput += data.toString();
  });

  // error handler
  child.on("error", (err) => {
    console.error("SPAWN ERROR:", err);
    return res.status(500).json({ error: "Failed to start program" });
  });

  // final response
  child.on("close", (code) => {
      isRunning = false; 
    console.log("Process closed:", code);

    if (output.trim().length > 0) {
      return res.json({ result: output });
    }

    return res.status(500).json({
      error: errorOutput || "No output"
    });
  });
});

app.listen(5000, () =>
  console.log("🚆 Backend running at http://localhost:5000")
);
