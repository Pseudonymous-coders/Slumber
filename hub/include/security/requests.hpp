#include <iostream>
#include <string>

#include <security/tokens.hpp>
#include <security/account.hpp>

//BOOST includes
#include <boost/optional.hpp>
#include <boost/thread.hpp>
#include <boost/chrono.hpp>

#include <cpprest/http_client.h> //Http/Rest client library
#include <cpprest/json.h> //JSON library
#include <cpprest/uri.h> //URI library
#include <cpprest/containerstream.h> //Async streams backed by STL containers
#include <cpprest/interopstream.h> //Async stream bridges
#include <cpprest/rawptrstream.h> //Async streams backed by raw pointer to memory
#include <cpprest/producerconsumerstream.h>  //Async streams for producer consumer scenarios

#include <util/config.hpp> //Global slumber definitions

#ifndef SLUMBER_REQUESTS_H
#define SLUMBER_REQUESTS_H

namespace Requests {

pplx::task<void> updateBandData(security::Account *account);

template<typename T>
void _Logger(const T &, const bool err = false);

}

#endif
