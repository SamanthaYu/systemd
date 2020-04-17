#define _GNU_SOURCE

#include <stdlib.h>
#include <sys/socket.h>

#include "bus-message.h"
#include "fileio.h"
#include "image-dbus.h"
#include "machined.h"
#include "machine-image.h"
#include "sd-bus.h"

int main() {
    Image* image;
    sd_bus_error* error;

    size_t size;
    sd_bus* bus = NULL;
    sd_bus_message* message = NULL;
    char* buffer = NULL;
    int r;

    // Generate a sd_bus_message
    char* fname = "test.txt";
    r = read_full_file(fname, &buffer, &size);
    if (r < 0) {
            log_error_errno(r, "Failed to open '%s': %m", fname);
            return EXIT_FAILURE;
    }

    r = sd_bus_new(&bus);
    assert_se(r >= 0);

    r = bus_message_from_malloc(bus, buffer, size, NULL, 0, NULL, &message);
    if (r == -EBADMSG)
        return 0;
    assert_se(r >= 0);
    TAKE_PTR(buffer);

    bus_image_method_clone(message, image, error);
}