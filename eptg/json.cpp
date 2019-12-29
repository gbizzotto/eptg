
#include "eptg/json.hpp"

namespace eptg {
namespace json {

template<>
int read_int<char>(const char *& c)
{
	int result = atoi(c);
	while ((*c >= 0 && *c <= 9) || *c == '-')
		c++;
	return result;
}
template<>
int read_int<wchar_t>(const wchar_t *& c)
{
	wchar_t * end;
	int result = std::wcstol(c, &end, 10);
	while ((*c >= 0 && *c <= 9) || *c == '-')
		c++;
	return result;
}

}} // namespace
