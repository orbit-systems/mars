#ifndef MARS_REPORTING_H
#define MARS_REPORTING_H

#include "compiler.h"

#include <stdio.h>

typedef enum ReportKind: u8 {
    REPORT_ERROR,
    REPORT_WARNING,
} ReportKind;

typedef enum ReportLabelKind: u8 {
    REPORT_LABEL_PRIMARY,
    REPORT_LABEL_SECONDARY,
} ReportLabelKind;

typedef struct ReportLabel {
    ReportLabelKind kind;
    bool skip;
    usize gutter_pos;
    SourceFileId id;
    usize start, end;
    string message;
} ReportLabel;

typedef struct Report {
    ReportKind kind;
    bool vertical_pad;
    string message;
    Vec(ReportLabel) labels;
    Vec(string) notes;
    const Vec(SourceFile) sources;
} Report;

Report* report_new(
    ReportKind kind, 
    string message, 
    const Vec(SourceFile) sources,
    bool vertical_pad
);
void report_destroy(Report* r);

void report_add_label(
    Report* r,
    ReportLabelKind kind,
    SourceFileId id,
    usize start, 
    usize end,
    string message
);
void report_add_note(Report* r, string note);

void report_render(
    FILE* out, 
    Report* r
);

#endif // MARS_REPORTING_H
