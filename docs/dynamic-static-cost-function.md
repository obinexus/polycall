# Sinphasé Dynamic vs Static Cost Functions

This document summarizes how LibPolyCall applies cost analysis within the Sinphasé governance framework.

## Dynamic Cost Function

A dynamic cost function evaluates component cost based on real-time metrics collected from the code base. In LibPolyCall this is implemented by `SinphaseCostCalculator` in `sinphase_governance/core/evaluator/cost_calculator.py`. Metrics such as lines of code, dependency count, and cyclomatic complexity are analyzed, then a weighted cost is calculated:

```
cost = (lines * lines_factor)
      + (complexity * complexity_factor)
      + (dependencies * dependency_factor)
      + (functions * function_factor)
```

The weights are configurable and may change between phases. Because metrics are recomputed each run, the resulting cost adapts to code changes. This "model" style approach lets developers see the impact of refactoring or new features immediately.

## Static Cost Function

A static cost function uses pre‑determined values for each component. The calculation does not change at runtime. This approach is used when cost data must remain consistent across builds or when live analysis is too expensive.

Static values can be derived from a previous dynamic analysis or from governance policy. Once recorded, they remain fixed until manually updated.

## Hybrid Usage

LibPolyCall's governance tooling allows either approach. During early development the dynamic calculation provides continuous feedback. As components stabilize, static values may be locked in to ensure repeatability and to support hotwiring of legacy features. Hotwiring lets developers gradually replace old API functions while keeping compatibility layers in place. Cost tracking helps ensure the new code remains within acceptable complexity thresholds.

