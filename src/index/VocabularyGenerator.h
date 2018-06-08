// Copyright 2018, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Johannes Kalmbach <johannes.kalmbach@gmail.com>
#pragma once

#include <string>
#include <sparsehash/sparse_hash_map>

#include "../global/Id.h"
#include "../global/Constants.h"

using std::string;
// _______________________________________________________________
// merge the partial vocabularies at basenamepartialVocabulary0 to basename +
// partialVocabulary + numFiles-1
// Directly Writes .vocabulary file at basename (no more need to pass through Vocabulary
// class
// Writes file "externalTextFile" which can be used to directly write external
// Literals
void mergeVocabulary(const std::string& basename, size_t numFiles);

// __________________________________________________________________________________________
// read the words and indices from the file and create hash map from it.
google::sparse_hash_map<string, Id> vocabMapFromPartialIndexedFile(const string& partialFile);
