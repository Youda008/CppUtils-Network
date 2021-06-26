//======================================================================================================================
// Project: CppUtils
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: abstraction over low-level system socket calls
//======================================================================================================================

#include "Socket.hpp"

#include "LangUtils.hpp"  // scope_guard
#include "CriticalError.hpp"

#ifdef _WIN32
	#include <winsock2.h>      // socket, closesocket
	#include <ws2tcpip.h>      // addrinfo

	using in_addr_t = unsigned long;  // windows does not use in_addr_t, it uses unsigned long instead
	using socklen_t = int;

	constexpr own::socket_t INVALID_SOCK = INVALID_SOCKET;
	constexpr own::system_error_t SUCCESS = ERROR_SUCCESS;
#else
	#include <unistd.h>        // open, close, read, write
	#include <fcntl.h>         // fnctl, O_NONBLOCK
	#include <sys/socket.h>    // socket
	#include <netdb.h>         // getaddrinfo, gethostbyname
	#include <netinet/in.h>    // sockaddr_in, in_addr, ntoh, hton
	#include <arpa/inet.h>     // inet_addr, inet_ntoa

	constexpr own::socket_t INVALID_SOCK = -1;
	constexpr system_error_t SUCCESS = 0;
#endif // _WIN32

#include <mutex>


namespace own {


//======================================================================================================================
//  error strings

const char * enumString( SocketError error )
{
	switch (error)
	{
		case SocketError::Success:              return "Success";
		case SocketError::AlreadyConnected:     return "AlreadyConnected";
		case SocketError::NotConnected:         return "NotConnected";
		case SocketError::NetworkingInitFailed: return "NetworkingInitFailed";
		case SocketError::HostNotResolved:      return "HostNotResolved";
		case SocketError::ConnectFailed:        return "ConnectFailed";
		case SocketError::SendFailed:           return "SendFailed";
		case SocketError::ConnectionClosed:     return "ConnectionClosed";
		case SocketError::Timeout:              return "Timeout";
		case SocketError::WouldBlock:           return "WouldBlock";
		case SocketError::AlreadyOpen:          return "AlreadyOpen";
		case SocketError::BindFailed:           return "BindFailed";
		case SocketError::ListenFailed:         return "ListenFailed";
		default:                                return "Other";
	}
}


//======================================================================================================================
//  network subsystem initialization and automatic termination

/// Represents OS-dependent networking subsystem.
/** This is a private class for internal use. */
class NetworkingSubsystem
{

 public:

	NetworkingSubsystem() : _initialized( false ) {}

	bool initializeIfNotAlready()
	{
		// this additional check is optimization for cases where it's already initialized, which is gonna be the majority
		if (_initialized)
			return true;

		// this method may get called from multiple threads, so we need to make sure they don't race
		std::unique_lock< std::mutex > lock( mtx );
		if (!_initialized)
		{
			_initialized = _initialize();
		}
		return _initialized;
	}

	~NetworkingSubsystem()
	{
		if (_initialized)
		{
			_terminate();
		}
	}

 private:

	bool _initialize()
	{
 #ifdef _WIN32
		return WSAStartup( MAKEWORD(2, 2), &_wsaData ) == NO_ERROR;
 #else
		return true;
 #endif // _WIN32
	}

	void _terminate()
	{
 #ifdef _WIN32
		WSACleanup();
 #else
 #endif // _WIN32
	}

 private:

 #ifdef _WIN32
	WSADATA _wsaData;
 #else
 #endif // _WIN32

