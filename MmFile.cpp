// --------------------------------------------------------------
// Workfile : MmFile.cpp
// Author : Johannes Posch
// Date : 20.10.2017
// Description : Implementation of class MmFile
// --------------------------------------------------------------


#include "MmFile.h"
#include "condReturn.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define GOTO_COND(x0, x1) if(x0) goto x1;


MmFile::MmFile(): m_mapped(false), m_opened(false), m_filesize(0),
                  m_fileDescriptor(0), m_memoryMapped(nullptr) {}

MmFile::~MmFile() {

    // close files if they are opened
    this->closeFile(false);
}

bool MmFile::fastMap(std::string const &filepath, file_mode const mode, const bool write, const unsigned long size) {

    bool success = true;

    //open the file
    success = this->openFile(filepath, mode);

    //on error goto error section
    GOTO_COND(!success, fastOpenError)

    //check if a size for resizing is given
    if (size > 0)
        success = this->resize(size);

    //on error goto error section
    GOTO_COND(!success, fastOpenError)

    //map the file
    success = this->map(write);

    //on error goto error section
    GOTO_COND(!success, fastOpenError)

    return true;

    //error section
fastOpenError:

    //release resources
    this->closeFile();

    return false;
}

bool MmFile::openFile(std::string const &filepath, file_mode const mode) {

    //check if the filepath is set
    RETURN_FALSE_COND(filepath.empty())
    //check if a file is already open
    RETURN_FALSE_COND(this->m_opened)
    //check if a file is already mapped
    RETURN_FALSE_COND(this->m_mapped)

    // setup flags with standard create and read write
    int file_flags = O_CREAT | O_RDWR;

    // check if file should be truncated
    if(mode & file_mode::trunc)
        file_flags |= O_TRUNC;

    errno = 0;

    // open the file and store handle
    this->m_fileDescriptor = open(filepath.c_str(), file_flags, S_IRUSR | S_IWUSR);

    // check if opening was successful
    if(this->m_fileDescriptor == -1){
        this->m_fileDescriptor = 0;
        return false;
    }

    // structure to store the file details
    struct stat file_stats;

    // read the file details and store in structure, close on failure
    if(fstat(this->m_fileDescriptor, &file_stats) == -1){
        this->closeFile(false);
        return false;
    }

    // check if the opened file is really a file
    if(!S_ISREG(file_stats.st_mode)){
        this->closeFile(false);
        return false;
    }

    // set the filesize and also the opened flag
    this->m_filesize = file_stats.st_size;
    this->m_opened = true;

    return true;
}

bool MmFile::resize(unsigned long const size) {

    // check if the file is open
    RETURN_FALSE_COND(!this->m_opened)
    // check if the file is mapped
    RETURN_FALSE_COND(this->m_mapped)

    bool success = (ftruncate(this->m_fileDescriptor, size) != -1);

    // return if failed
    RETURN_FALSE_COND(!success)

    this->m_filesize = size;

    return true;
}

bool MmFile::closeFile(bool const explicitSync) {

    // check if file is open
    RETURN_FALSE_COND(!this->m_opened)

    bool success = false;

    // only do if the file was mapped
    if(this->m_mapped) {
        // check if the file should be saved
        if(explicitSync) {
            // synchronize
            this->syncMemoryMapped();
        }
        // unmap memory
        this->unmapMemoryMapped();
    }

    // close the file handle
    success = (close(this->m_fileDescriptor) != -1);

    // reset the variables since no file is loaded
    this->m_fileDescriptor = 0;
    this->m_opened = false;
    this->m_filesize = 0;

    return success;
}

bool MmFile::map(bool const write) {

    // check if file was opened
    RETURN_FALSE_COND(!this->m_opened)
    // check if file was already mapped
    RETURN_FALSE_COND(this->m_mapped)

    // set properties
    int prot = PROT_READ | ((write) ? (PROT_WRITE) : (0x0));

    // map the files
    this->m_memoryMapped = mmap(0, this->m_filesize, prot, MAP_SHARED, this->m_fileDescriptor, 0);

    // check if mapping failed
    bool success = (this->m_memoryMapped != MAP_FAILED);

    // return on failure
    RETURN_FALSE_COND(!success)

    // set flag
    this->m_mapped = true;

    return success;
}

void* MmFile::getMemoryMappedAddress() const {
    return this->m_memoryMapped;;
}


bool MmFile::syncMemoryMapped() {

    // check if file was mapped before
    RETURN_FALSE_COND(!this->m_mapped)

    // synchronize memory in ram with file
    return (msync(this->m_memoryMapped, (size_t)this->m_filesize, MS_SYNC) != -1);
}

bool MmFile::unmapMemoryMapped() {

    // check if the file was mapped before
    RETURN_FALSE_COND(!this->m_mapped)

    // unmap the file and save result
    bool success = (munmap(this->m_memoryMapped, (size_t)this->m_filesize) != -1);

    //reset the variables since no file is mapped anymore
    this->m_mapped = false;
    this->m_memoryMapped = nullptr;

    return success;
}



