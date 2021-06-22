//======================================================================================================================
// Project: CppUtils
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Description: classes for network addresses
//======================================================================================================================

#ifndef CPPUTILS_NETADDRESS_INCLUDED
#define CPPUTILS_NETADDRESS_INCLUDED


#include "Essential.hpp"

#include "Span.hpp"

#include <iosfwd>


namespace own {


//======================================================================================================================
//  array utils for which it is not worth to include the big standard library headers here

namespace impl {
	void copyAddr( const uint8_t * src, uint8_t * dst, size_t size );
	int compareAddr( const uint8_t * a1, const uint8_t * a2, size_t size );
}


//======================================================================================================================

/** TODO */
template< size_t Size >
class GenericAddress
{

 protected:

	uint8_t _data [Size];

 public:

	GenericAddress() {}

	GenericAddress( fixed_const_byte_span< Size > data )
	{
		impl::copyAddr( data.data(), _data, Size );
	}
	GenericAddress( const GenericAddress< Size > & other )
	{
		impl::copyAddr( other._data, _data, Size );
	}
	GenericAddress< Size > & operator=( const GenericAddress< Size > & other )
	{
		impl::copyAddr( other._data, _data, Size );
		return *this;
	}

	fixed_const_byte_span< Size > data() const  { return fixed_const_byte_span< Size >( _data ); }

	uint8_t operator[]( size_t idx )        { return _data[ idx ]; }
	uint8_t operator[]( size_t idx ) const  { return _data[ idx ]; }

	bool operator==( const GenericAddress< Size > & other ) const { return impl::compareAddr( _data, other._data ) == 0; }
	bool operator< ( const GenericAddress< Size > & other ) const { return impl::compareAddr( _data, other._data ) < 0; }
	bool operator> ( const GenericAddress< Size > & other ) const { return impl::compareAddr( _data, other._data ) > 0; }

};


class IPAddr;

enum class IPVer
{
	_4,
	_6
};


/** TODO */
class IPv4Addr : public GenericAddress<4>
{
	friend class IPAddr;

 public:

	using GenericAddress<4>::GenericAddress;

	// single instruction construction and assignment
	IPv4Addr( fixed_const_byte_span<4> data )
	{
		*reinterpret_cast< uint32_t * >( _data ) = *reinterpret_cast< const uint32_t * >( data.data() );
	}
	IPv4Addr( const IPv4Addr & other ) : GenericAddress<4>()
	{
		*reinterpret_cast< uint32_t * >( _data ) = *reinterpret_cast< const uint32_t * >( other._data );
	}
	IPv4Addr & operator=( const IPv4Addr & other )
	{
		*reinterpret_cast< uint32_t * >( _data ) = *reinterpret_cast< const uint32_t * >( other._data );
		return *this;
	}

	friend std::ostream & operator<<( std::ostream & os, IPv4Addr addr );
	friend std::ostream & operator>>( std::istream & os, IPv4Addr & addr );
};


/** TODO */
class IPv6Addr : public GenericAddress<16>
{
	friend class IPAddr;

 public:

	using GenericAddress<16>::GenericAddress;

	friend std::ostream & operator<<( std::ostream & os, const IPv6Addr & addr );
	friend std::ostream & operator>>( std::istream & os, IPv6Addr & addr );
};


/** TODO */
class IPAddr : public GenericAddress<16>
{
	IPVer _version;

 public:

	using GenericAddress<16>::GenericAddress;

	IPAddr() {}

	IPAddr( const_byte_span data );
	IPAddr( std::initializer_list< uint8_t > initList );

	IPAddr( fixed_const_byte_span<4> data )
	{
		impl::copyAddr( data.data(), _data, 4 );
		_version = IPVer::_4;
	}
	IPAddr( fixed_const_byte_span<16> data )
	{
		impl::copyAddr( data.data(), _data, 16 );
		_version = IPVer::_6;
	}
	IPAddr( const IPv4Addr & addr )
	{
		impl::copyAddr( addr._data, _data, 4 );
		_version = IPVer::_4;
	}
	IPAddr( const IPv6Addr & addr )
	{
		impl::copyAddr( addr._data, _data, 16 );
		_version = IPVer::_6;
	}

	IPVer version() const  { return _version; }

	// TODO: error checks
	IPv4Addr v4() const  { return IPv4Addr( fixed_const_byte_span<4>( _data ) ); }
	IPv6Addr v6() const  { return IPv6Addr( fixed_const_byte_span<16>( _data ) ); }

	friend std::ostream & operator<<( std::ostream & os, const IPAddr & addr );
	friend std::ostream & operator>>( std::istream & os, IPAddr & addr );
};


//======================================================================================================================

class MACAddr : public GenericAddress<6>
{
 public:

	using GenericAddress<6>::GenericAddress;

	friend std::ostream & operator<<( std::ostream & os, const MACAddr & addr );
	friend std::ostream & operator>>( std::istream & os, MACAddr & addr );
};


//======================================================================================================================

struct Endpoint
{
	IPAddr addr;
	uint16_t port;
};


//======================================================================================================================


} // namespace own


#endif // CPPUTILS_NETADDRESS_INCLUDED
