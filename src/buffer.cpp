#include "buffer.h"

#include <sys/uio.h> // readv(2)

using namespace miniduo;

// begin member functions of class Buffer;
Buffer::Buffer()
    : buffer_(kCheapPrepend + kInitialSize),
      readerIndex_(kCheapPrepend),
      writerIndex_(readerIndex_)
{
    assert(readableBytes() == 0);
    assert(writableBytes() == kInitialSize);
    assert(prependableBytes() == kCheapPrepend);
}

void Buffer::swap(Buffer& rhs) {
    buffer_.swap(rhs.buffer_);
    std::swap(readerIndex_, rhs.readerIndex_);
    std::swap(writerIndex_, rhs.writerIndex_);
}

size_t Buffer::readableBytes() const {
    return writerIndex_ - readerIndex_;
}

size_t Buffer::writableBytes() const {
    return buffer_.size() - writerIndex_;
}

size_t Buffer::prependableBytes() const {
    return readerIndex_ - 0;
}

const char* Buffer::beginRead() const {
    return begin()+readerIndex_;
}

void Buffer::retrieve(size_t len) {
    assert(readableBytes() >= len);
    readerIndex_ += len;
}

void Buffer::retrieveUntil(const char* end) {
    assert(end >= beginRead() && end <= beginWrite());
    readerIndex_ += (end - beginRead());
}

void Buffer::retrieveAll() {
    readerIndex_ = kCheapPrepend;
    writerIndex_ = kCheapPrepend;
}

std::string Buffer::retrieveAsString() {
    std::string str(beginRead(), readableBytes());
    retrieveAll();
    return str;
}

void Buffer::append(const std::string& str) {
    // size_t len = str.length();
    // makeSpace(len);
    // assert(writableBytes() >= len);
    // std::copy(str.begin(), str.end(), beginWrite());
    // writerIndex_ += len;

    append(str.data(), str.length());
}

void Buffer::append(const char* data, size_t len) {
    makeSpace(len);
    assert(writableBytes() >= len);
    std::copy(data, data+len, beginWrite());
    writerIndex_ += len;
}

void Buffer::append(const void* data, size_t len) {
    append((const char*) data, len);
}

char* Buffer::beginWrite() {
    return begin() + writerIndex_;
}

const char* Buffer::beginWrite() const {
    return begin() + writerIndex_;
}

void Buffer::prepend(const void* data, size_t len) {
    assert(prependableBytes() >= len);
    readerIndex_ -= len;
    const char* beginPtr = static_cast<const char*> (data);
    std::copy(beginPtr, beginPtr+len, begin() + readerIndex_);
}

void Buffer::shrink(size_t reserve) {
    std::vector<char> buf(kCheapPrepend+readableBytes()+reserve);
    std::copy(beginRead(), beginRead()+readableBytes(), buf.begin() + kCheapPrepend);
    buffer_.swap(buf);
    assert(writableBytes() == reserve);
}

ssize_t Buffer::readFd(int fd, int* saveErrno) {
    char extrabuf[65536];
    struct iovec vec[2];
    const size_t writable = writableBytes();
    vec[0].iov_base = beginWrite();
    vec[0].iov_len = writable;
    vec[1].iov_base = extrabuf;
    vec[1].iov_len = sizeof(extrabuf);
    const ssize_t n = readv(fd, vec, 2);
    if(n < 0) {
        *saveErrno = errno;
    }
    else if(static_cast<size_t>(n) <= writable) {
        writerIndex_ += n;
    }
    else {
        writerIndex_ = buffer_.size();
        append(extrabuf, n - writable);
    }
    return n;
}

char* Buffer::begin() {
    return &*(buffer_.begin());
}

const char* Buffer::begin() const {
    return &*(buffer_.begin());
}

void Buffer::makeSpace(size_t len) {
    if(writableBytes() >= len) return;
    if(prependableBytes()-kCheapPrepend+writableBytes() >= len) {
        size_t readable = readableBytes();
        std::copy(begin(), beginWrite(), buffer_.begin()+kCheapPrepend);
        writerIndex_ = kCheapPrepend + readable;
        readerIndex_ = kCheapPrepend;        
    }
    else {
        buffer_.resize(writerIndex_ + len);
    }
    assert(writableBytes() >= len);
}

void Buffer::hasWritten(size_t len) {
    assert(len <= writableBytes());
    writerIndex_ += len;
}



// end member functions of class Buffer;
