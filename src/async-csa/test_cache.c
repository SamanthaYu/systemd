#include "hashmap.h"
#include "machine-image.h"

static void test_clear_hashmap() {
    Hashmap* cache = hashmap_new(NULL);

    Image* image = NULL;
    int r = image_from_path("/", &image);
    if (r < 0) {
        return r;
    }

    r = hashmap_put(cache, image->name, image);
    if (r < 0) {
        return r;
    }

    hashmap_clear(cache);

    // Checker should detect use-after-free here
    Image* image2 = image;
    free(image2);
}

int main(int argc, char *argv[]) {
    test_clear_hashmap();
}