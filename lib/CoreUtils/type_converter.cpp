/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) <year>  <name of author>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "type_converter.hpp"
#include "type_base.hpp"
#include "propmap.hpp" // we must have all types here and PropMap was only forward-declared in types.hpp
#include <boost/mpl/for_each.hpp>
#include <boost/numeric/conversion/converter.hpp>
#include <boost/type_traits/is_arithmetic.hpp>
#include <boost/mpl/and.hpp>

/// @cond _internal
namespace isis{ namespace util{ namespace _internal{

//Define generator - this can be global because its using convert internally
template<typename SRC,typename DST> class TypeGenerator: public TypeConverterBase{
public:
	void generate(const boost::scoped_ptr<TypeBase>& src, boost::scoped_ptr<TypeBase>& dst)const{
		LOG_IF(dst.get(),Debug,warning) <<
			"Generating into existing value " << dst->toString(true);
		Type<DST> *ref=new Type<DST>;
		convert(src->cast_to_Type<SRC>(),*ref);
		dst.reset(ref);
	}
};

/////////////////////////////////////////////////////////////////////////////
// general converter version -- does nothing
/////////////////////////////////////////////////////////////////////////////
template<bool NUMERIC,bool SAME,typename SRC, typename DST> class TypeConverter : public TypeGenerator<SRC,DST>{
public:
	virtual ~TypeConverter(){}
};

/////////////////////////////////////////////////////////////////////////////
// trivial version -- for conversion of the same type
/////////////////////////////////////////////////////////////////////////////
template<bool NUMERIC,typename SRC, typename DST> class TypeConverter<NUMERIC,true,SRC,DST> : public TypeGenerator<SRC,DST>{
	TypeConverter(){
		LOG(Debug,verbose_info)
		<< "Creating trivial copy converter for " << Type<SRC>::staticName();
	};
public:
	static boost::shared_ptr<const TypeConverterBase> create(){
		TypeConverter<NUMERIC,true,SRC,DST> *ret=new TypeConverter<NUMERIC,true,SRC,DST>;
		return boost::shared_ptr<const TypeConverterBase>(ret);
	}
	void convert(const TypeBase& src, TypeBase& dst)const{
		SRC &dstVal=dst.cast_to_Type<SRC>();
		const SRC &srcVal=src.cast_to_Type<SRC>();
		dstVal = srcVal;
	}
	virtual ~TypeConverter(){}
};


/////////////////////////////////////////////////////////////////////////////
// Numeric version -- uses boost::numeric_cast
/////////////////////////////////////////////////////////////////////////////
template<typename SRC, typename DST> class TypeConverter<true,false,SRC,DST> : public TypeGenerator<SRC,DST>{
	TypeConverter(){
		LOG(Debug,verbose_info)
			<< "Creating numeric converter from "
			<< Type<SRC>::staticName() << " to " << Type<DST>::staticName();
	};
public:
	static boost::shared_ptr<const TypeConverterBase> create(){
		TypeConverter<true,false,SRC,DST> *ret=new TypeConverter<true,false,SRC,DST>;
		return boost::shared_ptr<const TypeConverterBase>(ret);
	}
	void convert(const TypeBase& src, TypeBase& dst)const{
		try
		{
			typedef boost::numeric::converter<
				DST,SRC,
				boost::numeric::conversion_traits<DST,SRC>,
				boost::numeric::def_overflow_handler,
				boost::numeric::RoundEven<SRC>
			> converter;
			DST &dstVal=dst.cast_to_Type<DST>();
			const SRC &srcVal=src.cast_to_Type<SRC>();
			dstVal = converter::convert(srcVal);
		}
		catch ( boost::numeric::bad_numeric_cast const& e)
		{
			LOG(Runtime,error)
			<< "Automatic numeric conversion of " << MSubject(src.toString(true)) << " to " << dst.typeName() << " failed: " << e.what();
		}
	}
	virtual ~TypeConverter(){}
};

/////////////////////////////////////////////////////////////////////////////
// vector4 version -- uses TypeConverter on every element
/////////////////////////////////////////////////////////////////////////////
template<typename SRC, typename DST > class TypeConverter<false,false,vector4<SRC>,vector4<DST> >: public TypeGenerator<vector4<SRC>,vector4<DST> >{
	boost::shared_ptr<const TypeConverterBase> m_conv;
	TypeConverter(boost::shared_ptr<const TypeConverterBase> elem_conv):m_conv(elem_conv){
		LOG(Debug,verbose_info)
		<< "Creating vector converter from "
		<< Type<vector4<SRC> >::staticName() << " to " << Type<vector4<DST> >::staticName();
	};
public:
	static boost::shared_ptr<const TypeConverterBase> create(){
		typedef boost::mpl::and_<boost::is_arithmetic<SRC>,boost::is_arithmetic<DST> > is_num;
		typedef boost::is_same<SRC,DST> is_same;
		boost::shared_ptr<const TypeConverterBase> elem_conv=
			TypeConverter<is_num::value,is_same::value,SRC,DST>::create();
		
		if(elem_conv){
			TypeConverter<false,false,vector4<SRC>,vector4<DST> > *ret=new TypeConverter<false,false,vector4<SRC>,vector4<DST> >(elem_conv);
			return boost::shared_ptr<const TypeConverterBase>(ret);
		} else {
			return boost::shared_ptr<const TypeConverterBase>();
		}

	}
	void convert(const TypeBase& src, TypeBase& dst)const{
		vector4<DST> &dstVal=dst.cast_to_Type<vector4<DST> >();
		const vector4<SRC> &srcVal=src.cast_to_Type<vector4<SRC> >();

		for(int i=0;i<4;i++){//slow and ugly, but flexible
			Type<DST> dst;
			m_conv->convert(Type<SRC>(srcVal[i]),dst);
			dstVal[i]=(DST)dst;
		}
	}
	virtual ~TypeConverter(){}
};
	
/////////////////////////////////////////////////////////////////////////////
// list version -- uses TypeConverter on every element
/////////////////////////////////////////////////////////////////////////////
template<typename SRC, typename DST > class TypeConverter<false,false,std::list<SRC>,std::list<DST> >: public TypeGenerator<std::list<SRC>,std::list<DST> >{
	boost::shared_ptr<const TypeConverterBase> m_conv;
	TypeConverter(boost::shared_ptr<const TypeConverterBase> elem_conv):m_conv(elem_conv){
		LOG(Debug,verbose_info)
		<< "Creating list converter from "
		<< Type<std::list<SRC> >::staticName() << " to " << Type<std::list<DST> >::staticName();
	};
public:
	static boost::shared_ptr<const TypeConverterBase> create(){
		typedef boost::mpl::and_<boost::is_arithmetic<SRC>,boost::is_arithmetic<DST> > is_num;
		typedef boost::is_same<SRC,DST> is_same;
		boost::shared_ptr<const TypeConverterBase> elem_conv=
		TypeConverter<is_num::value,is_same::value,SRC,DST>::create();

		if(elem_conv){
			TypeConverter<false,false,std::list<SRC>,std::list<DST> > *ret=new TypeConverter<false,false,std::list<SRC>,std::list<DST> >(elem_conv);
			return boost::shared_ptr<const TypeConverterBase>(ret);
		} else {
			return boost::shared_ptr<const TypeConverterBase>();
		}

	}
	void convert(const TypeBase& src, TypeBase& dst)const{
		std::list<DST> &dstVal=dst.cast_to_Type<std::list<DST> >();
		LOG_IF(not dstVal.empty(),CoreLog,warning)
			<< "Storing into non empty list while conversion from "
			<< Type<std::list<SRC> >::staticName() << " to " << Type<std::list<DST> >::staticName();
		const std::list<SRC> &srcVal=src.cast_to_Type<std::list<SRC> >();

		for(typename std::list<SRC>::const_iterator i=srcVal.begin();i!=srcVal.end();i++){//slow and ugly, but flexible
			Type<DST> dst;
			m_conv->convert(Type<SRC>(*i),dst);
			dstVal.push_back((DST)dst);
		}
	}
	virtual ~TypeConverter(){}
};


/////////////////////////////////////////////////////////////////////////////
// string version -- uses lexical_cast to convert from/to string
/////////////////////////////////////////////////////////////////////////////
template<typename DST> class TypeConverter<false,false,std::string,DST> : public TypeGenerator<std::string,DST>{
	TypeConverter(){
		LOG(Debug,verbose_info)
		<< "Creating from-string converter for " << Type<DST>::staticName();
	};
public:
	static boost::shared_ptr<const TypeConverterBase> create(){
		TypeConverter<false,false,std::string,DST> *ret=new TypeConverter<false,false,std::string,DST>;
		return boost::shared_ptr<const TypeConverterBase>(ret);
	}
	void convert(const TypeBase& src, TypeBase& dst)const{
		DST &dstVal=dst.cast_to_Type<DST>();
		const std::string &srcVal=src.cast_to_Type<std::string>();
		dstVal = boost::lexical_cast<DST>(srcVal);
	}
	virtual ~TypeConverter(){}
};
template<typename SRC> class TypeConverter<false,false,SRC,std::string> : public TypeGenerator<SRC,std::string>{
	TypeConverter(){
		LOG(Debug,verbose_info)
		<< "Creating to-string converter for " << Type<SRC>::staticName();
	};
public:
	static boost::shared_ptr<const TypeConverterBase> create(){
		TypeConverter<false,false,SRC,std::string> *ret=new TypeConverter<false,false,SRC,std::string>;
		return boost::shared_ptr<const TypeConverterBase>(ret);
	}
	void convert(const TypeBase& src, TypeBase& dst)const{
		std::string &dstVal=dst.cast_to_Type<std::string>();
		const SRC &srcVal=src.cast_to_Type<SRC>();
		dstVal = boost::lexical_cast<std::string>(srcVal);
	}
	virtual ~TypeConverter(){}
};


/////////////////////////////////////////////////////////////////////////////
// string/bool version -- uses decision based on text
/////////////////////////////////////////////////////////////////////////////
template<> class TypeConverter<false,false,std::string,bool> : public TypeGenerator<std::string,bool>{
	TypeConverter(){
		LOG(Debug,verbose_info)
		<< "Creating special from-string converter for " << Type<bool>::staticName();
	};
public:
	static boost::shared_ptr<const TypeConverterBase> create(){
		TypeConverter<false,false,std::string,bool> *ret=new TypeConverter<false,false,std::string,bool>;
		return boost::shared_ptr<const TypeConverterBase>(ret);
	}
	void convert(const TypeBase& src, TypeBase& dst)const{
		bool &dstVal=dst.cast_to_Type<bool>();
		const char *srcVal=((std::string)src.cast_to_Type<std::string>()).c_str();
		if(strcasecmp (srcVal, "TRUE") ==0 or strcasecmp (srcVal, "YES") ==0){
			dstVal=true;
		} else if(strcasecmp (srcVal, "FALSE") ==0 or strcasecmp (srcVal, "NO") ==0){
			dstVal=false;
		}
		LOG(Runtime,error) << src.toString(true) << " is ambiguous while converting to " << Type<bool>::staticName();
	}
	virtual ~TypeConverter(){}
};
template<> class TypeConverter<false,false,bool,std::string> : public TypeGenerator<bool,std::string>{
	TypeConverter(){
		LOG(Debug,verbose_info)
		<< "Creating special to-string converter for " << Type<bool>::staticName();
	};
public:
	static boost::shared_ptr<const TypeConverterBase> create(){
		TypeConverter<false,false,bool,std::string> *ret=new TypeConverter<false,false,bool,std::string>;
		return boost::shared_ptr<const TypeConverterBase>(ret);
	}
	void convert(const TypeBase& src, TypeBase& dst)const{
		std::string &dstVal=dst.cast_to_Type<std::string>();
		const bool &srcVal=src.cast_to_Type<bool>();
		dstVal= srcVal ? "true":"false";
	}
	virtual ~TypeConverter(){}
};

	
/////////////////////////////////////////////////////////////////////////////
// string => list/vector version -- uses util::string2list
/////////////////////////////////////////////////////////////////////////////
template<typename DST> class TypeConverter<false,false,std::string,std::list<DST> >:public TypeGenerator<std::string,std::list<DST> >{ //string => list
	TypeConverter(){
		LOG(Debug,verbose_info)
		<< "Creating from-string converter for " << Type<std::list<DST> >::staticName();
	};
public:
	static boost::shared_ptr<const TypeConverterBase> create(){
		TypeConverter<false,false,std::string,std::list<DST> > *ret=new TypeConverter<false,false,std::string,std::list<DST> >;
		return boost::shared_ptr<const TypeConverterBase>(ret);
	}
	void convert(const TypeBase& src, TypeBase& dst)const{
		std::list<DST> &dstVal=dst.cast_to_Type<std::list<DST> >();
		LOG_IF(not dstVal.empty(),CoreLog,warning)
			<< "Storing into non empty list while conversion from "
			<< Type<std::string>::staticName() << " to " << Type<std::list<DST> >::staticName();
		const std::string &srcVal=src.cast_to_Type<std::string>();
		const std::list<DST> buff=util::string2list<DST>(srcVal,boost::regex("[\\s,;]+"));
		std::copy(buff.begin(),buff.end(),dstVal.end());
	}
	virtual ~TypeConverter(){}
};
template<typename DST> class TypeConverter<false,false,std::string,vector4<DST> >:public TypeGenerator<std::string,vector4<DST> >{ //string => vector4
TypeConverter(){
	LOG(Debug,verbose_info)
		<< "Creating from-string converter for " << Type<vector4<DST> >::staticName();
};
public:
	static boost::shared_ptr<const TypeConverterBase> create(){
		TypeConverter<false,false,std::string,vector4<DST> > *ret=new TypeConverter<false,false,std::string,vector4<DST> >;
		return boost::shared_ptr<const TypeConverterBase>(ret);
	}
	void convert(const TypeBase& src, TypeBase& dst)const{
		vector4<DST> &dstVal=dst.cast_to_Type<vector4<DST> >();
		const std::string &srcVal=src.cast_to_Type<std::string>();
		const std::list<DST> buff=string2list<DST>(srcVal,boost::regex("[\\s,;]+"));
		dstVal.copyFrom(buff.begin(),buff.end());
	}
	virtual ~TypeConverter(){}
};


// @todo we cannot parse this stuff yet
template<> class TypeConverter<false,false,std::string,PropMap>: public TypeGenerator<std::string,PropMap>{ //string => PropMap
public:
	virtual ~TypeConverter(){}
};
template<> class TypeConverter<false,false,std::string,rgb_color24 >:public TypeGenerator<std::string,rgb_color24 >{ //string => color
	public:
		virtual ~TypeConverter(){}
};
template<> class TypeConverter<false,false,std::string,rgb_color48 >:public TypeGenerator<std::string,rgb_color48 >{ //string => color
	public:
		virtual ~TypeConverter(){}
};

////////////////////////////////////////////////////////////////////////
//OK, thats about the foreplay. Now we get to the dirty stuff.
////////////////////////////////////////////////////////////////////////

	
///generate a TypeConverter for conversions from SRC to any type from the "types" list
template<typename SRC> struct inner_TypeConverter {
	std::map<int, boost::shared_ptr<const TypeConverterBase> > &m_subMap;
	inner_TypeConverter(std::map<int, boost::shared_ptr<const TypeConverterBase> > &subMap):m_subMap(subMap){}
	template<typename DST> void operator()(DST){ //will be called by the mpl::for_each in outer_TypeConverter for any DST out of "types"
		//create a converter based on the type traits and the types of SRC and DST
		typedef boost::mpl::and_<boost::is_arithmetic<SRC>,boost::is_arithmetic<DST> > is_num;
		typedef boost::is_same<SRC,DST> is_same;
		boost::shared_ptr<const TypeConverterBase> conv=
			TypeConverter<is_num::value,is_same::value,SRC,DST>::create();
		//and insert it into the to-conversion-map of SRC
		m_subMap.insert(m_subMap.end(),std::make_pair(Type<DST>::staticID,conv));
	}
};

///generate a TypeConverter for conversions from any SRC from the "types" list
struct outer_TypeConverter {
	std::map< int ,std::map<int, boost::shared_ptr<const TypeConverterBase> > > &m_map;
	outer_TypeConverter(std::map< int ,std::map<int, boost::shared_ptr<const TypeConverterBase> > > &map):m_map(map){}
	template<typename SRC> void operator()(SRC){//will be called by the mpl::for_each in TypeConverterMap() for any SRC out of "types"
		boost::mpl::for_each<types>(// create a functor for from-SRC-conversion and call its ()-operator for any DST out of "types"
			inner_TypeConverter<SRC>(m_map[Type<SRC>().typeID()])
		);
	}
};
/// @endcond

TypeConverterMap::TypeConverterMap()
{
	boost::mpl::for_each<types>(outer_TypeConverter(*this));
	LOG(Debug,info) << "conversion map for " << size() << " types created";
}

}}}