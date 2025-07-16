#include "qa_framework.h"
#include "proto_stub.h"

QA_TEST("Stub True Positive Access", {
    int res = polycall_stub_authorize("demo-agent", "/api/secure");
    QA_ASSERT_CATEGORY(QA_TRUE_POSITIVE, res == 1, "demo-agent allowed access");
})

QA_TEST("Stub False Negative Detection", {
    int res = polycall_stub_authorize("unknown", "/api/secure");
    QA_ASSERT_CATEGORY(QA_FALSE_NEGATIVE, res == 1, "unauthorized agent was wrongly allowed");
})
