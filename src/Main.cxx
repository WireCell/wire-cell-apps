/** Single-point main entry to Wire Cell Toolkit.
 */

#include "WireCellApps/Main.h"

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

Main::Main()
{
}

Main::~Main()
{
}



int Main::cmdline(int argc, char* argv[])
{
    po::options_description desc("Options");
    desc.add_options()
	("help,h", "wire-cell [options] [arguments]")
	("app,a", po::value< vector<string> >(),"application component to invoke")
	("config,c", po::value< vector<string> >(),"provide a configuration file")
	("plugin,p", po::value< vector<string> >(),"specify a plugin as name[:lib]")
//	("jsonpath,j", po::value< vector<string> >(),"specify a JSON path=value")
	("ext-str,V", po::value< vector<string> >(),"specify a Jsonnet external variable=value")
	("path,P", po::value< vector<string> >(),"add to JSON/Jsonnet search path")
    ;    

    po::variables_map opts;
    po::store(po::parse_command_line(argc, argv, desc), opts);
    po::notify(opts);    

    if (opts.count("help")) {
	cerr << desc << "\n";
	return 1;
    }

    if (opts.count("config")) {
        for (auto fname : opts["config"].as< vector<string> >()) {
            add_config(fname);
        }
    }

    if (opts.count("path")) {
        for (auto path : opts["path"].as< vector<string> >()) {
            add_path(path);
        }
    }

    // Get any external variables
    if (opts.count("ext-str")) {
        for (auto vev : opts["ext-str"].as< vector<string> >()) {
            auto vv = String::split(vev, "=");
            add_var(vv[0], vv[1]);
        }
    }
    // fixme: these aren't yet supported.
    // if (opts.count("jsonpath")) { 
    //     jsonpath_vars = opts["jsonpath"].as< vector<string> >();
    // }


    if (opts.count("plugin")) {
        for (auto plugin : opts["plugin"].as< vector<string> >()) {
            add_plugin(plugin);
        }
    }
    if (opts.count("app")) {
        for (auto app : opts["app"].as< vector<string> >()) {
            add_app(app);
        }
    }

    return 0;
}


void Main::add_plugin(const std::string& libname)
{
    m_plugins.push_back(libname);
}

void Main::add_app(const std::string& tn)
{
    m_apps.push_back(tn);
}

void Main::add_config(const std::string& filename)
{
    m_cfgfiles.push_back(filename);
}

void Main::add_var(const std::string& name, const std::string& value)
{
    m_extvars[name] = value;
}

void Main::add_path(const std::string& dirname)
{
    cerr << "Main::add_path not implemented\n";
    // fixme: how to do this?  use setenv()?  
}


void Main::initialize()
{
    for (auto filename : m_cfgfiles) {
        cerr << "Loading config: " << filename << " ...\n";
        Json::Value one;
        one = Persist::load(filename, m_extvars); // throws
        m_cfgmgr.extend(one);
        cerr << "...done\n";
    }


    // Find if we have our special configuration entry
    int ind = m_cfgmgr.index("wire-cell");
    Configuration main_cfg = m_cfgmgr.pop(ind);
    if (! main_cfg.isNull()) {
        for (auto plugin : get< vector<string> >(main_cfg, "data.plugins")) {
            cerr << "Config requests plugin: \"" << plugin << "\"\n";
            m_plugins.push_back(plugin);
        }
        for (auto app : get< vector<string> >(main_cfg, "data.apps")) {
            cerr << "Config requests app: \"" << app << "\"\n";
            m_apps.push_back(app);
        }
    }


    // Load any plugin shared libraries requested by user.
    PluginManager& pm = PluginManager::instance();
    for (auto plugin : m_plugins) {
	string pname, lname;
	std::tie(pname, lname) = String::parse_pair(plugin);
	cerr << "Adding plugin: " << plugin;
	if (lname.size()) {
	    cerr << " from library " << lname;
	}
	cerr << endl;
        pm.add(pname, lname);
    }


    // Apply any user configuration.  This is a two step.  First, just
    // assure all the components referenced in the configuration
    // sequence can be instantiated.  Then, find them again and
    // actually configure them.  This way, any problems fails fast.

    for (auto c : m_cfgmgr.all()) {
        if (c.isNull()) {
            continue;           // allow and ignore any totally empty configurations
        }
        if (c["type"].isNull()) {
            cerr << "All configuration must have a type attribute, got:\n"
                 << c << endl;
            THROW(ValueError() << errmsg{"got configuration sequence element lacking a type"});
        }
	string type = get<string>(c, "type");
	string name = get<string>(c, "name");
        cerr << "Construct component: \"" << type << ":" << name << "\"\n";
	auto cfgobj = Factory::lookup<IConfigurable>(type, name); // throws 
    }
    for (auto c : m_cfgmgr.all()) {
        if (c.isNull()) {
            continue;           // allow and ignore any totally empty configurations
        }
	string type = get<string>(c, "type");
	string name = get<string>(c, "name");
        cerr << "Configuring component: \"" << type << ":" << name << "\"\n";
	auto cfgobj = Factory::find<IConfigurable>(type, name); // throws 

        // Get component's hard-coded default config, update it with
        // anything the user may have provided and apply it.
	Configuration cfg = cfgobj->default_configuration();
	cfg = update(cfg, c["data"]);
        cfgobj->configure(cfg); // throws
    }
}

void Main::operator()()
{
    // Find all IApplications to execute
    vector<IApplication::pointer> app_objs;
    for (auto component : m_apps) {
	string type, name;
        std::tie(type,name) = String::parse_pair(component);
        auto a = Factory::find<IApplication>(type,name); // throws
        app_objs.push_back(a);
    }
    cerr << "Executing " << m_apps.size() << " apps:\n";
    for (size_t ind=0; ind < m_apps.size(); ++ind) {
	auto aobj = app_objs[ind];
	cerr << "Executing app: " << m_apps[ind] << endl;
	aobj->execute();        // throws
    }
}


