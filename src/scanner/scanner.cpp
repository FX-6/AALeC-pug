#include "scanner.h"

Scanner::Scanner(String inPath) :
    indentationChar_('.'),
    indentationLevel_(std::vector<int>()) {
    inFile_ = LittleFS.open(inPath, "r");

    if (!inFile_) {
        Serial.printf(
            "Error 1-1: Failed to open file for reading '%s'\n",
            inPath.c_str()
        );
        return;
    }
}

Scanner::~Scanner() {
    inFile_.close();
}

bool Scanner::scanPart(std::vector<Token> *tokens) {
    // Scan the indentation if thers is any
    if (isWhitespace()) {
        if (!scanIndentation(tokens)) {
            return false;
        }

        // Remove additional 0 size indentations from block expansion
        if (indentationLevel_.size() > 0 && indentationLevel_.back() == 0) {
            indentationLevel_.pop_back();
            tokens->push_back(Token(TokenType::Dedent));
        }
    } else if (check(':')) {
        // Block expansion, add a Indent Token and a 0 size indent level
        tokens->push_back(Token(TokenType::Indent));
        indentationLevel_.push_back(0);

        // Ignore the colon and following whitespace
        ignore();
        ignoreWhitespaces();
    } else if (indentationLevel_.size() > 0) {
        // If there is an indentation level, but no indentation, its a dedent
        while (indentationLevel_.size() > 0) {
            indentationLevel_.pop_back();
            tokens->push_back(Token(TokenType::Dedent));
        }

        // Remove additional 0 size indentations from block expansion
        if (indentationLevel_.size() > 0 && indentationLevel_.back() == 0) {
            indentationLevel_.pop_back();
            tokens->push_back(Token(TokenType::Dedent));
        }
    }

    // Scan a token if there is a token
    if (check("doctype")) {
        DoctypeData *data = nullptr;
        if (!scanDoctype(data)) {
            return false;
        }
        tokens->push_back(Token(data));
    } else if (check("<") || check('|')) {
        TextData *data = nullptr;
        if (!scanText(data)) {
            return false;
        }
        tokens->push_back(Token(data));
    } else if (check("//-")) {
        if (!ignoreComment()) {
            return false;
        }
    } else if (check("//")) {
        CommentData *data = nullptr;
        if (!scanComment(data)) {
            return false;
        }
        tokens->push_back(Token(data));
    } else if (check("include")) {
        IncludeData *data = nullptr;
        if (!scanInclude(data)) {
            return false;
        }
        tokens->push_back(Token(data));
    } else if (isIdentifierPart() || check('#') || check('.')) {
        TagData *data = nullptr;
        if (!scanTag(data)) {
            return false;
        }
        tokens->push_back(Token(data));
    }

    // Handle the part after the token
    if (isEndOfSource()) {
        tokens->push_back(Token(TokenType::EndOfSource));
    } else if (check('\n')) {
        ignore();
        tokens->push_back(Token(TokenType::EndOfPart));
    } else if (check(':')) {
        tokens->push_back(Token(TokenType::EndOfPart));
    } else {
        Serial.printf(
            "Error 1-2: Unexpected character (ASCII code: '%d')\n",
            inFile_.peek()
        );
        return false;
    }

    return true;
}

bool Scanner::check(char value) {
    return inFile_.peek() == value;
}

bool Scanner::check(String value) {
    // Remember the start position
    int start = inFile_.position();
    bool res = true;

    // Check each character
    for (char c : value) {
        if (inFile_.read() != c) {
            // Set the return value, end the loop
            res = false;
            break;
        }
    }

    // Reset the stream position
    inFile_.seek(start, SeekSet);
    return res;
}

String Scanner::consume(int amount) {
    String value = "";

    for (int i = 0; i < amount; i++) {
        value += (char)inFile_.read();
    }

    return value;
}

void Scanner::ignore(int amount) {
    inFile_.seek(amount, SeekCur);
}

