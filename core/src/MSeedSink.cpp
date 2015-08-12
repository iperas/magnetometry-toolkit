#include "MSeedSink.h"
#include "ErrorManager.h"
#include "MSeedWriter.h"

#include <algorithm>
#include <cassert>
#include <boost/format.hpp>

using boost::format;

/*!
    \class MSeedSink

    \brief ����� MSeedSink ��������� ������������ ������� ������ ������ �� SEDIS 
    � ����������� ��� � MSeed ������, ��������� SedisMSeedWriter.

    ������ �� SEDIS ��������� � ���� ������������ ������ ������. ������� ���� 80-������
    ��������� � ����� �����������, ����� ������ �� ���� ������. ����� ����� ��� �����������.

    ����� ��������� ������ �� ������, ������������ �������� ������ � �������� �� 
    � SedisMSeedWriter ��� ����������� � MSeed � ������ � ����.

    ��������� ������ SedisMSeedWriter �������������� �������� configureWriter
    ��� � ������������, ����� ���� ����� ����� �� �������� ������.

    �������� ������ �������������:
    \code
    char* data;

    // ...

    MSeedSink sink("SK", "RU", "IF", "myfile");

    sink.write(data, partOneSize);
    sink.write(data + partOneSize, partTwoSize);
    \endcode

    ������ ��������� �������� ������ (������� MSeed ������� � �����, ������� �� ����������, ���) 
    ������� �� �������� ����������� ������-������� SedisMSeedWriter.
*/

/// ����������� � ���������� �������
/*!
    \sa configureWriter
*/
MSeedSink::MSeedSink(const string& location, const string& network, const string& station, const string& outputFilename, EWriteType type, const vector<string>& chNames)
{
    init();
    configureWriter(location, network, station, outputFilename, type);
}

/// ����������� �� ���������
/*!
    ����� ������� ������ ��������� ��������� ������ �������� configureWriter

    \sa configureWriter
*/
MSeedSink::MSeedSink()
{
    init();
}

/// ��������� ����������� �������
/*!
    � ������, ����� ���������� ���������� ��� ��������� � ������ �������� �������, ������ ������� ��������� ���������
    ��� �������.

    \note �� ������� �������� ��������� �������, ���� ����� ���� ������ ��� ���� ������!
*/
void MSeedSink::configureWriter(const string& location, const string& network, const string& station, const string& outputFilename, EWriteType type, const vector<string>& chNames)
{
    _writer->setLocation(location);
    _writer->setNetwork(network);
    _writer->setStation(station);
    _writer->setFileName(outputFilename, (SedisMSeedWriter::EWriteType)type);
    if (!chNames.empty())
    {
        _writer->setChannelNames(chNames);
    } else {
        cout << "Warning: channel names is not set. Using standard names." << endl;
        vector<string> standardNames(6);
        standardNames[0] = "BHZ";
        standardNames[1] = "BHE";
        standardNames[2] = "BHS";
        standardNames[3] = "BHN";
        standardNames[4] = "BHW";
        standardNames[5] = "BLZ";
        _writer->setChannelNames(standardNames);
    }
}

void MSeedSink::init()
{
    _writer = new SedisMSeedWriter;
    _recordBuf.resize(_size = initBufSize);
    resetBuf();
}

MSeedSink::~MSeedSink()
{
    delete _writer;
}

void MSeedSink::addData(const char* data, std::streamsize count)
{
    if (_off + count > _size)
    {
        _recordBuf.resize(max(_off + count, _size * 2));
    }

    memcpy(&_recordBuf[_off], data, count);
    _off += count;
    _size = _recordBuf.size();
}

void MSeedSink::resetBuf()
{
    _off = 0;
    _recSize = -1;
}

int MSeedSink::recordSize()
{
    if (_recSize != -1)
        return _recSize;

    TSedisHeader* d_header = const_cast<TSedisHeader*>(reinterpret_cast<const TSedisHeader*>(&_recordBuf[0]));
    SedisHeader header(d_header);
    int dataSize = header.blockSamples() * header.sampleBytes();
    return _recSize = headerSize + dataSize;
}

