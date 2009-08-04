#ifndef ISISPROPERTY_HPP
#define ISISPROPERTY_HPP

#include <boost/shared_ptr.hpp>
#include <map>
#include "type.hpp"
#include "log.hpp"

namespace isis {

/**
common property class

	@author Enrico Reimer
*/

class Property:public boost::shared_ptr<TypeBase>{
	public:
	template<typename T> Property(const T& ref):boost::shared_ptr <TypeBase >(new Type<T>(ref)){ }
	Property(){ }
	template<typename T> operator T()const{
		const TypeBase *dummy=get();
		const Type<T> ret=
		dummy->cast_to_type<T>();
		return (T)ret;
	}
};

}

#endif

