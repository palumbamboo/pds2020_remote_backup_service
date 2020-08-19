//
// Created by Daniele Leto on 18/08/2020.
//

#include <iostream>

#include <boost/asio/read_until.hpp>
#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>

#include "Session.h"

Session::Session(tcp::socket socket) : socket{std::move(socket)} {}

void Session::start() {
    //TODO - why this is called two times?
    std::cout << "New session started.. " << std::endl;
    call_read();
}

void Session::call_read() {

    auto self(shared_from_this());
    socket.async_read_some(boost::asio::buffer(buffer,MaxLength),
                           [this, self] (boost::system::error_code ec, std::size_t length)
                           {
                                if(!ec) {
//                                    std::cout.write(buffer.data(), length);
                                    processRead(length);
                                }
                           }
    );
}

void Session::callWrite(std::size_t length) {
    auto self(shared_from_this());
    boost::asio::async_write(socket,boost::asio::buffer(buffer, length),
                        [this, self](boost::system::error_code ec, std::size_t length)
                        {
                            if(!ec) {
                                call_read();
                            }
                        }
    );
}

void Session::processRead(size_t t_bytesTransferred)
{
    BOOST_LOG_TRIVIAL(trace) << __FUNCTION__ << "(" << t_bytesTransferred << ")"
                             << ", in_avail = " << m_requestBuf_.in_avail() << ", size = "
                             << m_requestBuf_.size() << ", max_size = " << m_requestBuf_.max_size() << ".";

    std::istream requestStream(&m_requestBuf_);
    readData(requestStream);

    auto pos = m_fileName.find_last_of('\\');
    if (pos != std::string::npos)
        m_fileName = m_fileName.substr(pos + 1);

    createFile();

    // write extra bytes to file
    do {
        requestStream.read(buffer.data(), buffer.size());
//        BOOST_LOG_TRIVIAL(trace) << __FUNCTION__ << " write " << requestStream.gcount() << " bytes.";
        m_outputFile.write(buffer.data(), requestStream.gcount());
    } while (requestStream.gcount() > 0);

    auto self = shared_from_this();
    socket.async_read_some(boost::asio::buffer(buffer.data(), buffer.size()),
                             [this, self](boost::system::error_code ec, size_t bytes)
                             {
                                 if (!ec)
                                     doReadFileContent(bytes);
                                 else
                                     handleError(__FUNCTION__, ec);
                             });
}


void Session::readData(std::istream &stream)
{
    stream >> m_fileName;
    stream >> m_fileSize;
    stream.read(buffer.data(), 2);

    BOOST_LOG_TRIVIAL(trace) << m_fileName << " size is " << m_fileSize
                             << ", tellg = " << stream.tellg();
}


void Session::createFile()
{
    m_outputFile.open(m_fileName, std::ios_base::binary);
    if (!m_outputFile) {
        BOOST_LOG_TRIVIAL(error) << __LINE__ << ": Failed to create: " << m_fileName;
        return;
    }
}


void Session::doReadFileContent(size_t t_bytesTransferred)
{
    if (t_bytesTransferred > 0) {
        m_outputFile.write(buffer.data(), static_cast<std::streamsize>(t_bytesTransferred));

        BOOST_LOG_TRIVIAL(trace) << __FUNCTION__ << " recv " << m_outputFile.tellp() << " bytes";

        if (m_outputFile.tellp() >= static_cast<std::streamsize>(m_fileSize)) {
            std::cout << "Received file: " << m_fileName << std::endl;
            return;
        }
    }
    auto self = shared_from_this();
    socket.async_read_some(boost::asio::buffer(buffer.data(), buffer.size()),
                             [this, self](boost::system::error_code ec, size_t bytes)
                             {
                                 doReadFileContent(bytes);
                             });
}


void Session::handleError(std::string const& t_functionName, boost::system::error_code const& t_ec)
{
    BOOST_LOG_TRIVIAL(error) << __FUNCTION__ << " in " << t_functionName << " due to "
                             << t_ec << " " << t_ec.message() << std::endl;
}