void Scanner::ignoreWhitespaces(bool includeNewlines) {
    while (check(' ') || check('\t') || (includeNewlines && check('\n'))) {
        ignore();
    }
}

bool Scanner::isWhitespace() {
    return check(' ') || check('\t');
}

bool Scanner::isIdentifierPart() {
    return isAlphaNumeric(inFile_.peek()) || check('_');
}

bool Scanner::isEndOfSource() {
    return inFile_.peek() == -1;
}

bool Scanner::nextLineIndentationIsHigher() {
    // Get total size of current indentation
    int currentSize = 0;
    for (int size : indentationLevel_) {
        currentSize += size;
    }

    // Set the indentation char if not alread set
    if (indentationChar_ == '.') {
        // Not set...
        if (check("\n ")) {
            // ... and next line is indentet with a space
            indentationChar_ = ' ';
        } else if (check("\n\t")) {
            // ... and next line is indentet with a tab
            indentationChar_ = '\t';
        } else {
            // ... but next line isn't indentet anyways
            return false;
        }
    }

    // Generate string with '\n' + indentation size * indentation char + extra indentation char
    String higherIndentation = "\n";

    for (int i = 0; i < currentSize; i++) {
        higherIndentation += indentationChar_;
    }

    higherIndentation += indentationChar_;

    return check(higherIndentation);
}

bool Scanner::scanIndentation(std::vector<Token> *tokens) {
    // Set the used indentation char if not already set
    if (indentationChar_ == '.') {
        indentationChar_ = inFile_.peek();
    }

    // Check on what level we are
    uint level;
    for (level = 0; level < indentationLevel_.size(); level++) {
        int levelSize = indentationLevel_[level];

        // Create string
        String indentationString = "";
        for (int i = 0; i < levelSize; i++) {
            indentationString += indentationChar_;
        }

        if (check(indentationString)) {
            ignore(levelSize);
        } else {
            break;
        }
    }

    // If there are no more indentation chars
    if (!check(indentationChar_)) {
        // We are either on the same level -> nothing must be done
        // or a smaller level -> generate dedents and remove the levels
        while (level < indentationLevel_.size()) {
            indentationLevel_.pop_back();
            tokens->push_back(Token(TokenType::Dedent));
        }
    } else {
        // If there are more indentation chars
        // We are either on a new level -> add a new level
        // or the indentation chars are not enough to reach the next level -> error
        if (level == indentationLevel_.size()) {
            int newLevelSize = 0;

            while (check(indentationChar_)) {
                newLevelSize++;
                ignore();
            }

            indentationLevel_.push_back(newLevelSize);
            tokens->push_back(Token(TokenType::Indent));
        } else {
            Serial.printf("Error 1-3: Wrong indentation amount\n");
            return false;
        }
    }

    // If there are still spaces or tabs left
    // That means the wrong character was used for indentation
    if (check(' ') || check('\t')) {
        Serial.printf(
            "Error 1-4: Wrong indentation character (ASCII code: '%d')\n",
            inFile_.peek()
        );
        return false;
    }

    return true;
}

bool Scanner::scanDoctype(DoctypeData *&data) {
    // Ignore the "doctype" keyword and following spaces/tabs
    ignore(7);
    ignoreWhitespaces();

    // Get the doctype value
    String value = "";
    while (!check('\n')) {
        value += consume();
    }

    // Create the data
    if (value == "html" || value == "") {
        data = new DoctypeData(value, DoctypeShorthand::Html);
    } else if (value == "xml") {
        data = new DoctypeData(value, DoctypeShorthand::Xml);
    } else if (value == "transitional") {
        data = new DoctypeData(value, DoctypeShorthand::Transitional);
    } else if (value == "strict") {
        data = new DoctypeData(value, DoctypeShorthand::Strict);
    } else if (value == "frameset") {
        data = new DoctypeData(value, DoctypeShorthand::Frameset);
    } else if (value == "1.1") {
        data = new DoctypeData(value, DoctypeShorthand::OneDotOne);
    } else if (value == "basic") {
        data = new DoctypeData(value, DoctypeShorthand::Basic);
    } else if (value == "mobile") {
        data = new DoctypeData(value, DoctypeShorthand::Mobile);
    } else if (value == "plist") {
        data = new DoctypeData(value, DoctypeShorthand::Plist);
    } else {
        data = new DoctypeData(value, DoctypeShorthand::Other);
    }

    return true;
}

