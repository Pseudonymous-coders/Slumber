#include <map>
#include <iostream>
#include <sstream>
#include <string>

#include <dbus-c++/dbus.h>
#include "Translate-glue.h"

static const char* TRANSLATE_SERVICE_NAME = "org.apertium.mode";
static const char* TRANSLATE_OBJECT_PATH = "/";

class Translate
: public org::apertium::Translate,
  public DBus::IntrospectableProxy,
  public DBus::ObjectProxy
{
public:
    Translate(DBus::Connection& connection, const char* path, const char* name)
    : DBus::ObjectProxy(connection, path, name) {
    }
};

DBus::BusDispatcher dispatcher;

int main(int argc, char** argv) {
    DBus::default_dispatcher = &dispatcher;
    DBus::Connection bus = DBus::Connection::SessionBus();

    std::stringstream input;
    std::string mode;
    std::map< ::DBus::String, ::DBus::String> translate_options;

    if (argc != 2) {
        std::cerr << "usage: dbus_test <mode>" << std::endl;
        return 1;
    }

    mode = std::string(argv[1]);

    while (!std::cin.eof() and std::cin) {
        std::string str;
        std::cin >> str;
        input << str << " ";
    }

    Translate translator(bus, TRANSLATE_OBJECT_PATH, TRANSLATE_SERVICE_NAME);
    std::cout << translator.translate(mode, translate_options, input.str()) 
              << std::endl;
}
