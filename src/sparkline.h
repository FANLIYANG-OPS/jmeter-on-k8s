//
// Created by fly on 8/3/23.
//

#ifndef CACHE_1_0_0_SPARKLINE_H
#define CACHE_1_0_0_SPARKLINE_H


struct sample {
    double value;
    char *label;
};

struct sequence {
    int length;
    int labels;
    struct sample *sample;
    double min, max;
};

#define SPARKLINE_NO_FLAGS 0
#define SPARKLINE_FILL 1
#define SPARKLINE_LOG_SCALE 2

struct sequence *createSparklineSequence(void);

void sparklineSequenceAddSample(struct sequence *seq, double value, char *label);

void freeSparklineSequence(struct sequence *seq);

Sds sparklineRenderRange(Sds output, struct sequence *seq, int rows, int offset, int len, int flags);

Sds sparklineRender(Sds output, struct sequence *seq, int columns, int rows, int flags);

#endif //CACHE_1_0_0_SPARKLINE_H
