//======================================================================================================================
// Project: CppUtils
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: abstraction over low-level system socket calls
//======================================================================================================================

#ifndef CPPUTILS_SOCKET_INCLUDED
#define CPPUTILS_SOCKET_INCLUDED


#include "SystemErrorInfo.hpp"
#include "NetAddress.hpp"
#include "Span.hpp"

#include <chrono>  // timeout
#include <vector>  // send, recv
#include <array>
#include <cstring> // strlen

struct sockaddr;


namespace own {


//======================================================================================================================
//  types shared between multiple socket classes

/** our own unified error codes, because error codes from system calls vary from OS to OS */
enum class SocketError
{
	Success = 0,                ///< The operation was successful.
	// our own error states that don't have anything to do with the system sockets
	AlreadyConnected = 1,       ///< Connect operation failed because the socket is already connected. Call disconnect() first.
	NotConnected = 2,           ///< Operation failed because the socket is not connected. Call connect(...) first.
	// errors related to connect attempt
	NetworkingInitFailed = 10,  ///< Operation failed because underlying networking system could not be initialized. Call getLastSystemError() for more info.
	HostNotResolved = 11,       ///< The hostname you entered could not be resolved to IP address. Call getLastSystemError() for more info.
	ConnectFailed = 12,         ///< Could not connect to the target server, either it's down or the port is closed. Call getLastSystemError() for more info.
	// errors related to send operation
	SendFailed = 20,            ///< Send operation failed. Call getLastSystemError() for more info.
	// errors related to receive operation
	ConnectionClosed = 30,      ///< Server has closed the connection.
	Timeout = 31,               ///< Operation timed-out.
	WouldBlock = 32,            ///< Socket is set to non-blocking mode and there is no data in the system input buffer.
	// errors related to opening a server
	AlreadyOpen = 40,           ///< Opening server failed because the socket is already listening. Call close() first.
	BindFailed = 41,            ///< TODO
	ListenFailed = 42,          ///< TODO

	Other = 255                 ///< Other system error. Call getLastSystemError() for more info.
};

#ifdef _WIN32
	using socket_t = uintptr_t;  // should be SOCKET but let's not include the whole big winsock2.h just because of this
#else
	using socket_t = int;
#endif


//======================================================================================================================
///  private implementation details

namespace impl {

/** common for all socket classes */
class SocketCommon
{

 public:

	SocketError close();
	bool isOpen() const;

	system_error_t getLastSystemError() const   { return _lastSystemError; }
	bool setBlockingMode( bool enable )         { _isBlocking = enable; return _setBlockingMode( _socket, enable ); }  // TODO
	bool isBlocking() const                     { return _isBlocking; }

 protected:

	SocketCommon();
	SocketCommon( socket_t sock );
	~SocketCommon();

	SocketCommon( const SocketCommon & other ) = delete;
	SocketCommon( SocketCommon && other );
	SocketCommon & operator=( SocketCommon && other );

	static bool _shutdownSocket( socket_t sock );
	static bool _closeSocket( socket_t sock );
	static bool _setTimeout( socket_t sock, std::chrono::milliseconds timeout );
	static bool _isTimeout( system_error_t errorCode );
	static bool _isWouldBlock( system_error_t errorCode );
	static bool _setBlockingMode( socket_t sock, bool enable );

 protected:

	socket_t _socket;
	system_error_t _lastSystemError;
	bool _isBlocking;

};

} // namespace impl


//======================================================================================================================
/** abstraction over low-level TCP socket system calls */

class TcpClientSocket : public impl::SocketCommon
{

 public:

	TcpClientSocket();
	~TcpClientSocket();

	TcpClientSocket( const TcpClientSocket & other ) = delete;
	TcpClientSocket( TcpClientSocket && other );
	TcpClientSocket & operator=( TcpClientSocket && other );

	SocketError connect( const std::string & host, uint16_t port );
	SocketError connect( const IPAddr & addr, uint16_t port );
	SocketError disconnect();
	bool isConnected() const;

	/** This needs to be checked after TcpServerSocket::accept() */
	bool isValid() const;

	bool setTimeout( std::chrono::milliseconds timeout );

	/** Sends given number of bytes to the socket. If the system does not accept that amount of data all at once,
	  * it repeats the system calls until all requested data are sent. */
	SocketError send( span< const uint8_t > buffer );

	/** TODO */
	SocketError send( const std::string & strMessage )
	{
		return send( span< const char >( strMessage ).cast< const uint8_t >() );
	}
	SocketError send( const char * strMessage )
	{
		return send( std::string( strMessage ) );
	}

	/** Receive the given number of bytes from the socket. If the requested amount of data don't arrive all at once,
	  * it repeats the system calls until all requested data are received.
	  * The number of bytes actually received is stored in an output parameter.
	  * @param[out] received - how many bytes were really received */
	SocketError receive( span< uint8_t > buffer, size_t & received );

	/** Receive the given number of bytes from the socket. If the requested amount of data don't arrive all at once,
	  * it repeats the system calls until all requested data are received.
	  * After the call, the size of the vector will be equal to the number of bytes actually received.
	  * @param[in] size - how many bytes to receive */
	SocketError receive( std::vector< uint8_t > & buffer, size_t size )
	{
		buffer.resize( size );  // allocate the needed storage
		size_t received;
		SocketError result = receive( span< uint8_t >( buffer ), received );
		buffer.resize( received );  // let's return the user a vector only as big as how much we actually received
		return result;
	}

 protected:

	 // allow creating socket object from already initialized socket handle, but only for TcpServerSocket
	 friend class TcpServerSocket;
	 TcpClientSocket( socket_t sock ) : SocketCommon( sock ) {}

	 SocketError _connect( int family, int addrlen, struct sockaddr * addr );

};


//======================================================================================================================
/** abstraction over low-level TCP socket system calls */

class TcpServerSocket : public impl::SocketCommon
{

 public:

	TcpServerSocket();
	~TcpServerSocket();

	TcpServerSocket( const TcpServerSocket & other ) = delete;
	TcpServerSocket( TcpServerSocket && other );
	TcpServerSocket & operator=( TcpServerSocket && other );

	/** Opens a TCP server on selected port. */
	SocketError open( uint16_t port );

	TcpClientSocket accept();

};


//======================================================================================================================
/** abstraction over low-level UDP socket system calls */

class UdpSocket : public impl::SocketCommon
{

 public:

	UdpSocket();
	~UdpSocket();

	UdpSocket( const UdpSocket & other ) = delete;
	UdpSocket( UdpSocket && other );
	UdpSocket & operator=( UdpSocket && other );

	/** Opens an UDP socket on selected port. */
	SocketError open( uint16_t port );

	/** TODO */
	SocketError sendTo( const IPAddr & addr, uint16_t port, span< const uint8_t > buffer );

	SocketError sendTo( const IPAddr & addr, uint16_t port, const std::string & strMessage )
	{
		return sendTo( addr, port, span< const char >( strMessage ).cast< const uint8_t >() );
	}
	SocketError sendTo( const IPAddr & addr, uint16_t port, const char * strMessage )
	{
		return sendTo( addr, port, std::string( strMessage ) );
	}

	/** TODO */
	SocketError recvFrom( IPAddr & addr, uint16_t & port, span< uint8_t > buffer, size_t & received );

};


//======================================================================================================================


} // namespace own


#endif // CPPUTILS_SOCKET_INCLUDED
