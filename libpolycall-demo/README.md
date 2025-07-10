# LibPolyCall v3.3 Demo

This directory contains a local demonstration fixture for the LibPolyCall zero-trust protocol. It showcases basic command handling along with two practical scenarios:

- **MicroBank** – a simple banking microservice topology
- **Edge Compute** – a CDN cache simulation

The demo runs entirely offline using the Python standard library HTTP server.

## Usage

```bash
cd libpolycall-demo
npm install --offline || true  # no dependencies but keeps workflow consistent
npm run start
```

Open `http://127.0.0.1:8000` in your browser and experiment with the demo interface.

No external network access is required.
