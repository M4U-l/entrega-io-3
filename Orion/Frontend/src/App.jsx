import { useState, useEffect } from "react";
import "./App.css";

export default function App() {
  const [speed, setSpeed] = useState(50);
  const [error, setError] = useState(null);
  const [data, setData] = useState([]); // 🔹 datos del GPS/temperatura
  const [lastUpdate, setLastUpdate] = useState(null); // 🔹 hora de actualización

  const api = "http://13.220.199.79:4040/status"; // cambiar por la IP del backend que recibe comandos
  const tempApi = "https://<TU_API_GATEWAY>.execute-api.<region>.amazonaws.com/prod/readings"; // 🔹 cambiar por tu endpoint real de AWS

  // 🔹 Función para enviar comandos al backend del vehículo
  const handleControl = async (command) => {
    try {
      console.log(`Comando: ${command}, Velocidad: ${speed}%`);
      const response = await fetch(api, {
        method: "POST",
        headers: {
          "Content-Type": "application/json",
          "x-api-key": "AK90YTFGHJ007WQ", // consulta si tu API en AWS requiere o no API key
        },
        body: JSON.stringify({ cmd: command.toUpperCase(), speedness: speed }),
      });

      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }

      const data = await response.text();
      console.log("Respuesta:", data);
      setError(null);
    } catch (error) {
      console.error("Error al enviar el comando:", error);
      setError("Error al enviar el comando al servidor.");
    }
  };

  // 🔹 Control de velocidad
  const handleSpeedChange = (e) => {
    setSpeed(Number(e.target.value));
  };

  // 🔹 Función para obtener datos de temperatura desde AWS
  const fetchTemperatures = async () => {
    try {
      const res = await fetch(tempApi, {
        headers: {
          "Content-Type": "application/json",
          "x-api-key": "TU_API_KEY", // si tu API Gateway lo requiere
        },
      });
      if (!res.ok) throw new Error(`Error HTTP ${res.status}`);
      const json = await res.json();
      setData(json.readings || []);
      setLastUpdate(new Date().toLocaleTimeString());
    } catch (err) {
      console.error("Error al obtener temperaturas:", err);
      setData([]);
    }
  };

  // 🔹 Llamar periódicamente a AWS
  useEffect(() => {
    fetchTemperatures(); // primera carga
    const interval = setInterval(fetchTemperatures, 10000); // cada 10s
    return () => clearInterval(interval);
  }, []);

  // 🔹 Render principal
  return (
    <div className="control-container">
      {/* JOYSTICK */}
      <div className="joystick">
        <button onClick={() => handleControl("forward")} className="btn up">
          ↑
        </button>
        <div className="middle-row">
          <button onClick={() => handleControl("left")} className="btn left">
            ←
          </button>
          <button onClick={() => handleControl("stop")} className="btn stop">
            ⏹
          </button>
          <button onClick={() => handleControl("right")} className="btn right">
            →
          </button>
        </div>
        <button onClick={() => handleControl("backward")} className="btn down">
          ↓
        </button>
      </div>

      {/* CONTROL DE VELOCIDAD */}
      <div className="speed-control">
        <div
          className="speed-ring"
          style={{
            "--speed": speed,
          }}
        >
          <input
            type="range"
            min="0"
            max="100"
            value={speed}
            onChange={handleSpeedChange}
            className="speed-slider"
          />
        </div>
        <div className="speed-value">{speed}%</div>
      </div>

      {/* 🔹 MÓDULO DE TEMPERATURAS */}
      <div className="temp-module">
        <h2>Temperature Data</h2>
        <table className="temp-table">
          <thead>
            <tr>
              <th>Time</th>
              <th>Temp (°C)</th>
              <th>Latitude</th>
              <th>Longitude</th>
            </tr>
          </thead>
          <tbody>
            {data.length === 0 ? (
              <tr>
                <td colSpan="4">Loading data...</td>
              </tr>
            ) : (
              data.map((item, i) => (
                <tr key={i}>
                  <td>{new Date(item.timestamp).toLocaleTimeString()}</td>
                  <td>{item.sensors.temperature}</td>
                  <td>{item.location.lat.toFixed(4)}</td>
                  <td>{item.location.lon.toFixed(4)}</td>
                </tr>
              ))
            )}
          </tbody>
        </table>
        <div className="temp-meta">
          Last update: {lastUpdate || "--"}
        </div>
      </div>

      {/* Mostrar errores si ocurren */}
      {error && <div className="error">{error}</div>}
    </div>
  );
}
