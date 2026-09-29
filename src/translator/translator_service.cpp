#include <iostream>
#include <unistd.h>
#include <android-base/logging.h>

int main(int argc, char** argv) {
    android::base::InitLogging(argv, android::base::LogdLogger(android::base::SYSTEM));

    LOG(INFO) << "Translator service skeleton starting up.";
    LOG(INFO) << "This is a minimal controlled environment. No translation models loaded yet.";

    // Run in a minimal loop so init doesn't restart it immediately
    while (true) {
        sleep(60);
    }

    return 0;
}
