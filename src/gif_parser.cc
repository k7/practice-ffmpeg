#include <fstream>
#include <iostream>
#include <vector>

struct Header {
    char signature[3];
    char version[3];
    int beginPosition;
    int endPosition;
    std::istream &parse(std::istream &input) {
        beginPosition = static_cast<int>(input.tellg()) + 1;
        input.read(signature, sizeof(signature));
        input.read(version, sizeof(version));
        endPosition = input.tellg();
        return input;
    }
};

std::ostream &operator<<(std::ostream &os, const Header &d) {
    os << "Header("
       << "position=[" << d.beginPosition << "," << d.endPosition << "],"
       << "signature=" << d.signature << ", version=" << d.version << ")";
    return os;
}

struct LogicalScreenDescriptor {
    uint16_t logicalScreenWidth;
    uint16_t logicalScreenHeight;
    uint8_t packedFields;
    uint8_t backgroundColorIndex;
    uint8_t pixelAspectRatio;

    // from packed fields;
    bool globalColorTableFlag;
    uint8_t colorResolution;
    bool sortFlag;
    uint8_t sizeOfGlobalColorTable;

    int beginPosition;
    int endPosition;

    std::istream &parse(std::istream &input) {
        beginPosition = static_cast<int>(input.tellg()) + 1;
        input.read(reinterpret_cast<char *>(&logicalScreenWidth),
                   sizeof(logicalScreenWidth));
        input.read(reinterpret_cast<char *>(&logicalScreenHeight),
                   sizeof(logicalScreenHeight));
        input.read(reinterpret_cast<char *>(&packedFields),
                   sizeof(packedFields));
        input.read(reinterpret_cast<char *>(&backgroundColorIndex),
                   sizeof(backgroundColorIndex));
        input.read(reinterpret_cast<char *>(&pixelAspectRatio),
                   sizeof(pixelAspectRatio));
        endPosition = input.tellg();
        parsepackedFields();
        return input;
    }

