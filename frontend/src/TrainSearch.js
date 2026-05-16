import React, { useState } from "react";
import axios from "axios";
import { useRef, useEffect } from "react";

export default function TrainSearch() {
const [mode, setMode] = useState("direct");
const [source, setSource] = useState("");
const [destination, setDestination] = useState("");
const [loading, setLoading] = useState(false);
const [showOverview, setShowOverview] = useState(false);
const [suggestions, setSuggestions] = useState([]);
const [activeInput, setActiveInput] = useState(null); // "source" or "dest"
const suggestionRef = useRef(null);

//const [results, setResults] = useState([]);
const [results, setResults] = useState({
  direct: [],
  allRoutes: [],
  stations: [],
  tour: [],
  attractions: [],
  maps: [],
  planner: [],
  indirect: []
});

const button = {
  padding: "10px 20px",
  marginTop: "10px",
  background: "#0072ff",
  color: "white",
  border: "none",
  borderRadius: "8px",
  cursor: "pointer",
  animation: "pulse 1s infinite"
};



const modes = [
{ id: "direct", label: "🚆 Direct" },
{ id: "allRoutes", label: "🔄 All Routes" },
{ id: "stations", label: "📍 Nearby" },
{ id: "tour", label: "🌍 Tours" },
{ id: "attractions", label: "🏛 Attractions" },
{ id: "maps", label: "🗺 Maps" },
{ id: "planner", label: "🧠 Planner" },
{ id: "indirect", label: "⚡ Indirect Routes" }
];

const handleSearch = async () => {
  try {
    setLoading(true);   // ✅ START LOADING

    const res = await axios.post("http://localhost:5000/api/run", {
      mode,
      source,
      destination
    });

    const lines = res.data.result
      .split("\n")
      .filter((l) => l.trim() !== "");

    setResults(prev => ({
      ...prev,
      [mode]: lines
    }));

  } catch (err) {
    console.error(err);
  } finally {
    setLoading(false);  // ✅ STOP LOADING
  }
};

//const timeoutRef = React.useRef(null);
/*
const fetchSuggestions = (value) => {
  if (timeoutRef.current) clearTimeout(timeoutRef.current);

  timeoutRef.current = setTimeout(async () => {
    if (!value) {
      setSuggestions([]);
      return;
    }

    try {
      const res = await axios.get("http://localhost:5000/api/stations", {
        params: { q: value }
      });

      console.log("Suggestions:", res.data);
      setSuggestions(res.data);

    } catch (err) {
      console.error("Suggestion error:", err);
    }
    console.log("Typing:", value);
  }, 300);
};
*/
const fetchSuggestions = async (value) => {
  console.log("Typing:", value); // ✅ debug

  if (!value || value.length < 1) {
    setSuggestions([]);
    return;
  }

  try {
    const res = await fetch(
      `http://localhost:5000/api/stations?q=${value}`
    );

    const data = await res.json();

    console.log("Suggestions:", data); // ✅ debug

    setSuggestions(data);
  } catch (err) {
    console.error("Fetch error:", err);
  }
};




const renderInputs = () => {
switch (mode) {
case "stations":
return (
<input
placeholder="📍 Enter Location"
value={source}
onChange={(e) => setSource(e.target.value)}
style={input}
/>
);

case "tour":
return (
  <div style={{ display: "flex", gap: "10px" }}>
    
    <input
      placeholder="🏙 Enter City"
      value={source}
      onChange={(e) => setSource(e.target.value)}
      style={input}
    />

    <input
      placeholder="📅 Days"
      value={destination}
      onChange={(e) => setDestination(e.target.value)}
      style={input}
    />

  </div>
);

  case "attractions":
    return (
      <input
        placeholder="🏛 Enter City"
        value={source}
        onChange={(e) => setSource(e.target.value)}
        style={input}
      />
    );
default:
return (
  <div style={{
    display: "flex",
    gap: "10px",
    alignItems: "center",
    position: "relative"
  }}>

    {/* SOURCE */}
    <div style={{ position: "relative", flex: 1 }}>
      <input
        placeholder="📍 Source"
        value={source}
        onChange={(e) => {
          setSource(e.target.value);
          fetchSuggestions(e.target.value);
          setActiveInput("source");
        }}
        onFocus={() => setActiveInput("source")}
        style={input}
      />

      {activeInput === "source" && suggestions.length > 0 && (
        <Dropdown
          suggestions={suggestions}
          onSelect={(s) => {
            setSource(s.code);
            setSuggestions([]);
          }}
        />
      )}
    </div>

    {/* DESTINATION */}
    <div style={{ position: "relative", flex: 1 }}>
      <input
        placeholder="🏁 Destination"
        value={destination}
        onChange={(e) => {
          setDestination(e.target.value);
          fetchSuggestions(e.target.value);
          setActiveInput("dest");
        }}
        onFocus={() => setActiveInput("dest")}
        style={input}
      />

      {activeInput === "dest" && suggestions.length > 0 && (
        <Dropdown
          suggestions={suggestions}
          onSelect={(s) => {
            setDestination(s.code);
            setSuggestions([]);
          }}
        />
      )}
    </div>

  </div>
);

}

};

function Dropdown({ suggestions, onSelect }) {
  return (
    <div style={{
      position: "absolute",
      top: "100%",
      left: 0,
      right: 0,
      background: "#0f172a",
      borderRadius: "12px",
      marginTop: "6px",
      maxHeight: "260px",
      overflowY: "auto",
      zIndex: 999,
      boxShadow: "0 12px 30px rgba(0,0,0,0.6)"
    }}>
      <div ref={suggestionRef}>
      {suggestions.map((s, i) => (
        <div
          key={i}
          onClick={() => onSelect(s)}
          style={{
            padding: "12px",
            cursor: "pointer",
            borderBottom: "1px solid rgba(255,255,255,0.05)",
            transition: "0.2s"
          }}
          onMouseEnter={(e) =>
            e.currentTarget.style.background = "#1e293b"}
          onMouseLeave={(e) =>
            e.currentTarget.style.background = "transparent"}
        >
          <strong style={{ color: "#38bdf8" }}>{s.code}</strong> — {s.name}
          <div style={{ fontSize: "0.8rem", opacity: 0.7 }}>
            {s.division}
          </div>
        </div>
      ))}
      </div>
    </div>
  );
}




const parsedTrains = parseTrains(results.direct || []);
const parsedRoutes = parseRoutes(results.allRoutes || []);
const parsedStations = parseStations(results.stations || []);
const mapData = parseMap(results.maps || []);
const parsedIndirect = parseIndirect(results.indirect || []);
//const plannerData = parsePlanner(results.planner || []);

/*
results.planner?.forEach(line => {
  if (line.startsWith("PLAN_TYPE")) {
    if (current.length) {
      plans.push(parsePlanner(current));
      current = [];
    }
  }
  current.push(line);
});
*/
const plans = [];
let current = [];
let currentType = "";

// 🔥 parse properly with type
results.planner?.forEach(line => {
  if (line.startsWith("PLAN_TYPE")) {

    if (current.length) {
      const parsed = parsePlanner(current);
      parsed.type = currentType;
      plans.push(parsed);
      current = [];
    }

    currentType = line.includes("DIRECT") ? "DIRECT" : "AI";
  } else {
    current.push(line);
  }
});

if (current.length) {
  const parsed = parsePlanner(current);
  parsed.type = currentType;
  plans.push(parsed);
}

// 🔥 FINAL FILTER (IMPORTANT)
let hasAI = false;

const filteredPlans = plans.filter(p => {
  if (p.type === "AI") {
    if (hasAI) return false;  // ❌ skip duplicate AI
    hasAI = true;
  }
  return true;
});



if (current.length) plans.push(parsePlanner(current));
const directStyle = {
  background: "linear-gradient(135deg,#065f46,#10b981)",
  boxShadow: "0 0 20px rgba(16,185,129,0.5)"
};

const aiStyle = {
  background: "linear-gradient(135deg,#1e3a8a,#3b82f6)",
  boxShadow: "0 0 20px rgba(59,130,246,0.5)"
};

const overviewData = {
  direct: "Find direct trains between two stations across India using a database of over 11,113 trains.",
  
  allRoutes: "Discover up to 3 optimized bus routes between locations across India, including smart suggestions and travel insights.",

  stations: "Get the top 5 nearest railway stations from your entered location across India, with quick access to Google Maps directions.",

  tour: "Plan multi-day exploration journeys with curated travel routes and destination recommendations across India.",

  attractions: "Explore popular attractions and key places in any city across India along with useful details.",

  maps: "Get Google Maps directions between your source and destination, with a direct link to navigate easily.",

  planner: "Generate a complete journey plan using two approaches: direct routes and AI-powered smart indirect routes across India.",

  indirect: "Find indirect train routes with transfers at intermediate stations, optimized for travel time and efficiency across India."
};
useEffect(() => {
  function handleClickOutside(event) {
    if (
      suggestionRef.current &&
      !suggestionRef.current.contains(event.target)
    ) {
      setSuggestions([]);
    }
  }

  document.addEventListener("mousedown", handleClickOutside);
  return () => document.removeEventListener("mousedown", handleClickOutside);
}, []);




return (
<div style={{ ...container, background: getBackground(mode) }}> <h1 style={title}>🚆 Indian Travel Planner</h1> <p style={subtitle}>Smart Travel • Beautiful UI ✨</p>

  <div style={grid}>
    {modes.map((m) => (
      <div
        key={m.id}
       // onClick={() => setMode(m.id)}
        onClick={() => {
  setMode(m.id);
  setSource("");
  setDestination("");
}}
        style={{
          ...card,
          background: mode === m.id ? "#00eaff" : "#1f1f1f",
          color: mode === m.id ? "#000" : "#fff"
        }}
      >
        {m.label}
      </div>
    ))}
  </div>

{showOverview && (
  <div style={{
    marginTop: "10px",
    marginBottom: "15px",
    padding: "15px",
    borderRadius: "12px",
    background: "linear-gradient(135deg,#1e293b,#0f172a)",
    borderLeft: "4px solid #38bdf8",
    boxShadow: "0 8px 20px rgba(0,0,0,0.3)",
    animation: "fadeIn 0.3s ease"
  }}>
    <p style={{ lineHeight: "1.6", color: "#cbd5f5" }}>
      {overviewData[mode]}
    </p>
  </div>
)}




    <div style={{ ...searchBox, position: "relative", zIndex: 1000 }}>
    {renderInputs()}
   <button onClick={handleSearch} style={{
  ...button,
  opacity: loading ? 0.6 : 1,
  transform: loading ? "scale(0.95)" : "scale(1)",
  transition: "0.2s",
  cursor: loading ? "not-allowed" : "pointer"
}}>
  {loading ? "⏳ Searching..." : "🔍 Search"}
</button>
  </div>

  <div style={resultsBox}>
{mode === "direct" && (
  <div>
    <div style={{ display: "flex", alignItems: "center", gap: "10px" }}>
  <h2>🚆 Direct Trains</h2>

  <button
    onClick={() => setShowOverview(!showOverview)}
    style={{
      background: "linear-gradient(135deg,#38bdf8,#6366f1)",
      border: "none",
      borderRadius: "50%",
      width: "32px",
      height: "32px",
      color: "white",
      cursor: "pointer",
      fontWeight: "bold"
    }}
  >
    ℹ️
  </button>
</div>

    {/* ✅ If parsed works */}
    {parsedTrains.length > 0 ? (
      parsedTrains.map((t, i) => (
        <div key={i} style={{
          background: "linear-gradient(135deg,#1e3c72,#2a5298)",
          padding: "20px",
          borderRadius: "15px",
          marginBottom: "15px",
          boxShadow: "0 10px 25px rgba(0,0,0,0.3)"
        }}>
          <h2>🚆 {t.name}</h2>
          <p>📟 Train No: {t.number}</p>

          <div style={{ display: "flex", justifyContent: "space-between" }}>
            <div>🕒 {t.departure} ({t.from})</div>
            <div>🕘 {t.arrival} ({t.to})</div>
          </div>

          <p>⏱ Duration: {t.duration}</p>
          <p>📏 Distance: {t.distance} km</p>
          <p>🏷 {t.zone} | {t.type}</p>
        </div>
      ))
    ) : (
      // 🔥 fallback (VERY IMPORTANT)
      results.direct.map((r, i) => (
        <div key={i} style={resultCard}>{r}</div>
      ))
    )}
  </div>
)}

{mode === "allRoutes" && (
  <div>
<div style={{ display: "flex", alignItems: "center", gap: "10px" }}>
  <h2 style={{ marginBottom: "20px" }}>🧭 Intelligent Routes</h2>

  <button
    onClick={() => setShowOverview(!showOverview)}
    style={{
      background: "linear-gradient(135deg,#38bdf8,#6366f1)",
      border: "none",
      borderRadius: "50%",
      width: "32px",
      height: "32px",
      color: "white",
      cursor: "pointer",
      fontWeight: "bold"
    }}
  >
    ℹ️
  </button>
</div>



    {parsedRoutes.map((route, i) => (
      <div key={i} style={{
        background: "linear-gradient(135deg,#0f172a,#1e293b)",
        padding: "25px",
        borderRadius: "18px",
        marginBottom: "25px",
        boxShadow: "0 15px 40px rgba(0,0,0,0.4)"
      }}>

        {/* 🔥 ROUTE HEADER */}
        <div style={{
          display: "flex",
          justifyContent: "space-between",
          marginBottom: "15px"
        }}>
          <h3>🏆 Route {route.route}</h3>
          <div>
         { /*  ⏱ {route.duration} | 📏 {route.distance} | 🔁 {route.transfers}  */}
<div style={{
  display: "flex",
  gap: "12px",
  alignItems: "center"
}}>
  <span style={{
    background: "#0ea5e9",
    padding: "6px 10px",
    borderRadius: "20px",
    fontSize: "0.85rem"
  }}>
    ⏱ {formatSeconds(parseInt(route.duration))}
  </span>

  <span style={{
    background: "#10b981",
    padding: "6px 10px",
    borderRadius: "20px",
    fontSize: "0.85rem"
  }}>
    📏 {route.distance} km
  </span>

  <span style={{
    background: "#f59e0b",
    padding: "6px 10px",
    borderRadius: "20px",
    fontSize: "0.85rem"
  }}>
    🔁 {route.transfers} Transfers
  </span>
</div>

          </div>
        </div>

        {/* 🔥 TIMELINE */}
        <div style={{ borderLeft: "3px solid #00eaff", paddingLeft: "20px" }}>

          {route.steps.map((step, j) => {
            const style = getStepStyle(step.type);

            return (
              <div key={j} style={{
                background: style.bg,
                padding: "18px",
                borderRadius: "12px",
                marginBottom: "15px",
                position: "relative"
              }}>

                {/* timeline dot */}
                <div style={{
                  position: "absolute",
                  left: "-28px",
                  top: "20px",
                  width: "14px",
                  height: "14px",
                  background: "#00eaff",
                  borderRadius: "50%"
                }} />

                {/* STEP HEADER 
                <h4>
                  {style.icon} {step.type} • {step.name || ""}
                </h4>
                */}
<h4 style={{ fontSize: "1.1rem", fontWeight: "bold" }}>
  {style.icon} {step.type}
</h4>

<p style={{
  marginTop: "4px",
  fontSize: "0.95rem",
  color: "#cbd5f5"
}}>
  🪪 {step.name ? step.name : "Route / Service"}
</p>



                {/* FROM → TO 
                <div style={{
                  display: "flex",
                  justifyContent: "space-between",
                  marginTop: "10px"
                }}>
                  <span>📍 {step.from}</span>
                  <span>➡️</span>
                  <span>{step.to}</span>
                </div>
                */}

<div style={{
  display: "flex",
  alignItems: "center",
  gap: "10px",
  marginTop: "8px"
}}>
  <span>📍 {step.from}</span>
  <span style={{ opacity: 0.6 }}>➡️</span>
  <span style={{ fontWeight: "bold", color: "#38bdf8" }}>
    {step.to}
  </span>
</div>



                {/* TIME */}
                <div style={{
  display: "flex",
  alignItems: "center",
  gap: "10px",
  marginTop: "8px",
  fontSize: "0.9rem",
  opacity: 0.9
}}>
  <span>🕒 {step.departure || "-"}</span>

  <span style={{ opacity: 0.6 }}>➡️</span>

  <span>🕘 {step.arrival || "-"}</span>
</div>

                {/* META */}
                <div style={{
                  marginTop: "8px",
                  fontSize: "0.9rem"
                }}>
                  ⏱ {step.duration} | 📏 {step.distance}
                </div>

                {/* SPECIAL EXPLORE UI 🔥 */}
                {step.type?.toLowerCase() === "explore" && (
                  <div style={{
                    marginTop: "10px",
                    padding: "10px",
                    background: "rgba(255,255,255,0.1)",
                    borderRadius: "8px"
                  }}>
                    🌟 Suggested stop: Enjoy this place before continuing your journey
                  </div>
                )}

              </div>
            );
          })}
        </div>
      </div>
    ))}
  </div>
)}

 {mode === "stations" && (
  <div>
   
<div style={{ display: "flex", alignItems: "center", gap: "10px" }}>
  <h2>📍 Nearby Stations</h2>
  <button
    onClick={() => setShowOverview(!showOverview)}
    style={{
      background: "linear-gradient(135deg,#38bdf8,#6366f1)",
      border: "none",
      borderRadius: "50%",
      width: "32px",
      height: "32px",
      color: "white",
      cursor: "pointer",
      fontWeight: "bold"
    }}
  >
    ℹ️
  </button>
</div>



    {parsedStations.map((s, i) => (
      <div key={i} style={{
        background: "#1e293b",
        padding: "18px",
        borderRadius: "12px",
        marginBottom: "12px"
      }}>
        <h3>🚉 {s.name}</h3>

        <p>📍 {s.address}</p>

        <p>
          📏 {s.distance !== "-1" ? `${s.distance} km` : "Distance N/A"}
        </p>

        <a href={s.url} target="_blank" rel="noreferrer"
           style={{ color: "#38bdf8" }}>
          🧭 Open in Maps
        </a>
      </div>
    ))}
  </div>
)}

 {mode === "tour" && (

    <div style={{
  background: "linear-gradient(135deg,#0f172a,#1e293b)",
  padding: "25px",
  borderRadius: "20px",
  boxShadow: "0 20px 50px rgba(0,0,0,0.5)"
}}>

    {/* HEADER */}
    <div style={{ display:"flex", alignItems:"center", gap:"10px" }}>
      <h2>🌍 Smart Travel Planner</h2>

      <button onClick={() => setShowOverview(!showOverview)} style={{
        background:"linear-gradient(135deg,#38bdf8,#6366f1)",
        border:"none",
        borderRadius:"50%",
        width:"32px",
        height:"32px",
        color:"white",
        cursor:"pointer"
      }}>ℹ️</button>
    </div>

    {results.tour.map((line, i) => {

      // 🌅 DAY HEADER
      if (line.includes("DAY")) {
      return (
  <div key={i} style={{
    marginTop:"30px",
    padding:"12px 18px",
    background:"linear-gradient(135deg,#22c55e,#4ade80)",
    borderRadius:"12px",
    color:"#022c22",
    fontWeight:"bold",
    fontSize:"1.3rem",
    boxShadow:"0 10px 25px rgba(34,197,94,0.4)"
  }}>
    🌅 {line}
  </div>
);
      }

      // 📍 PLACE CARD
      if (line.includes("➡️")) {
     return (
  <div key={i} style={{
    background:"linear-gradient(135deg,#1e293b,#0f172a)",
    padding:"18px",
    borderRadius:"16px",
    marginTop:"15px",
    border:"1px solid rgba(255,255,255,0.05)",
    boxShadow:"0 10px 30px rgba(0,0,0,0.6)",
    transition:"0.3s"
  }}>
    <h3 style={{
      color:"#38bdf8",
      fontSize:"1.2rem"
    }}>
      {line.replace("➡️","📍")}
    </h3>
  </div>
);
      }

      // 🚗 TRAVEL
      if (line.includes("Travel"))
        {
        return <p key={i} style={{ color:"#d8fa15" }}>🚗 {line}</p>;
      }

      // 🏞 EXPLORE
      if (line.includes("Explore")) {
      //  return <p key={i}>🏞 {line}</p>;
        return <p key={i} style={{ color:"#4ade80" }}>🏞 {line}</p>;
      }

      // 💡 DESCRIPTION
      if (line.includes("💡")) {
       // return <p key={i} style={{ opacity:0.8 }}>{line}</p>;
       return (
  <p key={i} style={{
    opacity:0.85,
    fontStyle:"italic",
    marginTop:"5px"
  }}>
    {line}
  </p>
);
        
      }

      // 📍 GOOGLE MAPS
if (line.includes("https://")) {

  const cleanUrl = line
    .replace("📍 Route:", "")
    .trim();

  return (
    <a
      key={i}
      href={cleanUrl}
      target="_blank"
      rel="noreferrer"
      style={{
        display:"inline-block",
        marginTop:"10px",
        padding:"10px 16px",
        background:"linear-gradient(135deg,#22c55e,#16a34a)",
        color:"white",
        borderRadius:"10px",
        textDecoration:"none",
        fontWeight:"bold",
        boxShadow:"0 5px 15px rgba(34,197,94,0.5)"
      }}
    >
      🧭 Open Route
    </a>
  );
}

      // 🍽 FOOD / 🌙 RETURN
      if (line.includes("🍽") || line.includes("🌙")) {
     return (
  <div key={i} style={{
    marginTop:"10px",
    padding:"12px",
    background:"linear-gradient(135deg,#f59e0b,#facc15)",
    borderRadius:"10px",
    color:"#422006",
    fontWeight:"bold"
  }}>
    {line}
  </div>
);
      }

      return null;
    })}

  </div>
)}


{mode === "attractions" && (() => {

  const lines = results.attractions || [];

  return (

    <div style={{
      background:"linear-gradient(135deg,#020617,#0f172a,#111827)",
      minHeight:"100vh",
      padding:"40px",
      color:"white"
    }}>

     {/* HEADER */}
<div style={{
  display:"flex",
  alignItems:"center",
  gap:"12px",
  marginBottom:"25px"
}}>

  <h2 style={{
    fontSize:"2rem",
    fontWeight:"700",
    margin:0
  }}>
    🏛 Attractions
  </h2>

  <button
    onClick={() => setShowOverview(!showOverview)}
    style={{
      background:"linear-gradient(135deg,#38bdf8,#6366f1)",
      border:"none",
      borderRadius:"50%",
      width:"32px",
      height:"32px",
      color:"white",
      cursor:"pointer",
      fontWeight:"bold"
    }}
  >
    ℹ️
  </button>

</div>

      {lines.map((line, i) => {

        // 🔥 REMOVE JUNK
        if (
          !line.trim() ||
          line.includes("CSV Matrix") ||
          line.includes("INDIAN RAILWAYS") ||
          line.includes("SOURCE:") ||
          line.includes("DESTINATION:") ||
          line.includes("====")
        ) {
          return null;
        }

        // 🔎 Exploring
        if (line.includes("Exploring attractions")) {
          return (
            <div key={i}
              style={{
                background:"rgba(59,130,246,0.12)",
                border:"1px solid rgba(59,130,246,0.3)",
                padding:"18px",
                borderRadius:"18px",
                marginBottom:"25px",
                color:"#93c5fd",
                fontSize:"18px"
              }}
            >
              🔎 {line}
            </div>
          );
        }

        // ✨ TITLE
        if (line.includes("TOP ATTRACTIONS")) {
          return (
            <h2 key={i}
              style={{
                marginTop:"40px",
                marginBottom:"30px",
                fontSize:"28px",
                color:"#fde047"
              }}
            >
              ✨ Top Attractions
            </h2>
          );
        }

        // 🌟 Attraction Name
        if (line.includes("🌟")) {

          return (
            <div
              key={i}
              style={{
                background:"linear-gradient(135deg,#1e1b4b,#312e81)",
                borderRadius:"24px",
                padding:"22px",
                marginTop:"35px",
                boxShadow:"0 12px 40px rgba(0,0,0,0.45)",
                border:"1px solid rgba(255,255,255,0.08)"
              }}
            >
              <h2 style={{
                margin:0,
                color:"#f8fafc",
                fontSize:"24px",
                fontWeight:"800"
              }}>
                {line.replace(/[0-9.]/g,"")}
              </h2>
            </div>
          );
        }

        // 🕒 BEST TIME
        if (line.includes("Best time")) {

          return (
            <div key={i}
              style={{
                marginTop:"15px",
                marginBottom:"15px",
                color:"#4ade80",
                fontSize:"16px",
                fontWeight:"700"
              }}
            >
              ⏰ {line.replace("🕒","")}
            </div>
          );
        }

        // 📝 DESCRIPTION
        return (
          <div
            key={i}
            style={{
              marginTop:"15px",
              padding:"18px",
              borderRadius:"16px",
              background:"rgba(255,255,255,0.03)",
              border:"1px solid rgba(255,255,255,0.05)"
            }}
          >
            <p style={{
              margin:0,
              color:"#e2e8f0",
              fontSize:"18px",
              lineHeight:"1.9"
            }}>
              {line}
            </p>
          </div>
        );

      })}

    </div>
  );

})()}


  {mode === "maps" && (
  <div>
<div style={{ display: "flex", alignItems: "center", gap: "10px" }}>
  <h2 style={{ marginBottom: "20px" }}>🗺️ Google Maps Directions</h2>
  <button
    onClick={() => setShowOverview(!showOverview)}
    style={{
      background: "linear-gradient(135deg,#38bdf8,#6366f1)",
      border: "none",
      borderRadius: "50%",
      width: "32px",
      height: "32px",
      color: "white",
      cursor: "pointer",
      fontWeight: "bold"
    }}
  >
    ℹ️
  </button>
</div>


    {mapData.url ? (
      <div style={{
        background: "linear-gradient(135deg,#064e3b,#022c22)",
        padding: "25px",
        borderRadius: "18px",
        boxShadow: "0 15px 40px rgba(0,0,0,0.4)"
      }}>

        {/* ROUTE */}
        <h3 style={{ marginBottom: "15px" }}>
          🚀 Route Overview
        </h3>

        <div style={{
          display: "flex",
          alignItems: "center",
          justifyContent: "center",
          gap: "12px",
          fontSize: "1.1rem",
          marginBottom: "15px"
        }}>
          <span>📍 {mapData.source}</span>
          <span>➡️</span>
          <span style={{ color: "#22c55e", fontWeight: "bold" }}>
            {mapData.destination}
          </span>
        </div>

        {/* ACTION BUTTON */}
        <a
          href={mapData.url}
          target="_blank"
          rel="noreferrer"
          style={{
            display: "inline-block",
            marginTop: "15px",
            padding: "12px 20px",
            background: "#22c55e",
            color: "black",
            borderRadius: "10px",
            textDecoration: "none",
            fontWeight: "bold"
          }}
        >
          🧭 Open in Google Maps
        </a>

      </div>
    ) : (
      <div style={resultCard}>
        {results.maps.map((r, i) => <div key={i}>{r}</div>)}
      </div>
    )}
  </div>
)}

{mode === "planner" && (
  <div>

    {/* HEADER */}
    <div style={{
  textAlign: "center",
  fontSize: "2.5rem",
  fontWeight: "bold",
  marginBottom: "30px",
  background: "linear-gradient(90deg,#38bdf8,#6366f1)",
  WebkitBackgroundClip: "text",
  color: "transparent"
}}>
  ✨ Journey Guider ✨
</div>
<div style={{ display: "flex", alignItems: "center", gap: "10px" }}>


  <button
    onClick={() => setShowOverview(!showOverview)}
    style={{
      background: "linear-gradient(135deg,#38bdf8,#6366f1)",
      border: "none",
      borderRadius: "50%",
      width: "32px",
      height: "32px",
      color: "white",
      cursor: "pointer",
      fontWeight: "bold"
    }}
  >
    ℹ️
  </button>
</div>
{/*
    {plans
      .filter(p => p.steps && p.steps.length > 0)
      .map((p, i) => (
*/ 
        filteredPlans
  .filter(p => p.steps && p.steps.length > 0)
  .map((p, i) => (

      <div key={i} style={{ marginBottom: "40px" }}>

        {/* TITLE */}
        <h2 style={{
          fontSize: "1.6rem",
          fontWeight: "bold"
        }}>
         { /*
          {i === 0 
  ? "🚆 Direct Route" 
  : "🤖 AI Smart Route"}
  */}
  {p.type === "DIRECT"
  ? "🚆 Direct Route"
  : "🤖 AI Smart Route"}
        </h2>

        {/* SUMMARY */}
    <div style={{
  //...(i === 0 ? directStyle : aiStyle),
  ...(p.type === "DIRECT" ? directStyle : aiStyle),
  padding:"20px",
  borderRadius:"15px",
  marginBottom:"20px",
  color:"white",
  fontWeight:"bold"
}}>
          ⏱ {p.totalTime} | 📏 {p.distance}
        </div>

        {/* STEPS */}
        {p.steps.map((s,j)=>(
          <div key={j}
            style={{
              background: i === 0
  ? "linear-gradient(135deg,#064e3b,#065f46)"
  : "linear-gradient(135deg,#1e293b,#0f172a)",
              padding:"15px",
              borderRadius:"10px",
              marginBottom:"10px",
              transition:"0.3s"
            }}
            onMouseEnter={(e)=>{
              e.currentTarget.style.transform = "scale(1.03)";
            }}
            onMouseLeave={(e)=>{
              e.currentTarget.style.transform = "scale(1)";
            }}
          >
            <h4>{getPlannerIcon(s.mode)} {s.mode}</h4>
            <p>📍 {s.from} → {s.to}</p>
            <p>🕒 {s.time}</p>
            <p>⏱ {s.duration} | 📏 {s.distance}</p>
          </div>
        ))}

      </div>
    ))}
  </div>
)}


{mode === "indirect" && (
  <div>
<div style={{ display: "flex", alignItems: "center", gap: "10px" }}>
 <h2>⚡ Smart Routes</h2>

  <button
    onClick={() => setShowOverview(!showOverview)}
    style={{
      background: "linear-gradient(135deg,#38bdf8,#6366f1)",
      border: "none",
      borderRadius: "50%",
      width: "32px",
      height: "32px",
      color: "white",
      cursor: "pointer",
      fontWeight: "bold"
    }}
  >
    ℹ️
  </button>
</div>




    {parsedIndirect.map((route, i) => (
      <div key={i} style={{
        background: route.tag === "FASTEST"
          ? "linear-gradient(135deg,#065f46,#10b981)"
          : "#1e293b",
        padding: "25px",
        borderRadius: "18px",
        marginBottom: "25px",
        boxShadow: "0 15px 40px rgba(0,0,0,0.5)"
      }}>

        {/* HEADER */}
        <div style={{ display:"flex", justifyContent:"space-between" }}>
          <h3>
            {route.tag === "FASTEST" ? "🏆 FASTEST ROUTE" : `🔁 Route ${route.route}`}
          </h3>

          <div style={{ display:"flex", gap:"10px" }}>
            <span className="badge">⏱ {route.summary?.total || "-"}</span>
            <span className="badge">🔄 {route.summary?.changes || 0}</span>
          </div>
        </div>

<div style={{
  display: "inline-block",
  padding: "6px 12px",
  borderRadius: "20px",
  background: i === 0 ? "#10b981" : "#3b82f6",
  color: "white",
  fontSize: "0.8rem",
  marginBottom: "10px"
}}>
  {i === 0 ? "⚡ Fastest Route" : "🤖 AI Optimized"}
</div>


{/* SUMMARY CARD */}
<div style={{
  ...(i === 0 ? directStyle : aiStyle),
  padding:"20px",
  borderRadius:"15px",
  marginBottom:"20px",
  color:"white",
  fontWeight:"bold"
}}>
  <span>🔄 {route.summary?.changes || 0} Changes</span>
  <span>🕓 {route.summary.travel}</span>
  <span>⏳ {route.summary.layover}</span>
  <span style={{ color:"#22c55e", fontWeight:"bold" }}>
    🧮 {route.summary.total}
  </span>
</div>


  {route.layovers?.length > 0 && route.layovers.map((l, i) => (
  <div key={i} style={{
    background:"#7c2d12",
    padding:"8px",
    borderRadius:"8px",
    margin:"8px 0",
    fontSize:"0.85rem"
  }}>
    ⏳ Layover at {l.station} → {l.time}
  </div>
))}



        {/* PATH */}
        <p style={{ marginTop:"10px", color:"#38bdf8" }}>
          🛤 {route.path}
        </p>

        {/* TRAIN TIMELINE */}
        <div style={{ marginTop:"15px", borderLeft:"3px solid #22c55e", paddingLeft:"15px" }}>
            {route.trains?.length > 0 && route.trains.map((t, j) => (
            <div key={j} style={{
           //   background:"#0f172a",
              background: i === 0
  ? "linear-gradient(135deg,#064e3b,#065f46)"
  : "linear-gradient(135deg,#1e293b,#0f172a)",
              padding:"15px",
              borderRadius:"10px",
              marginBottom:"12px"
            }}>
              <h4>🚆 {t.name}</h4>
              <p>📟 {t.trainNo}</p>

              <div style={{ display:"flex", justifyContent:"center", gap:"10px" }}>
                <span>🕒 {t.departure}</span>
                <span>➡️</span>
                <span>🕘 {t.arrival}</span>
              </div>

              <p>⏱ {t.duration}</p>
            </div>
          ))}
        </div>

      </div>
    ))}
  </div>
)}


</div>
  </div>
);
}




