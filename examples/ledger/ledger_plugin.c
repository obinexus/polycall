/*
 * Flagship demo: a ledger operation plugin, loaded via `polycall run --load`
 * (docs/PLUGINS.md, docs/TODO.md P3). No core rebuild.
 *
 *   ledger.balance   idempotent      {"account":"string"} -> {"account","balance"}
 *   ledger.transfer  NOT idempotent  {"from","to","amount"} -> {"from","to","amount","from_balance","to_balance"}
 *
 * Accounts and balances are a fixed, deterministic, in-memory table (this is
 * a demo, not a real ledger): alice=100, bob=50, carol=0. The runtime serves
 * one thread per connection (docs/CONCURRENCY.md), so this table -- unlike
 * the runtime's own read-only operation registry -- is genuinely shared,
 * mutable state that more than one client can reach at the same instant;
 * every read and every transfer below runs under a single plugin-wide
 * mutex so concurrent transfers neither lose an update nor observe another
 * transfer half-applied.
 *
 * Input parsing is intentionally minimal, hand-rolled field extraction (not
 * a JSON parser): plugins only depend on include/polycall_runtime.h, and the
 * runtime always hands a plugin a flat, compact, known-shape JSON object
 * (src/runtime/runtime.c's dispatch_request re-emits "input" that way), so a
 * small scanner is enough and keeps the plugin dependency-free.
 */

#include "polycall_runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#  define PLUGIN_EXPORT __declspec(dllexport)
#  include <windows.h>
typedef CRITICAL_SECTION ledger_lock_t;
static void ledger_lock_init(ledger_lock_t *l) { InitializeCriticalSection(l); }
static void ledger_lock(ledger_lock_t *l) { EnterCriticalSection(l); }
static void ledger_unlock(ledger_lock_t *l) { LeaveCriticalSection(l); }
#else
#  define PLUGIN_EXPORT __attribute__((visibility("default")))
#  include <pthread.h>
typedef pthread_mutex_t ledger_lock_t;
static void ledger_lock_init(ledger_lock_t *l) { pthread_mutex_init(l, NULL); }
static void ledger_lock(ledger_lock_t *l) { pthread_mutex_lock(l); }
static void ledger_unlock(ledger_lock_t *l) { pthread_mutex_unlock(l); }
#endif

/* Guards every read and write of g_accounts below. Initialized once from
 * polycall_ops_register(), which the runtime calls exactly once per `--load`
 * of this plugin, before it starts serving any connection. */
static ledger_lock_t g_lock;

/* ---- tiny field extraction over a flat, compact JSON object ------------ */

static int find_field(const char *json, const char *key, const char **start)
{
    char pat[64];
    const char *p;
    snprintf(pat, sizeof pat, "\"%s\":", key);
    p = strstr(json, pat);
    if (!p) return 0;
    *start = p + strlen(pat);
    return 1;
}

static int get_str(const char *json, const char *key, char *out, size_t outcap)
{
    const char *p;
    size_t i = 0;
    if (!find_field(json, key, &p) || *p != '"') return 0;
    p++;
    while (*p && *p != '"' && i + 1 < outcap) out[i++] = *p++;
    out[i] = '\0';
    return *p == '"';
}

static int get_num(const char *json, const char *key, double *out)
{
    const char *p;
    char *end;
    if (!find_field(json, key, &p)) return 0;
    while (*p == ' ') p++;
    *out = strtod(p, &end);
    return end != p;
}

/* ---- fixed account table ------------------------------------------------ */

typedef struct { const char *name; long balance; } account_t;

static account_t g_accounts[] = {
    { "alice", 100 },
    { "bob",   50  },
    { "carol", 0   },
};
#define NUM_ACCOUNTS (int)(sizeof g_accounts / sizeof g_accounts[0])

static account_t *find_account(const char *name)
{
    int i;
    for (i = 0; i < NUM_ACCOUNTS; ++i) {
        if (!strcmp(g_accounts[i].name, name)) return &g_accounts[i];
    }
    return NULL;
}

/* ---- operations ---------------------------------------------------------- */

static polycall_op_status_t op_ledger_balance(
    const char *input_json, uint32_t deadline_ms,
    char *out, size_t out_cap, char *ec, size_t ec_cap,
    char *em, size_t em_cap, void *user)
{
    char account[64];
    account_t *a;
    (void)deadline_ms; (void)user;

    if (!get_str(input_json, "account", account, sizeof account) || !account[0]) {
        snprintf(ec, ec_cap, "input.invalid");
        snprintf(em, em_cap, "ledger.balance requires a string 'account'");
        return POLYCALL_OP_ERR_INPUT;
    }
    ledger_lock(&g_lock);
    a = find_account(account);
    if (!a) {
        ledger_unlock(&g_lock);
        snprintf(ec, ec_cap, "account.unknown");
        snprintf(em, em_cap, "no account '%s'", account);
        return POLYCALL_OP_ERR_NOTFOUND;
    }
    snprintf(out, out_cap, "{\"account\":\"%s\",\"balance\":%ld}", a->name, a->balance);
    ledger_unlock(&g_lock);
    return POLYCALL_OP_OK;
}

