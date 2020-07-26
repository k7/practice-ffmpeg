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
    int beginPosition;
    int endPosition;
    BoxHeader(std::istream &input) {
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
       << "position=[" << b.beginPosition << "," << b.endPosition << "],"
       << "size=" << +b.size << ", type=" << b.type << ")";
    return os;
}

struct Box {
    BoxHeader header;
    int beginPosition;
    int endPosition;
    std::vector<Box> sub_boxes;
    int level = 0;
    Box(BoxHeader h, int le = 0) : header(h), level(le) {
        beginPosition = h.beginPosition;
        endPosition = h.endPosition;
    }

    Box(BoxHeader h, std::istream &input, int le = 0) : Box(h, le) {
        beginPosition = h.beginPosition;
        endPosition = beginPosition + h.size - 1;
        if (h.type == "moov" || h.type == "trak" || h.type == "mdia" ||
            h.type == "minf" || h.type == "stbl") {
            while (input.good() && h.size > endPosition - beginPosition) {
                BoxHeader boxHeader(input);
                Box box(boxHeader, input, level + 1);
                sub_boxes.push_back(box);
                input.peek(); // triggered ios state check
            }
        } else {
            input.ignore(h.size - 8);
        }
    }
};

std::ostream &operator<<(std::ostream &os, const Box &b) {
    std::string prefix(b.level * 2, ' ');
    os << prefix;
    os << "Box("                                                           //
       << "position=[" << b.beginPosition << "," << b.endPosition << "], " //
       << b.header;                                                        //
    for (const auto &s : b.sub_boxes) {
        os << prefix << std::endl << s;
    }
    if (b.sub_boxes.size() > 0) {
        os << std::endl << prefix;
    }
    os << ")";
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
        BoxHeader boxHeader(input);
        Box box(boxHeader, input);
        std::cout << box << std::endl;
        input.peek(); // triggered ios state check
    }

    return 0;
}
