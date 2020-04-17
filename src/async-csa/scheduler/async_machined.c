#define _GNU_SOURCE

#include <stdlib.h>
#include <sys/socket.h>

#include "bus-message.h"
#include "fileio.h"
#include "image-dbus.h"
#include "machined.h"
#include "machine-image.h"
#include "sd-bus.h"

int main(int argc, char *argv[]) {
    Image* image = NULL;
    image_new(IMAGE_SUBVOLUME, "pretty", "path", "filename", /*read_only=*/true, /*crtime=*/0, /*mtime=*/0, &image);

    size_t size;
    sd_bus* bus = NULL;
    sd_bus_message* message = NULL;
    char* buffer = NULL;
    int r;

    // Generate a sd_bus_message
    const char* fname = "test.txt";
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

    const sd_bus_error bus_error = SD_BUS_ERROR_NULL;
    bus_image_method_clone(message, image, &bus_error);
}