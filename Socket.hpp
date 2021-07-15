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
#include <vector>  // recv
#include <cstring> // strlen

struct sockaddr;


namespace own {


//======================================================================================================================
//  types shared between multiple socket classes

/// our own unified error codes, because error codes from system calls vary from OS to OS
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
	BindFailed = 41,            ///< Failed to bind the socket to a specified network address and port. Call getLastSystemError() for more info.
	ListenFailed = 42,          ///< Failed to switch the socket to a listening state. Call getLastSystemError() for more info.

	Other = 255                 ///< Other system error. Call getLastSystemError() for more info.
};
const char * enumString( SocketError error ) noexcept;

#ifdef _WIN32
	using socket_t = uintptr_t;  // should be SOCKET but let's not include the whole big winsock2.h just because of this
#else
	using socket_t = int;
#endif


//======================================================================================================================
/// private implementation details

namespace impl {

/// common for all socket classes
class SocketCommon
{

 public:

	SocketError close() noexcept;
	bool isOpen() const noexcept;

	bool setBlockingMode( bool enable ) noexcept;
	bool isBlocking() const noexcept  { return _isBlocking; }

	system_error_t getLastSystemError() const noexcept  { return _lastSystemError; }

 protected:

	SocketCommon() noexcept;
	SocketCommon( socket_t sock ) noexcept;
	~SocketCommon() noexcept;

	SocketCommon( const SocketCommon & other ) = delete;
	SocketCommon( SocketCommon && other ) noexcept;
	SocketCommon & operator=( const SocketCommon & other ) = delete;
	SocketCommon & operator=( SocketCommon && other ) noexcept;

	static bool _shutdownSocket( socket_t sock ) noexcept;
	static bool _closeSocket( socket_t sock ) noexcept;
	static bool _setTimeout( socket_t sock, std::chrono::milliseconds timeout ) noexcept;
	static bool _isTimeout( system_error_t errorCode ) noexcept;
	static bool _isWouldBlock( system_error_t errorCode ) noexcept;
	static bool _setBlockingMode( socket_t sock, bool enable ) noexcept;

 protected:

	socket_t _socket;
	system_error_t _lastSystemError;
	bool _isBlocking;

};

} // namespace impl


//======================================================================================================================
/// Abstraction over low-level TCP socket system calls.
/** This class is used either by a client connecting to a server
  * or by a server after it accepts a connection from a client. */

class TcpSocket : public impl::SocketCommon
{

 public:

	TcpSocket() noexcept;
	~TcpSocket() noexcept;

	TcpSocket( const TcpSocket & other ) = delete;
	TcpSocket( TcpSocket && other ) noexcept;
	TcpSocket & operator=( const TcpSocket & other ) = delete;
	TcpSocket & operator=( TcpSocket && other ) noexcept;

	/// Connects to a specified endpoint determined by host name and port.
	/** First the host name is resolved to an IP address and then a connection to that address is established. */
	SocketError connect( const std::string & host, uint16_t port ) noexcept;

	/// Connects to a specified endpoint determined by IP address and port.
	SocketError connect( const IPAddr & addr, uint16_t port ) noexcept;

	/// Disconnects from the currently connected server.
	SocketError disconnect() noexcept;

	bool isConnected() const noexcept;

	/// This needs to be checked after TcpServerSocket::accept()
	bool isValid() const noexcept;

	/// Sets the timeout for further receive operations.
	bool setTimeout( std::chrono::milliseconds timeout ) noexcept;

	/// Sends given number of bytes to the socket.
	/** If the system does not accept that amount of data all at once,
	  * it repeats the system calls until all requested data are sent. */
	SocketError send( const_byte_span buffer ) noexcept;

	/// Convenience wrapper of send( const_byte_span ) for sending textual data.
	/** \param[in] message null-terminated buffer of chars */
	SocketError send( const char * message )
	{
		return send( const_char_span( message, strlen(message) ).toBytes() );
	}

	/// Receive the given number of bytes from the socket.
	/** If the requested amount of data don't arrive all at once,
	  * it repeats the system calls until all requested data are received.
	  * The number of bytes actually received is stored in an output parameter.
	  * \param[out] received how many bytes were really received */
	SocketError receive( byte_span buffer, size_t & received ) noexcept;

	/// Receive the given number of bytes from the socket.
	/** If the requested amount of data don't arrive all at once,
	  * it repeats the system calls until all requested data are received.
	  * After the call, the size of the vector will be equal to the number of bytes actually received.
	  * \param[in] size how many bytes to receive */
	SocketError receive( std::vector< uint8_t > & buffer, size_t size ) noexcept
	{
		buffer.resize( size );  // allocate the needed storage
		size_t received;
		SocketError result = receive( byte_span( buffer.data(), buffer.size() ), received );
		buffer.resize( received );  // let's return the user a vector only as big as how much we actually received
		return result;
	}

 protected:

	 // allow creating socket object from already initialized socket handle, but only for TcpServerSocket
	 friend class TcpServerSocket;
	 TcpSocket( socket_t sock ) noexcept : SocketCommon( sock ) {}

	 SocketError _connect( int family, int addrlen, struct sockaddr * addr ) noexcept;

};


//======================================================================================================================
/// Abstraction over low-level TCP server socket system calls.
/** This class is used by a server to listen to incomming connections. */

class TcpServerSocket : public impl::SocketCommon
{

 public:

	TcpServerSocket() noexcept;
	~TcpServerSocket() noexcept;

	TcpServerSocket( const TcpServerSocket & other ) = delete;
	TcpServerSocket( TcpServerSocket && other ) noexcept;
	TcpServerSocket & operator=( const TcpServerSocket & other ) = delete;
	TcpServerSocket & operator=( TcpServerSocket && other ) noexcept;

	/// Opens a TCP server on selected port.
	SocketError open( uint16_t port ) noexcept;

	/// Waits for an incomming connection request and then returns a socket representing the established connection.
	TcpSocket accept() noexcept;

};


//======================================================================================================================
/// Abstraction over low-level UDP socket system calls.

class UdpSocket : public impl::SocketCommon
{

 public:

	UdpSocket() noexcept;
	~UdpSocket() noexcept;

	UdpSocket( const UdpSocket & other ) = delete;
	UdpSocket( UdpSocket && other ) noexcept;
	UdpSocket & operator=( const UdpSocket & other ) = delete;
	UdpSocket & operator=( UdpSocket && other ) noexcept;

	/// Opens an UDP socket on selected port.
	SocketError open( uint16_t port = 0 ) noexcept;

	/// Sends a datagram to a specified address and port.
	SocketError sendTo( const Endpoint & endpoint, const_byte_span buffer );

	/// Convenience wrapper of sendTo( const Endpoint &, const_byte_span ) for sending textual data.
	/** \param[in] message null-terminated buffer of chars */
	SocketError sendTo( const Endpoint & endpoint, const char * message )
	{
		return sendTo( endpoint, const_char_span( message, strlen(message) ).toBytes() );
	}

	/// Waits for an incomming datagram and returns the packet data and the address and port it came from.
	SocketError recvFrom( Endpoint & endpoint, byte_span buffer, size_t & received );

};


//======================================================================================================================


} // namespace own


#endif // CPPUTILS_SOCKET_INCLUDED
