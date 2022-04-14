#include <sys/mman.h>
#include "fat_internal.h"
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <algorithm>

using namespace std;



struct clusterInfo
{
    uint32_t size;
    uint32_t clusterNum;
    int empty = -1;
};


uint64_t findSecOneOfClus(uint64_t N);
uint64_t readFatTable(uint64_t N) ;
bool isEOF(uint64_t FATContent);
std::vector<AnyDirEntry> getDirEntriesFromByteStart(uint64_t clusStart);
std::vector<AnyDirEntry> findRoot();
uint64_t getClusterFromPath(const std::string &path);
vector<string> getName(const std::string &path);
std::vector<AnyDirEntry> findDirEntriesOfClusN(uint32_t N);
int checkIfNameIsInDirEntries(string name, vector<AnyDirEntry> entriesInDir);
bool isDir(AnyDirEntry dirEntry);
uint32_t readClusEntry(AnyDirEntry dirEntry);
string toupper(string s);
clusterInfo getClusterInfoFromPath(const std::string &path);
uint32_t readSize(AnyDirEntry dirEntry);
void readCluster(uint8_t* buffer, uint64_t clusStart);


Fat32BPB *bpb = nullptr;

clusterInfo emptyCluster;


struct FATDiskImage
{
    int file;
    uint64_t FirstDataSector;
    uint64_t DataSec;
    vector<int> availableFD;
    clusterInfo fileDescriptorTable[128];
};

FATDiskImage image;

bool fat_mount(const std::string &path)
{
    const char *path_c = path.c_str();

    int file = open(path_c, O_RDWR);
    if (file == -1)
    {
        return false;
    }

    image.file = file;
    // read the BPB into the cluster
    void *data = mmap(NULL, 90, PROT_READ, MAP_PRIVATE, file, 0);
    if (data == MAP_FAILED)
    {
        return false;
    }
    bpb = (struct Fat32BPB *)data;

    uint16_t BPB_RootEntCnt = bpb->BPB_rootEntCnt;
    uint16_t BPB_BytsPerSec = bpb->BPB_BytsPerSec;
    // printf("%x \n", BPB_BytsPerSec);
    uint16_t BPB_ResvdSecCnt = bpb->BPB_RsvdSecCnt;
    uint8_t BPB_NumFATs = bpb->BPB_NumFATs;
    uint32_t FATSz = bpb->BPB_FATSz32; // should be correct, but can check for 16 != 0
    //uint32_t BPB_RootClus = bpb->BPB_RootClus;

    uint64_t RootDirSectors = ((BPB_RootEntCnt * 32) + (BPB_BytsPerSec - 1)) / BPB_BytsPerSec; // should be 0, always rounds up
    //printf("%lx \n", RootDirSectors);
    uint64_t FirstDataSector = BPB_ResvdSecCnt + (BPB_NumFATs * FATSz) + RootDirSectors; //
    //printf("%lx \n", FirstDataSector);
    image.FirstDataSector = FirstDataSector; 


    uint32_t TotSec = bpb->BPB_TotSec32; 
    uint64_t DataSec = TotSec - (BPB_ResvdSecCnt + (BPB_NumFATs * FATSz) + RootDirSectors);
    image.DataSec = DataSec;
    uint8_t BPB_SecPerClus = bpb->BPB_SecPerClus;

    uint64_t CountofClusters = DataSec / BPB_SecPerClus;
    if (CountofClusters < 65525)
    {
        return false;
    }

    for(int i = 0; i < 128; i++)
    {
        image.availableFD.push_back(i);
        image.fileDescriptorTable[i] = emptyCluster;
    }
        
    return true;
}

int fat_open(const std::string &path)
{
    if(bpb == nullptr)
    {
        return -1;
    }
    // check if file already open
    if ( getName(path).empty() )
    {
        return -1;
    }


    //path, get directory from path

    //set up table
    // get cluster of path
    //uint64_t clusNum = getClusterFromPath(path);
    clusterInfo clusInfo = getClusterInfoFromPath(path);
    if (clusInfo.empty == -1) 
    {
        return -1;
    }



    // vector<AnyDirEntry> result = fat_readdir(path);
    
    // if(result.empty())
    // {
        
    //     return -1;
    // }

    // get file descriptor
    if (image.availableFD.size() > 128)
    {
        printf("More than 128 fds available \n");
        return -1;
    }
    int fd = image.availableFD.back();
    image.availableFD.pop_back();

    image.fileDescriptorTable[fd] = clusInfo;
    //uint64_t clusNum = clusInfo.clusterNum;
    //cout << clusNum << endl;
    return fd;
}

