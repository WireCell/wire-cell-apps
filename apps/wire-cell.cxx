/** A general command line Wire Cell application. 
 *
 */

#include "WireCellUtil/ConfigManager.h"
#include "WireCellUtil/PluginManager.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/String.h"
#include "WireCellUtil/Point.h"

#include "WireCellIface/IConfigurable.h"
#include "WireCellIface/IApplication.h"

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
	("help,h", "wire-cell [options] [arguments]")
	("app,a", po::value< vector<string> >(),"application component to invoke")
	("config,c", po::value< vector<string> >(),"provide a configuration file")
	("plugin,p", po::value< vector<string> >(),"specify a plugin as name[:lib]")
	//("component,C", po::value< vector<string> >(),"specify a component")
	//("default,d", po::value< vector<string> >(),"dump default configuration of a component")
	//("default-output,D", po::value< string >(),"dump defaults to a file")
    ;    

    po::variables_map opts;
    po::store(po::parse_command_line(argc, argv, desc), opts);
    po::notify(opts);    

    if (opts.count("help")) {
	cerr << desc << "\n";
	return 1;
    }

    // load JSON into Configuration
    ConfigManager cfgmgr;
    if (opts.count("config")) {
	auto filenames = opts["config"].as< vector<string> >();
	cerr << "Have " << filenames.size() <<  " configuration files\n";
	for (auto filename : filenames) {
	    cerr << "Loading config: " << filename << "...\n";
	    cfgmgr.load(filename);
	    cerr << "...done\n";
	}
    }
    //cerr << "Loaded config:\n" << cfgmgr.dumps() << endl;

    int ind = cfgmgr.index("wire-cell");
    Configuration main_cfg = cfgmgr.pop(ind);

    // plugins and apps from config file and cmdline
    vector<string> plugins, apps;
    if (! main_cfg.isNull()) {
	plugins = get< vector<string> >(main_cfg, "data.plugins");
	apps = get< vector<string> >(main_cfg, "data.apps");
    }
    if (opts.count("plugin")) {
	auto plv = opts["plugin"].as< vector<string> >();
	plugins.insert(plugins.end(),plv.begin(),plv.end());
    }
    if (opts.count("app")) {
	auto av = opts["app"].as< vector<string> >();
	apps.insert(apps.end(),av.begin(),av.end());
    }

    
    PluginManager& pm = PluginManager::instance();
    for (auto plugin : plugins) {
	string pname, lname;
	std::tie(pname, lname) = parse_pair(plugin);
	cerr << "Adding plugin: " << plugin;
	if (lname.size()) {
	    cerr << " from library " << lname;
	}
	cerr << endl;
	auto ok = pm.add(pname, lname);
	if (!ok) return 1;
    }

    // apply configuration to all configurables
    for (auto c : cfgmgr.all()) {
	string type = get<string>(c, "type");
	string name = get<string>(c, "name");

	auto cfgobj = Factory::lookup<IConfigurable>(type, name); // throws 
	Configuration cfg = cfgobj->default_configuration();
	cfg = update(cfg, c["data"]);
	cfgobj->configure(cfg);
    }


    // run any apps
    vector<IApplication::pointer> app_objs;
    for (auto component : apps) {
	string type, name;
	    std::tie(type,name) = parse_pair(component);
	    auto a = Factory::lookup<IApplication>(type,name);
	    app_objs.push_back(a);
    }
    cerr << "Executing " << apps.size() << " apps:\n";
    for (int ind=0; ind<apps.size(); ++ind) {
	auto aobj = app_objs[ind];
	cerr << "Executing app: " << apps[ind] << endl;
	aobj->execute();
    }

    
    return 0;
}
