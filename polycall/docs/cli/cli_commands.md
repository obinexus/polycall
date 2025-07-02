# LibPolyCall CLI Command Structure

## Command Hierarchy

LibPolyCall's CLI follows a structured hierarchy:

```
polycall [command] [subcommand] [options/flags]
```

### Top-Level Commands

Each core module has a corresponding top-level command:

- `polycall auth` - Authentication and security commands
- `polycall config` - Configuration management commands
- `polycall edge` - Edge computing commands
- `polycall ffi` - Foreign Function Interface commands
- `polycall micro` - Microservice commands
- `polycall network` - Network communication commands
- `polycall protocol` - Protocol commands
- `polycall telemetry` - Telemetry and monitoring commands
- `polycall repl` - Interactive REPL commands

### Global Commands

In addition to module-specific commands, there are global commands:

- `polycall help` - Show help information
- `polycall version` - Show version information
- `polycall exit/quit` - Exit the CLI (in REPL mode)

## Subcommand Structure

Each top-level command has module-specific subcommands. For example:

### Authentication Commands

```
polycall auth help                    # Show auth help
polycall auth status                  # Check auth status
polycall auth configure [options]     # Configure auth settings
polycall auth token create [options]  # Create authentication token
polycall auth token verify [token]    # Verify authentication token
polycall auth token revoke [token]    # Revoke authentication token
```

### Telemetry Commands

```
polycall telemetry help                  # Show telemetry help
polycall telemetry status                # Check telemetry status
polycall telemetry configure [options]   # Configure telemetry settings
polycall telemetry start                 # Start telemetry collection
polycall telemetry stop                  # Stop telemetry collection
polycall telemetry metrics show [type]   # Show metrics by type
polycall telemetry logs show [options]   # Show logs with filtering
```

## Flag Composition

Flags follow standard conventions with both short and long forms:

- Short form: `-f`
- Long form: `--flag`
- Flags with values: `--flag=value` or `--flag value`

### Flag Types

1. **Boolean flags**: Simple on/off switches
   ```
   polycall telemetry start --verbose
   polycall auth token create --no-expiry
   ```

2. **Value flags**: Flags that take a value
   ```
   polycall config set --file=/path/to/config.json
   polycall telemetry logs show --level=error
   ```

3. **Repeatable flags**: Flags that can appear multiple times
   ```
   polycall telemetry metrics show --include=cpu --include=memory
   ```

### Flag Relationships

Flags can have different relationships:

#### Mutually Exclusive Flags

These flags cannot be used together:

```
polycall telemetry configure --enable --disable  # Error: mutually exclusive
```

#### Co-dependent Flags

These flags must be used together:

```
polycall auth token create --user=username --role=admin
```

#### Default Flags

Flags with default values that can be overridden:

```
polycall telemetry start  # Uses default interval of 5 seconds
polycall telemetry start --interval=10  # Overrides default interval
```

## REPL Mode Commands

In REPL (interactive) mode, commands can be entered directly:

```
polycall> auth status
Authentication system is active.

polycall> telemetry start --interval=2
Telemetry started with 2 second interval.

polycall> help
Available commands:
  auth        Authentication and security commands
  config      Configuration management commands
  ...

polycall> exit
```

## Examples of Complex Command Combinations

### Protocol Configuration and Testing

```
polycall protocol configure --port=8080 --secure --cert=/path/to/cert.pem
polycall protocol test --target=localhost:8080 --requests=100 --concurrent=10
```

### Cross-Language FFI Setup

```
polycall ffi bind --language=python --module=/path/to/module.py --interface=api.json
polycall ffi call --function=process_data --args=@input.json --output=result.json
```

### Microservice Deployment

```
polycall micro deploy --name=auth-service --image=auth:1.0 --replicas=3 --env-file=.env
polycall micro status --name=auth-service --watch
```

## Command Execution Flow

1. The CLI parses the command line arguments
2. It identifies the top-level command and subcommand
3. It validates flag combinations (checking for mutually exclusive flags)
4. It retrieves the appropriate command handler from the registry
5. It executes the command, passing all parsed arguments and flags
6. It returns the result code and displays output to the user