#include <fstream>
#include <iostream>
#include <variant>
#include <vector>

struct Header {
    char Signature[3];
    char Version[3];
    std::istream &parse(std::istream &input) {
        input.read(Signature, sizeof(Signature));
        input.read(Version, sizeof(Version));
        return input;
    }
};

std::ostream &operator<<(std::ostream &os, const Header &h) {
    os << "Header(Signature=" << h.Signature << ", Version=" << h.Version
       << ")";
    return os;
}

struct LogicalScreenDescriptor {
    uint16_t LogicalScreenWidth;
    uint16_t LogicalScreenHeight;
    uint8_t PackedFields;
    uint8_t BackgroundColorIndex;
    uint8_t PixelAspectRatio;

    // from packed fields;
    bool GlobalColorTableFlag;
    uint8_t ColorResolution;
    bool SortFlag;
    uint8_t SizeOfGlobalColorTable;

    std::istream &parse(std::istream &input) {
        input.read(reinterpret_cast<char *>(&LogicalScreenWidth),
                   sizeof(LogicalScreenWidth));
        input.read(reinterpret_cast<char *>(&LogicalScreenHeight),
                   sizeof(LogicalScreenHeight));
        input.read(reinterpret_cast<char *>(&PackedFields),
                   sizeof(PackedFields));
        input.read(reinterpret_cast<char *>(&BackgroundColorIndex),
                   sizeof(BackgroundColorIndex));
        input.read(reinterpret_cast<char *>(&PixelAspectRatio),
                   sizeof(PixelAspectRatio));
        parsePackedFields();
        return input;
    }