/* NOT idempotent: performs a real, one-shot balance mutation. The runtime
 * and every client here perform exactly one round trip per call and never
 * retry (see docs/CONFORMANCE.md) -- calling this twice moves money twice. */
static polycall_op_status_t op_ledger_transfer(
    const char *input_json, uint32_t deadline_ms,
    char *out, size_t out_cap, char *ec, size_t ec_cap,
    char *em, size_t em_cap, void *user)
{
    char from_name[64], to_name[64];
    double amount_d;
    long amount;
    account_t *from, *to;
    (void)deadline_ms; (void)user;

    if (!get_str(input_json, "from", from_name, sizeof from_name) || !from_name[0] ||
        !get_str(input_json, "to", to_name, sizeof to_name) || !to_name[0] ||
        !get_num(input_json, "amount", &amount_d)) {
        snprintf(ec, ec_cap, "input.invalid");
        snprintf(em, em_cap,
                 "ledger.transfer requires string 'from', 'to' and numeric 'amount'");
        return POLYCALL_OP_ERR_INPUT;
    }
    amount = (long)amount_d;
    if (amount_d != (double)amount || amount <= 0) {
        snprintf(ec, ec_cap, "input.invalid");
        snprintf(em, em_cap, "'amount' must be a positive integer");
        return POLYCALL_OP_ERR_INPUT;
    }
    if (!strcmp(from_name, to_name)) {
        snprintf(ec, ec_cap, "input.invalid");
        snprintf(em, em_cap, "'from' and 'to' must differ");
        return POLYCALL_OP_ERR_INPUT;
    }
    /* Account lookup, the funds check, and the mutation all run under one
     * critical section: two concurrent transfers out of the same account
     * must see each other's effect, not both check a stale balance and
     * both succeed against money that isn't there twice over. */
    ledger_lock(&g_lock);
    from = find_account(from_name);
    to = find_account(to_name);
    if (!from || !to) {
        ledger_unlock(&g_lock);
        snprintf(ec, ec_cap, "account.unknown");
        snprintf(em, em_cap, "no account '%s'", from ? to_name : from_name);
        return POLYCALL_OP_ERR_NOTFOUND;
    }
    if (from->balance < amount) {
        ledger_unlock(&g_lock);
        snprintf(ec, ec_cap, "funds.insufficient");
        snprintf(em, em_cap, "'%s' has %ld, cannot send %ld",
                 from->name, from->balance, amount);
        return POLYCALL_OP_ERR_INPUT;
    }

    /* the one-shot mutation: exactly this call's effect, once. */
    from->balance -= amount;
    to->balance += amount;

    snprintf(out, out_cap,
             "{\"from\":\"%s\",\"to\":\"%s\",\"amount\":%ld,"
             "\"from_balance\":%ld,\"to_balance\":%ld}",
             from->name, to->name, amount, from->balance, to->balance);
    ledger_unlock(&g_lock);
    return POLYCALL_OP_OK;
}

PLUGIN_EXPORT int POLYCALL_CALL
polycall_ops_register(polycall_runtime_t *rt, uint32_t abi_major)
{
    static const polycall_op_desc_t balance_desc = {
        "ledger", "balance", "query an account balance",
        "{\"account\":\"string\"}", "{\"account\":\"string\",\"balance\":\"integer\"}",
        true, op_ledger_balance, NULL
    };
    static const polycall_op_desc_t transfer_desc = {
        "ledger", "transfer", "move funds between two accounts (one-shot, not retried)",
        "{\"from\":\"string\",\"to\":\"string\",\"amount\":\"integer\"}",
        "{\"from\",\"to\",\"amount\",\"from_balance\",\"to_balance\"}",
        false, op_ledger_transfer, NULL
    };

    if (abi_major != (uint32_t)POLYCALL_RUNTIME_ABI_VERSION) {
        return POLYCALL_PLUGIN_ABI_MISMATCH;
    }
    ledger_lock_init(&g_lock);
    if (polycall_runtime_register(rt, &balance_desc) != 0) return POLYCALL_PLUGIN_ERROR;
    if (polycall_runtime_register(rt, &transfer_desc) != 0) return POLYCALL_PLUGIN_ERROR;
    return POLYCALL_PLUGIN_OK;
}