bool Scanner::scanTag(TagData *&data) {
    // Data required for the tag
    String name = "";
    String idLiteral = "";
    String classLiteral = "";
    std::vector<Attribute> attributes = std::vector<Attribute>();
    bool forcedVoidElement = false;
    String text = "";

    // Get the tag name
    if (isIdentifierPart()) {
        while (isIdentifierPart()) {
            name += consume();
        }
    } else if (check('#') || check('.')) {
        name = "div";
    }

    // ID Literal
    if (check('#')) {
        ignore();

        while (isIdentifierPart()) {
            idLiteral += consume();
        }
    }

    // Class Literal
    if (check('.') && !check(".\n")) {
        ignore();

        while (isIdentifierPart()) {
            classLiteral += consume();
        }
    }

    // Add the class literal if it exists (should be first attribute)
    if (classLiteral != "") {
        attributes.push_back(Attribute("class", classLiteral));
    }

    // Add the id literal if it exists (should be after the class literal)
    if (idLiteral != "") {
        attributes.push_back(Attribute("id", idLiteral));
    }

    // Scan attributes if there are any (should be after the id literal)
    if (check('(')) {
        // Get the additional attributes
        if (!scanTagAttributes(&attributes)) {
            return false;
        }
    }

    // Forced void element, text, block expansion, or nothing
    if (check('/')) {
        ignore();
        forcedVoidElement = true;
    } else if (check(' ') || check(".\n")) {
        text = scanTagText();
    } else if (!check(':') && !check('\n')) {
        Serial.printf(
            "Error 1-5: Unexpected character (ASCII code: '%d')\n",
            inFile_.peek()
        );
        return false;
    }

    data = new TagData(name, attributes, forcedVoidElement, text);
    return true;
}

bool Scanner::scanTagAttributes(std::vector<Attribute> *attributes) {
    // Ignore the leading '('
    if (check('(')) {
        ignore();
    } else {
        Serial.printf(
            "Error 1-6: Unexpected character (ASCII code: '%d')\n",
            inFile_.peek()
        );
        return false;
    }

    while (!check(')')) {
        // To allow an empty key with value
        bool quotedAttribute = false;

        // Ignore whitespaces before the attribute
        ignoreWhitespaces(true);

        // Get the key (possibly quoted)
        String key = "";
        if (check('"') || check('\'')) {
            quotedAttribute = true;

            // Get the quote
            char quote = consume()[0];

            // Consume until the quote
            // Backslash escaping doesnt matter
            while (!check(quote)) {
                key += consume();
            }

            // Ignore the closing quote
            ignore();
        } else {
            while (isIdentifierPart()) {
                key += consume();
            }
        }

        // Ignore whitespaces after the key
        ignoreWhitespaces(true);

        // Unescaped?
        bool escaped = true;
        if (check("!")) {
            ignore();
            escaped = false;
        }

        // Ignore the "=" if there is one
        String value = "";
        bool booleanAttribute = false;
        bool checked = false;
        if (check('=')) {
            ignore();

            // Ignore the whitespaces between the "=" and the value
            ignoreWhitespaces(true);

            // Get the value
            if (check('"') || check('\'')) {
                // Get the quote
                char quote = consume()[0];

                // Consume until the quote
                // ToDo: Backslash escaping
                while (!check(quote)) {
                    String c = consume();

                    // Replace bad chars if escaped
                    if (escaped) {
                        if (c == "\"") {
                            c = "&quot;";
                        } else if (c == "<") {
                            c = "&lt;";
                        } else if (c == ">") {
                            c = "&gt;";
                        } else if (c == "&") {
                            c = "&amp;";
                        }
                    }

                    value += c;
                }

                // Ignore the closing quote
                ignore();
            } else if (check("true")) {
                ignore(4);
                booleanAttribute = true;
                checked = true;
            } else if (check("false")) {
                ignore(5);
                booleanAttribute = true;
                checked = false;
            } else {
                Serial.printf(
                    "Error 1-7: Unexpected character (ASCII code: '%d')\n",
                    inFile_.peek()
                );
                return false;
            }
        } else {
            // Boolean attribute with no value
            booleanAttribute = true;
            checked = true;
        }

        // Ignore whitespaces and the possible commas after the attribute
        ignoreWhitespaces(true);
        if (check(',')) {
            ignore();
            ignoreWhitespaces(true);

            // Was it an empty space before a comma?
            if (key == "" && !quotedAttribute) {
                attributes->push_back(Attribute());
                continue;
            }
        }

        // Add the attribute
        if (booleanAttribute) {
            attributes->push_back(Attribute(key, checked));
        } else {
            attributes->push_back(Attribute(key, value));
        }
    }

    // Consume the trailing ')'
    ignore();

    return true;
}