bool fat_close(int fd)
{
    if(bpb == nullptr)
    {
        return false;
    }
    if(fd > 127 || fd < 0)
    {
        printf("Invalid File descriptor");
        return false;
    }
    if(image.fileDescriptorTable[fd].empty == -1)
    {
        printf("File closed \n");
        return false;
    }
    if (image.availableFD.size() > 127)
    {
        printf("More than 127 fds available");
        return false;
    }
    image.fileDescriptorTable[fd] = emptyCluster;
    image.availableFD.push_back(fd);
    return true;
}

//readCluster

int fat_pread(int fd, void *buffer, int count, int offset)
{
    clusterInfo clusInfo = image.fileDescriptorTable[fd];
    uint64_t N = clusInfo.clusterNum;
    int size = (int) clusInfo.size;
    int empty = clusInfo.empty;

    uint8_t* buffer_8 = (uint8_t*) buffer;
    if(bpb == nullptr)
    {
        return -1;
    }
    if(offset < 0 || count < 0)
    {
        printf("negative offset");
        return -1;
    }
    if(offset > size)
    {
        //printf("offset greater than size");
        return 0;
    }
    if (empty == -1)
    {
        printf("fd not open");
    }
    if( std::count(image.availableFD.begin(), image.availableFD.end(), fd) != 0 )
    {
        printf("fd in avaiableFDs");
        return -1;
    }



    int bytesRead = 0;

    //int sizeOfClus = bpb->BPB_BytsPerSec * bpb->BPB_SecPerClus;
    uint8_t tempBuffer[2048];
    int bufferIndex = 0;

    int readEnd = count+offset; 
    if(count == 0)
    {
        return 0;
    }

    if(readEnd > size)
    {
        readEnd = size;
    }
    

    while(1) 
    {
        if (readEnd < bytesRead)
        {
            break;
        }
        uint64_t FirstSectorOfCluster = findSecOneOfClus(N);
        
        uint64_t secStart =  FirstSectorOfCluster*512;

        //printf("%lx \n",clusStart);
        readCluster(tempBuffer,secStart);
        bytesRead += 2048;

        if (offset <= bytesRead && bytesRead-2048 <= readEnd)
        {        
            int start = 0;
            int end = 2048;
            if(bytesRead - 2048 <= offset && offset <= bytesRead) // edge case at offset = 2048, 2047 offset = 0,1
            {
               //read from offset-(bytesRead - 2048) to bytestRead - (bytesRead - 2048)

                start = offset-(bytesRead - 2048);
                // for (int i = offset-(bytesRead - 2048); i < 2048; i++)
                // {
                //     buffer_8[bufferIndex] = tempBuffer[i];
                //     bufferIndex++;
                // }
            }
            if ( bytesRead-2048 <= readEnd && readEnd <= bytesRead ) // edge case at readEnd = 2048 no, 2047 readEnd = 0, 1
            {

                end = readEnd-(bytesRead - 2048);
                //read from offset-(bytesRead - 2048) to bytestRead - (bytesRead - 2048)
                // for (int i = 0; i < 2048; i++)
                // {
                //     buffer_8[bufferIndex] = tempBuffer[i];
                //     bufferIndex++;
                // }
            }
            for (int i = start; i < end; i++)
            {
                buffer_8[bufferIndex] = tempBuffer[i];
                bufferIndex++;
            }
        }

        N = readFatTable(N);
        //printf("%x \n", N);
        
    }
    return readEnd-offset;

}

vector<AnyDirEntry> fat_readdir(const std::string &path)
{
    vector<AnyDirEntry> emp;
    if(bpb == nullptr)
    {
        return emp;
    }
    vector<AnyDirEntry> curDirEntries;
    vector<string> entryNames = getName(path);
    curDirEntries = findRoot();

    if (path == "/")
    {
        return curDirEntries;
    }
    //cout << "Size: " << entryNames.size() << endl;
    if(entryNames.size() == 0)
    {
        return emp;
    } 

    // note, .. to root does not work

    for(size_t  i = 0; i < entryNames.size(); i++) // 10 number of sub directories
    {
        string name = entryNames.at(i);
        int index = checkIfNameIsInDirEntries(name, curDirEntries);
        if ( index < 0)
        {
            return emp;
        }
        AnyDirEntry dirEntry = curDirEntries.at(index);
        if (isDir(dirEntry))
        {
            uint32_t clusNum = readClusEntry(dirEntry);
            //cout << clusNum << endl;
            
            curDirEntries = findDirEntriesOfClusN(clusNum);
            

        }
        else
        {

            return emp;
        }

        
    }
    return curDirEntries;
}

