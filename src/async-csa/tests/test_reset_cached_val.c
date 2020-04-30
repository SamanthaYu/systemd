#include "hashmap.h"
#include "machine-image.h"

#include <stdio.h>

static void test_clear_cache(void) {
    Hashmap* cache = hashmap_new(NULL);

    Image* image1 = NULL;
    Image* image2 = NULL;
    Image* image3 = NULL;
    image_from_path("/", &image1);
    image_from_path("/", &image2);
    image_from_path("/", &image3);

    hashmap_put(cache, "Cache_Image2", image2);
    hashmap_put(cache, "Cache_Image3", image3);

    hashmap_clear(cache);
    printf("Image %s\n", image1->name);

    // Checker should not detect use-after-free here
    // We are setting image2's name to image1's
    image2->name = image1->name;
}

int main(int argc, char *argv[]) {
    test_clear_cache();
}