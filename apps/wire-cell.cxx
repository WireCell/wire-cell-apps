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

template <typename T>
std::vector<T> as_vector(ptree const& pt, ptree::key_type const& key)
{
    cerr << "as_vector("<<key<<")"<<endl;
    std::vector<T> r;
    auto children = pt.get_child(key);
    for (auto item : children) {
	T val = item.second.get_value<T>();
        r.push_back(val);
	cerr << key << " : " << val << endl;
    }
    return r;
}

Configuration prefix_config(const std::string& prefix, Configuration cfg)
{
    Configuration ret;
    std::string withdot = prefix + ".";
    for (auto child : cfg) {
	std::string key = withdot+child.first;
	ptree val = child.second;
	cerr << key << endl;
	ret.put_child(key, val);
    }
    return ret;
}
Configuration lookup_config(const Configuration& top, const std::string& path,
			    Configuration defaults)
{
    Configuration params;
    try {
	params = top.get_child(path);
    }
    catch (ptree_bad_path) {
	return defaults;
    }
    ConfigurationMerge merger(defaults);
    merger(params);
    return defaults;
}

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
    ConfigurationMerge merger(config);
    if (opts.count("config")) {
	auto filenames = opts["config"].as< vector<string> >();
	for (auto filename : filenames) {
	    cout << "Loading config: " << filename << endl;
	    Configuration more = configuration_load(filename);
	    merger(more);
	}
    }
    cerr << "Loaded config: " << configuration_dumps(config) << endl;

    vector<string> plugins = as_vector<string>(config, "app.plugins");

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
	    Configuration cfg = lookup_config(config, component,
					      cfgobj->default_configuration());
	    cfgobj->configure(cfg);
	    ptree thisconfig = prefix_config(component, cfg);
	    cerr << configuration_dumps(thisconfig,"json") << endl;
	}
    }
    
    if (opts.count("dump-config")) {
	auto filename = opts["dump-config"].as<string>();
	string format = "";
	if (filename == "-") {
	    filename = "/dev/stdout";
	}
	configuration_dump(filename, config, format);
    }

    return 0;
}
