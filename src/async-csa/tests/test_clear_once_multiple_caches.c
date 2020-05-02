#include "hashmap.h"
#include "machine-image.h"

#include <stdio.h>

static void test_clear_once_multiple_caches(void) {
    Hashmap* cache1 = hashmap_new(NULL);
    Hashmap* cache2 = hashmap_new(NULL);

    Image* image1 = NULL;
    Image* image2 = NULL;
    image_from_path("/", &image1);
    image_from_path("/", &image2);

    hashmap_put(cache1, "Cache1_Image1", image1);
    hashmap_put(cache1, "Cache1_Image2", image2);
    hashmap_put(cache2, "Cache2_Image2", image2);

    hashmap_clear(cache2);
    printf("Image %s\n", image1->name);

    // Checker should detect use-after-free here
    printf("Image %s\n", image2->name);
}

int main(int argc, char *argv[]) {
    test_clear_once_multiple_caches();
}