	std::mutex mtx;
	bool _initialized;

};

static NetworkingSubsystem g_netSystem;  // this will get initialized on first use and terminated on process exit


//======================================================================================================================
//  SocketCommon

namespace impl {

SocketCommon::SocketCommon()
:
	_socket( INVALID_SOCK ),
	_lastSystemError( SUCCESS ),
	_isBlocking( true )
{}

SocketCommon::SocketCommon( socket_t sock )
:
	_socket( sock ),
	_lastSystemError( SUCCESS ),
	_isBlocking( true )
{}

SocketCommon::~SocketCommon()
{
	if (_socket == INVALID_SOCK)
	{
		return;
	}

	_shutdownSocket( _socket );

	_closeSocket( _socket );
}

SocketCommon::SocketCommon( SocketCommon && other )
{
	*this = move( other );
}

SocketCommon & SocketCommon::operator=( SocketCommon && other )
{
	_socket = other._socket;
	_lastSystemError = other._lastSystemError;
	_isBlocking = other._isBlocking;
	other._socket = INVALID_SOCK;
	other._lastSystemError = 0;
	other._isBlocking = false;

	return *this;
}

SocketError SocketCommon::close()
{
	if (_socket == INVALID_SOCK)
	{
		return SocketError::NotConnected;
	}

	if (!_shutdownSocket( _socket ))
	{
		_lastSystemError = getLastError();
		_socket = INVALID_SOCK;
		return SocketError::Other;
	}

	if (!_closeSocket( _socket ))
	{
		_lastSystemError = getLastError();
		_socket = INVALID_SOCK;
		return SocketError::Other;
	}

	_lastSystemError = getLastError();
	_socket = INVALID_SOCK;
	return SocketError::Success;
}

bool SocketCommon::isOpen() const
{
	return _socket != INVALID_SOCK;
}

bool SocketCommon::_shutdownSocket( socket_t sock )
{
 #ifdef _WIN32
	return ::shutdown( sock, SD_BOTH ) == 0;
 #else
	return ::shutdown( sock, SHUT_RDWR ) == 0;
 #endif // _WIN32
}

bool SocketCommon::_closeSocket( socket_t sock )
{
 #ifdef _WIN32
	return ::closesocket( sock ) == 0;
 #else
	return ::close( sock ) == 0;
 #endif // _WIN32
}

bool SocketCommon::_setTimeout( socket_t sock, std::chrono::milliseconds timeout_ms )
{
 #ifdef _WIN32
	DWORD timeout = (DWORD)timeout_ms.count();
 #else
	struct timeval timeout;
	timeout.tv_sec  = timeout_ms.count() / 1000;
	timeout.tv_usec = (timeout_ms.count() % 1000) * 1000;
 #endif // _WIN32

	return ::setsockopt( sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout) ) == 0;
}

bool SocketCommon::_isTimeout( system_error_t errorCode )
{
 #ifdef _WIN32
	return errorCode == WSAETIMEDOUT;
 #else
	return errorCode == EAGAIN || errorCode == EWOULDBLOCK || errorCode == ETIMEDOUT;
 #endif // _WIN32
}

bool SocketCommon::_isWouldBlock( system_error_t errorCode )
{
 #ifdef _WIN32
	return errorCode == WSAEWOULDBLOCK;
 #else
	return errorCode == EAGAIN || errorCode == EWOULDBLOCK;
 #endif // _WIN32
}

bool SocketCommon::_setBlockingMode( socket_t sock, bool enable )
{
#ifdef _WIN32
	unsigned long mode = enable ? 0 : 1;
	return ::ioctlsocket( sock, (long)FIONBIO, &mode ) == 0;
#else
	int flags = ::fcntl( sock, F_GETFL, 0 );
	if (flags == -1)
		return false;

	if (enable)
		flags &= ~O_NONBLOCK;
	else
		flags |= O_NONBLOCK;

	return ::fcntl( sock, F_SETFL, flags ) == 0;
#endif
}

} // namespace impl


//======================================================================================================================
//  TcpSocket

TcpClientSocket::TcpClientSocket() : impl::SocketCommon() {}

TcpClientSocket::~TcpClientSocket() {}  // delegate to SocketCommon

TcpClientSocket::TcpClientSocket( TcpClientSocket && other )
{
	*this = move( other );
}

TcpClientSocket & TcpClientSocket::operator=( TcpClientSocket && other )
{
	return static_cast< TcpClientSocket & >( impl::SocketCommon::operator=( move( other ) ) );
}

SocketError TcpClientSocket::connect( const std::string & host, uint16_t port )
{
	if (_socket != INVALID_SOCK)
	{
		return SocketError::AlreadyConnected;
	}

	bool initialized = g_netSystem.initializeIfNotAlready();
	if (!initialized)
	{
		_lastSystemError = getLastError();
		return SocketError::NetworkingInitFailed;
	}

	char portStr [8];
	snprintf( portStr, sizeof(portStr), "%hu", ushort(port) );

	struct addrinfo hint;
	memset( &hint, 0, sizeof(hint) );
	hint.ai_family = AF_UNSPEC;      // IPv4 or IPv6, it doesn't matter
	hint.ai_socktype = SOCK_STREAM;  // but only TCP!

	// find protocol family and address of the host
	struct addrinfo * ainfo;
	if (::getaddrinfo( host.c_str(), portStr, &hint, &ainfo ) != SUCCESS)
	{
		_lastSystemError = getLastError();
		return SocketError::HostNotResolved;
	}
	auto ainfo_guard = at_scope_end_do( [ &ainfo ]() { ::freeaddrinfo( ainfo ); } );

	return _connect( ainfo->ai_family, (int)ainfo->ai_addrlen, ainfo->ai_addr );
}

