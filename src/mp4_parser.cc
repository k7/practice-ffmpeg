#include <fstream>
#include <iostream>
#include <string>
#include <vector>

inline uint32_t swap_endian(uint32_t a) {
    return ((a & 0xff000000) >> 24) | ((a & 0x00ff0000) >> 8) |
           ((a & 0xff00) << 8) | ((a & 0xff) << 24);
}

struct BoxHeader {
    uint32_t size;
    std::string type;
    std::string parent_type;
    int beginPosition;
    int endPosition;
    BoxHeader(std::istream &input, std::string parent) : parent_type(parent) {
        beginPosition = static_cast<int>(input.tellg()) + 1;

        input.read(reinterpret_cast<char *>(&size), sizeof(size));
        size = swap_endian(size);

        char ctype[5] = {0, 0, 0, 0, 0};
        input.read(ctype, sizeof(ctype) - 1);
        type = std::string(ctype);

        endPosition = input.tellg();
    }
};

std::ostream &operator<<(std::ostream &os, const BoxHeader &b) {
    os << "BoxHeader("
       << "type=" << b.type            //
       << ", parent=" << b.parent_type //
       << ", size="
       << +b.size //
       //    << ", position=[" << b.beginPosition << "," << b.endPosition << "]"
       << ")";
    return os;
}

struct Box {
    BoxHeader header;
    int beginPosition;
    int endPosition;
    std::vector<Box> sub_boxes;
    Box(BoxHeader h) : header(h) {
        beginPosition = h.beginPosition;
        endPosition = h.endPosition;
    }

    Box(BoxHeader h, std::istream &input) : Box(h) {
        beginPosition = h.beginPosition;
        endPosition = beginPosition + h.size - 1;

        // 这些是纯容器，不包含字段的
        if (h.type == "moov" || h.type == "trak"                        //
            || h.type == "mdia" || h.type == "minf" || h.type == "stbl" //
            || h.type == "udta" || h.type == "edts") {
            while (input.good() && endPosition - beginPosition < h.size) {
                BoxHeader boxHeader(input, h.type);
                Box box(boxHeader, input);
                sub_boxes.push_back(box);
                input.peek(); // triggered ios state check
            }
        } else {
            input.ignore(h.size - 8);
        }
    }
};

std::ostream &operator<<(std::ostream &os, const Box &b) {

    os << "Box("                                                           //
       << b.header                                                         //
       << ", position=[" << b.beginPosition << "," << b.endPosition << "]" //
       << ")" << std::endl;
    for (const auto &s : b.sub_boxes) {
        os << s;
    }
    return os;
}

int main(int argc, char *argv[]) {

    if (argc <= 1) {
        std::cout << "Usage: mp4_parser <mp4 file>" << std::endl;
        return 0;
    }

    std::string file(argv[1]);

    std::ifstream input(file, std::ios::binary);

    if (!input) {
        std::cout << "file is not exists: " << file << std::endl;
        return -1;
    }

    while (input.good()) {
        BoxHeader boxHeader(input, "root");
        Box box(boxHeader, input);
        std::cout << box;
        input.peek(); // triggered ios state check
    }

    return 0;
}
