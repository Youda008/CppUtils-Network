//======================================================================================================================
// Project: CppUtils
//----------------------------------------------------------------------------------------------------------------------
// Author:      Jan Broz (Youda008)
// Created on:  2.11.2020
// Description: classes for binary serialization into binary buffers via operators << and >>
//======================================================================================================================

#ifndef CPPUTILS_BUFFER_STREAM_INCLUDED
#define CPPUTILS_BUFFER_STREAM_INCLUDED


#include "Essential.hpp"

#include "LangUtils.hpp"

#include <string>
#include <vector>


namespace own {


//======================================================================================================================
/** binary buffer input stream allowing serialization via operator << */

class BinaryOutputStream
{

 public:

	/** WARNING: The class takes non-owning reference to a buffer stored somewhere else and doesn't remember anything
	  * about its original type. You are responsible for making sure the buffer exists at least as long as this object
	  * and for allocating the storage big enough for all write operations to fit in. */
	BinaryOutputStream( span< uint8_t > buffer ) : _curPos( buffer.begin() ), _endPos( buffer.end() ) {}

	BinaryOutputStream( const BinaryOutputStream & other ) = delete;
	BinaryOutputStream( BinaryOutputStream && other ) = delete;
	BinaryOutputStream & operator=( const BinaryOutputStream & other ) = delete;

	void reset( span< uint8_t > buffer )         { _curPos = buffer.begin(); _endPos = buffer.end(); }

	//-- atomic elements -----------------------------------------------------------------------------------------------

	BinaryOutputStream & operator<<( uint8_t b )
	{
		*_curPos = b;
		_curPos++;
		return *this;
	}

	BinaryOutputStream & operator<<( char c )
	{
		*_curPos = uint8_t(c);
		_curPos++;
		return *this;
	}

	//-- integers ------------------------------------------------------------------------------------------------------

	/** Converts an arbitrary integral number from native format to big endian and writes it into the buffer. */
	template< typename IntType, typename std::enable_if< std::is_integral<IntType>::value, int >::type = 0 >
	void writeIntBE( IntType native )
	{
		uint8_t * const bigEndian = _curPos;

		// indexed variant is more optimizable than variant with pointers https://godbolt.org/z/McT3Yb
		size_t pos = sizeof( native );
		while (pos > 0) {  // can't use traditional for-loop approach, because index is unsigned
			--pos;         // so we can't check if it's < 0
			bigEndian[ pos ] = uint8_t( native & 0xFF );
			native = IntType( native >> 8 );
		}

		_curPos += sizeof( IntType );
	}

	/** Converts an arbitrary integral number from native format to little endian and writes it into the buffer. */
	template< typename IntType, typename std::enable_if< std::is_integral<IntType>::value, int >::type = 0 >
	void writeIntLE( IntType native )
	{
		uint8_t * const littleEndian = _curPos;

		// indexed variant is more optimizable than variant with pointers https://godbolt.org/z/McT3Yb
		size_t pos = 0;
		while (pos < sizeof( IntType )) {
			littleEndian[ pos ] = uint8_t( native & 0xFF );
			native = IntType( native >> 8 );
			++pos;
		}

		_curPos += sizeof( IntType );
	}

	/** Converts an integer representation of an enum value from native format to big endian and writes it into the buffer. */
	template< typename EnumType, typename std::enable_if< std::is_enum<EnumType>::value, int >::type = 0 >
	void writeEnumBE( EnumType native )
	{
		writeIntBE( enumToInt( native ) );
	}

	/** Converts an integer representation of an enum value from native format to little endian and writes it into the buffer. */
	template< typename EnumType, typename std::enable_if< std::is_enum<EnumType>::value, int >::type = 0 >
	void writeEnumLE( EnumType native )
	{
		writeIntLE( enumToInt( native ) );
	}

	//-- strings and arrays --------------------------------------------------------------------------------------------

	void writeBytes( span< const uint8_t > buffer );

	/** Writes a string WITHOUT its null terminator to the buffer. */
	void writeString( const std::string & str )  { writeBytes( make_span( str ).cast< const uint8_t >() ); }

	/** Writes a string WITH its null terminator to the buffer. */
	void writeString0( const std::string & str );

	BinaryOutputStream & operator<<( span< const uint8_t > buffer )
	{
		writeBytes( buffer );
		return *this;
	}

	/** Writes a string and its null terminator to the buffer. */
	BinaryOutputStream & operator<<( const std::string & str )
	{
		writeString0( str );
		return *this;
	}

	/** Writes specified number of zero bytes to the buffer. */
	void writeZeros( size_t numZeroBytes );

	//-- error handling ------------------------------------------------------------------------------------------------

	//bool hasFailed() const { return _failed; }

 private:

