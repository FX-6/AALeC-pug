#include "AALeC-pug.h"

#include <LittleFS.h>

#include "parser/parser.h"

bool aalec_pug(String inPath, String outPath) {
    File inFile = LittleFS.open(inPath, "r");

    if (outPath == "") {
        // Default output path
        outPath = inPath + ".html";
    }

    // Infile doesn't exist
    if (!inFile.isFile()) {
        return -1;
    }

    inFile.close();

    Parser parser = Parser(inPath, outPath);

    return parser.parse();
}
