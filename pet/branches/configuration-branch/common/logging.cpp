#include <cstdarg>
#include <cstdio>

#include "logging.h"

PrintfBuffer::PrintfBuffer(char *buffer, int size)
  : buffer_(buffer), size_(size), nWritten_(0)
{
  buffer_[0] = '\0';
}

PrintfBuffer::~PrintfBuffer() {
}

char* PrintfBuffer::getContents() {
  return buffer_;
}

bool PrintfBuffer::isOverflowed() {
  return nWritten_ > size_ - 1;
}

int PrintfBuffer::vprintf(char *fmt, va_list ap) {
  int res;
  
  if(isOverflowed())
    return -1;
  
  res = vsnprintf(buffer_ + nWritten_, size_ - nWritten_, fmt, ap);

  if(res >= 0)
    nWritten_ += res;
  else
    nWritten_ += size_ + 1; //let's pretend we have an overflow
  
  return res;
}

PrintfBuffer::PrintfBuffer(const PrintfBuffer &pb) : size_(0) {
}

int pbprintf(PrintfBuffer *printfBuffer, char *fmt, ...) {
  va_list ap;
  int res;

  va_start(ap, fmt);
  res = printfBuffer->vprintf(fmt, ap);
  va_end(ap);
  
  return res;
}
