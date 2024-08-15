#include "parser.h"

Parser::Parser(String inPath, String outPath, DoctypeDialect doctype) :
    inPath_(inPath),
    outPath_(outPath),
    outFile_(),
    doctype_(doctype),
    scanner_(Scanner(inPath)),
    tags_(std::vector<String>()),
    addNewlineFor_(TextType::InnerText) {}

bool Parser::parse() {
    // Open the output file
    outFile_ = LittleFS.open(outPath_, "w");
    if (!outFile_) {
        Serial.printf(
            "Error 2-1: Failed to open file for writing '%s'\n",
            outPath_.c_str()
        );
        return false;
    }

    // Parse it
    while (true) {
        std::vector<Token> tokens = std::vector<Token>();
        if (!scanner_.scanPart(tokens)) {
            outFile_.close();
            return false;
        }
        Token token = nextToken(tokens);

        // Handle indentation
        if (token.type == TokenType::Indent) {
            token = nextToken(tokens);
        } else if (token.type == TokenType::Dedent) {
            // Close the current level
            closeTag();

            // Close all dedented levels
            while (token.type == TokenType::Dedent) {
                closeTag();
                token = nextToken(tokens);
            }
        } else if (token.type != TokenType::EndOfPart
                   && token.type != TokenType::EndOfSource) {
            // Close the current level
            closeTag();
        }

        // Parse the main token
        switch (token.type) {
            case TokenType::Doctype:
                parseDoctype(token.doctype);
                token = nextToken(tokens);
                break;
            case TokenType::Tag:
                parseTag(token.tag);
                token = nextToken(tokens);
                break;
            case TokenType::Text:
                parseText(token.text);
                token = nextToken(tokens);
                break;
            case TokenType::Comment:
                parseComment(token.comment);
                token = nextToken(tokens);
                break;
            case TokenType::Include:
                if (!parseInclude(token.include)) {
                    outFile_.close();
                    return false;
                }
                token = nextToken(tokens);
                break;
            default:
                break;
        }

        // Handle the end token
        if (token.type == TokenType::EndOfSource) {
            // Close remaining tags
            while (!tags_.empty()) {
                closeTag();
            }

            // End the loop
            break;
        } else if (token.type != TokenType::EndOfPart || tokens.size() > 0) {
            outFile_.close();
            Serial.printf("Error 2-2: unexpected token\n");
            return false;
        }
    }

    // Close the output file
    outFile_.close();

    return true;
}

Token Parser::nextToken(std::vector<Token> &tokens) {
    Token token = tokens.front();
    tokens.erase(tokens.begin());
    return token;
}

bool Parser::isVoidElement(String tag) {
    return (
        tag == "area" || tag == "base" || tag == "br" || tag == "col"
        || tag == "embed" || tag == "hr" || tag == "img" || tag == "input"
        || tag == "link" || tag == "meta" || tag == "param" || tag == "source"
        || tag == "track" || tag == "wbr"
    );
}

void Parser::closeTag() {
    if (tags_.empty()) {
        return;
    }

    String tag = tags_.back();
    tags_.pop_back();

    if (tag != "") {
        outFile_.printf("</%s>", tag.c_str());
        handleTextNewline();
    }
}

void Parser::handleTextNewline(TextType textType) {
    if (textType != TextType::InnerText) {
        if (addNewlineFor_ == textType) {
            outFile_.print('\n');
        } else {
            addNewlineFor_ = textType;
        }
    } else {
        addNewlineFor_ = TextType::InnerText;
    }
}

void Parser::parseDoctype(DoctypeData data) {
    // Set the current doctype if its not set yet
    if (doctype_ == DoctypeDialect::None) {
        switch (data.doctypeType) {
            case DoctypeShorthand::Html:
                doctype_ = DoctypeDialect::HTML;
                break;
            case DoctypeShorthand::Xml:
                doctype_ = DoctypeDialect::XML;
                break;
            default:
                doctype_ = DoctypeDialect::None;
                break;
        }
    } else {
        Serial.printf("Error 2-3: doctype already set\n");
    }

    // Append the HTML to the output
    outFile_.print(data.toHTMLString());

    tags_.push_back("");
}

void Parser::parseTag(TagData data) {
    // Handle the pipe newline
    handleTextNewline();

    // Open the tag
    outFile_.printf("<%s", data.name.c_str());

    // Add attributes
    for (Attribute attribute : data.attributes) {
        // Add the attribute
        outFile_.printf(" %s", attribute.key.c_str());
        if (attribute.booleanAttribute && doctype_ != DoctypeDialect::HTML) {
            outFile_.printf("=\"%s\"", attribute.key.c_str());
        } else if (!attribute.booleanAttribute) {
            outFile_.printf("=\"%s\"", attribute.value.c_str());
        }
    }

    // (Forced) void element?
    if (data.isVoidElement) {
        outFile_.print("/>");

        tags_.push_back("");
    } else if (isVoidElement(data.name)) {
        switch (doctype_) {
            case DoctypeDialect::HTML:
                outFile_.print(">");
                break;
            case DoctypeDialect::XML:
                outFile_.printf("></%s>", data.name.c_str());
                break;
            case DoctypeDialect::None:
                outFile_.print("/>");
        }

        tags_.push_back("");
    } else {
        outFile_.printf(">%s", data.text.c_str());
        tags_.push_back(data.name);
    }
}

void Parser::parseText(TextData data) {
    // Handle the pipe newline
    handleTextNewline(data.textType);

    // Add the text to the output
    outFile_.print(data.value.c_str());

    tags_.push_back("");
}

void Parser::parseComment(CommentData data) {
    // Handle the pipe newline
    handleTextNewline();

    // Add the comment to the output
    outFile_.printf("<!--%s-->", data.value.c_str());

    tags_.push_back("");
}

bool Parser::parseInclude(IncludeData data) {
    // Handle the pipe newline
    handleTextNewline();

    // Get the directory path
    String direcotryPath = data.path[0] != '/'
        ? inPath_.substring(0, inPath_.lastIndexOf("/") + 1)
        : "";

    // Get the includeFilePath
    String includeFilePath = direcotryPath + data.path;

    // Check for recursion
    if (includeFilePath == inPath_) {
        Serial.printf(
            "Error 2-4: Recursive include of '%s'\n",
            includeFilePath.c_str()
        );
        return false;
    }

    // Open the file
    File includeFile = LittleFS.open(includeFilePath, "r");
    if (!includeFile) {
        Serial.printf(
            "Error 2-5: Failed to open include file '%s'\n",
            includeFilePath.c_str()
        );
        return false;
    }

    // Parse the file if it is a pug file
    if (includeFilePath.length() > 5 && data.path.endsWith(".pug")) {
        // Close the source file
        includeFile.close();

        // Generate a path for the compiled file
        String outFilePath = includeFilePath + ".html";

        // Parse the file
        Parser parser(includeFilePath, outFilePath, doctype_);
        if (!parser.parse()) {
            Serial.printf(
                "Error 2-6: Failed to parse included file '%s'\n",
                includeFilePath.c_str()
            );
            return false;
        }

        // Open, append, and close the compiled file
        File outFile = LittleFS.open(outFilePath, "r");
        if (!outFile.isFile()) {
            Serial.printf(
                "Error 2-7: Failed to open compiled file '%s'\n",
                outFilePath.c_str()
            );
            return false;
        }
        outFile_.print(outFile.readString().c_str());
        outFile.close();
    } else {
        // Append and close the file
        outFile_.print(includeFile.readString().c_str());
        includeFile.close();
    }

    tags_.push_back("");

    return true;
}