    void parsepackedFields() {
        globalColorTableFlag = (packedFields & 0b1000'0000) >> 7;
        colorResolution = (1 >> ((packedFields & 0b0111'0000) >> 4)) - 1;
        sortFlag = (packedFields & 0b0000'1000) >> 3;
        sizeOfGlobalColorTable = (1 >> ((packedFields & 0b0000'0111))) - 1;
    }
};

std::ostream &operator<<(std::ostream &os, const LogicalScreenDescriptor &d) {
    os << " LogicalScreenDescriptor(" << std::endl
       << "  position=[" << d.beginPosition << "," << d.endPosition << "],\n"
       << "  logicalScreenWidth=" << d.logicalScreenWidth << ",\n"
       << "  logicalScreenHeight=" << d.logicalScreenHeight << ",\n"
       << std::hex << std::showbase //
       << "  packedFields=" << +d.packedFields << ",\n"
       << std::resetiosflags(std::ios::hex | std::ios::showbase) //
       << "    [p]globalColorTableFlag=" << d.globalColorTableFlag << ",\n"
       << "    [p]colorResolution=" << +d.colorResolution << ",\n"
       << "    [p]sortFlag=" << d.sortFlag << ",\n"
       << "    [p]sizeOfGlobalColorTable=" << +d.sizeOfGlobalColorTable << ",\n"
       << "  backgroundColorIndex=" << +d.backgroundColorIndex << ",\n"
       << "  pixelAspectRatio=" << +d.pixelAspectRatio << ",\n"
       << ",\n"
       << " )";
    return os;
}

struct Color {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    int beginPosition;
    int endPosition;

    std::istream &parse(std::istream &input) {
        beginPosition = static_cast<int>(input.tellg()) + 1;
        input.read(reinterpret_cast<char *>(&red), sizeof(red));
        input.read(reinterpret_cast<char *>(&green), sizeof(green));
        input.read(reinterpret_cast<char *>(&blue), sizeof(blue));
        endPosition = input.tellg();
        return input;
    }
};

struct ColorTable {
    std::vector<Color> colors;
    int beginPosition;
    int endPosition;
    std::istream &parse(std::istream &input, int size) {
        beginPosition = static_cast<int>(input.tellg()) + 1;
        colors = std::vector<Color>(size);
        for (int i = 0; i < size; ++i) {
            colors[i].parse(input);
        }
        endPosition = input.tellg();
        return input;
    }
};

std::ostream &operator<<(std::ostream &os, const ColorTable &d) {
    os << "ColorTable("
       << "position=[" << d.beginPosition << "," << d.endPosition << "],"
       << "size=" << d.colors.size() //
       << ")";
    return os;
}

struct SubBlock {
    uint8_t size;
    std::vector<uint8_t> data;
    int beginPosition;
    int endPosition;
    std::istream &parse(std::istream &input) {
        beginPosition = static_cast<int>(input.tellg()) + 1;
        input.read(reinterpret_cast<char *>(&size), sizeof(size));
        if (size == 0) {
            return input;
        }
        data.reserve(size);
        input.read(reinterpret_cast<char *>(data.data()), size);
        endPosition = input.tellg();
        return input;
    }
};

std::ostream &operator<<(std::ostream &os, const SubBlock &d) {
    os << "SubBlock("
       << "position=[" << d.beginPosition << "," << d.endPosition << "],"
       << "size=" << +d.size; //
    os << std::hex << std::showbase;
    for (const auto &u : d.data) {
        os << static_cast<int>(u);
    }
    os << std::resetiosflags(std::ios::hex | std::ios::showbase) //
       << ")";
    return os;
}

// <Logical Screen> ::=      Logical Screen Descriptor [Global Color Table]
struct LogicScreen {
    LogicalScreenDescriptor logicalScreenDescriptor;
    ColorTable globalColorTabel;
    int beginPosition;
    int endPosition;
    std::istream &parse(std::istream &input) {
        beginPosition = static_cast<int>(input.tellg()) + 1;
        logicalScreenDescriptor.parse(input);
        if (logicalScreenDescriptor.globalColorTableFlag) {
            globalColorTabel.parse(
                input, logicalScreenDescriptor.sizeOfGlobalColorTable + 1);
        }
        endPosition = input.tellg();
        return input;
    }
};

std::ostream &operator<<(std::ostream &os, const LogicScreen &d) {
    os << "LogicScreen(" << std::endl
       << "position=[" << d.beginPosition << "," << d.endPosition << "], "
       << std::endl
       << d.logicalScreenDescriptor << std::endl
       << d.globalColorTabel << std::endl
       << ")";
    return os;
}

struct PlainTextExtension {
    int beginPosition;
    int endPosition;
    std::istream &parse(std::istream &input) {
        beginPosition = static_cast<int>(input.tellg()) + 1;
        endPosition = input.tellg();
        return input;
    }
};

std::ostream &operator<<(std::ostream &os, const PlainTextExtension &d) {
    os << "PlainTextExtension("
       << "position=[" << d.beginPosition << "," << d.endPosition << "])";

    return os;
}

struct TableBasedImageData {
    uint8_t lzwMinimumCodeSize;
    std::vector<SubBlock> imageData;
    int beginPosition;
    int endPosition;
    std::istream &parse(std::istream &input) {
        beginPosition = static_cast<int>(input.tellg()) + 1;
        input.read(reinterpret_cast<char *>(&lzwMinimumCodeSize),
                   sizeof(lzwMinimumCodeSize));
        while (true) {
            SubBlock block;
            block.parse(input);
            imageData.push_back(block);
            if (block.size == 0) {
                break;
            }
        }
        endPosition = input.tellg();
        return input;
    }
};

std::ostream &operator<<(std::ostream &os, const TableBasedImageData &d) {
    os << "TableBasedImageData("
       << "position=[" << d.beginPosition << "," << d.endPosition << "], "
       << "lzwMinimumCodeSize=" << +d.lzwMinimumCodeSize << ","
       << "ImageData=(size=" << d.imageData.size() << ")";

    os << ")";
    return os;
}

struct ImageDescriptor {
    uint8_t imageSeparator;
    uint16_t imageLeftPosition;
    uint16_t imageTopPosition;
    uint16_t imageWidth;
    uint16_t imageHeight;
    uint8_t packedFields;
    bool localColorTableFlag;
    bool interlaceFlag;
    bool sortFlag;
    uint8_t reserved;
    uint8_t sizeOfLocalColorTable;
    int beginPosition;
    int endPosition;
    std::istream &parse(std::istream &input) {
        beginPosition = static_cast<int>(input.tellg()) + 1;
        input.read(reinterpret_cast<char *>(&imageSeparator),
                   sizeof(imageSeparator));
        input.read(reinterpret_cast<char *>(&imageLeftPosition),
                   sizeof(imageLeftPosition));
        input.read(reinterpret_cast<char *>(&imageTopPosition),
                   sizeof(imageTopPosition));
        input.read(reinterpret_cast<char *>(&imageWidth), sizeof(imageWidth));
        input.read(reinterpret_cast<char *>(&imageHeight), sizeof(imageHeight));
        input.read(reinterpret_cast<char *>(&packedFields),
                   sizeof(packedFields));
        endPosition = input.tellg();

        parsepackedFields();
        return input;
    }

    void parsepackedFields() {
        localColorTableFlag = (packedFields & 0b1000'0000) >> 7;
        interlaceFlag = (packedFields & 0b0100'0000) >> 6;
        sortFlag = (packedFields & 0b0010'0000) >> 5;
        reserved = (packedFields & 0b001'1000) >> 3;
        sizeOfLocalColorTable = (1 >> ((packedFields & 0b0000'0111))) - 1;
    }
};

std::ostream &operator<<(std::ostream &os, const ImageDescriptor &d) {
    os << "    ImageDescriptor(" << std::endl
       << "      position=[" << d.beginPosition << "," << d.endPosition
       << "],\n"
       << std::hex << std::showbase
       << "      imageSeparator=" << +d.imageSeparator << ",\n"
       << std::resetiosflags(std::ios::hex | std::ios::showbase)
       << "      imageLeftPosition=" << +d.imageLeftPosition << ",\n"
       << "      imageTopPosition=" << +d.imageTopPosition << ",\n"
       << "      imageWidth=" << +d.imageWidth << ",\n"
       << "      imageHeight=" << +d.imageHeight << ",\n"
       << std::hex << std::showbase                              //
       << "      packedFields=" << +d.packedFields << ",\n"      //
       << std::resetiosflags(std::ios::hex | std::ios::showbase) //
       << "        [p]localColorTableFlag=" << d.localColorTableFlag << ",\n"
       << "        [p]interlaceFlag=" << d.interlaceFlag << ",\n"
       << "        [p]sortFlag=" << d.sortFlag << ",\n"
       << std::hex << std::showbase //
       << "        [p]reserved=" << +d.reserved << ",\n"
       << std::resetiosflags(std::ios::hex | std::ios::showbase) //
       << "        [p]sizeOfLocalColorTable=" << +d.sizeOfLocalColorTable
       << ",\n"
       << "  )";

    return os;
}

// <Table-Based Image> ::=   Image Descriptor [Local Color Table] Image Data
struct TableBasedImage {
    ImageDescriptor imageDescriptor;
    ColorTable localColorTable;
    TableBasedImageData imageData;
    int beginPosition;
    int endPosition;
    std::istream &parse(std::istream &input) {
        beginPosition = static_cast<int>(input.tellg()) + 1;
        imageDescriptor.parse(input);
        if (imageDescriptor.localColorTableFlag) {
            localColorTable.parse(input,
                                  imageDescriptor.sizeOfLocalColorTable + 1);
        }
        imageData.parse(input);
        endPosition = input.tellg();
        return input;
    }
};

std::ostream &operator<<(std::ostream &os, const TableBasedImage &d) {
    os << "    TableBasedImage(" << std::endl
       << "      position=[" << d.beginPosition << "," << d.endPosition << "]\n"
       << d.imageDescriptor << ", " << d.localColorTable << ", " << d.imageData
       << ")";
    return os;
}

// <Graphic-Rendering Block> ::=  <Table-Based Image>  | Plain Text Extension
struct GraphicRenderingBlock {
    enum class Type {
        Unknown,
        PlainTextExtension,
        TableBasedImage,
    };
    PlainTextExtension plainTextExtension;
    TableBasedImage tableBasedImage;
    Type type = Type::Unknown;
    int beginPosition;
    int endPosition;
    std::istream &parse(std::istream &input) {
        beginPosition = static_cast<int>(input.tellg()) + 1;
        uint8_t extensionIntroducer = input.get();
        uint8_t extensionLabel = input.get();
        input.unget();
        input.unget();
        if (extensionIntroducer == 0x21 && extensionLabel == 0x01) {
            type = Type::PlainTextExtension;
            plainTextExtension.parse(input);
        } else {
            type = Type::TableBasedImage;
            tableBasedImage.parse(input);
        }
        endPosition = input.tellg();
        return input;
    }
};

std::ostream &operator<<(std::ostream &os, const GraphicRenderingBlock &d) {
    os << "  GraphicRenderingBlock(" << std::endl;
    os << "    position=[" << d.beginPosition << "," << d.endPosition << "]"
       << std::endl;
    if (d.type == GraphicRenderingBlock::Type::PlainTextExtension) {
        os << d.plainTextExtension;
    } else {
        os << d.tableBasedImage;
    }
    os << "  )";
    return os;
}

// <Special-Purpose Block> ::=    Application Extension  | Comment Extension
struct SpecialPurposeBlock {};

struct ApplicationExtension {
    uint8_t extensionIntroducer;
    uint8_t extensionLabel;
    uint8_t blockSize;
    uint8_t applicationIdentifier[8];
    uint8_t applAuthenticationCode[3];
    std::vector<SubBlock> applicationData; // 15. Data Sub-blocks

    int beginPosition;
    int endPosition;
    std::istream &parse(std::istream &input) {
        beginPosition = static_cast<int>(input.tellg()) + 1;
        input.read(reinterpret_cast<char *>(&extensionIntroducer),
                   sizeof(extensionIntroducer));
        input.read(reinterpret_cast<char *>(&extensionLabel),
                   sizeof(extensionLabel));
        input.read(reinterpret_cast<char *>(&blockSize), sizeof(blockSize));
        input.read(reinterpret_cast<char *>(&applicationIdentifier),
                   sizeof(applicationIdentifier));
        input.read(reinterpret_cast<char *>(&applAuthenticationCode),
                   sizeof(applAuthenticationCode));
        while (true) {
            SubBlock block;
            block.parse(input);
            applicationData.push_back(block);
            if (block.size == 0) {
                break;
            }
        }
        endPosition = input.tellg();
        return input;
    }
};

std::ostream &operator<<(std::ostream &os, const ApplicationExtension &d) {
    os << "ApplicationExtension(" << std::endl
       << "  position=[" << d.beginPosition << "," << d.endPosition << "],\n"
       << std::hex << std::showbase //
       << "  extensionIntroducer=" << +d.extensionIntroducer << ",\n"
       << "  extensionLabel=" << +d.extensionLabel << ",\n"
       << std::resetiosflags(std::ios::hex | std::ios::showbase) //
       << "  blockSize=" << +d.blockSize << ",\n"
       << "  applicationIdentifier=" << d.applicationIdentifier << ",\n"
       << "  applAuthenticationCode=" << d.applAuthenticationCode << ",\n"
       << "  ApplicationData=(size=" << d.applicationData.size() << ",\n"
       << ")";
    return os;
}

struct CommentExtension {
    uint8_t extensionIntroducer;
    uint8_t commentLabel;
    std::vector<SubBlock> commentData;

    int beginPosition;
    int endPosition;
    std::istream &parse(std::istream &input) {
        beginPosition = static_cast<int>(input.tellg()) + 1;
        input.read(reinterpret_cast<char *>(&extensionIntroducer),
                   sizeof(extensionIntroducer));
        input.read(reinterpret_cast<char *>(&commentLabel),
                   sizeof(commentLabel));
        while (true) {
            SubBlock block;
            block.parse(input);
            commentData.push_back(block);
            if (block.size == 0) {
                break;
            }
        }
        endPosition = input.tellg();
        return input;
    }
};

std::ostream &operator<<(std::ostream &os, const CommentExtension &d) {
    os << "CommentExtension("
       << "position=[" << d.beginPosition << "," << d.endPosition << "]"
       << ")" << std::endl;
    return os;
}

struct GraphicControlExtension {
    uint8_t extensionIntroducer;
    uint8_t graphicControlLabel;
    uint8_t blockSize;
    uint8_t packedFields;
    uint16_t delayTime;
    uint8_t transparentColorIndex;
    uint8_t blockTerminator;

    // from packed fields;
    uint8_t reserved;
    uint8_t disposalMethod;
    bool userInputFlag;
    bool transparentColorFlag;

    int beginPosition;
    int endPosition;
    std::istream &parse(std::istream &input) {
        beginPosition = static_cast<int>(input.tellg()) + 1;
        input.read(reinterpret_cast<char *>(&extensionIntroducer),
                   sizeof(extensionIntroducer));
        input.read(reinterpret_cast<char *>(&graphicControlLabel),
                   sizeof(graphicControlLabel));
        input.read(reinterpret_cast<char *>(&blockSize), sizeof(blockSize));
        input.read(reinterpret_cast<char *>(&packedFields),
                   sizeof(packedFields));
        input.read(reinterpret_cast<char *>(&delayTime), sizeof(delayTime));
        input.read(reinterpret_cast<char *>(&transparentColorIndex),
                   sizeof(transparentColorIndex));
        input.read(reinterpret_cast<char *>(&blockTerminator),
                   sizeof(blockTerminator));
        endPosition = input.tellg();
        parsepackedFields();
        return input;
    }

    void parsepackedFields() {
        reserved = (packedFields & 0b1110'0000) >> 5;
        disposalMethod = (packedFields & 0b0001'1100) >> 2;
        userInputFlag = (packedFields & 0b0000'0010) >> 1;
        transparentColorFlag = (packedFields & 0b0000'0001);
    }
};

std::ostream &operator<<(std::ostream &os, const GraphicControlExtension &d) {
    os << "  GraphicControlExtension(" << std::endl
       << "    position=[" << d.beginPosition << "," << d.endPosition << "],\n"
       << std::hex << std::showbase
       << "    extensionIntroducer=" << +d.extensionIntroducer << ",\n"
       << "    graphicControlLabel=" << +d.graphicControlLabel << ",\n"
       << std::resetiosflags(std::ios::hex | std::ios::showbase)
       << "    blockSize=" << +d.blockSize << ",\n"                         //
       << "    delayTime=" << +d.delayTime << ",\n"                         //
       << "    transparentColorIndex=" << +d.transparentColorIndex << ",\n" //
       << std::hex << std::showbase                                         //
       << "    packedFields=" << +d.packedFields << ",\n"                   //
       << "      [p]reserved=" << +d.reserved << ",\n"
       << "      [p]disposalMethod=" << +d.disposalMethod << ",\n"
       << "      [p]userInputFlag=" << d.userInputFlag << ",\n"
       << "      [p]transparentColorFlag=" << +d.transparentColorFlag << ",\n"
       << std::resetiosflags(std::ios::hex | std::ios::showbase) //
       << "  )";
    return os;
}

// <Graphic Block> ::=       [Graphic Control Extension] <Graphic-Rendering
// Block>
struct GraphicBlock {
    GraphicControlExtension graphicControlExtension;
    GraphicRenderingBlock graphicRenderingBlock;

    int beginPosition;
    int endPosition;
    std::istream &parse(std::istream &input) {
        beginPosition = static_cast<int>(input.tellg()) + 1;
        graphicControlExtension.parse(input);
        graphicRenderingBlock.parse(input);
        endPosition = input.tellg();
        return input;
    }
};

std::ostream &operator<<(std::ostream &os, const GraphicBlock &d) {
    os << "GraphicBlock(" //
       << std::endl       //
       << "  position=[" << d.beginPosition << "," << d.endPosition << "]"
       << std::endl //
       << d.graphicControlExtension << std::endl
       << d.graphicRenderingBlock << std::endl
       << ")";
    return os;
}

struct Trailer {
    char trailer;
    int beginPosition;
    int endPosition;
    std::istream &parse(std::istream &input) {
        beginPosition = static_cast<int>(input.tellg()) + 1;
        input.read(reinterpret_cast<char *>(&trailer), sizeof(trailer));
        endPosition = input.tellg();
        return input;
    }
};

// <GIF Data Stream> ::= Header <Logical Screen> <Data>* Trailer
// <Logical Screen>  ::= Logical Screen Descriptor [Global Color Table]
// <Data>            ::= <Graphic Block>  |
//                       <Special-Purpose Block>
//
// <Graphic Block>   ::= [Graphic Control Extension] <Graphic-Rendering-Block>
//
// <Graphic-Rendering Block> ::=  <Table-Based Image>  |
//                                Plain Text Extension
//
// <Table-Based Image> ::=   Image Descriptor [Local Color Table] Image Data
// <Special-Purpose Block> ::=    Application Extension  |
//                                Comment Extension

struct GifDataStream {
    Header header;
    LogicScreen logicScreen;
    ApplicationExtension applicationExtension;
    std::vector<GraphicBlock> graphicBlocks;
    std::vector<CommentExtension> commentExtensions;
    Trailer trailer;

    int beginPosition;
    int endPosition;
    std::istream &parse(std::istream &input) {
        beginPosition = static_cast<int>(input.tellg()) + 1;
        if (header.parse(input)) {
            std::cout << header << std::endl;
        }

        if (logicScreen.parse(input)) {
            std::cout << logicScreen << std::endl;
        }

        while (input.peek() == 0x21) {
            // uint8_t extensionIntroducer =
            input.get();
            uint8_t extensionLabel = input.get();
            input.unget();
            input.unget();
            if (extensionLabel == 0xFF) {
                applicationExtension.parse(input);
                std::cout << applicationExtension << std::endl;
            } else if (extensionLabel == 0xFE) {
                CommentExtension commentExtension;
                commentExtension.parse(input);
                std::cout << commentExtension << std::endl;
                commentExtensions.push_back(commentExtension);
            } else if (extensionLabel == 0xF9) { // Graphic Control Extension
                // <Graphic Block> ::=
                //   [Graphic Control Extension] <Graphic-Rendering Block>
                GraphicBlock graphicBlock;
                graphicBlock.parse(input);
                graphicBlocks.push_back(graphicBlock);
                std::cout << graphicBlock << std::endl;
            } else {
                break;
            }
        }

        if (input.peek() == 0x3B) {
            trailer.parse(input);
            endPosition = input.tellg();
        } else {
            std::cout << "Error"                   //
                      << std::hex << std::showbase //
                      << +input.peek()             //
                      << std::resetiosflags(std::ios::hex |
                                            std::ios::showbase) //
                      << std::endl;                             //
        }

        return input;
    }
};

int main(int argc, char *argv[]) {

    if (argc <= 1) {
        std::cout << "Usage: gif_parser <gif file>" << std::endl;
        return 0;
    }

    std::string file(argv[1]);

    std::ifstream input(file, std::ios::binary);

    if (!input) {
        std::cout << "file is not exists: " << file << std::endl;
        return -1;
    }

    if (input.peek() != 'G') {
        std::cout << "it is not a gif file: " << file << std::endl;
        return -1;
    }

    GifDataStream gif;
    gif.parse(input);

    std::cout << "frame counts: " << gif.graphicBlocks.size() << std::endl;

    return 0;
}