/* ---------- HELPERS ---------- */

function getBackground(mode) {
switch (mode) {
case "direct":
return "linear-gradient(135deg,#1d2671,#c33764)";
case "stations":
return "linear-gradient(135deg,#11998e,#38ef7d)";
case "tour":
return "linear-gradient(135deg,#f7971e,#ffd200)";
case "maps":
  return "linear-gradient(135deg,#0f172a,#065f46,#22c55e)";
case "indirect":
return `
radial-gradient(circle at 10% 20%, #1e1b4b, transparent 40%),
radial-gradient(circle at 90% 30%, #4c1d95, transparent 40%),
radial-gradient(circle at 50% 90%, #7c3aed, transparent 40%),
linear-gradient(135deg,#020617,#020617)
`;
case "planner":
return `
radial-gradient(circle at 20% 20%, #22c55e, transparent 40%),
radial-gradient(circle at 80% 30%, #0ea5e9, transparent 40%),
radial-gradient(circle at 50% 80%, #6366f1, transparent 40%),
linear-gradient(135deg,#020617,#020617)
`;
default:
return "linear-gradient(135deg,#0f2027,#203a43,#2c5364)";
}
}

function parseTrains(lines) {
  const trains = [];
  let current = null;

  lines.forEach(line => {
    if (line.trim() === "TRAIN_START") current = {};
    else if (line.trim() === "TRAIN_END") {
      trains.push(current);
      current = null;
    } else if (current) {
      const parts = line.split(":");
      if (parts.length >= 2) {
        const key = parts[0].trim();
        const value = parts.slice(1).join(":").trim();
        current[key] = value;
      }
    }
  });

  return trains;
}

