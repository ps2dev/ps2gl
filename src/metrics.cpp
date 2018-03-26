//
// ps2gl metrics module
//
//  Written by Stefan Boberg
//

#include "ps2gl/metrics.h"
#include <string.h>

Metric_t g_Metrics[kMetricsCount];

void pglResetMetrics()
{
    memset(g_Metrics, 0, sizeof(g_Metrics));
}
