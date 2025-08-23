#pragma once

#include "compiler_compatibility.h"
#include <stdarg.h>
#include <stdio.h>

#define BATCHED_WRITER_BUFFER_SIZE (128 * 1024) // 128KB buffer

typedef struct {
  FILE *output_file;
  char buffer[BATCHED_WRITER_BUFFER_SIZE];
  size_t buffer_pos;
} BatchedWriter;

BatchedWriter batched_writer_init(FILE *output_file);
void batched_writer_close(BatchedWriter *writer);

void batched_writer_write(BatchedWriter *writer, const char *str);
void batched_writer_printf(BatchedWriter *writer, const char *format, ...)
    FORMAT_PRINTF(2, 3);
void batched_writer_vprintf(BatchedWriter *writer, const char *format,
                            va_list args);

void batched_writer_flush(BatchedWriter *writer);