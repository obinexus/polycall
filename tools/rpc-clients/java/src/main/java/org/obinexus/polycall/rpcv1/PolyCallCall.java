package org.obinexus.polycall.rpcv1;

/**
 * A generic polycall_rpc v1 CLI: one round trip, no retry.
 *
 *   java org.obinexus.polycall.rpcv1.PolyCallCall <host:port> <service> <operation> <json-input> [deadline_ms]
 *
 * Prints the RESPONSE payload exactly as received (for byte-identical
 * cross-language comparison in tests/conformance/) and exits with the same
 * code contract as the C CLI's `polycall call` (docs/RPC.md).
 */
public final class PolyCallCall {

    private static final int EXIT_OK = 0;
    private static final int EXIT_OTHER = 1;
    private static final int EXIT_USAGE = 2;
    private static final int EXIT_CONFIG = 3;
    private static final int EXIT_UNSUPPORTED = 4;
    private static final int EXIT_TRANSPORT = 5;
    private static final int EXIT_DEADLINE = 6;
    private static final int EXIT_AUTH = 7;

    private PolyCallCall() { }

    private static int exitFor(String code) {
        if (code == null) return EXIT_OTHER;
        switch (code) {
            case "request.malformed":
            case "input.invalid":
                return EXIT_CONFIG;
            case "operation.unknown":
            case "item.unknown":
                return EXIT_UNSUPPORTED;
            case "deadline.exceeded":
                return EXIT_DEADLINE;
            case "auth.denied":
                return EXIT_AUTH;
            default:
                return EXIT_OTHER;
        }
    }

    public static void main(String[] args) {
        if (args.length < 3 || args.length > 5) {
            System.err.println("usage: PolyCallCall <host:port> <service> <operation> <json-input> [deadline_ms]");
            System.exit(EXIT_USAGE);
        }
        String endpoint = args[0];
        String service = args[1];
        String operation = args[2];
        String input = args.length >= 4 ? args[3] : "null";
        int deadlineMs = 5000;
        if (args.length == 5) {
            try {
                deadlineMs = Integer.parseInt(args[4]);
            } catch (NumberFormatException ignored) {
                // keep default
            }
        }

        int colon = endpoint.lastIndexOf(':');
        if (colon <= 0) {
            System.err.println("polycall-call: invalid endpoint, expected host:port");
            System.exit(EXIT_USAGE);
        }
        String host = endpoint.substring(0, colon);
        int port;
        try {
            port = Integer.parseInt(endpoint.substring(colon + 1));
        } catch (NumberFormatException e) {
            System.err.println("polycall-call: invalid port in endpoint");
            System.exit(EXIT_USAGE);
            return;
        }

        try {
            PolyCallRpcClient.RpcResult r =
                    PolyCallRpcClient.call(host, port, service, operation, input, deadlineMs);
            System.out.println(r.rawPayload);
            System.exit(r.ok ? EXIT_OK : exitFor(r.errorCode));
        } catch (PolyCallRpcClient.DeadlineException e) {
            System.err.println("polycall-call: " + e.getMessage());
            System.exit(EXIT_DEADLINE);
        } catch (PolyCallRpcClient.TransportException e) {
            System.err.println("polycall-call: " + e.getMessage());
            System.exit(EXIT_TRANSPORT);
        }
    }
}
