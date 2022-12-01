#pragma once

#include <string>
#include <vector>
#include <cassert>
#include <algorithm>

namespace miniduo
{

class Buffer {

public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;
    static const char kCRLF[];

    Buffer();

    void swap(Buffer& rhs);
    size_t readableBytes() const;
    size_t writableBytes() const;
    size_t prependableBytes() const;
    // begin ptr of readable data;
    const char* beginRead() const ;
    // retrieve readable data;
    void retrieve(size_t len) ;
    void retrieveUntil(const char* end);
    void retrieveAll();
    // return all readable str as a string
    std::string retrieveAsString();
    // append new str to the writable zoom
    void append(const std::string& str);
    void append(const char* data, size_t len);
    void append(const void* data, size_t len);
    // begin ptr of writable zoom
    char* beginWrite();
    const char* beginWrite() const;
    // add prepend data
    void prepend(const void* data, size_t len);
    // shrink the writable len to reserve
    void shrink(size_t reserve);
    /// @brief use readv(2) to read data from fd into buffer;
    ssize_t readFd(int fd, int* savedErrno);

    /// @brief  make space for write data with length of 'len';
    void makeSpace(size_t len);

    void hasWritten(size_t len);

    const char* findCRLF() const {
        const char* crlf = std::search(begin(), beginWrite(), kCRLF, kCRLF+2);
        return crlf == beginWrite() ? nullptr : crlf;
    };
    const char* findCRLF(const char* start) const {
        assert(beginRead() <= start);
        assert(start <= beginWrite());
        const char* crlf = std::search(start, beginWrite(), kCRLF, kCRLF+2);
        return crlf == beginWrite() ? nullptr : crlf;
    };

private:
    // begin ptr of buffer_
    char* begin();
    const char* begin() const;
    

private:
    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
}; // class Buffer
    
} // namespace miniduo

