/** A general command line Wire Cell application. 
 *
 */

#include "WireCellUtil/Configuration.h"
#include "WireCellUtil/PluginManager.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellIface/IConfigurable.h"

#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>

#include <string>
#include <vector>
#include <iostream>

using namespace WireCell;
using namespace std;
namespace po = boost::program_options;
using namespace boost::algorithm;
using namespace boost::property_tree;


int main(int argc, char* argv[])
{
    po::options_description desc("Options");
    desc.add_options()
	("help", "wire-cell [options] [argments]")
	("config,c", po::value< vector<string> >(),"set configuration file")
	("plugin,p", po::value< vector<string> >(),"specify a plugin as name:lib")
	//("component,C", po::value< vector<string> >(),"specify a component")
	("default,d", po::value< vector<string> >(),"dump default configuration of given component class")
	("default-output,D", po::value< string >(),"dump defaults to a file")
    ;    

    po::variables_map opts;
    po::store(po::parse_command_line(argc, argv, desc), opts);
    po::notify(opts);    

    if (opts.count("help")) {
	cout << desc << "\n";
	return 1;
    }

    // load JSON into Configuration
    Configuration config;
    if (opts.count("config")) {
	auto filenames = opts["config"].as< vector<string> >();
	for (auto filename : filenames) {
	    cout << "Loading config: " << filename << endl;
	    Configuration more = configuration_load(filename);
	    update(config, more);
	}
    }
    cerr << "Loaded config: " << configuration_dumps(config) << endl;


    // plugins from config file and cmdline
    vector<string> plugins = get< vector<string> >(config, "wire-cell.plugins");
    if (opts.count("plugin")) {
	auto plv = opts["plugin"].as< vector<string> >();
	plugins.insert(plugins.end(),plv.begin(),plv.end());
    }
    PluginManager& pm = PluginManager::instance();
    for (auto plugin : plugins) {
	string libname = "";
	string::size_type colon = plugin.find(":");
	if (colon != string::npos) {
	    libname = plugin.substr(colon+1, plugin.size()-colon);
	    plugin = plugin.substr(0,colon);
	}
	cout << "Loading plugin: " << plugin;
	if (libname.size()) {
	    cout << " from library " << libname;
	}
	cout << endl;
	
	pm.add(plugin, libname);
    }


    if (opts.count("default")) {
	Configuration top;
	int failed_component_lookup = 0;
	for (auto component : opts["default"].as<vector <string> >()) {
	    string compclass = component;
	    string compname = "";
	    string::size_type colon = component.find(":");
	    if (colon != string::npos) {
		compname = component.substr(colon+1, component.size()-colon);
		compclass = component.substr(0,colon);
	    }
	    auto cfgobj = Factory::lookup<IConfigurable>(compclass,compname);
	    if (!cfgobj) {
		cerr << "Failed lookup component \"" << component << "\"\n";
		++failed_component_lookup;
		continue;
	    }
	    Configuration cfg = cfgobj->default_configuration();
	    Configuration inst;
	    inst[compname] = cfg;
	    top[compclass] = inst;
	}
	if (failed_component_lookup) { return 1; }

	string filename = "/dev/stdout";
	if (opts.count("default-output")) {
	    filename = opts["default-output"].as<string>();
	}
	configuration_dump(filename, top);
    }
    

    /// now run something!

    return 0;
}