/// ������ ������ � ����� �� ��������� � MSeed
/*!
    ������� ����������� �������� ������. ��� ���������� ����� SEDIS ������, 
    ������ ���������� �� ��������� � SedisMSeedWriter.
*/
std::streamsize MSeedSink::write(const char* s, std::streamsize n)
{
#ifdef DEBUG
    cout << "MSeedSink::write() enter: n = " << n << endl;
#endif
    // Return the number of characters consumed.
    int ret = n;

    while (n > 0)
    {
        if (_off + n < headerSize)
        {
            // ������������ ������ ��� ������ ������
            addData(s,n);
            break;
        } else {
            if (_off < headerSize)
            {
                // ���������� ������ ����� �� �������� ��������� sedis
                int headerTailSize = headerSize - _off;
                addData(s,headerTailSize);
                s += headerTailSize;
                n -= headerTailSize;
                // � ������ �����, �������� ��� ��� ���������� ����� MSeed - �� ������ 12 ������
                //vector<char> headerMark(12);
                //strncpy(&headerMark[0], "SeismicData\0", 12);
                // TODO: ��������� �������� ����� ������ � �����
                //_recordBuf.resize(29);
                //memcpy(&_recordBuf[0], "zz123456zSeismicData", 20);
                //_off = 20;
                char* headerMark = "SeismicData\0";
                BufIter begin = _recordBuf.begin();
                BufIter end = _recordBuf.begin() + _off;
                BufIter headerIt = std::search(begin, end, headerMark, headerMark+12);
                if (headerIt != begin) // ���������� ������ �� �������� �������, ���� ������ ������
                {
                    int skipped;
                    if (headerIt != end) // ������ ������ �������, �������� ������ � ������
                    {
                        skipped = _off - std::distance(headerIt, end);
                        string warning(str(format("Garbage in data stream, skipping %1% bytes!") % skipped));
                        sErrorMgr.addError(warning);
                        cout << warning << endl;

                        _recSize = -1;
                        _off = std::distance(headerIt, end);
                        std::copy(headerIt, end, begin);
                        if (_off < headerSize)
                        {
                            continue;
                        }
                    } else { // ������ ������ �� �������, ��� �����, ����� ��������� 11 ���� - ��� ����� ���� ������ ������ (�������� zzSeismicDa)
                        skipped = _off - 11;
                        string warning(str(format("Garbage in data stream, skipping %1% bytes!") % skipped));
                        sErrorMgr.addError(warning);
                        cout << warning << endl;

                        _recSize = -1;
                        BufIter newBegin = end - 11;
                        _off = 11;
                        std::copy(newBegin, end, begin);
                        continue;
                    }
                }
            }
            int recSize = recordSize();
            if (_off + n < recSize)
            {
                // ������������ ������ ��� ������ ������
                addData(s,n);
                break;
            } else {
                // ������ ����������.
                // �������� � ����� ������� ������
                int recTailSize = recSize - _off;
                addData(s,recTailSize);
                s += recTailSize;
                n -= recTailSize;
                // ...����������� � ������...
                _writer->addRecord(_recordBuf);
                resetBuf();
                // �������� ���������� � ��������� ��������
            }
        }
    }
    if (_off < headerSize)
    {
        cout << (format("MSeed Parser: received %1% bytes, buffered %2% bytes") % ret % _off) << endl;
    } else {
        cout << (format("MSeed Parser: received %1% bytes, progress %2%/%3%") % ret % _off % recordSize()) << endl;
    }
#ifdef DEBUG
    if (_off >= headerSize)
    {
        TSedisHeader* d_header = const_cast<TSedisHeader*>(reinterpret_cast<const TSedisHeader*>(&_recordBuf[0]));
        SedisHeader header(d_header);

        cout << header;
    }
    cout << endl;
#endif
    return ret;
}

bool assertHeader(const char* header)
{
     TSedisHeader* d_header = const_cast<TSedisHeader*>(reinterpret_cast<const TSedisHeader*>(header));
     return false;
}

/// ����� ������ ������ � ������� SedisMSeedWriter
/*!
    ��� ������, ������� �� ���� �������� �������, ����� ����������!

    \sa SedisMSeedWriter::reset
*/
void MSeedSink::reset()
{
    resetBuf();
    _writer->reset();
}
