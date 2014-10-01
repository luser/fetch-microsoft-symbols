/*
 * Copyright 2014 Mozilla Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <mspack.h>
#include <fcntl.h>
#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace {
  typedef std::vector<uint8_t> bytes;
  struct cab_file : public mspack_file {
    bytes* buf;
    size_t offset;
  };

  struct mspack_file* cab_open(struct mspack_system *self,
                               const char *filename,
                               int mode) {
    struct cab_file* f = new cab_file;
    f->buf = reinterpret_cast<bytes*>(const_cast<char*>(filename));
    f->offset = 0;
    return f;
  }

  void cab_close(struct mspack_file *file) {
    cab_file* f = reinterpret_cast<cab_file*>(file);
    delete f;
  }

  int cab_read(struct mspack_file *file,
               void *buffer,
               int bytes) {
    cab_file* f = reinterpret_cast<cab_file*>(file);
    if (f->offset >= f->buf->size())
      return 0;
    int count = std::min(bytes, int(f->buf->size() - f->offset));
    memcpy(buffer, &(*f->buf)[f->offset], count);
    f->offset += count;
    return count;
  }

  int cab_write(struct mspack_file *file,
                void *buffer,
                int bytes) {
    cab_file* f = reinterpret_cast<cab_file*>(file);
    uint8_t* b = reinterpret_cast<uint8_t*>(buffer);
    if (f->offset == f->buf->size()) {
      f->buf->insert(f->buf->begin() + f->offset, b, b+bytes);
    } else {
      if (f->offset + bytes > f->buf->size()) {
        f->buf->resize(f->offset + bytes);
      }
      memcpy(&(*f->buf)[f->offset], buffer, bytes);
    }
    f->offset += bytes;
    return bytes;
  }

  int cab_seek(struct mspack_file *file,
               off_t offset,
               int mode) {
    cab_file* f = reinterpret_cast<cab_file*>(file);
    if (mode == MSPACK_SYS_SEEK_START) {
      f->offset = offset;
    } else if (mode == MSPACK_SYS_SEEK_CUR) {
      f->offset += offset;
    } else {
      f->offset = f->buf->size() + offset;
    }
    return 0;
  }

  off_t cab_tell(struct mspack_file *file) {
    cab_file* f = reinterpret_cast<cab_file*>(file);
    return f->offset;
  }

  void cab_message(struct mspack_file *file,
                   const char *format,
                   ...) {
  }

  void* cab_alloc(struct mspack_system *self,
                 size_t bytes) {
    return malloc(bytes);
  }

  void cab_free(void *ptr) {
    free(ptr);
  }

  void cab_copy(void *src,
                void *dest,
                size_t bytes) {
    memcpy(dest, src, bytes);
  }
} // namespace

bool extract_cab(std::vector<uint8_t>& cab_bytes,
                 std::vector<uint8_t>& file_bytes) {
  struct mspack_system sys = {
    cab_open,
    cab_close,
    cab_read,
    cab_write,
    cab_seek,
    cab_tell,
    cab_message,
    cab_alloc,
    cab_free,
    cab_copy,
    nullptr,
  };
  struct mscab_decompressor* dc = mspack_create_cab_decompressor(&sys);
  struct mscabd_cabinet* cab =
    dc->open(dc, reinterpret_cast<char*>(&cab_bytes));
  if (!cab) {
    mspack_destroy_cab_decompressor(dc);
    return false;
  }
  dc->extract(dc, &cab->files[0], reinterpret_cast<char*>(&file_bytes));
  dc->close(dc, cab);
  mspack_destroy_cab_decompressor(dc);
  return true;
}
