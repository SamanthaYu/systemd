#define _GNU_SOURCE

#include <stdlib.h>
#include <sys/socket.h>

#include "bus-message.h"
#include "bus-polkit.h"
#include "fileio.h"
#include "image-dbus.h"
#include "machine.h"
#include "machine-image.h"
#include "sd-bus.h"

int manager_new(Manager **ret);
static Manager* manager_unref(Manager *m);
DEFINE_TRIVIAL_CLEANUP_FUNC(Manager*, manager_unref);

DEFINE_PRIVATE_HASH_OPS_WITH_VALUE_DESTRUCTOR(machine_hash_ops, char, string_hash_func, string_compare_func, Machine, machine_free);

int manager_new(Manager **ret) {
    _cleanup_(manager_unrefp) Manager *m = NULL;
    int r;

    assert(ret);

    m = new0(Manager, 1);
    if (!m)
        return -ENOMEM;

    m->machines = hashmap_new(&machine_hash_ops);
    m->machine_units = hashmap_new(&string_hash_ops);
    m->machine_leaders = hashmap_new(NULL);

    if (!m->machines || !m->machine_units || !m->machine_leaders)
        return -ENOMEM;

    r = sd_event_default(&m->event);
    if (r < 0)
        return r;

    r = sd_event_add_signal(m->event, NULL, SIGINT, NULL, NULL);
    if (r < 0)
        return r;

    r = sd_event_add_signal(m->event, NULL, SIGTERM, NULL, NULL);
    if (r < 0)
        return r;

    (void) sd_event_set_watchdog(m->event, true);

    *ret = TAKE_PTR(m);
    return 0;
}

Manager* manager_unref(Manager *m) {
    if (!m)
    return NULL;

    while (m->operations)
    operation_free(m->operations);

    assert(m->n_operations == 0);

    hashmap_free(m->machines); /* This will free all machines, so that the machine_units/machine_leaders is empty */
    hashmap_free(m->machine_units);
    hashmap_free(m->machine_leaders);
    hashmap_free(m->image_cache);

    sd_event_source_unref(m->image_cache_defer_event);
    sd_event_source_unref(m->nscd_cache_flush_event);

    bus_verify_polkit_async_registry_free(m->polkit_registry);

    sd_bus_flush_close_unref(m->bus);
    sd_event_unref(m->event);

    return mfree(m);
}

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
        bus, /*path=*/"/org/freedesktop/machine1/image/test", "interface", (void*) manager, (void**) &found_image, &bus_error);
    if (r < 0)
        return r;

    // TODO(samanthayu): Fix checker to detect hashmap_clear()
    free(manager->image_cache);

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
    query.registry = manager->image_cache;

    r = async_polkit_callback(request_msg, (void*) &query, &bus_error);
    if (r < 0)
        return r;
}