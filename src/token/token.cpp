#include "token.h"

Token::Token(TokenType type) : type(type) {}

Token::Token(DoctypeData data) : type(TokenType::Doctype), doctype(data) {}

Token::Token(TagData data) : type(TokenType::Tag), tag(data) {}

Token::Token(TextData data) : type(TokenType::Text), text(data) {}

Token::Token(CommentData data) : type(TokenType::Comment), comment(data) {}

Token::Token(IncludeData data) : type(TokenType::Include), include(data) {}

DoctypeData::DoctypeData() : value(""), doctypeType(DoctypeShorthand::Other) {}

DoctypeData::DoctypeData(String value, DoctypeShorthand doctypeType) :
    value(value),
    doctypeType(doctypeType) {}

String DoctypeData::toHTMLString() {
    switch (doctypeType) {
        case DoctypeShorthand::Html:
            return "<!DOCTYPE html>";
        case DoctypeShorthand::Xml:
            return "<?xml version=\"1.0\" encoding=\"utf-8\" ?>";
        case DoctypeShorthand::Transitional:
            return "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">";
        case DoctypeShorthand::Strict:
            return "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\">";
        case DoctypeShorthand::Frameset:
            return "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Frameset//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-frameset.dtd\">";
        case DoctypeShorthand::OneDotOne:
            return "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">";
        case DoctypeShorthand::Basic:
            return "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML Basic 1.1//EN\" \"http://www.w3.org/TR/xhtml-basic/xhtml-basic11.dtd\">";
        case DoctypeShorthand::Mobile:
            return "<!DOCTYPE html PUBLIC \"-//WAPFORUM//DTD XHTML Mobile 1.2//EN\" \"http://www.openmobilealliance.org/tech/DTD/xhtml-mobile12.dtd\">";
        case DoctypeShorthand::Plist:
            return "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">";
        default:
            return "<!DOCTYPE " + value + ">";
    }
}

Attribute::Attribute() : key(""), booleanAttribute(false), value("") {}

Attribute::Attribute(String key) :
    key(key),
    booleanAttribute(true),
    value("") {}

Attribute::Attribute(String key, String value) :
    key(key),
    booleanAttribute(false),
    value(value) {}

TagData::TagData() : name(""), attributes(), isVoidElement(false), text("") {}

TagData::TagData(
    String name,
    std::vector<Attribute> attributes,
    bool isVoidElement,
    String text
) :
    name(name),
    attributes(attributes),
    isVoidElement(isVoidElement),
    text(text) {}

TextData::TextData() : value(""), textType(TextType::InnerText) {}

TextData::TextData(String value, TextType textType) :
    value(value),
    textType(textType) {}

CommentData::CommentData() : value("") {}

CommentData::CommentData(String value) : value(value) {}

IncludeData::IncludeData() : path("") {}

IncludeData::IncludeData(String path) : path(path) {}
