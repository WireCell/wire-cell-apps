#include "WireCellIface/IWireParameters.h"
#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/Testing.h"

void test_load_something()
{
    auto factory = WireCell::Factory::lookup_factory<WireCell::IWireParameters>("WireParams");
    Assert(factory);

    auto wp = WireCell::Factory::lookup<WireCell::IWireParameters>("WireParams");
    Assert(wp);

}


int main()
{
    test_load_something();
}
