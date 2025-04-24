#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <foxdbg.h>

#include <turbojpeg.h>

#include <stdint.h>
#include <stdio.h>

#include <windows.h>


#include <json/json.hpp>

using json = nlohmann::json;

size_t encode_image_byte_array_to_buffer_optimized(uint8_t* tx_buffer, size_t tx_buffer_size, int width, int height, int components, const uint8_t* compressedImage, size_t compressedSize) {

    // Estimate required buffer size (same as before)
    size_t estimated_json_overhead = 100; // A safe estimate
    size_t estimated_data_size = compressedSize * 4; // Max 3 digits + comma per byte
    size_t estimated_total_size = estimated_json_overhead + estimated_data_size + 10;

    if (estimated_total_size > tx_buffer_size - 13) {
        fprintf(stderr, "Estimated JSON message too large for buffer\n");
        return 0; // Indicate failure
    }

    char* buffer_ptr = (char*)tx_buffer + 13; // Start writing after the initial 13 bytes
    size_t bytes_written = 0;

    // Write the fixed JSON parts
    bytes_written += sprintf(buffer_ptr + bytes_written, "{\"width\":%d,", width);
    bytes_written += sprintf(buffer_ptr + bytes_written, "\"height\":%d,", height);
    bytes_written += sprintf(buffer_ptr + bytes_written, "\"channels\":%d,", components);
    bytes_written += sprintf(buffer_ptr + bytes_written, "\"encoding\":\"jpeg\",");
    bytes_written += sprintf(buffer_ptr + bytes_written, "\"data\":[");

    // Write the byte array with manual conversion and fewer calls
    for (size_t i = 0; i < compressedSize; ++i) {
        uint8_t byte = compressedImage[i];
        if (byte >= 100) {
            buffer_ptr[bytes_written++] = '0' + (byte / 100);
            buffer_ptr[bytes_written++] = '0' + ((byte / 10) % 10);
            buffer_ptr[bytes_written++] = '0' + (byte % 10);
        } else if (byte >= 10) {
            buffer_ptr[bytes_written++] = '0' + (byte / 10);
            buffer_ptr[bytes_written++] = '0' + (byte % 10);
        } else {
            buffer_ptr[bytes_written++] = '0' + byte;
        }

        if (i < compressedSize - 1) {
            buffer_ptr[bytes_written++] = ',';
        }
    }

    // Close the data array and the JSON object
    buffer_ptr[bytes_written++] = ']';
    buffer_ptr[bytes_written++] = '}';
    buffer_ptr[bytes_written] = '\0'; // Null terminate


    return bytes_written; // Return the actual size of the JSON data
}

#define TX_BUFFER_SIZE (1024*1024*1024) // 10MB tx buffer
static uint8_t tx_buffer[1024*1024*1024]; /* 10MB tx buffer */


int main(int argc, char *argv[])
{
    /* Initialize the debugger */
    foxdbg_init();

    int width;
    int height;
    int components = 3;

    uint8_t* img_file = stbi_load("examples/c/banana.png", &width, &height, &components, STBI_rgb);

    components = 3;

    tjhandle jpegCompressor = tjInitCompress();

    unsigned long compressedSize = 0;
    unsigned char* compressedImage = NULL;

    int pixelFormat = TJPF_RGB; // Use RGB pixel format
    int jpegSubsamp = TJSAMP_420; // Default to 4:2:0 subsampling

    int jpegQuality = 25;

    #if 0
    {
        LARGE_INTEGER start, end, frequency;
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&start);

        json image_data = {
            {"width", width},
            {"height", height},
            {"channels", components},
            {"encoding", "jpeg"},
            {"data", json::array()}
        };
    
        std::vector<uint8_t> blob(
            (uint8_t*)img_file,
            (uint8_t*)(img_file + width * height * components)
        );
    
        image_data["data"] = std::move(blob);
    
        std::string json_str = image_data.dump();
        size_t json_len = json_str.length();
    
        if (json_len > sizeof(tx_buffer) - 13)
        {
            fprintf(stderr, "JSON message too large\n");
            return 0;
        }
    
        memcpy(tx_buffer + 13, json_str.c_str(), json_len);
    

        QueryPerformanceCounter(&end);
        double elapsed = (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart;
        int microseconds = (int)(elapsed * 1000000.0);
        printf("TIME BEFORE: %d us\n", microseconds);

    }
    


    {
        LARGE_INTEGER start, end, frequency;
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&start);

        /*
        **  Small Image:
        **  100: 2.28ms
        **  50: 1.33ms
        **  25: 0.919s
        **
        **  Example 1280x720 Image:
        **  Compression time: 4.4ms
        **  Size: 24KB vs 3.7MB
        **  JSON Encode Time: 1.1ms vs 100ms
        **  Buffer encode time: 5ms vs 580ms
        */

        
        int result = tjCompress2(
            jpegCompressor,
            img_file,
            width,
            0, // Pitch
            height,
            pixelFormat,
            &compressedImage,
            &compressedSize,
            jpegSubsamp,
            jpegQuality,
            TJFLAG_FASTDCT
        );

        size_t json_len = encode_image_byte_array_to_buffer_optimized(tx_buffer, TX_BUFFER_SIZE, width, height, components, compressedImage, compressedSize);

        QueryPerformanceCounter(&end);
        double elapsed = (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart;
        int microseconds = (int)(elapsed * 1000000.0);
        printf("TIME AFTER: %d us\n", microseconds);

        printf("JSON LEBNGTH: %lu\n", json_len);


    }
    #endif 


    
    
    printf("Image size: %d x %d, components: %d\n", width, height, components);

    
    

    printf("Image size: %d x %d, components: %d\n", width, height, components);

    printf("Image compressed size: %lu bytes vs %lu\n", compressedSize, width * height * components);

    int foxglove_topic_id = foxdbg_get_topic_id("/sensors/camera");
    
    /* wait for user */

    //while (1)
    {
        
        foxdbg_image_t image = {
            width,
            height,
            components,
            compressedSize,
            compressedImage,
            compressedSize
        };

        //foxdbg_send_image_compressed(foxglove_topic_id, image);
    }

    getchar();

    tjFree(compressedImage);
    tjDestroy(jpegCompressor);

    foxdbg_shutdown();

    return 0;
}

    

