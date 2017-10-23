// --------------------------------------------------------------
// Workfile : MmFile.cpp
// Author : Johannes Posch
// Date : 20.10.2017
// Description : Implementation of class MmFile
// --------------------------------------------------------------


#include "MmFile.h"
#include "condReturn.h"

#ifndef _WIN32
	// includes for linux / unix
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#endif //_WIN32

#define GOTO_COND(x0, x1) if(x0) goto x1;

#ifdef _WIN32
// windows code
MmFile::MmFile() : m_mapped(false), m_opened(false), m_filesize(0),
					m_fileDescriptor(INVALID_HANDLE_VALUE), m_memoryMapped(INVALID_HANDLE_VALUE),
					m_memoryBaseHandle(INVALID_HANDLE_VALUE) {}
#else
// unix code
MmFile::MmFile(): m_mapped(false), m_opened(false), m_filesize(0),
                  m_fileDescriptor(0), m_memoryMapped(nullptr) {}
#endif //_WIN32

MmFile::~MmFile() {

    // close files if they are opened
    this->closeFile(false);
}

bool MmFile::fastMap(std::string const &filepath, const bool write, const unsigned long size) {

    bool success = true;

    //open the file
    success = this->openFile(filepath);

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
//TODO: Remove mode
bool MmFile::openFile(std::string const &filepath) {

    //check if the filepath is set
	RETURN_FALSE_COND(filepath.empty())
	//check if a file is already open
	RETURN_FALSE_COND(this->m_opened)
	//check if a file is already mapped
	RETURN_FALSE_COND(this->m_mapped)

#ifdef _WIN32
	// windows code

	// get the Handle to the file
	this->m_fileDescriptor = CreateFile(filepath.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, 0, 0);
	
	// check if the file created successful
	if (this->m_fileDescriptor == NULL || this->m_fileDescriptor == INVALID_HANDLE_VALUE) {
		this->m_fileDescriptor = INVALID_HANDLE_VALUE;
		return false;
	}

	// get the filesize
	this->m_filesize = GetFileSize(this->m_fileDescriptor, NULL);
#else
	// unix code

    // setup flags with standard create and read write
    int file_flags = O_CREAT | O_RDWR;

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

#endif // _Win32
    this->m_opened = true;

    return true;
}

bool MmFile::resize(unsigned long const size) {

    // check if the file is open
	RETURN_FALSE_COND(!this->m_opened)
	// check if the file is mapped
	RETURN_FALSE_COND(this->m_mapped)

	bool success = false;

#ifdef _WIN32
	// windows code

	// set the pointer to the new end
	success = SetFilePointer(this->m_fileDescriptor, size, NULL, FILE_BEGIN);

	// RETURN IF FAILED
	RETURN_FALSE_COND(!success)

	// save the new file size
	success = SetEndOfFile(this->m_fileDescriptor);

	// reset the position to the beginning
	SetFilePointer(this->m_fileDescriptor, 0, 0, FILE_BEGIN);
#else
	// unix code

	// truncate file
    success = (ftruncate(this->m_fileDescriptor, size) != -1);
    
#endif // _WIN32

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
#ifdef _WIN32
	// windows code

	//close handle
	success = CloseHandle(this->m_fileDescriptor);

	this->m_fileDescriptor = INVALID_HANDLE_VALUE;
#else
	// unix code

    // close the file handle
    success = (close(this->m_fileDescriptor) != -1);
	this->m_fileDescriptor = 0;
#endif //_WIN32
    // reset the variables since no file is loaded
    
    this->m_opened = false;
    this->m_filesize = 0;

    return success;
}

bool MmFile::map(bool const write) {

    // check if file was opened
	RETURN_FALSE_COND(!this->m_opened)
	// check if file was already mapped
	RETURN_FALSE_COND(this->m_mapped)

	bool success = true;
#ifdef _WIN32
	// windows code

	// create the mapping
	this->m_memoryMapped = CreateFileMapping(this->m_fileDescriptor, 0, ((write) ? (PAGE_READWRITE) : (PAGE_READONLY)), 0, this->m_filesize, 0);

	//  check the handle
	if (this->m_memoryMapped == NULL || this->m_memoryMapped == INVALID_HANDLE_VALUE)
		success = false;
	
	// create the view of the mapping
	this->m_memoryBaseHandle = MapViewOfFile(this->m_memoryMapped, ((write) ? (FILE_MAP_WRITE) : (FILE_MAP_READ)), 0, 0, 0);

	// check the handle
	if (this->m_memoryBaseHandle == NULL || this->m_memoryBaseHandle == INVALID_HANDLE_VALUE)
		success = false;
#else
    // set properties
    int prot = PROT_READ | ((write) ? (PROT_WRITE) : (0x0));

    // map the files
    this->m_memoryMapped = mmap(0, this->m_filesize, prot, MAP_SHARED, this->m_fileDescriptor, 0);

    // check if mapping failed
    success = (this->m_memoryMapped != MAP_FAILED);

#endif //_WIN32
    // return on failure
    RETURN_FALSE_COND(!success)

    // set flag
    this->m_mapped = true;

    return success;
}

void* MmFile::getMemoryMappedAddress() const {
#ifdef _WIN32
	// windows code

	return static_cast<void *>(this->m_memoryBaseHandle);
#else
	// unix code

    return this->m_memoryMapped;
#endif //_WIN32
}


bool MmFile::syncMemoryMapped() {

    // check if file was mapped before
	RETURN_FALSE_COND(!this->m_mapped)

#ifdef _WIN32
	// windows code

	// save file to disk
	return FlushViewOfFile(this->m_memoryBaseHandle, this->m_filesize) != 0;
#else
	// unix code

    // synchronize memory in ram with file
    return (msync(this->m_memoryMapped, (size_t)this->m_filesize, MS_SYNC) != -1);
#endif //_WIN32
}

bool MmFile::unmapMemoryMapped() {

    // check if the file was mapped before
	RETURN_FALSE_COND(!this->m_mapped)

	bool success = true;

#ifdef _WIN32
	// windows code

	success = UnmapViewOfFile(this->m_memoryBaseHandle);

	//check and close handle
	if (this->m_memoryBaseHandle == INVALID_HANDLE_VALUE)
		CloseHandle(this->m_memoryBaseHandle);

	// check and close handle
	if(this->m_memoryMapped == INVALID_HANDLE_VALUE)
		CloseHandle(this->m_memoryMapped);

	// reset the variables since no file is mapped anymore
	this->m_memoryBaseHandle = INVALID_HANDLE_VALUE;
	this->m_memoryMapped = INVALID_HANDLE_VALUE;
#else
	// unix code

    // unmap the file and save result
    success = (munmap(this->m_memoryMapped, (size_t)this->m_filesize) != -1);

	// reset the variables since no file is mapped anymore
	this->m_memoryMapped = nullptr;
#endif //_WIN32

    this->m_mapped = false;

    return success;
}