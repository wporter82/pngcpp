#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <zlib.h>
#include <arpa/inet.h>

constexpr const char* PNG_SIGNATURE = "\x89PNG\r\n\x1A\n";

struct IHDR
{
    unsigned char width[4] = "\0";
    unsigned char height[4] = "\0";
    unsigned char bit_depth = 8;
    unsigned char color_type = 2;
    unsigned char compression_method = 0;
    unsigned char filter_method = 0;
    unsigned char interlace_method = 0;
};

void write_png_chunk(std::ofstream& stream, std::string chunk_type, std::vector<uint8_t> chunk_data)
{
    int chunk_length = chunk_data.size();
    
    // combine the type and data so we can calc the CRC
    std::vector<uint8_t> combined;
    combined.reserve(chunk_type.size() + chunk_data.size());
    combined.insert(combined.end(), chunk_type.begin(), chunk_type.end());
    combined.insert(combined.end(), chunk_data.begin(), chunk_data.end());

    // Calc CRC
    uint32_t crc = crc32(0L, Z_NULL, 0);
    for (size_t i = 0; i < combined.size(); i++)
    {
        crc = crc32(crc, ((uint8_t*)&combined[0]) + i, 1);
    }

    // get 'big endian' version for writing to disk
    uint32_t len = htonl(chunk_length);
    stream.write(reinterpret_cast<const char*>(&len), 4);
    stream << chunk_type;
    
    for (auto& a : chunk_data)
    {
        stream << a;
    }
    
    // get 'big endian' version for writing to disk
    uint32_t crcbe = htonl(crc);
    stream.write(reinterpret_cast<const char*>(&crcbe), 4);
}

// 0=R, 1=G, 2=B
uint8_t read_rgb_subpixel(std::vector<uint8_t>& rgb_data, size_t width, size_t x, size_t y, size_t subpixel)
{
    return rgb_data[3 * ((width * y) + x) + subpixel];
}

std::vector<uint8_t> apply_png_filters(std::vector<uint8_t>& rgb_data, size_t width, size_t height)
{
    std::vector<uint8_t> filtered;
    for (size_t y = 0; y < height; y++)
    {
        filtered.push_back(0);
        for (size_t x = 0; x < width; x++)
        {
            filtered.push_back(read_rgb_subpixel(rgb_data, width, x, y, 0));
            filtered.push_back(read_rgb_subpixel(rgb_data, width, x, y, 1));
            filtered.push_back(read_rgb_subpixel(rgb_data, width, x, y, 2));
        }
    }

    return filtered;
}

void encode_png_ihdr(IHDR& ihdr, uint32_t width, uint32_t height)
{
    ihdr.width[2] = (width >> 8);
    ihdr.width[3] = width;
    
    ihdr.height[2] = (height >> 8);
    ihdr.height[3] = height;
}

int main(int argc, char** argv)
{
    if (argc < 5)
    {
        std::cerr << "Error: incorrect parapeters provided." << std::endl;
        std::cerr << "Usage: " << argv[0] << " infile.rgb outfile.png width height" << std::endl;
        return 1;
    }
    const int width = atoi(argv[3]);
    const int height = atoi(argv[4]);

    std::ifstream fin(argv[1], std::ios_base::in | std::ios_base::binary);

    std::vector<uint8_t> rgb_data((std::istreambuf_iterator<char>(fin)), std::istreambuf_iterator<char>());
    fin.close();

    IHDR ihdr;
    encode_png_ihdr(ihdr, width, height);

    // convert IHDR struct into byte array
    auto ptr = reinterpret_cast<uint8_t*>(&ihdr);
    auto buffer = std::vector<uint8_t>(ptr, ptr + sizeof(ihdr));

    std::vector<uint8_t> filtered(apply_png_filters(rgb_data, width, height));
    uint8_t* filtered_ptr = reinterpret_cast<uint8_t*>(&filtered);

    Bytef* buf = (Bytef*)malloc(filtered.size());
    uLongf dest_size;
    dest_size = (uLongf)filtered.size();

    std::vector<uint8_t> idat;
    std::vector<uint8_t> iend;
    std::ofstream fout(argv[2], std::ios_base::out | std::ios_base::binary);

    int res = compress2(buf, &dest_size, (Bytef*)filtered.data(), filtered.size(), Z_BEST_COMPRESSION);
    switch (res)
    {
    case Z_OK:
        idat.reserve(dest_size);
        for (uLongf i = 0; i < dest_size; i++)
        {
            idat.push_back(buf[i]);
        }

        fout.write(PNG_SIGNATURE, sizeof(PNG_SIGNATURE));
        write_png_chunk(fout, "IHDR", buffer);
        write_png_chunk(fout, "IDAT", idat);
        write_png_chunk(fout, "IEND", iend);
        break;

    case Z_MEM_ERROR:
        std::cerr << "Out of Memory Error" << std::endl;
        break;
    case Z_DATA_ERROR:
        std::cerr << "Error reading input file" << std::endl;
        break;
    case Z_STREAM_END:
    case Z_NEED_DICT:
    case Z_ERRNO:
    case Z_STREAM_ERROR:
    case Z_BUF_ERROR:
    case Z_VERSION_ERROR:
        std::cerr << "Compression Error" << std::endl;
        break;
    
    default:
        break;
    }

    free(buf);
    fout.close();
    return res;
}