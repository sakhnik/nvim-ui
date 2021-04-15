#include <boost/ut.hpp>

#if _WIN32
#include <io.h>
#define ISATTY _isatty
#define FILENO _fileno
#else
#include <unistd.h>
#define ISATTY isatty
#define FILENO fileno
#endif

int main(int argc, char *argv[])
{
    bool is_tty = ISATTY(FILENO(stdout));

    using namespace boost::ut;
    cfg<override> = {
        .filter = argc > 1 ? argv[1] : "",
        .colors = {
            .none = is_tty ? "\033[0m" : "",
            .pass = is_tty ? "\033[32m" : "",
            .fail = is_tty ? "\033[31m" : "",
        },
        .dry_run = false,
    };
    expect(1_i == 1);
    return 0;
}
