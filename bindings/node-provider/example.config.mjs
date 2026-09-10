// Language-native PolyCall configuration: a real ES module exporting real JS
// values (objects, numbers, booleans, arrays). No DSL, no hand-written JSON.
//
// Expresses the same two-service "shop" topology as the C and Python examples.

export default {
  project: {
    name: "shop",
    extension_namespace: "obinexus",
  },
  services: [
    {
      id: "web",
      language: "node",
      bind_address: "0.0.0.0",
      host_port: 3000,
      target_port: 3001,
      workspace: "/opt/shop/web",
      // timeout_ms / max_connections omitted -> core defaults (30000 / 64)
    },
    {
      id: "inventory",
      language: "c",
      host_port: 8080,
      target_port: 9090,
      workspace: "/opt/shop/inventory",
      timeout_ms: 15000,
      max_connections: 128,
    },
  ],
};
