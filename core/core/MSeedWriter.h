#ifndef MSEEDWRITER_H
#define MSEEDWRITER_H

#include <ctime>
#include <fstream>
#include <string>
#include <vector>
#include <cassert>
#include <libmseed.h>
#include <boost/format.hpp>

#include "Sedis.h"

using std::ofstream;
using std::string;
using std::vector;
using std::ios;
using boost::format;

static BTime tm_to_BTime(tm ctime)
{
    BTime btime;
    btime.year = 1900 + ctime.tm_year;
    btime.day = ctime.tm_yday + 1;
    btime.hour = ctime.tm_hour;
    btime.min = ctime.tm_min;
    btime.sec = ctime.tm_sec;
    btime.fract = 0;
    return btime;
}

typedef int int32;

// ������� ��� ������ mseed Record � ����
static void recorder(char* record, int reclen, void* pvOutStream)
{
    ofstream* outstream = reinterpret_cast<ofstream*>(pvOutStream);
    outstream->write(record, reclen);
}

// ��� ����������� ����������� ������� ����� ������ ������� �� ����� SEDIS
//#define WRITE_CURRENTTIME

class SedisMSeedWriter
{
public:
    SedisMSeedWriter();
    ~SedisMSeedWriter();

    enum EWriteType
    {
        FILE_SINGLE = 0,
        FILE_MULTI = 1
    };
    // ������ ����� ��� ������������� mseed ����� (��� ���������� mseed � ������� '-%d')
    inline void setFileName(const string& filename, EWriteType type) { _filename = filename; _writeType = type; }
    string curFileName() const;
    // TODO: ������ �� ������������. ����� ������ = ����� ������� addRecord(...)
    /// � ������, ��������� ������ ����� ���������, ��� ��� ������ MSeed ������ ��������� � ������ �����.
    void setSplitSize(int size);
    /// �������� ����. 2 �������.
    inline void setNetwork(const string& network) { _network = network; }
    /// �������� �������. 5 ��������.
    inline void setStation(const string& station) { _station = station; }
    /// �������� �������. 2 �������.
    inline void setLocation(const string& location) { _location = location; }
    /// �������� ������� - ������ �� 6 �����
    inline void setChannelNames(const vector<string>& names) { assert(names.size() == 6); _chNames = names; }
    /// ����� ������������ ������� mseed
    inline void setReclen(int len) { _recLen = len; }
    // �������� ������
    inline void setEncoding(int encoding) { _encoding = encoding; }
    // ���������� ������ SEDIS � ����
    void addRecord(const vector<char>& data);
    void reset();
private:
    string _network;
    string _station;
    string _location;
    string _filename;
    EWriteType _writeType;
    vector<string> _chNames; // �������� �������

    int _recLen;
    int _encoding;

    ofstream* _curFile;
    int _curFileIndex;
    
    void openFile();
    void closeFile();

    //MSRecord *msr;
};

#endif // MSEEDWRITER_H
