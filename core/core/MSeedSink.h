#ifndef MSEEDSINK_H
#define MSEEDSINK_H

#include <iostream>
#include <string>
#include <vector>
#include <iosfwd>                          // streamsize
#include <boost/iostreams/operations.hpp>  // write

using std::string;
using std::vector;
using std::max;
using std::cout;
using std::endl;

namespace io = boost::iostreams;

//#define DEBUG

class SedisMSeedWriter;

class MSeedSink
{
public:
    typedef char          char_type; /*!< char_type */
    typedef io::sink_tag  category;  /*!< ��������� ���������� - Sink */

    // ��� ������ � ����. ����������� �������� ������������� ��������� ����� ��� �������� ������� ��� ������ ������ ���������� �������
    enum EWriteType
    {
        FILE_SINGLE = 0,
        FILE_MULTI = 1
    };

    MSeedSink(const string& location, const string& network, const string& station, const string& outputFilename, EWriteType type, const vector<string>& chNames = vector<string>());
    MSeedSink();
    ~MSeedSink();

    // ��������� ����������
    void configureWriter(const string& location, const string& network, const string& station, const string& outputFilename, EWriteType type, const vector<string>& chNames = vector<string>());

    // ������� ������� ������ ��� mseed � ��� ������
    std::streamsize write(const char* s, std::streamsize n);

    // ������� ������ � ����� ������� � ������������ ������
    void reset();

private:
    // ���������� ������ � ������
    void addData(const char* data, std::streamsize count);
    bool assertHeader(const char* header); // �������� ������. �� ����� ������ ������ 80 ���� (� �����)
    void resetBuf();
    // ���������� ������� sedis ������ �� ������ ������ �� �� ���������
    int recordSize();
    void init();

// private data:
    typedef vector<char>::iterator BufIter;
    vector<char> _recordBuf;
    int _off, _size, _recSize; // �������� � ������ (���������� ���������� ������ � ���), ������ ������ (������.size()), ������ ������� ������ sedis (��� ������ �� ��������� sedis)
    
    static const int headerSize = 80;
    static const int initBufSize = 4096;
    
    SedisMSeedWriter* _writer;
};

#endif // MSEEDSINK_H
