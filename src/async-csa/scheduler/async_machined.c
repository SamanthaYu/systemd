#define _GNU_SOURCE

#include <stdlib.h>
#include <sys/socket.h>

#include "bus-message.h"
#include "bus-polkit.h"
#include "fileio.h"
#include "image-dbus.h"
#include "machined.h"
#include "machine-image.h"
#include "sd-bus.h"

int main(int argc, char *argv[]) {
    int r;

    // Generate a sd_bus
    sd_bus* bus = NULL;
    r = sd_bus_new(&bus);
    assert_se(r >= 0);

    // Read file into buffer
    const char* fname = "fname";
    char* buffer = NULL;
    size_t size;
    r = read_full_file(fname, &buffer, &size);
    if (r < 0) {
            log_error_errno(r, "Failed to open '%s': %m", fname);
            return EXIT_FAILURE;
    }

    // Generate sd_bus_message request and reply
    sd_bus_message* request_msg = NULL;
    r = bus_message_from_malloc(bus, buffer, size, NULL, 0, NULL, &request_msg);
    if (r == -EBADMSG)
        return 0;

    sd_bus_message* reply_msg = NULL;
    r = bus_message_from_malloc(bus, buffer, size, NULL, 0, NULL, &reply_msg);
    if (r == -EBADMSG)
        return 0;

    // Generate an image
    // TODO(samanthayu): Call the static function, image_new(), instead
    Image* image = NULL;
    r = image_find(IMAGE_MACHINE, "bus_label", &image);

    // 1. Call an sd_bus_table handler; e.g. bus_image_method_clone()
    // - Normally, these method handlers will trigger async_polkit_callback(), but we will call it directly
    sd_bus_error bus_error;
    bus_image_method_clone(request_msg, image, &bus_error);
    if (r == -ENOENT)
        return 0;
    if (r < 0)
        return r;

    // 2. Call image_object_find(), which will trigger image_flush_cache()
    // TODO(samanthayu): Generate the image_flush_cache() defer event from image_object_find()
    // - For now, we'll just have image_object_find() call image_flush_cache() directly
    Manager* manager = NULL;
    r = manager_new(&manager);
    if (r < 0)
        return r;

    Image* found_image;
    r = image_object_find(
        bus, /*path=*/"/org/freedesktop/machine1/image/test", "interface", (void*) manager, &found_image, &bus_error);
    if (r < 0)
        return r;

    // TODO(samanthayu): Fix checker to detect hashmap_clear()
    hashmap_clear(manager->polkit_registry);

    // 3. Call async_polkit_callback()
    // query->slot doesn't seem to be defined anywhere
    AsyncPolkitQuery query;
    query.request = request_msg;
    query.reply = reply_msg;
    query.callback = sd_bus_get_current_handler(request_msg->bus);
    if (!query.callback)
        return -EINVAL;
    query.userdata = sd_bus_get_current_userdata(request_msg->bus);

    // This registry should trigger the use-after-free
    query.registry = manager->polkit_registry;

    r = async_polkit_callback(request_msg, (void*) &query, &bus_error);
    if (r < 0)
        return r;
}