void readCluster(uint8_t* buffer, uint64_t clusStart)
{
    vector<AnyDirEntry> vecOfRootDirEntries;
    size_t num_bytes_read = pread(image.file, buffer, 2048, clusStart);
    if (num_bytes_read != 2048)
    {
        printf("pread fail in readCluster");
    }
} 


int checkIfNameIsInDirEntries(string name,vector<AnyDirEntry> entriesInDir)
{
    // check if dir entry name is 00 or e5
    for(size_t  i = 0; i < entriesInDir.size(); i++) // 10 number of sub directories
    {
        AnyDirEntry entry = entriesInDir.at(i);
        uint8_t fChar = entry.dir.DIR_Name[0];
        if (fChar == 0x00 || fChar == 0xE5)
        {
            continue;
        }

        string file_name = string(entry.dir.DIR_Name, entry.dir.DIR_Name + 8);
        while (file_name.back() == ' ') 
        {
            file_name.pop_back();
        }
        if (entry.dir.DIR_Name[8] != ' ') 
        {
            file_name += '.';
            file_name += std::string(entry.dir.DIR_Name + 8, entry.dir.DIR_Name + 11);
            while (file_name.back() == ' ') 
            {
                file_name.pop_back();
            }
        }
        string file_name_up = toupper(file_name);
        string name_up = toupper(name);

        if ( name_up == file_name_up)
        {
            return i;
        }
    }
    return -1;
}

string toupper(string s)
{
    const char* str_c = s.c_str();
    string new_s = ""; 

    // Loop
    for(size_t i = 0; i < s.length(); i++) // 10 number of sub directories
    {
        new_s.push_back(  toupper(str_c[i])  );
    }
    return new_s;
}


clusterInfo getClusterInfoFromPath(const std::string &path)
{
    vector<AnyDirEntry> curDirEntries;
    curDirEntries = findRoot();
    vector<string> entryNames = getName(path);
    clusterInfo clusInfo;
    AnyDirEntry dirEntry;

    for(size_t  i = 0; i < entryNames.size(); i++) // 10 number of sub directories
    {
        string name = entryNames.at(i);
        int index = checkIfNameIsInDirEntries(name, curDirEntries);
        if ( index < 0)
        {
            return emptyCluster;
        }
        dirEntry = curDirEntries.at(index);
        if (isDir(dirEntry))
        {
            uint32_t clusNum_dir = readClusEntry(dirEntry);
            curDirEntries = findDirEntriesOfClusN(clusNum_dir);
        }
        else
        {
            clusInfo.clusterNum = readClusEntry(dirEntry);
            clusInfo.size = readSize(dirEntry);
            clusInfo.empty = 1;
            return clusInfo;
        }
    }
    return emptyCluster; 
}

uint64_t getClusterFromPath(const std::string &path)
{
    vector<AnyDirEntry> curDirEntries;
    curDirEntries = findRoot();
    vector<string> entryNames = getName(path);
    uint32_t clusNum = 0;

    for(size_t  i = 0; i < entryNames.size(); i++) // 10 number of sub directories
    {
        string name = entryNames.at(i);
        int index = checkIfNameIsInDirEntries(name, curDirEntries);
        if ( index < 0)
        {
            return -1;
        }
        AnyDirEntry dirEntry = curDirEntries.at(index);
        if (isDir(dirEntry))
        {
            uint32_t clusNum_dir = readClusEntry(dirEntry);
            curDirEntries = findDirEntriesOfClusN(clusNum_dir);
        }
        else
        {
            clusNum = readClusEntry(dirEntry);
        }
    }
    return clusNum; 
}

uint32_t readClusEntry(AnyDirEntry dirEntry)
{
    uint32_t first_cluster = dirEntry.dir.DIR_FstClusLO | (dirEntry.dir.DIR_FstClusHI << 16);
    return first_cluster;
}

uint32_t readSize(AnyDirEntry dirEntry)
{
    if(isDir(dirEntry))
    {
        return 0;
    }
    uint32_t size = dirEntry.dir.DIR_FileSize;
    return size;
}


bool isDir(AnyDirEntry dirEntry)
{
    return (dirEntry.dir.DIR_Attr & DIRECTORY) != 0;
}

//vector<AnyDirEntry> getNextDir(string h, vector<AnyDirEntry>)
//{
//    return NULL;
//}



