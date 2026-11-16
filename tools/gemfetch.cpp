// gemfetch <url> [known_hosts]: fetch one Gemini URL with padshell's client
// and print the status, meta and body. Desktop testing aid.
#include <cstdio>

#include "../src/gemini/client.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: gemfetch <url> [known_hosts]\n");
        return 2;
    }
    gem::Url url = gem::Url::parse(argv[1]);
    gem::Response r = gem::fetch(url, argc > 2 ? argv[2] : "/tmp/gemfetch_known_hosts");
    if (r.error != gem::Response::Error::None) {
        std::printf("error %d: %s\n", (int)r.error, r.errorText.c_str());
        return 1;
    }
    std::printf("status %d meta \"%s\" cert %s body %zu bytes\n%s", r.status, r.meta.c_str(), r.fingerprint.c_str(),
                r.body.size(), r.body.c_str());
    return 0;
}
