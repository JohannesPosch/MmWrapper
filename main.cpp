#include <iostream>
#include <cstring>
#include "MmFile.h"

//testdata
struct data_struct{
    bool b;
    char c;
    int i;
    float f;
    double d;
    char str[40];
};

using namespace std;

static const string testfile = "/tmp/test.dat";
static const unsigned long new_file_size = sizeof(data_struct) * 1;
static const data_struct test_data = {false, 'H', 25, 2390.0001f, 230.00000038, "Dies ist ein Teststring"};

// This function compares two data_struct elements
bool structEquals(data_struct const& elem1, data_struct const& elem2);

int main() {
    cout << "Testdriver for MmFile" << endl;

    MmFile file;

    if (!file.openFile(testfile, file_mode::trunc)){
        cerr << "[ERROR] could not create file: " << testfile << endl;
        return EXIT_FAILURE;
    }

    if (!file.resize(new_file_size)){
        cerr << "[ERROR] could not resize to size (byte): " << new_file_size << endl;
        return EXIT_FAILURE;
    }

    if (!file.map(true)){
        cerr << "[ERROR] could not map file" << endl;
        return EXIT_FAILURE;
    }

    data_struct* ptr = (data_struct*) file.getMemoryMappedAddress();

    if(ptr == nullptr){
        cerr << "[ERROR] did not get valid pointer to file in RAM" << endl;
        return EXIT_FAILURE;
    }

    *ptr = test_data;

    if(!file.closeFile()){
        cerr << "[ERROR] could not close file" << endl;
        return EXIT_FAILURE;
    }

    // reopen the file to test if the data is correctly written to the file

    if(!file.fastMap(testfile)){
        cerr << "[ERROR] could not fastmap file" << endl;
        return EXIT_FAILURE;
    }

    ptr = (data_struct*) file.getMemoryMappedAddress();

    if(ptr == nullptr){
        cerr << "[ERROR] did not get valid pointer to file in RAM" << endl;
        return EXIT_FAILURE;
    }

    if(structEquals(test_data, *ptr))
        cout << "[INFO] Write, Read successful" << endl;
    else
        cout << "[WARNING] Write, Read error" << endl;

    file.closeFile();

    return EXIT_SUCCESS;
}

bool structEquals(data_struct const& elem1, data_struct const& elem2){
    return (elem1.b == elem2.b && elem1.c == elem2.c && elem1.d == elem2.d && elem1.i == elem2.i && elem1.f == elem2.f && strcmp(elem1.str, elem2.str) == 0);
}