import express from 'express';
import { createServer } from 'http';
import { WebSocketServer } from 'ws';
import path, { dirname } from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

const app = express();
const server = createServer(app);
const wss = new WebSocketServer({ server });
const PORT = process.env.PORT || 3000;

// Serve static files from the actual public folder (sibling of dist)
const publicDir = path.resolve(__dirname, '../public');
app.use(express.static(publicDir));

// Fallback for root
app.get('/', (_req, res) => {
  res.sendFile(path.join(publicDir, 'index.html'));
});

// WebSocket connection handler
wss.on('connection', (ws) => {
  console.log('New client connected');
  
  // Send a welcome message to the client
  ws.send(JSON.stringify({
    type: 'connection',
    message: 'Connected to Edge Detection Server',
    timestamp: new Date().toISOString()
  }));

  // Handle incoming messages
  ws.on('message', (message: string) => {
    try {
      const data = JSON.parse(message);
      console.log('Received:', data);
      
      // Echo the message back to the client
      ws.send(JSON.stringify({
        ...data,
        serverTime: new Date().toISOString()
      }));
    } catch (error) {
      console.error('Error processing message:', error);
    }
  });

  // Handle client disconnection
  ws.on('close', () => {
    console.log('Client disconnected');
  });
});

// Broadcast a dummy frame periodically (1 FPS) to demonstrate viewer
const dummySvg = (counter: number) => {
  const svg = `<svg xmlns='http://www.w3.org/2000/svg' width='640' height='480'>
    <rect width='100%' height='100%' fill='black'/>
    <text x='50%' y='50%' dominant-baseline='middle' text-anchor='middle' font-size='48' fill='white'>Frame ${counter}</text>
  </svg>`;
  const base64 = Buffer.from(svg).toString('base64');
  return `data:image/svg+xml;base64,${base64}`;
};

let frameCounter = 0;
setInterval(() => {
  frameCounter++;
  const dataUrl = dummySvg(frameCounter);
  const payload = {
    type: 'frame',
    url: dataUrl,
    width: 640,
    height: 480,
  };
  const fpsMsg = {
    type: 'fps',
    value: 1.0,
  };
  wss.clients.forEach((client) => {
    try {
      client.send(JSON.stringify(payload));
      client.send(JSON.stringify(fpsMsg));
    } catch (e) {
      // ignore
    }
  });
}, 1000);

// Start the server
server.listen(PORT, () => {
  console.log(`Server running on http://localhost:${PORT}`);
});

export default server;