function parseRoutes(lines) {
  const routes = [];
  let currentRoute = null;
  let currentStep = null;

  lines.forEach(line => {
    line = line.trim();

    if (line === "ROUTE_START") currentRoute = { steps: [] };

    else if (line === "ROUTE_END") {
      routes.push(currentRoute);
      currentRoute = null;
    }

    else if (line === "STEP_START") currentStep = {};

    else if (line === "STEP_END") {
      currentRoute.steps.push(currentStep);
      currentStep = null;
    }

    else if (currentStep) {
      const [k, ...rest] = line.split(":");
      currentStep[k] = rest.join(":");
    }

    else if (currentRoute) {
      const [k, ...rest] = line.split(":");
      currentRoute[k] = rest.join(":");
    }
  });

  return routes;
}


function getStepStyle(type) {
  switch (type?.toLowerCase()) {
    case "bus":
      return {
        bg: "#1e3a8a",
        icon: "🚌"
      };
    case "cab":
      return {
        bg: "#6d28d9",
        icon: "🚕"
      };
    case "explore":
      return {
        bg: "#92400e",
        icon: "🌄"
      };
    default:
      return {
        bg: "#1f2937",
        icon: "📍"
      };
  }
}

function formatSeconds(sec) {
  if (!sec) return "";
  const hrs = Math.floor(sec / 3600);
  const mins = Math.floor((sec % 3600) / 60);
  return `${hrs} hr ${mins} min`;
}

