// --------------------------------------------------------------
// Workfile : MmFile.h
// Author : Johannes Posch
// Date : 20.10.2017
// Description : This class is a wrapper for the linux systemcalls
//  to memorymap a file for read / write.
// --------------------------------------------------------------

#ifndef MMFILE_H
#define MMFILE_H
#include <string>

enum file_mode{
    trunc = 0x1,
    normal = 0x2
};


class MmFile {
public:
    MmFile();

    ~MmFile();

    // --------------------------------------------------------------
    // fastMap:
    //  This function opens a file does a resize if the size argument
    //  is greater than 0 and maps the file. The function does a
    //  cleanup on error
    // --------------------------------------------------------------
    bool fastMap(std::string const &filepath, file_mode const mode = normal, bool const write = false,
                 unsigned long const size = 0);

    // --------------------------------------------------------------
    // openFile:
    //  This function opens a file from the given path to map it later
    //  Mode specifies if the file should be truncated on opening
    // --------------------------------------------------------------
    bool openFile(std::string const &filepath, file_mode const mode);

    // --------------------------------------------------------------
    // resize:
    //  This function does a resize on a previously opened file
    //  The file also gets truncated on resize
    // --------------------------------------------------------------
    bool resize(unsigned long const size);

    // --------------------------------------------------------------
    // closeFile:
    //  This function unmaps the file from memory if it was mapped
    //  before and also releases the file resources.
    //  explicitSync triggers a synchronisation with the filesystem
    //  before the file get's closed
    // --------------------------------------------------------------
    bool closeFile(bool const explicitSync = false);

    // --------------------------------------------------------------
    // map:
    //  This function does the mapping of the previously opened file
    //  into memory, this function must get called after opening a
    //  file
    // --------------------------------------------------------------
    bool map(bool const write = false);

    // --------------------------------------------------------------
    // getMemoryMappedAddress:
    //  This function returns the startaddress of the mapped file
    //  nullptr will be returned if the file is not mapped
    // --------------------------------------------------------------
    void* getMemoryMappedAddress() const;

    // --------------------------------------------------------------
    // syncMemoryMapped:
    //  This function does a synchronisation on the filesystem
    // --------------------------------------------------------------
    bool syncMemoryMapped();

private:

    // --------------------------------------------------------------
    // unmapMemoryMapped:
    //  This function unmaps the file from memory
    // --------------------------------------------------------------
    bool unmapMemoryMapped();

    void* m_memoryMapped;
    int m_fileDescriptor;

    __off_t m_filesize;
    bool m_mapped;
    bool m_opened;
};


#endif //MMFILE_H
