#ifndef AALEC_PUG_H
#define AALEC_PUG_H

#include <Arduino.h>

/**
 * @brief Compiles a given pug file
 *
 * @param inPath Path to the pug file
 * @param outPath Path to the output file (optional), default is inPath + ".html"
 * @return true Compiling was successfull
 * @return false Compiling was unsuccessfull, see serial output for details
 */
bool aalec_pug(String inPath, String outPath = "");

#endif  // AALEC_PUG_H
