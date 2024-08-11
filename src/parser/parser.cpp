#include "parser.h"

Parser::Parser(String inPath, String outPath, DoctypeDialect doctype) :
    directory_(inPath.substring(0, inPath.lastIndexOf('/'))),
    doctype_(doctype),
    scanner_(Scanner(inPath)),
    tags_(std::vector<String>()),
    indentet_(false) {
    outFile_ = LittleFS.open(outPath, "w");

    if (!outFile_) {
        Serial.printf(
            "Error 2-1: Failed to open file for writing '%s'\n",
            outPath.c_str()
        );
        return;
    }
}

Parser::~Parser() {
    outFile_.close();
}

bool Parser::parse() {
    while (true) {
        std::vector<Token> *tokens = new std::vector<Token>();
        if (!scanner_.scanPart(tokens)) {
            return false;
        }
        Token token = nextToken(tokens);

        // Handle indentation
        if (token.type == TokenType::Indent) {
            indentet_ = true;
            token = nextToken(tokens);
        } else if (token.type == TokenType::Dedent) {
            while (token.type == TokenType::Dedent) {
                closeTag();
                token = nextToken(tokens);
            }
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

            return true;
        } else if (token.type != TokenType::EndOfPart || tokens->size() > 0) {
            Serial.printf("Error 2-2: unexpected token\n");
            return false;
        }
    }

    return false;
}

Token Parser::nextToken(std::vector<Token> *tokens) {
    Token token = tokens->front();
    tokens->erase(tokens->begin());
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

void Parser::closeTagIfNecessary() {
    if (!indentet_) {
        closeTag();
    } else {
        indentet_ = false;
    }
}

void Parser::handleTextNewline(bool isPipeText, bool isLiteralHTML) {
    if (isPipeText) {
        if (addPipeTextNewline_) {
            outFile_.print('\n');
        } else {
            addPipeTextNewline_ = true;
            addLiteralHTMLNewline_ = false;
        }
    } else if (isLiteralHTML) {
        if (addLiteralHTMLNewline_) {
            outFile_.print('\n');
        } else {
            addLiteralHTMLNewline_ = true;
            addPipeTextNewline_ = false;
        }
    } else {
        addPipeTextNewline_ = false;
        addLiteralHTMLNewline_ = false;
    }
}

void Parser::parseDoctype(DoctypeData *data) {
    // Set the current doctype if its not set yet
    if (doctype_ == DoctypeDialect::None) {
        switch (data->doctypeType) {
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
    outFile_.print(data->toHTMLString());
}

void Parser::parseTag(TagData *data) {
    // Close previous tag if not indentet
    closeTagIfNecessary();
    // Handle the pipe newline
    handleTextNewline();

    // Open the tag
    outFile_.printf("<%s", data->name.c_str());

    // Add attributes
    for (Attribute attribute : data->attributes) {
        // Skip boolean attributes with no value
        if (attribute.booleanAttribute && !attribute.checked) {
            continue;
        }

        // Add the attribute
        outFile_.printf(" %s", attribute.key.c_str());
        if (attribute.booleanAttribute && doctype_ != DoctypeDialect::HTML) {
            outFile_.printf("=\"%s\"", attribute.key.c_str());
        } else if (!attribute.booleanAttribute) {
            outFile_.printf("=\"%s\"", attribute.value.c_str());
        }
    }

    // (Forced) void element?
    if (data->isVoidElement) {
        outFile_.print("/>");

        tags_.push_back("");
    } else if (isVoidElement(data->name)) {
        switch (doctype_) {
            case DoctypeDialect::HTML:
                outFile_.print(">");
                break;
            case DoctypeDialect::XML:
                outFile_.printf("></%s>", data->name.c_str());
                break;
            case DoctypeDialect::None:
                outFile_.print("/>");
        }

        tags_.push_back("");
    } else {
        outFile_.printf(">%s", data->text.c_str());
        tags_.push_back(data->name);
    }
}

void Parser::parseText(TextData *data) {
    // Close previous tag if not indentet
    closeTagIfNecessary();
    // Handle the pipe newline
    handleTextNewline(data->pipeText, !data->pipeText);

    // Add the text to the output
    outFile_.print(data->value.c_str());

    tags_.push_back("");
}

void Parser::parseComment(CommentData *data) {
    // Close previous tag if not indentet
    closeTagIfNecessary();
    // Handle the pipe newline
    handleTextNewline();

    // Add the comment to the output
    outFile_.printf("<!--%s-->", data->value.c_str());

    tags_.push_back("");
}

bool Parser::parseInclude(IncludeData *data) {
    // Close previous tag if not indentet
    closeTagIfNecessary();
    // Handle the pipe newline
    handleTextNewline();

    // Get the includeFilePath
    String includeFilePath =
        data->path[0] == '/' ? data->path : directory_ + data->path;

    // Open the file (relative or absolute)
    File includeFile = LittleFS.open(includeFilePath, "r");

    // Check that it worked
    if (!includeFile) {
        Serial.printf(
            "Error 2-4: Failed to open include file '%s'\n",
            includeFilePath.c_str()
        );
        return false;
    }

    // Parse the file if it is a pug file
    if (includeFilePath.length() > 5 && data->path.endsWith(".pug")) {
        String outPath = includeFilePath + ".html";

        Parser parser(includeFilePath, outPath, doctype_);
        if (!parser.parse()) {
            return false;
        }
    }

    // Append the file
    outFile_.print(includeFile.readString().c_str());

    // Close the file
    includeFile.close();

    tags_.push_back("");

    return true;
}
