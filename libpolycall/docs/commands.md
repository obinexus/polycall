

---

### 🔐 `auth_commands.c`
**Purpose:** Manage authentication subsystems.
- Handles identity management, token generation, policies.
- Enforces security with zero-trust principles.

---

### ⚙️ `config_commands.c`
**Purpose:** Work with config files (`Polycallfile`, `.polycallrc`).
- Load and validate hierarchical configs.
- Useful for setup, dynamic reloading, and debugging.

---

### 🌐 `network_commands.c`
**Purpose:** Interface with LibPolyCall's networking layer.
- Start/stop servers, configure ports.
- List clients, endpoints, and network status.

---

### 🧬 `protocol_commands.c`
**Purpose:** Interact with cross-language protocol features.
- Trigger message encoding/decoding.
- Test protocol state machines and context setup.

---

### 🧠 `ffi_commands.c`
**Purpose:** Manage Foreign Function Interface (FFI) layer.
- Configure bindings for Java, Python, COBOL, etc.
- Useful for integrating with language-specific logic.

---

### ☁️ `micro_commands.c`
**Purpose:** Isolate microservice components.
- Enforces permission/resource limits.
- Implements polling intervals and isolation policies.

---

### 🌍 `edge_commands.c`
**Purpose:** Optimize edge computing behaviors.
- Move computation closer to data.
- Reduce latency via intelligent node selection.

---

### 🔐 `telemetry_commands.c`
**Purpose:** Inspect and control telemetry reporting.
- Enable or disable logging.
- Report metrics like bandwidth, event frequency.

---

### 🔧 `command_registry.c`
**Purpose:** Registers all commands with the REPL.
- Central place where commands are made available.

---

### 🧪 `repl_commands.c`
**Purpose:** Implements extra REPL utilities.
- Might support meta commands (e.g., `.exit`, `.help`).

---

### 🌐 `main.c` and `repl.c`
**Purpose:** Entry points.
- `main.c`: initializes PolyCall runtime.
- `repl.c`: command loop for interactive use.

---


