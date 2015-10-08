#include "WireCellIface/IWireParameters.h"

#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Testing.h"

#include <dlfcn.h>

#include <iostream>
using namespace std;
using namespace WireCell;

int main()
{
    void* library = dlopen("libWireCellGen.so", RTLD_NOW);
    Assert(library);
    
    void* thing = dlsym(library, "force_link_WireParams");
    Assert(thing);

    int* number = reinterpret_cast<int*>(thing);
    cerr << "Number is " << *number << endl;

    void* reg_fun_ptr = dlsym(library, "register_WireParams_Factory");
    Assert(reg_fun_ptr);

    cerr << "Registering WireParams" << endl;
    typedef void (*registration_function)();
    registration_function regfun = reinterpret_cast<registration_function>(reg_fun_ptr);
    regfun();

    // void* see_ptr = dlsym(library, "see_WireParams_force_link");
    // Assert(see_ptr);
    // typedef int (*see_force_link)();
    // see_force_link see_func = reinterpret_cast<see_force_link>(see_ptr);
    // cerr << see_func() << endl;

    typedef NamedFactoryRegistry<IWireParameters> IWPFactoryRegistry;
    IWPFactoryRegistry* iwp1 = &Singleton< IWPFactoryRegistry >::Instance();
    IWPFactoryRegistry* iwp2 = &Singleton< IWPFactoryRegistry >::Instance();
    Assert(iwp1);
    Assert(iwp2);
    Assert(iwp1 == iwp2);

    cerr << "Looking up WireParams factory in NFR: " << iwp1 << endl;
    auto inst1 = iwp1->lookup_factory("WireParams");
    auto inst2 = iwp1->lookup_factory("WireParams");

    Assert(inst1);
    Assert(inst2);
    Assert(inst1 == inst2);

    auto factory = Factory::lookup_factory<IWireParameters>("WireParams");
    Assert(factory);

    auto wp1 = Factory::lookup<IWireParameters>("WireParams");
    Assert(wp1);
    auto wp2 = Factory::lookup<IWireParameters>("WireParams");
    Assert(wp2);
    auto wp3 = Factory::lookup<IWireParameters>("WireParams","MyWireParameters");
    Assert(wp3);

    Assert(wp2 == wp1);
    Assert(wp3 != wp1);

    return 0;
}


