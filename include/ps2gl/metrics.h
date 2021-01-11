//
// ps2gl Metrics Module definitions
//
//  Original author: Stefan Boberg (boberg@team17.com)
//

#define PS2GL_METRICS_ENABLE 1

enum MetricsEnum {
    /// Number of textures uploaded to the GS
    kMetricsTextureUploadCount,

    /// Number of bytes of texture data uploaded to the GS
    kMetricsTextureUploadBytes,

    /// Number of CLUTs uploaded to the GS
    kMetricsClutUploadCount,

    /// Number of VU1 renderer code uploads
    kMetricsRendererUpload,

    /// Number of texture binds (glBindTexture())
    kMetricsBindTexture,

    /// Total number of metrics quantities
    kMetricsCount,
};

typedef unsigned long long Metric_t; // 64-bit integer

extern Metric_t g_Metrics[kMetricsCount];

/** Reset all metric values to zero
  */
extern void pglResetMetrics();

/** Get value of specified metric
  */
inline Metric_t pglGetMetric(MetricsEnum eMetric)
{
    return g_Metrics[eMetric];
}

/** Reset specified metric
  */
inline void pglResetMetric(MetricsEnum eMetric)
{
    g_Metrics[eMetric] = 0;
}

/** Increase metric value by specified amount
  */
inline void pglAddToMetric(MetricsEnum eMetric, Metric_t Value = 1)
{
#if PS2GL_METRICS_ENABLE
    g_Metrics[eMetric] += Value;
#endif
}
