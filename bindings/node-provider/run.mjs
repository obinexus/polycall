// PolyCall Node provider entry point.
//
//   node run.mjs <path-to-config-module>
//
// Loads an explicitly named config module (never scans directories), prints
// exactly one canonical JSON envelope on stdout, exits 0. Any error: message
// on stderr, exit 1. This is the process the C sub-process host spawns.

import { pathToFileURL } from "node:url";
import { resolve } from "node:path";
import { emitFromModule } from "./polycall-provider.mjs";

const arg = process.argv[2];
if (!arg) {
  process.stderr.write("usage: node run.mjs <config-module>\n");
  process.exit(2);
}

try {
  await emitFromModule(pathToFileURL(resolve(arg)).href);
} catch (e) {
  process.stderr.write(`polycall-node-provider: ${e && e.message ? e.message : e}\n`);
  process.exit(1);
}
