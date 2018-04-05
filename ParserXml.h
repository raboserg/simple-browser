
#include <boost/cast.hpp>
#include <boost/foreach.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/serialization/singleton.hpp>

//typedef boost::gregorian::date Date;

//struct Signal1
//{
//	unsigned     id;
//	std::string  currency;
// Date         dateOpen;
//	Date         dateClose;
//	std::string  type;
//	double       price;
//	double       tp;
//	double       sl;
//};

struct Signal
{
	int id;
	std::string  currency;
    std::string  dateOpen;
	std::string  dateClose;
	std::string  type;
	std::string  price;
	std::string  tp;
	std::string  sl;
};

typedef std::vector<Signal> Signals;
typedef std::vector<std::wstring> SignalsLine;
typedef std::vector<std::string> SignalsLineChar;

class ParserXml : public boost::serialization::singleton<ParserXml>
{
	SignalsLineChar signalsLine;
	void clear(){signalsLine.resize(0);}
	//signalsLine.clear();
public:
	
	int getSignalCount(){return signalsLine.size();}
	
	SignalsLineChar getSignals(){return signalsLine;}
	
	void createSignalCharStrings(std::stringstream& ss)
	{
		clear();
		boost::property_tree::basic_ptree<std::string, std::string> pt;
		boost::property_tree::xml_parser::read_xml(ss, pt);
		BOOST_FOREACH(boost::property_tree::ptree::value_type const& v, pt.get_child("signals")) {
			if( v.first == "signal" ) {
				std::string lineSignal;
				lineSignal =  v.second.get<std::string>("id") + ",";
				lineSignal += v.second.get<std::string>("currency") + ",";
				lineSignal += v.second.get<std::string>("time-open") + ",";
				lineSignal += v.second.get<std::string>("time-close") + ",";
				lineSignal += v.second.get<std::string>("type") + ",";
				lineSignal += v.second.get<std::string>("price") + ",";
				lineSignal += v.second.get<std::string>("tp") + ",";
				lineSignal += v.second.get<std::string>("sl");
				signalsLine.push_back(lineSignal);
			}
		 }
	}

	SignalsLine getSignalStrings(std::wstringstream& ss){
		SignalsLine signalsLine;
		//boost::property_tree::basic_ptree<std::wstring, std::wstring> pt;
		using boost::property_tree::wptree;
		wptree pt;
		read_xml(ss, pt);
		//boost::property_tree::xml_parser::read_xml(ss, pt); 
		BOOST_FOREACH(wptree::value_type const& v, pt.get_child(L"signals")) {
			if( v.first == L"signal" ) {
				std::wstring lineSignal;
				lineSignal = v.second.get<std::wstring>(L"id") + L",";
				lineSignal += v.second.get<std::wstring>(L"currency") + L",";
				lineSignal += v.second.get<std::wstring>(L"time-open") + L",";
				lineSignal += v.second.get<std::wstring>(L"time-close") + L",";
				lineSignal += v.second.get<std::wstring>(L"type") + L",";
				lineSignal += v.second.get<std::wstring>(L"price") + L",";
				lineSignal += v.second.get<std::wstring>(L"tp") + L",";
				lineSignal += v.second.get<std::wstring>(L"sl");
				signalsLine.push_back(lineSignal);
			}
		 }
		return signalsLine;
	}

	void getSignalsVector(const std::string& response, Signals& signals){
		std::stringstream ss(response); 
		using boost::property_tree::ptree;
		ptree pt;
		read_xml(ss, pt);
		BOOST_FOREACH(ptree::value_type const& v, pt.get_child("signals")) {
			if( v.first == "signal" ) {
				Signal signal;
				signal.id = v.second.get<int>("id");
				signal.currency = v.second.get<std::string>("currency");
				signal.dateOpen = v.second.get<std::string>("time-open");
				signal.dateClose = v.second.get<std::string>("time-close");
				signal.type = v.second.get<std::string>("type");
				signal.price = v.second.get<std::string>("price");
				signal.tp = v.second.get<std::string>("tp");
				signal.sl = v.second.get<std::string>("sl");
				signals.push_back(signal);
			}
		 }
	}
};