String Scanner::scanTagText() {
    String text = "";

    // Inline in a tag or block in a tag?
    if (check(' ')) {
        // Inline in a tag

        // Ignore the leading space
        ignore();

        // Consume until the '\n'
        while (!check('\n')) {
            text += consume();
        }
    } else if (check(".\n")) {
        // Block in a tag

        // Ignore the leading "."
        ignore();

        // While indentation is higher, consume lines
        while (nextLineIndentationIsHigher()) {
            // Consume the '\n' of the current line, ignore the first
            if (text != "") {
                text += consume();
            } else {
                ignore();
            }

            // Ignore the whitespace between the '\n' and the next line
            ignoreWhitespaces();

            // Consume the next line up until the '\n'
            while (!check('\n')) {
                text += consume();
            }
        }
    }

    return text;
}

bool Scanner::scanText(TextData *&data) {
    bool pipeText = false;
    String value = "";

    // Ignore the pipe and following whitespaces
    if (check('|')) {
        ignore();
        ignoreWhitespaces();
        pipeText = true;
    }

    while (!check('\n')) {
        value += consume();
    }

    data = new TextData(value, pipeText);
    return true;
}

bool Scanner::ignoreComment() {
    // Ignore this line up until the '\n'
    while (!check('\n')) {
        ignore();
    }

    // While indentation is higher, ignore lines
    while (nextLineIndentationIsHigher()) {
        // Ignore the '\n' of the current line
        ignore();

        // Ignore the next line up until the '\n'
        while (!check('\n')) {
            ignore();
        }
    }

    return true;
}

bool Scanner::scanComment(CommentData *&data) {
    // Ignore the leading "//"
    ignore(2);

    // Text of the comment
    String value = "";

    // Consume this line up until the '\n'
    while (!check('\n')) {
        value += consume();
    }

    // Ignore the '\n' of the first line (only to mimic PUG closer)
    bool firstLine = true;

    // While indentation is higher, consume lines
    while (nextLineIndentationIsHigher()) {
        // Consume the '\n' of the current line, ignore it if it is the first line
        if (!firstLine) {
            value += consume();
        } else {
            ignore();
            firstLine = false;
        }

        // Ignore the whitespace between the '\n' and the next line
        ignoreWhitespaces();

        // Consume the next line up until the '\n'
        while (!check('\n')) {
            value += consume();
        }
    }

    data = new CommentData(value);
    return true;
}

bool Scanner::scanInclude(IncludeData *&data) {
    // Ignore the leading "include"
    ignore(7);

    // Ignore whitespaces
    ignoreWhitespaces();

    // Get the path
    String path = "";

    while (!check('\n')) {
        path += consume();
    }

    data = new IncludeData(path);
    return true;
}
