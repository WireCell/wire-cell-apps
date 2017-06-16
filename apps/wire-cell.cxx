/** A general command line Wire Cell application. 
 *
 */

#include "WireCellUtil/ConfigManager.h"
#include "WireCellUtil/PluginManager.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Persist.h"
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
//	("jsonpath,j", po::value< vector<string> >(),"specify a JSON path=value")
	("ext-str,V", po::value< vector<string> >(),"specify a Jsonnet external variable=value")
	("jpath,J", po::value< vector<string> >(),"add to JSON/Jsonnet search path")
	("output-config,C", po::value<string>(),"output as-configured info to given file")
	("dump-default,D", po::value< vector<string> >(),"dump the default configuration of a given component")
        ("dump-file", po::value<string>()->default_value("/dev/stderr"), "dump to this file")

    ;    

    po::variables_map opts;
    po::store(po::parse_command_line(argc, argv, desc), opts);
    po::notify(opts);    

    if (opts.count("help")) {
	cerr << desc << "\n";
	return 1;
    }

    // Get any external variables
    Persist::externalvars_t jsonnet_vars, jsonpath_vars;
    if (opts.count("ext-str")) {
        for (auto vev : opts["ext-str"].as< vector<string> >()) {
            auto vv = String::split(vev, "=");
            jsonnet_vars[vv[0]] = vv[1];
        }
    }
    // fixme: these aren't yet supported.
    // if (opts.count("jsonpath")) { 
    //     jsonpath_vars = opts["jsonpath"].as< vector<string> >();
    // }

    // load JSON into Configuration
    ConfigManager cfgmgr;
    if (opts.count("config")) {
	auto filenames = opts["config"].as< vector<string> >();
	cerr << "Have " << filenames.size() <<  " configuration files\n";
	for (auto filename : filenames) {
	    cerr << "Loading config: " << filename << " ...\n";
            Json::Value one;
            try {
                one = Persist::load(filename, jsonnet_vars);
            }
            catch (Exception& e) {
                cerr << "Failed to load " << filename << endl;
                cerr << errstr(e) << endl;
                return 1;
            }
	    cfgmgr.extend(one);
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

    
    // Load any plugin shared libraries requested by user.
    PluginManager& pm = PluginManager::instance();
    for (auto plugin : plugins) {
	string pname, lname;
	std::tie(pname, lname) = String::parse_pair(plugin);
	cerr << "Adding plugin: " << plugin;
	if (lname.size()) {
	    cerr << " from library " << lname;
	}
	cerr << endl;
	auto ok = pm.add(pname, lname);
	if (!ok) return 1;
    }


    // Apply any user configuration.
    for (auto c : cfgmgr.all()) {
        if (c.isNull()) {
            continue;           // allow and ignore any totally empty configurations
        }
        if (c["type"].isNull()) {
            cerr << "All configuration must have a type attribute, got:\n"
                 << c << endl;
            return 1;
        }

	string type = get<string>(c, "type");
	string name = get<string>(c, "name");
        cerr << "Configuring component: \"" << type << ":" << name << "\"\n";
	auto cfgobj = Factory::lookup<IConfigurable>(type, name); // throws 
	Configuration cfg = cfgobj->default_configuration();
	cfg = update(cfg, c["data"]);
        try {
            cfgobj->configure(cfg);
        }
        catch (Exception& e) {
            cerr << "Failed to configure component \"" << type << ":" << name << "\"\n";
            cerr << errstr(e) << endl;
            return 1;
        }
    }


    // As side effect, dump default configuration of given configurables.
    if (opts.count("dump-default")) {
        ConfigManager defcm;
        auto todump = opts["dump-default"].as< vector<string> >();
        for (auto type : todump) {
            auto obj = Factory::lookup<IConfigurable>(type);
            Configuration cfg = obj->default_configuration();
            defcm.add(cfg, type);
        }
        std::string dump_file = opts["dump-file"].as<string>();
        cerr << "Dumping to: " << dump_file << endl;
        Persist::dump(dump_file, defcm.all(), true);
    }

    // As a side effect, dump actual configuration
    if (opts.count("output-config")) {
        std::string fname = opts["output-config"].as<string>();
        cerr << "Outputing config to: " << fname << endl;
        Persist::dump(fname, cfgmgr.all(), true);
    }

    // run any apps
    vector<IApplication::pointer> app_objs;
    for (auto component : apps) {
	string type, name;
        std::tie(type,name) = String::parse_pair(component);
        auto a = Factory::lookup<IApplication>(type,name);
        app_objs.push_back(a);
    }
    cerr << "Executing " << apps.size() << " apps:\n";
    for (size_t ind=0; ind<apps.size(); ++ind) {
	auto aobj = app_objs[ind];
	cerr << "Executing app: " << apps[ind] << endl;
	aobj->execute();
    }

    
    return 0;
}