	uint8_t * _curPos;  ///< current position in the buffer
	uint8_t * _endPos;  ///< ending position in the buffer
	//bool _failed = false;  ///< the end was reached while attemting to write into buffer

};


//======================================================================================================================
/** binary buffer input stream allowing deserialization via operator >> */

class BinaryInputStream
{

 public:

	/** WARNING: The class takes non-owning reference to a buffer stored somewhere else and doesn't remember anything
	  * about its original type. You are responsible for making sure the buffer exists at least as long as this object. */
	BinaryInputStream( span< const uint8_t > buffer ) : _curPos( buffer.data() ), _endPos( buffer.data() + buffer.size() ) {}

	BinaryInputStream( const BinaryInputStream & other ) = delete;
	BinaryInputStream( BinaryInputStream && other ) = delete;
	BinaryInputStream & operator=( const BinaryInputStream & other ) = delete;

	void reset( span< const uint8_t > buffer )     { _curPos = buffer.data(); _endPos = buffer.data() + buffer.size(); }

	//-- atomic elements -----------------------------------------------------------------------------------------------

	uint8_t get()
	{
		if (canRead( 1 )) {
			return *(_curPos++);
		} else {
			return 0;
		}
	}

	char getChar()
	{
		return char( get() );
	}

	BinaryInputStream & operator>>( uint8_t & b )
	{
		b = get();
		return *this;
	}

	BinaryInputStream & operator>>( char & b )
	{
		b = getChar();
		return *this;
	}

	//-- integers ------------------------------------------------------------------------------------------------------

	/** Reads an arbitrary integral number from the buffer and converts it from big endian to native format.
	  * (return value variant) */
	template< typename IntType, typename std::enable_if< std::is_integral<IntType>::value, int >::type = 0 >
	IntType readIntBE()
	{
		IntType native = 0;

		if (!canRead( sizeof( IntType ) )) {
			return native;
		}

		const uint8_t * const bigEndian = _curPos;

		// indexed variant is more optimizable than variant with pointers https://godbolt.org/z/McT3Yb
		size_t pos = 0;
		while (pos < sizeof( IntType )) {
			native = IntType( native << 8 );
			native = IntType( native | bigEndian[ pos ] );
			++pos;
		}

		_curPos += sizeof( IntType );

		return native;
	}

	/** Reads an arbitrary integral number from the buffer and converts it from big endian to native format.
	  * (output parameter variant) */
	template< typename IntType, typename std::enable_if< std::is_integral<IntType>::value, int >::type = 0 >
	bool readIntBE( IntType & native )
	{
		native = readIntBE< IntType >();
		return !_failed;
	}

	/** Reads an arbitrary integral number from the buffer and converts it from little endian to native format.
	  * (return value variant) */
	template< typename IntType, typename std::enable_if< std::is_integral<IntType>::value, int >::type = 0 >
	IntType readIntLE()
	{
		IntType native = 0;

		if (!canRead( sizeof( IntType ) )) {
			return native;
		}

		const uint8_t * const littleEndian = _curPos;

		// indexed variant is more optimizable than variant with pointers https://godbolt.org/z/McT3Yb
		size_t pos = sizeof( IntType );
		while (pos > 0) {  // can't use traditional for-loop approach, because index is unsigned
			--pos;         // so we can't check if it's < 0
			native = IntType( native << 8 );
			native = IntType( native | littleEndian[ pos ] );
		}

		_curPos += sizeof(IntType);

		return native;
	}

	/** Reads an arbitrary integral number from the buffer and converts it from little endian to native format.
	  * (output parameter variant) */
	template< typename IntType, typename std::enable_if< std::is_integral<IntType>::value, int >::type = 0 >
	bool readIntLE( IntType & native )
	{
		native = readIntLE< IntType >();
		return !_failed;
	}

	/** Reads an integer representation of an enum value from the buffer and converts it from big endian to native format.
	  * (return value variant) */
	template< typename EnumType, typename std::enable_if< std::is_enum<EnumType>::value, int >::type = 0 >
	EnumType readEnumBE()
	{
		return EnumType( readIntBE< typename std::underlying_type< EnumType >::type >() );
	}

	/** Reads an integer representation of an enum value from the buffer and converts it from big endian to native format.
	  * (output parameter variant) */
	template< typename EnumType, typename std::enable_if< std::is_enum<EnumType>::value, int >::type = 0 >
	bool readEnumBE( EnumType & native )
	{
		native = readEnumBE< EnumType >();
		return !_failed;
	}

	/** Reads an integer representation of an enum value from the buffer and converts it from little endian to native format.
	  * (return value variant) */
	template< typename EnumType, typename std::enable_if< std::is_enum<EnumType>::value, int >::type = 0 >
	EnumType readEnumLE()
	{
		return EnumType( readIntLE< typename std::underlying_type< EnumType >::type >() );
	}

