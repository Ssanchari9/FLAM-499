class EdgeDetectionViewer {
  private canvas: HTMLCanvasElement;
  private ctx: CanvasRenderingContext2D;
  private ws: WebSocket | null = null;
  private statusElement: HTMLElement;
  private fpsElement: HTMLElement;
  private lastFrameTime: number = 0;
  private frameTimes: number[] = [];
  private frameCount: number = 0;
  private readonly FPS_UPDATE_INTERVAL = 1000; // Update FPS every second
  private lastFpsUpdate: number = 0;

  constructor() {
    this.canvas = document.getElementById('videoCanvas') as HTMLCanvasElement;
    this.ctx = this.canvas.getContext('2d')!;
    this.statusElement = document.getElementById('status')!;
    this.fpsElement = document.getElementById('fps')!;
    
    this.initWebSocket();
    this.setupEventListeners();
    this.updateStatus('Initializing...');
    
    // Start the render loop
    requestAnimationFrame(this.render.bind(this));
  }

  private initWebSocket(): void {
    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    const wsUrl = `${protocol}//${window.location.host}`;
    
    this.ws = new WebSocket(wsUrl);
    
    this.ws.onopen = () => {
      this.updateStatus('Connected to server');
      console.log('WebSocket connection established');
    };
    
    this.ws.onmessage = (event) => {
      try {
        const data = JSON.parse(event.data);
        this.handleMessage(data);
      } catch (error) {
        console.error('Error parsing WebSocket message:', error);
      }
    };
    
    this.ws.onclose = () => {
      this.updateStatus('Disconnected from server. Reconnecting...');
      console.log('WebSocket connection closed');
      // Attempt to reconnect after a delay
      setTimeout(() => this.initWebSocket(), 3000);
    };
    
    this.ws.onerror = (error) => {
      console.error('WebSocket error:', error);
      this.updateStatus('Connection error');
    };
  }

  private handleMessage(data: any): void {
    switch (data.type) {
      case 'frame':
        this.handleFrameData(data);
        break;
      case 'status':
        this.updateStatus(data.message);
        break;
      case 'fps':
        this.updateFpsDisplay(data.value);
        break;
      default:
        console.log('Received message:', data);
    }
  }

  private handleFrameData(data: any): void {
    if (!data.imageData) return;
    
    const now = performance.now();
    const frameTime = now - this.lastFrameTime;
    this.lastFrameTime = now;
    
    // Update frame times for FPS calculation
    this.frameTimes.push(frameTime);
    this.frameCount++;
    
    // Update FPS display every second
    if (now - this.lastFpsUpdate > this.FPS_UPDATE_INTERVAL) {
      const avgFrameTime = this.frameTimes.reduce((a, b) => a + b, 0) / this.frameTimes.length;
      const fps = 1000 / avgFrameTime;
      this.updateFpsDisplay(fps);
      
      // Reset counters
      this.frameTimes = [];
      this.frameCount = 0;
      this.lastFpsUpdate = now;
    }
    
    // Draw the frame
    this.drawFrame(data.imageData);
  }

  private drawFrame(imageData: string): void {
    const img = new Image();
    img.onload = () => {
      // Maintain aspect ratio
      const canvasAspect = this.canvas.width / this.canvas.height;
      const imgAspect = img.width / img.height;
      
      let renderWidth, renderHeight, offsetX, offsetY;
      
      if (imgAspect > canvasAspect) {
        // Image is wider than canvas
        renderWidth = this.canvas.width;
        renderHeight = this.canvas.width / imgAspect;
        offsetX = 0;
        offsetY = (this.canvas.height - renderHeight) / 2;
      } else {
        // Image is taller than canvas
        renderHeight = this.canvas.height;
        renderWidth = this.canvas.height * imgAspect;
        offsetX = (this.canvas.width - renderWidth) / 2;
        offsetY = 0;
      }
      
      // Clear and draw the image
      this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
      this.ctx.drawImage(img, offsetX, offsetY, renderWidth, renderHeight);
    };
    
    img.src = `data:image/jpeg;base64,${imageData}`;
  }

  private updateStatus(message: string): void {
    this.statusElement.textContent = message;
    console.log(`Status: ${message}`);
  }

  private updateFpsDisplay(fps: number): void {
    this.fpsElement.textContent = `FPS: ${fps.toFixed(1)}`;
  }

  private setupEventListeners(): void {
    window.addEventListener('resize', this.handleResize.bind(this));
    
    // Handle window close
    window.addEventListener('beforeunload', () => {
      if (this.ws) {
        this.ws.close();
      }
    });
  }

  private handleResize(): void {
    // Adjust canvas size while maintaining aspect ratio
    const container = this.canvas.parentElement;
    if (!container) return;
    
    const containerWidth = container.clientWidth;
    const containerHeight = container.clientHeight;
    
    // Set canvas size to match container
    this.canvas.width = containerWidth;
    this.canvas.height = containerHeight;
  }

  private render(): void {
    // Main render loop
    // This can be used for animations or other continuous updates
    
    // Request next frame
    requestAnimationFrame(this.render.bind(this));
  }
}

// Initialize the viewer when the DOM is fully loaded
document.addEventListener('DOMContentLoaded', () => {
  new EdgeDetectionViewer();
});
