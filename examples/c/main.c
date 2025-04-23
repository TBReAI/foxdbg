#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <foxdbg.h>

#include <stdint.h>
#include <stdio.h>




int main(int argc, char *argv[])
{
    /* Initialize the debugger */
    foxdbg_init();

    int width = 440;
    int height = 400;
    int components = 3;

    //uint8_t* img = stbi_load("examples/c/banana.jpg", &width, &height, &components, STBI_rgb);

    FILE *file = fopen("examples/c/banana.jpg", "rb");

    if (!file) {
        fprintf(stderr, "Error opening file\n");
        return 1;
    }

    fseek(file, 0, SEEK_END);

    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    uint8_t *img = (uint8_t *)malloc(file_size);
    if (!img) {
        fprintf(stderr, "Error allocating memory\n");
        fclose(file);
        return 1;
    }

    size_t bytes_read = fread(img, 1, file_size, file);
    if (bytes_read != file_size) {
        fprintf(stderr, "Error reading file\n");
        free(img);
        fclose(file);
        return 1;
    }
    fclose(file);

    

    printf("Image size: %d x %d, components: %d\n", width, height, components);

    int foxglove_topic_id = foxdbg_get_topic_id("/sensors/camera");
    
    /* wait for user */

    while (1)
    {
        foxdbg_image_t image = {
            .width = width,
            .height = height,
            .channels = components,
            .image_size = width * height * components,
            .buffer = img,
            .buffer_size = width * height * components
        };

        foxdbg_send_image_compressed(foxglove_topic_id, image);
    }

    foxdbg_shutdown();

    return 0;
}

    

