# PyPolyCall - Python Binding for LibPolyCall Trial

This is the official Python binding for LibPolyCall Trial, demonstrating polyglot system capabilities.

## Features

- HTTP Client/Server communication
- Book API example (matching Node implementation)
- LibPolyCall protocol compliance
- Port mapping configuration (3001:8084)
- Built-in error handling and logging

## Quick Start

1. **Setup Environment**:
   ```bash
   python3 -m venv venv
   source venv/bin/activate  # On Windows: venv\Scripts\activate
   ```

2. **Run Test Client**:
   ```bash
   python client/test_client_api.py
   ```

3. **Verify Connection**:
   - Client connects to localhost:8084
   - Creates a test book via POST /books
   - Retrieves books via GET /books

## Configuration

Port mapping is defined in `.polycallrc`:
```ini
port=3001:8084
server_type=python
workspace=/opt/polycall/services/python
```

## Security

- No fallback ports allowed
- Binding verification enforced
- O(n) complexity maintained for dev experience

## Part of LibPolyCall Trial Ecosystem

This binding works alongside:
- NodePolyCall
- GoPolyCall  
- JavaPolyCall

All bindings are legally supported under the official PolyCall trial.