function getPlannerIcon(mode) {
  switch (mode?.toLowerCase()) {
    case "train":
      return "🚆";
    case "bus":
      return "🚌";
    case "local":
      return "🛺";
    case "drive":
      return "🚗";
    case "walk":
      return "🚶";
    default:
      return "📍";
  }
}



function parseStations(lines) {
  const stations = [];
  let current = null;

  lines.forEach(line => {
    line = line.trim();

    if (line === "STATION_START") current = {};
    else if (line === "STATION_END") {
      stations.push(current);
      current = null;
    } else if (current) {
      const [k, ...rest] = line.split(":");
      current[k] = rest.join(":");
    }
  });

  return stations;
}

function parseMap(lines) {
  let data = {};

  lines.forEach(line => {
    line = line.trim();

    if (line.includes(":")) {
      const [k, ...rest] = line.split(":");
      data[k] = rest.join(":");
    }
  });

  return data;
}

function parseIndirect(lines) {
  const routes = [];
  let route = null, train = null;

  lines.forEach(line => {
    line = line.trim();

    if (line === "ROUTE_START") route = { trains: [], layovers: [] };

    else if (line === "ROUTE_END") {
      routes.push(route);
      route = null;
    }

    else if (line.startsWith("LAYOVER:")) {
      const val = line.replace("LAYOVER:", "").split(",");
      route.layovers.push({
        station: val[0],
        time: val[1]
      });
    }

    else if (line === "TRAIN_START") train = {};

    else if (line === "TRAIN_END") {
      route.trains.push(train);
      train = null;
    }

    else if (line === "SUMMARY_START") route.summary = {};

    else if (line === "SUMMARY_END") {}

    else if (train) {
      const [k,...r] = line.split(":");
      train[k] = r.join(":");
    }

    else if (route?.summary && line.includes(":")) {
      const [k,...r] = line.split(":");
      route.summary[k] = r.join(":");
    }

    else if (route && line.includes(":")) {
      const [k,...r] = line.split(":");
      route[k] = r.join(":");
    }
  });

  return routes;
}