vector<string> getName(const std::string &path)
{
    vector<string> ret;
    string temp = "";
    if(path[0] != '/' )
    {
        return ret;
    }
    for(size_t i = 1; i < path.length() ; i++) // 10 number of sub directories
    {
        if(path[i] == '/' )
        {
            if (temp.length() == 0)
            {
                printf("Malformed directory");
                ret.clear();
                return ret;
            }
            ret.push_back(temp);
            temp = "";
        }
        else
        {
            temp.push_back(path[i]);
        }
    }
    ret.push_back(temp);
    return ret;
}


std::vector<AnyDirEntry> findRoot()
{                                          
    uint32_t BPB_RootClus = bpb->BPB_RootClus;
    vector<AnyDirEntry> vecOfRootDirEntries;
    uint32_t N = BPB_RootClus;
    while(1) 
    {
        if (isEOF(N))
        {
            break;
        }
        uint64_t FirstSectorOfCluster = findSecOneOfClus(N);
        
        uint64_t clusStart =  FirstSectorOfCluster*512;
        //printf("%lx \n",clusStart);

        vector<AnyDirEntry> tempClusDirEntries = getDirEntriesFromByteStart(clusStart); // get dir entries from known byte start
        vecOfRootDirEntries.insert(vecOfRootDirEntries.end(), tempClusDirEntries.begin(), tempClusDirEntries.end()); // insert into total dir entries
        N = readFatTable(N);

        //printf("%x \n", N);
        
    }
    return vecOfRootDirEntries;
}

std::vector<AnyDirEntry> findDirEntriesOfClusN(uint32_t N)
{                                          
    vector<AnyDirEntry> vecOfRootDirEntries;
    if(N == 0)
    {
        return findRoot();
    }
    while(1) 
    {
        if (isEOF(N))
        {
            break;
        }
        uint64_t FirstSectorOfCluster = findSecOneOfClus(N);
        
        uint64_t secStart =  FirstSectorOfCluster*512;
        //printf("%lx \n",clusStart);

        vector<AnyDirEntry> tempClusDirEntries = getDirEntriesFromByteStart(secStart); // get dir entries from known byte start
        vecOfRootDirEntries.insert(vecOfRootDirEntries.end(), tempClusDirEntries.begin(), tempClusDirEntries.end()); // insert into total dir entries
        N = readFatTable(N);

        //printf("%x \n", N);
        
    }
    return vecOfRootDirEntries;
}


bool isEOF(uint64_t FATContent)
{
    bool IsEOF = false;
    if(FATContent >= 0x0FFFFFF8)
    {
        IsEOF = true;
    }
    return IsEOF;
}

std::vector<AnyDirEntry> getDirEntriesFromByteStart(uint64_t clusStart)
{
    vector<AnyDirEntry> vecOfRootDirEntries;
    for(uint64_t i = clusStart; i < clusStart + bpb->BPB_BytsPerSec * bpb->BPB_SecPerClus; i+=32) // read only a cluster
    {
        uint8_t data[32];
        size_t num_bytes_read = pread(image.file, data, 32, i);
        if (num_bytes_read != 32)
        {
            printf("pread fail");
            break;
        }

        AnyDirEntry *rootDir = (union AnyDirEntry *) data;
        uint8_t *DIR_Name = rootDir->dir.DIR_Name; // check if  it is end (00) or deleted (E5)
        if (DIR_Name[0] == 0x00)
        {
            break;
        }
        vecOfRootDirEntries.push_back(*rootDir);
        //lseek(image.file, 32, SEEK_CUR);
    }

    return vecOfRootDirEntries;
}



uint64_t findSecOneOfClus(uint64_t N) // valid cluster number
{
    return ((N - 2) * bpb->BPB_SecPerClus + image.FirstDataSector);
}

// sampledisk32.raw

uint64_t readFatTable(uint64_t N)  // valid cluster number
{
    uint64_t FATOffset = N * 4;
    uint64_t ThisFATSecNum = bpb->BPB_RsvdSecCnt + (FATOffset / bpb->BPB_BytsPerSec);
    uint64_t ThisFATEntOffset = (FATOffset % bpb->BPB_BytsPerSec);

    uint64_t startSec = ThisFATSecNum*512;

    uint32_t data[1];
    int num_bytes_read = pread(image.file, data, 4, startSec+ThisFATEntOffset); //read(image.file, data, 32);
    uint32_t FAT32ClusEntryVal =  *data & 0x0FFFFFFF;
    if (num_bytes_read != 4)
    {
        //printf("read fat table fail: %lx, %d \n", N, num_bytes_read);
    }
    return FAT32ClusEntryVal;
}

//mount sampledisk32.raw
//mount testdisk1.raw