	/** Reads an integer representation of an enum value from the buffer and converts it from little endian to native format.
	  * (output parameter variant) */
	template< typename EnumType, typename std::enable_if< std::is_enum<EnumType>::value, int >::type = 0 >
	bool readEnumLE( EnumType & native )
	{
		native = readEnumLE< EnumType >();
		return !_failed;
	}

	//-- strings and arrays --------------------------------------------------------------------------------------------

	bool readBytes( span< uint8_t > buffer );

	/** Reads a string of specified size from the buffer.
	  * (output parameter variant) */
	bool readString( std::string & str, size_t size );

	/** Reads a string of specified size from the buffer.
	  * (return value variant) */
	std::string readString( size_t size )
	{
		std::string str;
		readString( str, size );
		return str;
	}

	/** Reads a string from the buffer until a null terminator is found.
	  * (output parameter variant) */
	bool readString0( std::string & str );

	/** Reads a string from the buffer until a null terminator is found.
	  * (return value variant) */
	std::string readString0()
	{
		std::string str;
		readString0( str );
		return str;
	}

	BinaryInputStream & operator>>( span< uint8_t > buffer )
	{
		readBytes( buffer );
		return *this;
	}

	/** Reads a string from the buffer until a null terminator is found. */
	BinaryInputStream & operator>>( std::string & str )
	{
		readString0( str );
		return *this;
	}

	/** Reads the remaining data from the current position until the end of the buffer to a vector. */
	bool readRemaining( std::vector< uint8_t > & buffer );

	/** Reads the remaining data from the current position until the end of the buffer to a string. */
	bool readRemaining( std::string & str );

	/** Moves over specified number of bytes without returning them to the user. */
	bool skip( size_t numBytes );

	//-- error handling ------------------------------------------------------------------------------------------------

	void setFailed() { _failed = true; }
	void resetFailed() { _failed = false; }
	bool hasFailed() const { return _failed; }

 private:

	bool canRead( size_t size )
	{
		// the _failed flag can be true already from the previous call, in that case it will stay failed
		_failed |= _curPos + size > _endPos;
		return !_failed;
	}

 private:

	const uint8_t * _curPos;
	const uint8_t * _endPos;
	bool _failed = false;

};


//======================================================================================================================


} // namespace own


//======================================================================================================================
//  this allows you to use operator << and >> for integers and enums without losing the choice between little/big endian

#define MAKE_LITTLE_ENDIAN_DEFAULT \
	template< typename IntType, typename std::enable_if< std::is_integral<IntType>::value, int >::type = 0 > \
	static own::BinaryOutputStream & operator<<( own::BinaryOutputStream & stream, IntType val ) \
	{\
		stream.writeIntLE( val ); \
		return stream; \
	}\
	template< typename EnumType, typename std::enable_if< std::is_enum<EnumType>::value, int >::type = 0 > \
	static own::BinaryOutputStream & operator<<( own::BinaryOutputStream & stream, EnumType val ) \
	{\
		stream.writeEnumLE( val ); \
		return stream; \
	}\
	template< typename IntType, typename std::enable_if< std::is_integral<IntType>::value, int >::type = 0 > \
	static own::BinaryInputStream & operator>>( own::BinaryInputStream & stream, IntType & val ) \
	{\
		stream.readIntLE( val ); \
		return stream; \
	}\
	template< typename EnumType, typename std::enable_if< std::is_enum<EnumType>::value, int >::type = 0 > \
	static own::BinaryInputStream & operator>>( own::BinaryInputStream & stream, EnumType & val ) \
	{\
		stream.readEnumLE( val ); \
		return stream; \
	}

#define MAKE_BIG_ENDIAN_DEFAULT \
	template< typename IntType, typename std::enable_if< std::is_integral<IntType>::value, int >::type = 0 > \
	static own::BinaryOutputStream & operator<<( own::BinaryOutputStream & stream, IntType val ) \
	{\
		stream.writeIntBE( val ); \
		return stream; \
	}\
	template< typename EnumType, typename std::enable_if< std::is_enum<EnumType>::value, int >::type = 0 > \
	static own::BinaryOutputStream & operator<<( own::BinaryOutputStream & stream, EnumType val ) \
	{\
		stream.writeEnumBE( val ); \
		return stream; \
	}\
	template< typename IntType, typename std::enable_if< std::is_integral<IntType>::value, int >::type = 0 > \
	static own::BinaryInputStream & operator>>( own::BinaryInputStream & stream, IntType & val ) \
	{\
		stream.readIntBE( val ); \
		return stream; \
	}\
	template< typename EnumType, typename std::enable_if< std::is_enum<EnumType>::value, int >::type = 0 > \
	static own::BinaryInputStream & operator>>( own::BinaryInputStream & stream, EnumType & val ) \
	{\
		stream.readEnumBE( val ); \
		return stream; \
	}


//======================================================================================================================


#endif // CPPUTILS_BUFFER_STREAM_INCLUDED
