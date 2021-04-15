#include <boost/ut.hpp>

int main(int argc, char *argv[])
{
    using namespace boost::ut;
    cfg<override> = {.filter = argc > 1 ? argv[1] : ""};
    expect(1_i == 1);
    return 0;
}