    void parsePackedFields() {
        GlobalColorTableFlag = (PackedFields & 0b1000'0000) >> 7;
        ColorResolution = (1 >> ((PackedFields & 0b0111'0000) >> 4)) - 1;
        SortFlag = (PackedFields & 0b0000'1000) >> 3;
        SizeOfGlobalColorTable = (1 >> ((PackedFields & 0b0000'0111))) - 1;
    }
};

std::ostream &operator<<(std::ostream &os, const LogicalScreenDescriptor &d) {
    os << " LogicalScreenDescriptor(" << std::endl
       << "  LogicalScreenWidth=" << d.LogicalScreenWidth << ",\n"
       << "  LogicalScreenHeight=" << d.LogicalScreenHeight << ",\n"
       << std::hex << std::showbase //
       << "  PackedFields=" << +d.PackedFields << ",\n"
       << std::resetiosflags(std::ios::hex | std::ios::showbase) //
       << "    [p]GlobalColorTableFlag=" << d.GlobalColorTableFlag << ",\n"
       << "    [p]ColorResolution=" << +d.ColorResolution << ",\n"
       << "    [p]SortFlag=" << d.SortFlag << ",\n"
       << "    [p]SizeOfGlobalColorTable=" << +d.SizeOfGlobalColorTable << ",\n"
       << "  BackgroundColorIndex=" << +d.BackgroundColorIndex << ",\n"
       << "  PixelAspectRatio=" << +d.PixelAspectRatio << ",\n"
       << " )";
    return os;
}

struct Color {
    uint8_t red;
    uint8_t green;
    uint8_t blue;

    std::istream &parse(std::istream &input) {
        input.read(reinterpret_cast<char *>(&red), sizeof(red));
        input.read(reinterpret_cast<char *>(&green), sizeof(green));
        input.read(reinterpret_cast<char *>(&blue), sizeof(blue));
        return input;
    }
};

struct ColorTable {
    std::vector<Color> colors;
    std::istream &parse(std::istream &input, int size) {
        colors = std::vector<Color>(size);
        for (int i = 0; i < size; ++i) {
            colors[i].parse(input);
        }
        return input;
    }
};

std::ostream &operator<<(std::ostream &os, const ColorTable &d) {
    os << "ColorTable("
       << "size=" << d.colors.size() << ")";
    return os;
}

struct SubBlock {
    uint8_t size;
    std::vector<uint8_t> data;

    std::istream &parse(std::istream &input) {
        input.read(reinterpret_cast<char *>(&size), sizeof(size));
        if (size == 0) {
            return input;
        }
        data.reserve(size);
        input.read(reinterpret_cast<char *>(data.data()), size);
        return input;
    }
};

std::ostream &operator<<(std::ostream &os, const SubBlock &d) {
    os << "SubBlock("
       << "size=" << +d.size; //
    os << std::hex << std::showbase;
    for (const auto &u : d.data) {
        os << static_cast<int>(u);
    }
    os << std::resetiosflags(std::ios::hex | std::ios::showbase); //
    os << ")";
    return os;
}

// <Logical Screen> ::=      Logical Screen Descriptor [Global Color Table]
struct LogicScreen {
    LogicalScreenDescriptor logicalScreenDescriptor;
    ColorTable globalColorTabel;

    std::istream &parse(std::istream &input) {
        logicalScreenDescriptor.parse(input);
        if (logicalScreenDescriptor.GlobalColorTableFlag) {
            globalColorTabel.parse(
                input, logicalScreenDescriptor.SizeOfGlobalColorTable + 1);
        }
        return input;
    }
};

std::ostream &operator<<(std::ostream &os, const LogicScreen &d) {
    os << "LogicScreen(" << std::endl
       << d.logicalScreenDescriptor << std::endl
       << d.globalColorTabel << std::endl
       << ")";
    return os;
}

struct PlainTextExtension {
    std::istream &parse(std::istream &input) { return input; }
};

std::ostream &operator<<(std::ostream &os, const PlainTextExtension &d) {
    os << "PlainTextExtension()" << std::endl;
    return os;
}

struct TableBasedImageData {
    uint8_t LZWMinimumCodeSize;
    std::vector<SubBlock> imageData;
    std::istream &parse(std::istream &input) {
        input.read(reinterpret_cast<char *>(&LZWMinimumCodeSize),
                   sizeof(LZWMinimumCodeSize));
        while (true) {
            SubBlock block;
            block.parse(input);
            imageData.push_back(block);
            if (block.size == 0) {
                break;
            }
        }
        return input;
    }
};

std::ostream &operator<<(std::ostream &os, const TableBasedImageData &d) {
    os << "TableBasedImageData("
       << "LZWMinimumCodeSize=" << +d.LZWMinimumCodeSize << ","
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
    std::istream &parse(std::istream &input) {
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

        parsePackedFields();
        return input;
    }

    void parsePackedFields() {
        localColorTableFlag = (packedFields & 0b1000'0000) >> 7;
        interlaceFlag = (packedFields & 0b0100'0000) >> 6;
        sortFlag = (packedFields & 0b0010'0000) >> 5;
        reserved = (packedFields & 0b001'1000) >> 3;
        sizeOfLocalColorTable = (1 >> ((packedFields & 0b0000'0111))) - 1;
    }
};

std::ostream &operator<<(std::ostream &os, const ImageDescriptor &d) {
    os << "    ImageDescriptor(" << std::endl
       << std::hex << std::showbase
       << "      imageSeparator=" << +d.imageSeparator << ",\n"
       << std::resetiosflags(std::ios::hex | std::ios::showbase)
       << "      imageLeftPosition=" << +d.imageLeftPosition << ",\n"
       << "      imageTopPosition=" << +d.imageTopPosition << ",\n"
       << "      imageWidth=" << +d.imageWidth << ",\n"
       << "      imageHeight=" << +d.imageHeight << ",\n"
       << std::hex << std::showbase                              //
       << "      PackedFields=" << +d.packedFields << ",\n"      //
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

    std::istream &parse(std::istream &input) {
        imageDescriptor.parse(input);
        if (imageDescriptor.localColorTableFlag) {
            localColorTable.parse(input,
                                  imageDescriptor.sizeOfLocalColorTable + 1);
        }
        imageData.parse(input);
        return input;
    }
};

std::ostream &operator<<(std::ostream &os, const TableBasedImage &d) {
    os << "    TableBasedImage(" << std::endl
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
    std::istream &parse(std::istream &input) {
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
        return input;
    }
};

std::ostream &operator<<(std::ostream &os, const GraphicRenderingBlock &d) {
    os << "  GraphicRenderingBlock(" << std::endl;
    if (d.type == GraphicRenderingBlock::Type::PlainTextExtension) {
        os << d.plainTextExtension;
    } else {
        os << d.tableBasedImage;
    }
    os << std::endl << "  )";
    return os;
}

// <Special-Purpose Block> ::=    Application Extension  | Comment Extension
struct SpecialPurposeBlock {};

struct ApplicationExtension {
    uint8_t ExtensionIntroducer;
    uint8_t ExtensionLabel;
    uint8_t BlockSize;
    uint8_t ApplicationIdentifier[8];
    uint8_t ApplAuthenticationCode[3];
    std::vector<SubBlock> applicationData; // 15. Data Sub-blocks

    std::istream &parse(std::istream &input) {
        input.read(reinterpret_cast<char *>(&ExtensionIntroducer),
                   sizeof(ExtensionIntroducer));
        input.read(reinterpret_cast<char *>(&ExtensionLabel),
                   sizeof(ExtensionLabel));
        input.read(reinterpret_cast<char *>(&BlockSize), sizeof(BlockSize));
        input.read(reinterpret_cast<char *>(&ApplicationIdentifier),
                   sizeof(ApplicationIdentifier));
        input.read(reinterpret_cast<char *>(&ApplAuthenticationCode),
                   sizeof(ApplAuthenticationCode));
        while (true) {
            SubBlock block;
            block.parse(input);
            applicationData.push_back(block);
            if (block.size == 0) {
                break;
            }
        }
        return input;
    }
};

std::ostream &operator<<(std::ostream &os, const ApplicationExtension &d) {
    os << "ApplicationExtension(" << std::endl
       << std::hex << std::showbase //
       << "  ExtensionIntroducer=" << +d.ExtensionIntroducer << ",\n"
       << "  ExtensionLabel=" << +d.ExtensionLabel << ",\n"
       << std::resetiosflags(std::ios::hex | std::ios::showbase) //
       << "  BlockSize=" << +d.BlockSize << ",\n"
       << "  ApplicationIdentifier=" << d.ApplicationIdentifier << ",\n"
       << "  ApplAuthenticationCode=" << d.ApplAuthenticationCode << ",\n"
       << "ApplicationData=(size=" << d.applicationData.size() << ",\n"
       << ")";
    return os;
}

struct CommentExtension {
    uint8_t extensionIntroducer;
    uint8_t commentLabel;
    std::vector<SubBlock> commentData;
    std::istream &parse(std::istream &input) {
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
        return input;
    }
};

std::ostream &operator<<(std::ostream &os, const CommentExtension &d) {
    os << "CommentExtension()" << std::endl;
    return os;
}

struct GraphicControlExtension {
    uint8_t ExtensionIntroducer;
    uint8_t GraphicControlLabel;
    uint8_t BlockSize;
    uint8_t PackedFields;
    uint16_t DelayTime;
    uint8_t TransparentColorIndex;
    uint8_t BlockTerminator;

    // from packed fields;
    uint8_t Reserved;
    uint8_t DisposalMethod;
    bool UserInputFlag;
    bool TransparentColorFlag;

    std::istream &parse(std::istream &input) {
        input.read(reinterpret_cast<char *>(&ExtensionIntroducer),
                   sizeof(ExtensionIntroducer));
        input.read(reinterpret_cast<char *>(&GraphicControlLabel),
                   sizeof(GraphicControlLabel));
        input.read(reinterpret_cast<char *>(&BlockSize), sizeof(BlockSize));
        input.read(reinterpret_cast<char *>(&PackedFields),
                   sizeof(PackedFields));
        input.read(reinterpret_cast<char *>(&DelayTime), sizeof(DelayTime));
        input.read(reinterpret_cast<char *>(&TransparentColorIndex),
                   sizeof(TransparentColorIndex));
        input.read(reinterpret_cast<char *>(&BlockTerminator),
                   sizeof(BlockTerminator));
        parsePackedFields();
        return input;
    }

    void parsePackedFields() {
        Reserved = (PackedFields & 0b1110'0000) >> 5;
        DisposalMethod = (PackedFields & 0b0001'1100) >> 2;
        UserInputFlag = (PackedFields & 0b0000'0010) >> 1;
        TransparentColorFlag = (PackedFields & 0b0000'0001);
    }
};

std::ostream &operator<<(std::ostream &os, const GraphicControlExtension &d) {
    os << "  GraphicControlExtension(" << std::endl
       << std::hex << std::showbase
       << "    ExtensionIntroducer=" << +d.ExtensionIntroducer << ",\n"
       << "    GraphicControlLabel=" << +d.GraphicControlLabel << ",\n"
       << std::resetiosflags(std::ios::hex | std::ios::showbase)
       << "    BlockSize=" << +d.BlockSize << ",\n"                         //
       << "    DelayTime=" << +d.DelayTime << ",\n"                         //
       << "    TransparentColorIndex=" << +d.TransparentColorIndex << ",\n" //
       << std::hex << std::showbase                                         //
       << "    PackedFields=" << +d.PackedFields << ",\n"                   //
       << "      [p]Reserved=" << +d.Reserved << ",\n"
       << "      [p]DisposalMethod=" << +d.DisposalMethod << ",\n"
       << "      [p]UserInputFlag=" << d.UserInputFlag << ",\n"
       << "      [p]TransparentColorFlag=" << +d.TransparentColorFlag << ",\n"
       << std::resetiosflags(std::ios::hex | std::ios::showbase) << "  )";
    return os;
}

// <Graphic Block> ::=       [Graphic Control Extension] <Graphic-Rendering
// Block>
struct GraphicBlock {
    GraphicControlExtension graphicControlExtension;
    GraphicRenderingBlock graphicRenderingBlock;

    std::istream &parse(std::istream &input) {
        graphicControlExtension.parse(input);
        graphicRenderingBlock.parse(input);
        return input;
    }
};

std::ostream &operator<<(std::ostream &os, const GraphicBlock &d) {
    os << "GraphicBlock(" //
       << std::endl       //
       << d.graphicControlExtension << std::endl
       << d.graphicRenderingBlock << std::endl
       << ")";
    return os;
}

struct Trailer {
    char trailer;
    std::istream &parse(std::istream &input) {
        input.read(reinterpret_cast<char *>(&trailer), sizeof(trailer));
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

    std::istream &parse(std::istream &input) {
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
            return input;
        } else {
        }

        std::cout << std::hex << std::showbase << +input.peek() << std::endl;
        char ch;
        input.read(&ch, 1);
        std::cout << std::hex << std::showbase << +input.peek() << std::endl;
        return input;
    }
};

int main() {
    std::ifstream input("/tmp/big.gif", std::ios::binary);

    if (!input) {
        std::cout << "no valid input file" << std::endl;
        return -1;
    }

    GifDataStream gif;
    gif.parse(input);

    std::cout << "frame counts: " << gif.graphicBlocks.size() << std::endl;

    return 0;
}