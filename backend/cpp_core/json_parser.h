// json_parser.h
#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include <string>
#include <vector>
#include "train.h"

std::vector<Train> loadTrainsFromJSON(const std::string& filename);

#endif // JSON_PARSER_H
