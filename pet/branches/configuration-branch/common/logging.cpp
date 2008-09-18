#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cassert>

#include "logging.h"

int root,
  logPack,
  logUnpack,
  logMorph,
  logLexproc,
  logGrammar,
  logGenerics,
  logAppl,
  logParse,
  logSM,
  logXML,
  logTsdb,
  logSyntax,
  logSemantic;

std::string prio_names[] = {
  "fatal", "alert", "critical", "error",
  "warning", "notice", "info", "debug", ""
};

#if 0
#if HAVE_LIBLOG4CXX
  log4cxx::LoggerPtr PrintfBuffer::logger(
                                   log4cxx::Logger::getLogger("PrintfBuffer"));
#endif // HAVE_LIBLOG4CXX

IPrintfHandler::~IPrintfHandler() {
}

PrintfBuffer::PrintfBuffer(int size, int chunkSize)
  : size_(size), chunkSize_(chunkSize), nWritten_(1)
{
  assert(size_ > 1 && chunkSize_ > 1);
  
  buffer_ = (char*)malloc(size_ * (sizeof *buffer_));
  if(buffer_ == 0) {
    LOG_FATAL(logger, "malloc returned 0");
    exit(1);
  }
  buffer_[0] = '\0';
}

PrintfBuffer::~PrintfBuffer() {
  free(buffer_);
}

char* PrintfBuffer::getContents() {
  assert(nWritten_ <= size_);
  assert(buffer_[nWritten_-1] == '\0');
  return buffer_;
}

int PrintfBuffer::vprintf(char *fmt, va_list ap) {
  assert(nWritten_ <= size_);
  assert(buffer_[nWritten_-1] == '\0');
  
  int res;
  bool fits = false;
  
  //try vsnprintf and increarse buffer until string fits
  do {
    res = vsnprintf((buffer_ + nWritten_ - 1), (size_ - nWritten_ + 1),
                    fmt, ap);
    
    if(res < 0) {
      LOG_FATAL(logger, "vnsprintf returned < 0");
      exit(1);
    }
    
    if(nWritten_ + res <= size_) {
      nWritten_ += res;
      fits = true;
    }
    else {
      LOG(logger, Level::DEBUG,
          "we need realloc, size_ == %d, nWritten_ == %d, res == %d",
          size_, nWritten_, res);
      int max = res > chunkSize_ ? res : chunkSize_;
      size_ += max;
      buffer_ = (char*)realloc(buffer_, size_ * (sizeof *buffer_));
      if(buffer_ == 0) {
        LOG_FATAL(logger, "realloc returned 0");
        exit(1);
      }
    }
  }
  while(!fits);

  assert(nWritten_ <= size_);
  assert(buffer_[nWritten_-1] == '\0');
  return res;
}

PrintfBuffer::PrintfBuffer(const PrintfBuffer &pb) : size_(0) {
}

StreamPrinter::StreamPrinter(FILE *file) : file_(file) {
}

StreamPrinter::~StreamPrinter() {
}

int StreamPrinter::vprintf(char *fmt, va_list ap) {
  return vfprintf(file_, fmt, ap);
}

int pbprintf(IPrintfHandler &iph, const char *fmt, ...) {
  va_list ap;
  int res;

  va_start(ap, fmt);
  res = iph.vprintf(fmt, ap);
  va_end(ap);
  
  return res;
}

#endif
