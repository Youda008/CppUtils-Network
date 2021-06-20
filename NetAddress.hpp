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

/** todo */
template< size_t size >
class GenericAddress
{

 protected:

	uint8_t _data [size];

 public:

	GenericAddress() {}

	GenericAddress( const GenericAddress< size > & other ) { *this = other; }
	GenericAddress< size > & operator=( const GenericAddress< size > & other )
	{
		impl::copyAddr( other._data, _data, size );
		return *this;
	}

	span< const uint8_t > data() const { return span< const uint8_t >( _data, _data + size ); }

	uint8_t operator[]( size_t idx )        { return _data[ idx ]; }
	uint8_t operator[]( size_t idx ) const  { return _data[ idx ]; }

	bool operator==( const GenericAddress< size > & other ) const { return compareData( _data, other._data ) == 0; }
	bool operator< ( const GenericAddress< size > & other ) const { return compareData( _data, other._data ) < 0; }
	bool operator> ( const GenericAddress< size > & other ) const { return compareData( _data, other._data ) > 0; }

};


class IPAddr;


/** todo */
class IPv4Addr : public GenericAddress<4>
{
	friend class IPAddr;
 public:
	using GenericAddress<4>::GenericAddress;
	IPv4Addr( span< const uint8_t > data );
	// TODO: fast single-instruction constructors and assigment

	friend std::ostream & operator<<( std::ostream & os, IPv4Addr addr );
	friend std::ostream & operator>>( std::istream & os, IPv4Addr & addr );
};


/** todo */
class IPv6Addr : public GenericAddress<16>
{
	friend class IPAddr;
 public:
	using GenericAddress<16>::GenericAddress;
	IPv6Addr( span< const uint8_t > data );
	// TODO: static span without size check

	friend std::ostream & operator<<( std::ostream & os, const IPv6Addr & addr );
	friend std::ostream & operator>>( std::istream & os, IPv6Addr & addr );
};


/** todo */
class IPAddr : public GenericAddress<16>
{

 public:

	enum class Ver
	{
		None,
		_4,
		_6
	};

	using GenericAddress<16>::GenericAddress;
	IPAddr() : _version( Ver::None ) {}
	IPAddr( span< const uint8_t > data );
	IPAddr( std::initializer_list< uint8_t > initList );
	IPAddr( const IPv4Addr & addr )  { impl::copyAddr( addr._data, _data, 4 ); }
	IPAddr( const IPv6Addr & addr )  { impl::copyAddr( addr._data, _data, 16 ); }
	// TODO: static span without size check
	// TODO: span and version
	// TODO: initializer list

	Ver version() const { return _version; }

	// TODO: error checks
	IPv4Addr v4() const  { return IPv4Addr( span< const uint8_t >( _data, 4 ) ); }
	IPv6Addr v6() const  { return IPv6Addr( span< const uint8_t >( _data, 16 ) ); }

	friend std::ostream & operator<<( std::ostream & os, const IPAddr & addr );
	friend std::ostream & operator>>( std::istream & os, IPAddr & addr );

 private:

	Ver _version;

};


//======================================================================================================================

inline void test()
{
	GenericAddress<20> addr1;
	GenericAddress<20> addr2( addr1 );
	GenericAddress<20> addr3{};
	addr3 = addr1;
}


} // namespace own


#endif // CPPUTILS_NETADDRESS_INCLUDED
