// src/core/qa/metrics.h
typedef struct {
    double code_coverage;
    double test_accuracy;
    double error_handling_ratio;
    double performance_factor;
} qa_metrics_t;

double calculate_quality_score(qa_metrics_t* metrics, double* weights);
