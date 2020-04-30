#include "hashmap.h"
#include "machine-image.h"

#include <stdio.h>

static void clear(Hashmap* cache) {
    hashmap_clear(cache);
}

static void test_interprocedural_clear(void) {
    Hashmap* cache = hashmap_new(NULL);

    Image* image1 = NULL;
    Image* image2 = NULL;
    Image* image3 = NULL;
    image_from_path("/", &image1);
    image_from_path("/", &image2);
    image_from_path("/", &image3);

    hashmap_put(cache, "Cache_Image2", image2);
    hashmap_put(cache, "Cache_Image3", image3);

    clear(cache);
    printf("Image %s\n", image1->name);

    // Checker should detect use-after-free here
    printf("Image %s\n", image2->name);
}

int main(int argc, char *argv[]) {
    test_interprocedural_clear();
}