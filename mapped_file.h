#pragma once

#include <sys/mman.h> // For mmap, munmap, MADV_SEQUENTIAL
#include <sys/stat.h> // For fstat, struct stat
#include <fcntl.h>    // For open, O_RDONLY
#include <unistd.h>   // For close
#include <cstdint>    // For uint8_t
#include <cstddef>    // For size_t
#include <stdexcept>  // For std::runtime_error
#include <iostream>

class MappedFile{

public:
    explicit MappedFile(const char* filepath){
        fd = open(filepath, O_RDONLY);
        if(fd < 0){
            throw std::runtime_error("Unable to open the ITCH file.");
        }

        struct stat st;
        if(fstat(fd, &st) < 0){
            close(fd);
            throw std:: runtime_error("Failed to stat the ITCH file.");
        }

        // Exact size of the binary in bites
        size_ = st.st_size;

        // Mapping the file directly to the virtual address space
        void* addr = mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, fd, 0);
        if(addr == MAP_FAILED){
            close(fd);
            throw std:: runtime_error("Failed to map the ITCH file.");
        }

        data_ = static_cast<const uint8_t*> (addr);

        // Advise kernel to read this file sequentially from start to end
        madvise(addr, size_, MADV_SEQUENTIAL);

    }

    ~MappedFile() {
        if(data_ && data_ != MAP_FAILED){
            munmap(const_cast<uint8_t*>(data_), size_);
        }

        if(fd>=0) close(fd);
    }

    // Best practice: Prevent copying
    MappedFile(const MappedFile&) = delete;
    MappedFile& operator = (const MappedFile&) = delete;

    const uint8_t* data() const { return data_; }
    size_t size() const { return size_; }
private:
    int fd = -1;
    size_t size_ = 0;
    const uint8_t* data_ = nullptr;
};