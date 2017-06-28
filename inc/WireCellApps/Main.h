/** This is the main entry point to the WCT.  It provides a single
 * command line method or optionally a family of fine-grained methods
 * for setup and running.
 *
 * One use of this is in the `wire-cell` command line program.  It may
 * also be use to embed WCT into a larger application or framework. */

#ifndef WIRECELL_MAIN
#define WIRECELL_MAIN

#include "WireCellUtil/ConfigManager.h"
#include "WireCellUtil/PluginManager.h"
#include "WireCellUtil/Persist.h"

#include <string>
#include <vector>



namespace WireCell {
    class Main {

    public:

        Main();
        ~Main();

        /// Single-point entry to Wire Cell.
        ///
        /// Pass in literal command line arguments and return a return
        /// code.  See `wire-cell --help` for args.
        ///
        /// Or, one can use subsequent methods for more fine-grained
        /// setup and execution.
        int cmdline(int argc, char* argv[]);


        /// Individual setup methods called by cmdline() or called
        /// explicitly by external application/frameork:

        /// Add an IApplication component to execute as a `type:name`
        /// string.
        void add_app(const std::string& tn);

        /// Append a top-level JSON/Jsonnet configuration file to the
        /// configuration sequence.
        void add_config(const std::string& filename);

        /// Add an external variable so that it may be referenced in
        /// the configuration files.
        void add_var(const std::string& name, const std::string& value);

        /// Add an element to the configuration path in which
        /// configuration files may be found.
        void add_path(const std::string& dirname);

        /// Add a plugin library in which components may be found.
        /// The libname may lack the initial "lib" prefix and file
        /// extension.
        void add_plugin(const std::string& libname);


        /// Call once after all setup has been done and before
        /// running.
        void initialize();

        /// Run any and all application components once.
        void operator()();


    private:
        ConfigManager m_cfgmgr;
        std::vector<std::string> m_plugins, m_apps, m_cfgfiles;
        Persist::externalvars_t m_extvars;
    };



}
#endif