SocketError TcpClientSocket::connect( const IPAddr & addr, uint16_t port )
{
	if (_socket != INVALID_SOCK)
	{
		return SocketError::AlreadyConnected;
	}

	bool initialized = g_netSystem.initializeIfNotAlready();
	if (!initialized)
	{
		_lastSystemError = getLastError();
		return SocketError::NetworkingInitFailed;
	}

	struct sockaddr_storage saddr; int addrlen;
	endpointToSockaddr( { addr, port }, (struct sockaddr *)&saddr, addrlen );

	return _connect( saddr.ss_family, addrlen, (struct sockaddr *)&saddr );
}

SocketError TcpClientSocket::_connect( int family, int addrlen, struct sockaddr * addr )
{
	// create a corresponding socket
	_socket = ::socket( family, SOCK_STREAM, 0 );
	if (_socket == INVALID_SOCK)
	{
		_lastSystemError = getLastError();
		return SocketError::Other;
	}

	if (::connect( _socket, addr, addrlen ) != SUCCESS)
	{
		_lastSystemError = getLastError();
		_closeSocket( _socket );
		_socket = INVALID_SOCK;
		return SocketError::ConnectFailed;
	}

	_lastSystemError = getLastError();
	return SocketError::Success;
}

SocketError TcpClientSocket::disconnect()
{
	return impl::SocketCommon::close();
}

bool TcpClientSocket::isConnected() const
{
	return impl::SocketCommon::isOpen();
}

bool TcpClientSocket::isValid() const
{
	return _socket != INVALID_SOCK;
}

bool TcpClientSocket::setTimeout( std::chrono::milliseconds timeout )
{
	bool success = _setTimeout( _socket, timeout );
	_lastSystemError = getLastError();
	return success;
}

SocketError TcpClientSocket::send( const_byte_span buffer )
{
	if (_socket == INVALID_SOCK)
	{
		return SocketError::NotConnected;
	}

	const uint8_t * sendBegin = buffer.data();
	size_t sendSize = buffer.size();
	while (sendSize > 0)
	{
		int sent = ::send( _socket, (const char *)sendBegin, (int)sendSize, 0 );
		if (sent < 0)
		{
			_lastSystemError = getLastError();
			return SocketError::SendFailed;
		}
		sendBegin += sent;
		sendSize -= size_t( sent );
	}

	_lastSystemError = getLastError();
	return SocketError::Success;
}

SocketError TcpClientSocket::receive( byte_span buffer, size_t & totalReceived )
{
	if (_socket == INVALID_SOCK)
	{
		return SocketError::NotConnected;
	}

	uint8_t * recvBegin = buffer.data();
	size_t recvSize = buffer.size();
	while (recvSize > 0)
	{
		int received = ::recv( _socket, (char *)recvBegin, (int)recvSize, 0 );
		if (received <= 0)
		{
			_lastSystemError = getLastError();
			totalReceived = buffer.size() - recvSize;  // this is how much we failed to receive

			if (received == 0)
			{
				_closeSocket( _socket );  // server closed, so let's close on our side too
				_socket = INVALID_SOCK;
				return SocketError::ConnectionClosed;
			}
			else if (!_isBlocking && _isWouldBlock( _lastSystemError ))
			{
				return SocketError::WouldBlock;
			}
			else if (_isTimeout( _lastSystemError ))
			{
				return SocketError::Timeout;
			}
			else
			{
				return SocketError::Other;
			}
		}
		recvBegin += received;
		recvSize -= size_t( received );
	}

	_lastSystemError = getLastError();
	totalReceived = buffer.size();
	return SocketError::Success;
}

//======================================================================================================================
//  TcpServerSocket

TcpServerSocket::TcpServerSocket() : impl::SocketCommon() {}

TcpServerSocket::~TcpServerSocket() {}  // delegate to SocketCommon

TcpServerSocket::TcpServerSocket( TcpServerSocket && other )
{
	*this = move( other );
}

TcpServerSocket & TcpServerSocket::operator=( TcpServerSocket && other )
{
	return static_cast< TcpServerSocket & >( impl::SocketCommon::operator=( move( other ) ) );
}

