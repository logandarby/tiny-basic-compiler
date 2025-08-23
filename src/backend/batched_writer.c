#include "batched_writer.h"
#include "dz_debug.h"
#include <stdio.h>
#include <string.h>

BatchedWriter batched_writer_init(FILE *output_file) {
  if (output_file == NULL) {
    DZ_THROW("batched_writer_init() called with NULL output_file");
  }
  return (BatchedWriter){.output_file = output_file, .buffer_pos = 0};
}

void batched_writer_flush(BatchedWriter *writer) {
  if (writer == NULL) {
    DZ_THROW("batched_writer_flush() called with NULL writer");
  }
  if (writer->buffer_pos > 0) {
    size_t bytes_written =
        fwrite(writer->buffer, 1, writer->buffer_pos, writer->output_file);
    if (bytes_written != writer->buffer_pos) {
      DZ_THROW(
          "batched_writer_flush() failed to write %zu bytes (only wrote %zu)",
          writer->buffer_pos, bytes_written);
    }
    writer->buffer_pos = 0;
  }
}

void batched_writer_close(BatchedWriter *writer) {
  batched_writer_flush(writer);
}

static void batched_writer_ensure_space(BatchedWriter *writer,
                                        size_t needed_space) {
  if (writer->buffer_pos + needed_space >= BATCHED_WRITER_BUFFER_SIZE) {
    batched_writer_flush(writer);
  }
}

void batched_writer_write(BatchedWriter *writer, const char *str) {
  if (writer == NULL) {
    DZ_THROW("batched_writer_write() called with NULL writer");
  }
  if (str == NULL) {
    DZ_THROW("batched_writer_write() called with NULL string");
  }

  size_t len = strlen(str);

  // If the string is larger than our buffer, just write it directly
  if (len >= BATCHED_WRITER_BUFFER_SIZE) {
    batched_writer_flush(writer);
    size_t bytes_written = fwrite(str, 1, len, writer->output_file);
    if (bytes_written != len) {
      DZ_THROW("batched_writer_write() failed to write large string of %zu "
               "bytes (only wrote %zu)",
               len, bytes_written);
    }
    return;
  }

  batched_writer_ensure_space(writer, len);
  memcpy(writer->buffer + writer->buffer_pos, str, len);
  writer->buffer_pos += len;
}

void batched_writer_vprintf(BatchedWriter *writer, const char *format,
                            va_list args) {
  if (writer == NULL) {
    DZ_THROW("batched_writer_vprintf() called with NULL writer");
  }
  if (format == NULL) {
    DZ_THROW("batched_writer_vprintf() called with NULL format string");
  }

  // Try to format directly into the buffer
  size_t remaining_space = BATCHED_WRITER_BUFFER_SIZE - writer->buffer_pos;
  va_list args_copy;
  va_copy(args_copy, args);
  int result = vsnprintf(writer->buffer + writer->buffer_pos, remaining_space,
                         format, args_copy);
  va_end(args_copy);

  if (result < 0) {
    DZ_THROW("batched_writer_vprintf() formatting error with format: '%.100s'",
             format);
  }

  size_t formatted_len = (size_t)result;

  // If it fit in the buffer, just update the position
  if (formatted_len < remaining_space) {
    writer->buffer_pos += formatted_len;
    return;
  }

  // Otherwise, flush and try again
  batched_writer_flush(writer);
  remaining_space = BATCHED_WRITER_BUFFER_SIZE - writer->buffer_pos;

  result = vsnprintf(writer->buffer + writer->buffer_pos, remaining_space,
                     format, args);

  if (result < 0) {
    DZ_THROW("batched_writer_vprintf() formatting error after flush with "
             "format: '%.100s'",
             format);
  }

  if ((size_t)result >= remaining_space) {
    DZ_THROW("batched_writer_vprintf() formatted string too large (%d bytes) "
             "for buffer (%zu bytes available) with format: '%.100s'",
             result, remaining_space, format);
  }

  writer->buffer_pos += result;
}

void batched_writer_printf(BatchedWriter *writer, const char *format, ...) {
  va_list args;
  va_start(args, format);
  batched_writer_vprintf(writer, format, args);
  va_end(args);
}