function parsePlanner(lines) {
  let plan = {};
  let steps = [];
  let step = null;

  lines.forEach(line => {
    line = line.trim();

    if (line === "STEP_START") step = {};

    else if (line === "STEP_END") {
      steps.push(step);
      step = null;
    }

    else if (step) {
      const [k,...r] = line.split(":");
      step[k] = r.join(":");
    }

    else if (line.includes(":")) {
      const [k,...r] = line.split(":");
      plan[k] = r.join(":");
    }
  });

  plan.steps = steps;
  return plan;
}



/* ---------- STYLES ---------- */

const container = {
minHeight: "100vh",
padding: "2rem",
color: "white",
fontFamily: "Segoe UI"
};

const title = { fontSize: "2.5rem" };

const subtitle = { opacity: 0.7 };

const grid = {
display: "grid",
gridTemplateColumns: "repeat(4,1fr)",
gap: "10px",
marginBottom: "20px"
};

const card = {
padding: "12px",
borderRadius: "10px",
cursor: "pointer",
textAlign: "center"
};

const searchBox = {
  padding: "20px",
  borderRadius: "18px",
  background: "rgba(0,0,0,0.3)",
  backdropFilter: "blur(15px)",
  boxShadow: "0 10px 30px rgba(0,0,0,0.5)"
};

const input = {
  margin: "5px",
  padding: "12px",
  borderRadius: "10px",
  border: "1px solid rgba(255,255,255,0.1)",
  background: "rgba(0,0,0,0.4)",
  color: "white",
  outline: "none",
  width: "100%",
  backdropFilter: "blur(10px)"
};

const resultsBox = {
marginTop: "20px"
};

const resultCard = {
background: "rgba(255,255,255,0.05)",
padding: "10px",
margin: "8px 0",
borderRadius: "10px"
};