SocketError TcpServerSocket::open( uint16_t port )
{
	if (_socket != INVALID_SOCK)
	{
		return SocketError::AlreadyOpen;
	}

	bool initialized = g_netSystem.initializeIfNotAlready();
	if (!initialized)
	{
		_lastSystemError = getLastError();
		return SocketError::NetworkingInitFailed;
	}

	// TODO: IPv4 vs IPv6
	struct sockaddr_in saddr;
	inet_pton( AF_INET, "127.0.0.1", &saddr.sin_addr );
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons( port );

	// create a corresponding socket
	_socket = ::socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if (_socket == INVALID_SOCK)
	{
		_lastSystemError = getLastError();
		return SocketError::Other;
	}

	// bind the socket to a local port
	if (::bind( _socket, (sockaddr *)&saddr, sizeof(saddr) ) != 0)
	{
		_lastSystemError = getLastError();
		_closeSocket( _socket );
		_socket = INVALID_SOCK;
		return SocketError::BindFailed;
	}

	// set the socket to a listen state
	static constexpr uint BACKLOG = 16;  // system queue for incoming connection requests TODO: user specified
	if (::listen( _socket, BACKLOG ) != 0)
	{
		_lastSystemError = getLastError();
		_closeSocket( _socket );
		_socket = INVALID_SOCK;
		return SocketError::ListenFailed;
	}

	_lastSystemError = getLastError();
	return SocketError::Success;
}

TcpClientSocket TcpServerSocket::accept()
{
	if (_socket == INVALID_SOCK)
	{
		return TcpClientSocket();
	}

	// TODO: pass addr to the user
	struct sockaddr_storage clientAddr;
	socklen_t claddrSize = sizeof(clientAddr);

	socket_t clientSocket = ::accept( _socket, (struct sockaddr *)&clientAddr, &claddrSize );
	if (clientSocket == INVALID_SOCK)
	{
		_lastSystemError = getLastError();
		return TcpClientSocket();
	}

	_lastSystemError = getLastError();
	return TcpClientSocket( clientSocket );
}


//======================================================================================================================
//  UdpSocket

UdpSocket::UdpSocket() : impl::SocketCommon() {}

UdpSocket::~UdpSocket() {}  // delegate to SocketCommon

UdpSocket::UdpSocket( UdpSocket && other )
{
	*this = move( other );
}

UdpSocket & UdpSocket::operator=( UdpSocket && other )
{
	return static_cast< UdpSocket & >( impl::SocketCommon::operator=( move( other ) ) );
}

SocketError UdpSocket::open( uint16_t port )
{
	if (_socket != INVALID_SOCK)
	{
		return SocketError::AlreadyOpen;
	}

	bool initialized = g_netSystem.initializeIfNotAlready();
	if (!initialized)
	{
		_lastSystemError = getLastError();
		return SocketError::NetworkingInitFailed;
	}

	// TODO: IPv4 vs IPv6
	struct sockaddr_in saddr;
	inet_pton( AF_INET, "127.0.0.1", &saddr.sin_addr );
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons( port );

	// create a corresponding socket
	_socket = ::socket( AF_INET, SOCK_DGRAM, 0 );
	if (_socket == INVALID_SOCK)
	{
		_lastSystemError = getLastError();
		return SocketError::Other;
	}

	// bind the socket to a local port
	if (port != 0)
	{
		if (::bind( _socket, (sockaddr *)&saddr, sizeof(saddr) ) != 0)
		{
			_lastSystemError = getLastError();
			_closeSocket( _socket );
			_socket = INVALID_SOCK;
			return SocketError::BindFailed;
		}
	}

	_lastSystemError = getLastError();
	return SocketError::Success;
}

SocketError UdpSocket::sendTo( const Endpoint & endpoint, const_byte_span buffer )
{
	struct sockaddr_storage saddr; int addrlen;
	endpointToSockaddr( endpoint, (struct sockaddr *)&saddr, addrlen );

	int sent = ::sendto( _socket, (const char *)buffer.data(), (int)buffer.size(), 0, (struct sockaddr *)&saddr, addrlen );
	if (sent < 0)
	{
		_lastSystemError = getLastError();
		return SocketError::SendFailed;
	}

	_lastSystemError = getLastError();
	return SocketError::Success;
}

SocketError UdpSocket::recvFrom( Endpoint & endpoint, byte_span buffer, size_t & totalReceived )
{
	struct sockaddr_storage saddr; int addrlen;
	memset( &saddr, 0, sizeof(saddr) );
	addrlen = sizeof(saddr);

	int received = ::recvfrom( _socket, (char *)buffer.data(), (int)buffer.size(), 0, (struct sockaddr *)&saddr, &addrlen );
	if (received < 0)
	{
		_lastSystemError = getLastError();
		if (!_isBlocking && _isWouldBlock( _lastSystemError ))
		{
			return SocketError::WouldBlock;
		}
		else if (_isTimeout( _lastSystemError ))
		{
			return SocketError::Timeout;
		}
		else
		{
			return SocketError::Other;
		}
	}

	sockaddrToEndpoint( (struct sockaddr *)&saddr, endpoint );

	totalReceived = size_t( received );
	_lastSystemError = getLastError();
	return SocketError::Success;
}


//======================================================================================================================


} // namespace own
