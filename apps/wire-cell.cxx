/** A general command line Wire Cell application. */

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
	("plugin,p", po::value< vector<string> >(),"specify a plugin")
	("component,C", po::value< vector<string> >(),"specify a component")
	("dump-config", po::value< string >(),"dump configuration to file")
    ;    

    po::variables_map opts;
    po::store(po::parse_command_line(argc, argv, desc), opts);
    po::notify(opts);    

    if (opts.count("help")) {
	cout << desc << "\n";
	return 1;
    }

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
    vector<string> plugins = get< vector<string> >(config, "app.plugins");
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

    if (opts.count("component")) {
	for (auto component : opts["component"].as<vector <string> >()) {
	    string compclass = component;
	    string compname = "";
	    string::size_type colon = component.find(":");
	    if (colon != string::npos) {
		compname = component.substr(colon+1, component.size()-colon);
		compclass = component.substr(0,colon);
	    }
	    auto cfgobj = Factory::lookup<IConfigurable>(compclass,compname);
	    if (!cfgobj) {
		cerr << "Failed lookup component " << compclass << ":" << compname << endl;
		return 1;
	    }
	    Configuration cfg = cfgobj->default_configuration();
	    Configuration more = branch(config, component);
	    update(cfg, more);
	    cfgobj->configure(cfg);
	    config[component] = cfg;
	    //cerr << "Configure " << component << "\n---\n" << cfg << "\n---\n" << endl;

	    //ptree thisconfig = prefix_config(component, cfg);
	    //cerr << configuration_dumps(thisconfig,"json") << endl;
	}
    }
    
    if (opts.count("dump-config")) {
	auto filename = opts["dump-config"].as<string>();
	string format = "";
	if (filename == "-") {
	    filename = "/dev/stdout";
	}
	cerr <<"Dumping configration:" << endl;
	configuration_dump(filename, config);
    }

    return 0;
}
