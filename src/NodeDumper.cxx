#include "WireCellApps/NodeDumper.h"
#include "WireCellUtil/String.h"
#include "WireCellUtil/Type.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Configuration.h"
#include "WireCellUtil/Persist.h"
#include "WireCellIface/INode.h"

WIRECELL_FACTORY(NodeDumper, WireCellApps::NodeDumper, WireCell::IApplication, WireCell::IConfigurable);


using namespace std;
using namespace WireCell;
using namespace WireCellApps;


NodeDumper::NodeDumper()
    : m_cfg(default_configuration())
{
}

NodeDumper::~NodeDumper()
{
}

void NodeDumper::configure(const Configuration& config)
{
    m_cfg = config;
}

WireCell::Configuration NodeDumper::default_configuration() const
{
    // yo dawg, I heard you liked dumping so I made a dumper that dumps the dumper.
    std::string json = R"({
"filename":"/dev/stdout",
"nodes":["WireSource","BoundCells","Diffuser","Digitizer","Drifter","Framer","PlaneDuctor","PlaneSliceMerger",
         "Tiling","TrackDepos","WireGenerator","WireSummarizer","ChannelCellSelector","CellSliceSink"]
})";

    return Persist::loads(json);
}


void NodeDumper::execute()
{
    Configuration all;

    int nnodes = m_cfg["nodes"].size();
    std::vector<std::string> types;
    if (0 == nnodes) {
	types = Factory::known_classes<INode>();
	nnodes = types.size();
	cerr << "Dumping all known node classes ("<<nnodes<<")\n";
    }
    else {
	cerr << "Dumping: ("<<nnodes<<")\n";
	for (auto type_cfg : m_cfg["nodes"]) {
	    std::string type = convert<string>(type_cfg);
	    types.push_back(type);
	    cerr << "\t" << type << endl;
	}
    }


    for (auto type : types) {

	INode::pointer node;
	try {
	    node = Factory::lookup<INode>(type);
	}
	catch (FactoryException& fe) {
	    cerr << "NodeDumper: Failed lookup node: \"" << type << "\"\n";
	    continue;
	}

	Configuration one;
	one["type"] = type;
	for (auto intype : node->input_types()) {
	    one["input_types"].append(demangle(intype));
	}
	for (auto intype : node->output_types()) {
	    one["output_types"].append(demangle(intype));
	}
	one["concurrency"] = node->concurrency();
	one["category"] = node->category();
	
	all.append(one);
    }

    Persist::dump(get<string>(m_cfg, "filename"), all);
}




