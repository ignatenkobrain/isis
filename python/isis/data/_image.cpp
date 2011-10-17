#include "_image.hpp"

namespace isis
{
namespace python
{
namespace data
{

_Image::_Image( PyObject *p )
	: boost::python::wrapper< Image >(), self( p )
{
	init();
}

_Image::_Image( PyObject *p, const isis::data::Image &base )
	: Image( base ), boost::python::wrapper< Image >(), self( p )
{
	init();
}

void _Image::init()
{
	m_Min = util::Singletons::get<isis::python::core::_internal::TypesMap, 10>().at(
			getMinMax().first->getTypeID() )->convert( *getMinMax().first );
	m_Max = util::Singletons::get<isis::python::core::_internal::TypesMap, 10>().at(
			getMinMax().second->getTypeID() )->convert( *getMinMax().second );
}

std::list< Chunk > _Image::_getChunksAsVector(void )
{
	std::list<Chunk> retChunkList;
	std::vector<Chunk>  chunkList( copyChunksToVector() );
	BOOST_FOREACH( std::vector<Chunk> ::reference ref, chunkList ) {
		retChunkList.push_back( ref );
	}
	return retChunkList;
}

Chunk _Image::_getChunkAs(const size_t& first, const size_t& second, const size_t& third, const size_t& fourth, const std::string& type)
{
	Chunk ret = getChunk( first, second, third, fourth ); // get a cheap copy
	ret.convertToType( util::getTransposedTypeMap( true, true )[type] );
	return ret;
}

const std::string _Image::_getMainOrientation()
{
	switch( getMainOrientation() ) {
	case sagittal:
		return std::string( "sagittal" );
		break;
	case reversed_sagittal:
		return std::string( "reversed_sagittal" );
		break;
	case axial:
		return std::string( "axial" );
		break;
	case reversed_axial:
		return std::string( "reversed_axial" );
		break;
	case coronal:
		return std::string( "coronal" );
		break;
	case reversed_coronal:
		return std::string( "reversed_coronal" );
		break;
	default:
		return std::string( "unknown" );
		break;
	}
}

void _Image::_transformCoords(boost::python::list matrix, const bool& center)
{
	std::vector< boost::python::list > rows;
	for ( boost::python::ssize_t i = 0; i < boost::python::len( matrix ); ++i ) {
		rows.push_back( boost::python::extract< boost::python::list >( matrix[i] ) );
	}
	boost::numeric::ublas::matrix<float> boostMatrix( 3, 3 );

	for ( unsigned short i = 0; i < 3; i++ ) {
		for ( unsigned short j = 0; j < 3; j++ ) {
			boostMatrix( i, j ) = boost::python::extract<float> ( rows[i][j] );
		}
	}
	transformCoords( boostMatrix, center );
}

bool _Image::_makeOfType( _internal::image_types type )
{
	return convertToType( static_cast<unsigned short>(type) );
}

size_t _Image::_spliceDownTo(const isis::data::dimensions dims )
{
	return spliceDownTo( dims );
}
Image _Image::_deepCopy()
{
	switch( getMajorTypeID() ) {
	case ValuePtr<int8_t>::staticID:
		return MemImage<int8_t>( *this );
		break;
	case ValuePtr<uint8_t>::staticID:
		return MemImage<uint8_t>( *this );
		break;
	case ValuePtr<int16_t>::staticID:
		return MemImage<int16_t>( *this );
		break;
	case ValuePtr<uint16_t>::staticID:
		return MemImage<uint16_t>( *this );
		break;
	case ValuePtr<int32_t>::staticID:
		return MemImage<int32_t>( *this );
		break;
	case ValuePtr<uint32_t>::staticID:
		return MemImage<uint32_t>( *this );
		break;
	case ValuePtr<float>::staticID:
		return MemImage<float>( *this );
		break;
	case ValuePtr<double>::staticID:
		return MemImage<double>( *this );
		break;
	default:
		LOG( Runtime, error ) << "Unregistered pixel type " << util::getTypeMap()[this->getMajorTypeID()] << ".";
		return MemImage<int8_t>( *this );
	}
}

Image _Image::_deepCopy(_internal::image_types type)
{
	
	Image retImage = _deepCopy();
	retImage.convertToType( static_cast<unsigned short> ( type ) );

	return retImage;
}


}
}
}