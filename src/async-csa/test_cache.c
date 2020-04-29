#include "hashmap.h"
#include "machine-image.h"

static void test_clear_hashmap(void) {
    Hashmap* cache = hashmap_new(NULL);

    Image* image1 = NULL;
    Image* image2 = NULL;
    Image* image3 = NULL;
    image_from_path("/", &image1);
    image_from_path("/", &image2);
    image_from_path("/", &image3);

    hashmap_put(cache, "Image1", image1);
    hashmap_put(cache, "Image2", image2);
    hashmap_put(cache, "Image3", image3);

    hashmap_clear(cache);

    // Checker should detect use-after-free here
    printf("Image %s\n", image1->name);
}

int main(int argc, char *argv[]) {
    test_clear_hashmap();
}