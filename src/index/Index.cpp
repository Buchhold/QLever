// Copyright 2014, University of Freiburg,
// Chair of Algorithms and Data Structures.
// Author: Björn Buchhold (buchhold@informatik.uni-freiburg.de)

#include "./Index.h"
#include <algorithm>
#include <cmath>
#include <stxxl/algorithm>
#include <stxxl/map>
#include <unordered_set>
#include "../parser/NTriplesParser.h"
#include "../parser/TsvParser.h"
#include "../util/Conversions.h"
#include "./VocabularyGenerator.h"

using std::array;

// _____________________________________________________________________________
Index::Index()
    : _usePatterns(false),
      _maxNumPatterns(std::numeric_limits<PatternID>::max() - 2) {}

// _____________________________________________________________________________
void Index::createFromTsvFile(const string& tsvFile, const string& onDiskBase,
                              bool allPermutations, bool onDiskLiterals) {
  _onDiskBase = onDiskBase;
  string indexFilename = _onDiskBase + ".index";
  size_t nofLines = passTsvFileForVocabulary(tsvFile, onDiskLiterals);
  ExtVec v(nofLines);
  passTsvFileIntoIdVector(tsvFile, v, onDiskLiterals);
  if (onDiskLiterals) {
    _vocab.externalizeLiterals(_onDiskBase + ".literals-index");
  }
  _vocab.writeToFile(onDiskBase + ".vocabulary");
  // PSO permutation
  LOG(INFO) << "Sorting for PSO permutation..." << std::endl;
  stxxl::sort(begin(v), end(v), SortByPSO(), STXXL_MEMORY_TO_USE);
  LOG(INFO) << "Sort done." << std::endl;
  LOG(INFO) << "Performing unique to ensure RDF semantics..." << std::endl;
  LOG(INFO) << "Size before: " << v.size() << std::endl;
  auto last = std::unique(begin(v), end(v));
  v.resize(size_t(last - v.begin()));
  LOG(INFO) << "Done: unique." << std::endl;
  LOG(INFO) << "Size after: " << v.size() << std::endl;
  createPermutation(indexFilename + ".pso", v, _psoMeta, 1, 0, 2);
  // POS permutation
  LOG(INFO) << "Sorting for POS permutation..." << std::endl;
  ;
  stxxl::sort(begin(v), end(v), SortByPOS(), STXXL_MEMORY_TO_USE);
  LOG(INFO) << "Sort done." << std::endl;
  createPermutation(indexFilename + ".pos", v, _posMeta, 1, 2, 0);
  if (allPermutations) {
    // SPO permutation
    LOG(INFO) << "Sorting for SPO permutation..." << std::endl;
    stxxl::sort(begin(v), end(v), SortBySPO(), STXXL_MEMORY_TO_USE);
    LOG(INFO) << "Sort done." << std::endl;
    createPermutation(indexFilename + ".spo", v, _spoMeta, 0, 1, 2);
    if (_usePatterns) {
      LOG(INFO) << "Vector already sorted for pattern creation." << std::endl;
      createPatterns(indexFilename + ".patterns", v, _hasRelation, _hasPattern,
                     _patterns, _maxNumPatterns);
    }
    // SOP permutation
    LOG(INFO) << "Sorting for SOP permutation..." << std::endl;
    stxxl::sort(begin(v), end(v), SortBySOP(), STXXL_MEMORY_TO_USE);
    LOG(INFO) << "Sort done." << std::endl;
    createPermutation(indexFilename + ".sop", v, _sopMeta, 0, 2, 1);
    // OSP permutation
    LOG(INFO) << "Sorting for OSP permutation..." << std::endl;
    stxxl::sort(begin(v), end(v), SortByOSP(), STXXL_MEMORY_TO_USE);
    LOG(INFO) << "Sort done." << std::endl;
    createPermutation(indexFilename + ".osp", v, _ospMeta, 2, 0, 1);
    // OPS permutation
    LOG(INFO) << "Sorting for OPS permutation..." << std::endl;
    stxxl::sort(begin(v), end(v), SortByOPS(), STXXL_MEMORY_TO_USE);
    LOG(INFO) << "Sort done." << std::endl;
    createPermutation(indexFilename + ".ops", v, _opsMeta, 2, 1, 0);
  } else if (_usePatterns) {
    LOG(INFO) << "Sorting for pattern creation..." << std::endl;
    stxxl::sort(begin(v), end(v), SortBySPO(), STXXL_MEMORY_TO_USE);
    LOG(INFO) << "Sort done." << std::endl;
    createPatterns(indexFilename + ".patterns", v, _hasRelation, _hasPattern,
                   _patterns, _maxNumPatterns);
  }
  openFileHandles();
}

// _____________________________________________________________________________
void Index::createFromNTriplesFile(const string& ntFile,
                                   const string& onDiskBase,
                                   bool allPermutations, bool onDiskLiterals) {
  _onDiskBase = onDiskBase;
  string indexFilename = _onDiskBase + ".index";
  size_t nofLines = passNTriplesFileForVocabulary(ntFile, onDiskLiterals, 10000);

  if (onDiskLiterals) {
    _vocab.externalizeLiteralsFromTextFile(onDiskBase + "externalTextFile", onDiskBase + ".literals-index");
  }

  // clear vocabulary to save ram (only iformation from partial binary files used from now on).
  // TODO: this can be done directly when merging the partial vocabularies.
  _vocab = Vocabulary();
  ExtVec v(nofLines);
  passNTriplesFileIntoIdVector(ntFile, v, onDiskLiterals, 10000);
  LOG(INFO) << "Sorting for PSO permutation..." << std::endl;
  stxxl::sort(begin(v), end(v), SortByPSO(), STXXL_MEMORY_TO_USE);
  LOG(INFO) << "Sort done." << std::endl;
  LOG(INFO) << "Performing unique to ensure RDF semantics..." << std::endl;
  LOG(INFO) << "Size before: " << v.size() << std::endl;
  auto last = std::unique(begin(v), end(v));
  v.resize(size_t(last - v.begin()));
  LOG(INFO) << "Done: unique." << std::endl;
  LOG(INFO) << "Size after: " << v.size() << std::endl;
  createPermutation(indexFilename + ".pso", v, _psoMeta, 1, 0, 2);
  LOG(INFO) << "Sorting for POS permutation..." << std::endl;
  ;
  stxxl::sort(begin(v), end(v), SortByPOS(), STXXL_MEMORY_TO_USE);
  LOG(INFO) << "Sort done." << std::endl;
  ;
  createPermutation(indexFilename + ".pos", v, _posMeta, 1, 2, 0);
  if (allPermutations) {
    // SPO permutation
    LOG(INFO) << "Sorting for SPO permutation..." << std::endl;
    stxxl::sort(begin(v), end(v), SortBySPO(), STXXL_MEMORY_TO_USE);
    LOG(INFO) << "Sort done." << std::endl;
    createPermutation(indexFilename + ".spo", v, _spoMeta, 0, 1, 2);
    if (_usePatterns) {
      LOG(INFO) << "Vector already sorted for pattern creation." << std::endl;
      createPatterns(indexFilename + ".patterns", v, _hasRelation, _hasPattern,
                     _patterns, _maxNumPatterns);
    }
    // SOP permutation
    LOG(INFO) << "Sorting for SOP permutation..." << std::endl;
    stxxl::sort(begin(v), end(v), SortBySOP(), STXXL_MEMORY_TO_USE);
    LOG(INFO) << "Sort done." << std::endl;
    createPermutation(indexFilename + ".sop", v, _sopMeta, 0, 2, 1);
    // OSP permutation
    LOG(INFO) << "Sorting for OSP permutation..." << std::endl;
    stxxl::sort(begin(v), end(v), SortByOSP(), STXXL_MEMORY_TO_USE);
    LOG(INFO) << "Sort done." << std::endl;
    createPermutation(indexFilename + ".osp", v, _ospMeta, 2, 0, 1);
    // OPS permutation
    LOG(INFO) << "Sorting for OPS permutation..." << std::endl;
    stxxl::sort(begin(v), end(v), SortByOPS(), STXXL_MEMORY_TO_USE);
    LOG(INFO) << "Sort done." << std::endl;
    createPermutation(indexFilename + ".ops", v, _opsMeta, 2, 1, 0);
  } else if (_usePatterns) {
    LOG(INFO) << "Sorting for pattern creation..." << std::endl;
    stxxl::sort(begin(v), end(v), SortBySPO(), STXXL_MEMORY_TO_USE);
    LOG(INFO) << "Sort done." << std::endl;
    createPatterns(indexFilename + ".patterns", v, _hasRelation, _hasPattern,
                   _patterns, _maxNumPatterns);
  }
  openFileHandles();
}

// _____________________________________________________________________________
size_t Index::passTsvFileForVocabulary(const string& tsvFile,
                                       bool onDiskLiterals) {
  LOG(INFO) << "Making pass over TsvFile " << tsvFile << " for vocabulary."
            << std::endl;
  array<string, 3> spo;
  TsvParser p(tsvFile);
  ad_utility::HashSet<string> items;
  size_t i = 0;
  while (p.getLine(spo)) {
    if (ad_utility::isXsdValue(spo[2])) {
      spo[2] = ad_utility::convertValueLiteralToIndexWord(spo[2]);
    }
    if (onDiskLiterals && isLiteral(spo[2]) && shouldBeExternalized(spo[2])) {
      spo[2] = string({EXTERNALIZED_LITERALS_PREFIX}) + spo[2];
    }
    items.insert(spo[0]);
    items.insert(spo[1]);
    items.insert(spo[2]);
    ++i;
    if (i % 10000000 == 0) {
      LOG(INFO) << "Lines processed: " << i << '\n';
    }
  }
  LOG(INFO) << "Pass done.\n";
  _vocab.createFromSet(items);
  return i;
}

// _____________________________________________________________________________
void Index::passTsvFileIntoIdVector(const string& tsvFile, ExtVec& data,
                                    bool onDiskLiterals) {
  LOG(INFO) << "Making pass over TsvFile " << tsvFile
            << " and creating stxxl vector.\n";
  array<string, 3> spo;
  TsvParser p(tsvFile);
  auto vocabMap = _vocab.asMap();
  size_t i = 0;
  // write using vector_bufwriter
  ExtVec::bufwriter_type writer(data);
  while (p.getLine(spo)) {
    if (ad_utility::isXsdValue(spo[2])) {
      spo[2] = ad_utility::convertValueLiteralToIndexWord(spo[2]);
    }
    if (onDiskLiterals && isLiteral(spo[2]) && shouldBeExternalized(spo[2])) {
      spo[2] = string({EXTERNALIZED_LITERALS_PREFIX}) + spo[2];
    }
    writer << array<Id, 3>{{vocabMap.find(spo[0])->second,
                            vocabMap.find(spo[1])->second,
                            vocabMap.find(spo[2])->second}};
    ++i;
    if (i % 10000000 == 0) {
      LOG(INFO) << "Lines processed: " << i << '\n';
    }
  }
  writer.finish();
  LOG(INFO) << "Pass done.\n";
}

// _____________________________________________________________________________
size_t Index::passNTriplesFileForVocabulary(const string& ntFile,
                                            bool onDiskLiterals, size_t linesPerPartial) {
  array<string, 3> spo; 
  NTriplesParser p(ntFile);
  ad_utility::HashSet<string> items;
  size_t i = 0; 
  size_t numFiles = 0; 
  while (p.getLine(spo)) {
    if (ad_utility::isXsdValue(spo[2])) {
      spo[2] = ad_utility::convertValueLiteralToIndexWord(spo[2]);
    }    
    if (onDiskLiterals && isLiteral(spo[2]) && shouldBeExternalized(spo[2])) {
      spo[2] = string({EXTERNALIZED_LITERALS_PREFIX}) + spo[2];
    }    
    
    // Duplicated in pass for Id Vector, externalize to function
    for (size_t k = 0; k < 3; ++k) {
      items.insert(spo[k]);
    }

    ++i;
    if (i % 10000000 == 0) {
      std::cout << "Lines processed: " << i << '\n';
    }

    if (i % linesPerPartial == 0) {
      LOG(INFO) << "Lines processed: " << i << '\n';
      std::cout << "writing partial vocab no. " << numFiles << std::endl;
      Vocabulary vocab;
      vocab.createFromSet(items);
      vocab.writeToBinaryFileForMerging(_onDiskBase + "partialVocabulary" + std::to_string(numFiles));
      items.clear();
      numFiles++;
    }
  }
  // write Remainder
  //
  LOG(INFO) << "Lines processed: " << i << '\n';
  if (items.size() > 0) {
    // write remainder
    
    std::cout << "writing partial vocab no. " << numFiles << std::endl;
    Vocabulary vocab;
    vocab.createFromSet(items);
    vocab.writeToBinaryFileForMerging(_onDiskBase + "partialVocabulary" + std::to_string(numFiles));
    items.clear();
    numFiles++;
  }
  LOG(INFO) << "Merging vocabulary\n";
  mergeVocabulary(_onDiskBase, numFiles);
  LOG(INFO) << "Pass done.\n";
  return i;
}

// _____________________________________________________________________________
void Index::passNTriplesFileIntoIdVector(const string& ntFile, ExtVec& data,
                                         bool onDiskLiterals, size_t linesPerPartial) {
  LOG(INFO) << "Making pass over NTriples " << ntFile
            << " and creating stxxl vector.\n";
  array<string, 3> spo;
  NTriplesParser p(ntFile);
  LOG(INFO) << "Reading partial vocab ...\n";
  google::sparse_hash_map<string, Id> vocabMap = vocabMapFromPartialIndexedFile(_onDiskBase + "partialVocabulary0");
  LOG(INFO) << "done reading partial vocab\n"; 
  size_t i = 0;
  size_t numFiles = 0;
  // write using vector_bufwriter
  ExtVec::bufwriter_type writer(data);
  while (p.getLine(spo)) {
    if (ad_utility::isXsdValue(spo[2])) {
      spo[2] = ad_utility::convertValueLiteralToIndexWord(spo[2]);
    }
    if (onDiskLiterals && isLiteral(spo[2]) && shouldBeExternalized(spo[2])) {
      spo[2] = string({EXTERNALIZED_LITERALS_PREFIX}) + spo[2];
    }

    // Duplicated in pass for Id Vector, externalize to function
    bool broken = false;
    for (size_t k = 0; k < 3; ++k) {
      if (vocabMap.find(spo[k]) == vocabMap.end()) {
        LOG(INFO) << "not found in partial Vocab: " << spo[k] << '\n';
        broken = true;
      }
    }
    if (broken) continue;
    writer << array<Id, 3>{{
                               vocabMap.find(spo[0])->second,
                               vocabMap.find(spo[1])->second,
                               vocabMap.find(spo[2])->second
                           }};
    ++i;
    if (i % 100000 == 0) {
      LOG(INFO) << "Lines processed: " << i << '\n';
    }

    if (i % linesPerPartial == 0) {
      numFiles++;
      LOG(INFO) << "Lines processed: " << i << '\n';
      std::cout << "reading partial vocab no. " << numFiles << std::endl;
      vocabMap = vocabMapFromPartialIndexedFile(_onDiskBase + "partialVocabulary" + std::to_string(numFiles));
      LOG(INFO) << "done reading partial vocab\n";
    }
  }
  writer.finish();
  LOG(INFO) << "Pass done.\n";
}

// _____________________________________________________________________________
void Index::createPermutation(const string& fileName, Index::ExtVec const& vec,
                              IndexMetaData& metaData, size_t c0, size_t c1,
                              size_t c2) {
  if (vec.size() == 0) {
    LOG(WARN) << "Attempt to write an empty index!" << std::endl;
    return;
  }
  ad_utility::File out(fileName.c_str(), "w");
  LOG(INFO) << "Creating an on-disk index permutation of " << vec.size()
            << " elements / facts." << std::endl;
  // Iterate over the vector and identify relation boundaries
  size_t from = 0;
  Id currentRel = vec[0][c0];
  off_t lastOffset = 0;
  vector<array<Id, 2>> buffer;
  bool functional = true;
  Id lastLhs = std::numeric_limits<Id>::max();
  for (ExtVec::bufreader_type reader(vec); !reader.empty(); ++reader) {
    if ((*reader)[c0] != currentRel) {
      auto md = writeRel(out, lastOffset, currentRel, buffer, functional);
      metaData.add(md.first, md.second);
      buffer.clear();
      lastOffset = metaData.getOffsetAfter();
      currentRel = (*reader)[c0];
      functional = true;
    } else {
      if ((*reader)[c1] == lastLhs) {
        functional = false;
      }
    }
    buffer.emplace_back(array<Id, 2>{{(*reader)[c1], (*reader)[c2]}});
    lastLhs = (*reader)[c1];
  }
  if (from < vec.size()) {
    auto md = writeRel(out, lastOffset, currentRel, buffer, functional);
    metaData.add(md.first, md.second);
  }

  LOG(INFO) << "Done creating index permutation." << std::endl;
  LOG(INFO) << "Writing statistics for this permutation:\n"
            << metaData.statistics() << std::endl;

  LOG(INFO) << "Writing Meta data to index file...\n";
  out << metaData;
  off_t startOfMeta = metaData.getOffsetAfter();
  out.write(&startOfMeta, sizeof(startOfMeta));
  out.close();
  LOG(INFO) << "Permutation done.\n";
}

// _____________________________________________________________________________
void Index::createPatterns(const string& fileName, const ExtVec& vec,
                           CompactStringVector<Id, Id>& hasRelation,
                           std::vector<PatternID>& hasPattern,
                           CompactStringVector<size_t, Id>& patterns,
                           size_t maxNumPatterns) {
  if (vec.size() == 0) {
    LOG(WARN) << "Attempt to write an empty index!" << std::endl;
    return;
  }
  IndexMetaData meta;
  typedef std::unordered_map<Pattern, size_t> PatternsCountMap;

  LOG(INFO) << "Creating patterns file..." << std::endl;
  PatternsCountMap patternCounts;  //(4096 * 16, 4096 * 16);

  // determine the most common patterns
  Pattern pattern;

  size_t patternIndex = 0;
  Id currentRel;
  currentRel = vec[0][0];
  bool isValidPattern = true;
  size_t numInvalidPatterns = 0;
  size_t numValidPatterns = 0;
  // DEGBUG CODE
  std::map<size_t, size_t> predicateCounts;

  for (ExtVec::bufreader_type reader(vec); !reader.empty(); ++reader) {
    if ((*reader)[0] != currentRel) {
      currentRel = (*reader)[0];
      predicateCounts[patternIndex]++;
      if (isValidPattern) {
        numValidPatterns++;
        auto it = patternCounts.find(pattern);
        if (it == patternCounts.end()) {
          patternCounts.insert(std::pair<Pattern, size_t>(pattern, size_t(1)));
        } else {
          (*it).second++;
        }
      } else {
        numInvalidPatterns++;
      }
      isValidPattern = true;
      pattern.clear();
      patternIndex = 0;
    }
    // don't list predicates twice
    if (patternIndex == 0 || pattern[patternIndex - 1] != ((*reader)[1])) {
      pattern.push_back((*reader)[1]);
      patternIndex++;
    }
  }
  // process the last entry
  if (isValidPattern) {
    auto it = patternCounts.find(pattern);
    if (it == patternCounts.end()) {
      patternCounts.insert(std::pair<Pattern, size_t>(pattern, size_t(1)));
    } else {
      (*it).second++;
    }
  }
  LOG(INFO) << "Counted patterns and found " << patternCounts.size()
            << " distinct patterns." << std::endl;
  LOG(INFO) << "Patterns where found for " << numValidPatterns << " entities."
            << std::endl;
  LOG(INFO) << "Discarded the patterns of " << numInvalidPatterns
            << " entities"
               " because they were to large."
            << std::endl;

  // DEBUG CODE
  // write out the mapping from predicate counts to number of occurences
  ad_utility::File df((fileName + ".debug").c_str(), "w");
  for (auto it : predicateCounts) {
    string s =
        std::to_string(it.first) + "\t" + std::to_string(it.second) + "\n";
    df.write(s.c_str(), s.size());
  }
  df.close();

  // stores patterns sorted by their number of occurences
  size_t actualNumPatterns = patternCounts.size() < maxNumPatterns
                                 ? patternCounts.size()
                                 : maxNumPatterns;
  LOG(INFO) << "Using " << actualNumPatterns << " of the "
            << patternCounts.size() << " patterns that were found in the data."
            << std::endl;
  std::vector<std::pair<Pattern, size_t>> sortedPatterns;
  sortedPatterns.reserve(actualNumPatterns);
  for (auto& it : patternCounts) {
    if (sortedPatterns.size() < maxNumPatterns) {
      sortedPatterns.push_back(it);
      if (sortedPatterns.size() == maxNumPatterns) {
        LOG(DEBUG) << "Sorting patterns after initial insertions." << std::endl;
        // actuall sort the sorted patterns
        std::sort(sortedPatterns.begin(), sortedPatterns.end(),
                  [](const std::pair<Pattern, size_t>& first,
                     const std::pair<Pattern, size_t>& second) -> bool {
                    return first.second > second.second;
                  });
      }
    } else {
      if (it.second > sortedPatterns.back().second) {
        // The new element is larger than the smallest element in the vector.
        // Insert it into the correct position in the vector using binary
        // search.
        sortedPatterns.pop_back();
        auto sortedIt =
            std::lower_bound(sortedPatterns.begin(), sortedPatterns.end(), it,
                             [](const std::pair<Pattern, size_t>& p1,
                                const std::pair<Pattern, size_t>& p2) -> bool {
                               return p1.second > p2.second;
                             });
        sortedPatterns.insert(sortedIt, it);
      }
    }
  }
  if (sortedPatterns.size() < maxNumPatterns) {
    LOG(DEBUG) << "Sorting patterns after all insertions." << std::endl;
    // actuall sort the sorted patterns
    std::sort(sortedPatterns.begin(), sortedPatterns.end(),
              [](const std::pair<Pattern, size_t>& first,
                 const std::pair<Pattern, size_t>& second) -> bool {
                return first.second > second.second;
              });
  }

  LOG(DEBUG) << "Number of sorted patterns: " << sortedPatterns.size()
             << std::endl;

  // store the actual patterns
  std::vector<std::vector<Id>> buffer;
  buffer.reserve(sortedPatterns.size());
  for (const auto& p : sortedPatterns) {
    buffer.push_back(p.first._data);
  }
  patterns.build(buffer);

  std::unordered_map<Pattern, Id> patternSet;
  patternSet.reserve(sortedPatterns.size());
  for (size_t i = 0; i < sortedPatterns.size(); i++) {
    patternSet.insert(std::pair<Pattern, Id>(sortedPatterns[i].first, i));
  }

  LOG(DEBUG) << "Pattern set size: " << patternSet.size() << std::endl;

  // Associate entities with patterns if possible, store has-relation otherwise
  // stxxl::VECTOR_GENERATOR<std::pair<Id, Id>>::result entityPatterns;
  // stxxl::VECTOR_GENERATOR<std::pair<Id, Id>>::result entityHasRelation;
  std::vector<std::array<Id, 2>> entityHasPattern;
  std::vector<std::array<Id, 2>> entityHasRelation;

  size_t numEntitiesWithPatterns = 0;
  size_t numEntitiesWithoutPatterns = 0;
  size_t numInvalidEntities = 0;
  pattern.clear();
  currentRel = vec[0][0];
  patternIndex = 0;
  // Create the has-relation and has-pattern predicates
  for (ExtVec::bufreader_type reader2(vec); !reader2.empty(); ++reader2) {
    if ((*reader2)[0] != currentRel) {
      std::unordered_map<Pattern, Id>::iterator it;
      if (isValidPattern) {
        it = patternSet.find(pattern);
      } else {
        it = patternSet.end();
        numInvalidEntities++;
      }
      if (it == patternSet.end()) {
        numEntitiesWithoutPatterns++;
        // The pattern does not exist, use the has-relation predicate instead
        for (size_t i = 0; i < patternIndex; i++) {
          entityHasRelation.push_back(
              std::array<Id, 2>{currentRel, pattern[i]});
        }
      } else {
        numEntitiesWithPatterns++;
        // The pattern does exist, add an entry to the has-pattern predicate
        entityHasPattern.push_back(std::array<Id, 2>{currentRel, it->second});
      }
      pattern.clear();
      currentRel = (*reader2)[0];
      patternIndex = 0;
      isValidPattern = true;
    }
    // don't list predicates twice
    if (patternIndex == 0 || pattern[patternIndex - 1] != ((*reader2)[1])) {
      pattern.push_back((*reader2)[1]);
      patternIndex++;
    }
  }
  // process the last element
  std::unordered_map<Pattern, Id>::iterator it;
  if (isValidPattern) {
    it = patternSet.find(pattern);
  } else {
    it = patternSet.end();
  }
  if (it == patternSet.end()) {
    numEntitiesWithoutPatterns++;
    // The pattern does not exist, use the has-relation predicate instead
    for (size_t i = 0; i < patternIndex; i++) {
      entityHasRelation.push_back(std::array<Id, 2>{currentRel, pattern[i]});
    }
  } else {
    numEntitiesWithPatterns++;
    // The pattern does exist, add an entry to the has-pattern predicate
    entityHasPattern.push_back(std::array<Id, 2>{currentRel, it->second});
  }

  LOG(DEBUG) << "Number of entity-has-pattern entries: "
             << entityHasPattern.size() << std::endl;
  LOG(DEBUG) << "Number of entity-has-relation entries: "
             << entityHasRelation.size() << std::endl;

  LOG(INFO) << numEntitiesWithPatterns
            << " of the databases entities have been assigned a pattern."
            << std::endl;
  LOG(INFO) << numEntitiesWithoutPatterns
            << " of the databases entities have not been assigned a pattern."
            << std::endl;
  LOG(INFO) << "Of these " << numInvalidEntities
            << " would have to large a pattern." << std::endl;
  LOG(DEBUG) << "Total number of entities: "
             << (numEntitiesWithoutPatterns + numEntitiesWithPatterns)
             << std::endl;

  // ensure neither of the relations is empty
  if (entityHasPattern.size() == 0) {
    entityHasPattern.push_back(std::array<Id, 2>{ID_NO_VALUE, ID_NO_VALUE});
  }
  if (entityHasRelation.size() == 0) {
    entityHasRelation.push_back(std::array<Id, 2>{ID_NO_VALUE, ID_NO_VALUE});
  }

  // Store all data in the file
  ad_utility::File file(fileName.c_str(), "w");
  off_t afterRelations = 0;
  off_t afterPatterns = 0;
  // write the has-pattern and has-relation relations into the file
  auto md1 = writeRel(file, afterRelations, Id(0), entityHasPattern, true);
  meta.add(md1.first, md1.second);
  afterRelations = meta.getOffsetAfter();
  auto md2 = writeRel(file, afterRelations, Id(1), entityHasRelation, false);
  meta.add(md2.first, md2.second);
  afterRelations = meta.getOffsetAfter();

  size_t patternsPartSize = patterns.write(file);
  afterPatterns = afterRelations + patternsPartSize;

  // write the meta data into the file
  file << meta;

  // the last part of the file contains offset information for the varius parts
  file.write(&afterRelations, sizeof(off_t));
  file.write(&afterPatterns, sizeof(off_t));
  file.close();

  hasPattern.resize(entityHasPattern.back()[0] + 1);
  // hasRelation.resize(entityHasRelation.back()[0] + 1);

  // create the has-relation and has-pattern lookup vectors
  size_t pos = 0;
  for (size_t i = 0; i < entityHasPattern.size(); i++) {
    while (entityHasPattern[i][0] > pos) {
      hasPattern[pos] = NO_PATTERN;
      pos++;
    }
    hasPattern[pos] = entityHasPattern[i][1];
    pos++;
  }

  pos = 0;
  vector<vector<Id>> hasRelationTmp;
  if (!(entityHasRelation.size() == 1 &&
        entityHasRelation[0][0] == ID_NO_VALUE)) {
    for (size_t i = 0; i < entityHasRelation.size(); i++) {
      Id current = entityHasRelation[i][0];
      while (current > pos) {
        hasRelationTmp.emplace_back();
        pos++;
      }
      hasRelationTmp.emplace_back();
      while (i < entityHasRelation.size() &&
             entityHasRelation[i][0] == current) {
        hasRelationTmp.back().push_back(entityHasRelation[i][1]);
        i++;
      }
      pos++;
    }
  }
  hasRelation.build(hasRelationTmp);

  LOG(INFO) << "Done creating patterns file." << std::endl;
}

// _____________________________________________________________________________
pair<FullRelationMetaData, BlockBasedRelationMetaData> Index::writeRel(
    ad_utility::File& out, off_t currentOffset, Id relId,
    const vector<array<Id, 2>>& data, bool functional) {
  LOG(TRACE) << "Writing a relation ...\n";
  AD_CHECK_GT(data.size(), 0);
  LOG(TRACE) << "Calculating multiplicities ...\n";
  ad_utility::HashSet<Id> distinctC1;
  ad_utility::HashSet<Id> distinctC2;
  for (size_t i = 0; i < data.size(); ++i) {
    if (!functional) {
      distinctC1.insert(data[i][0]);
    }
    distinctC2.insert(data[i][1]);
  }
  double multC1 = functional ? 1.0 : data.size() / double(distinctC1.size());
  double multC2 = data.size() / double(distinctC2.size());
  LOG(TRACE) << "Done calculating multiplicities.\n";
  FullRelationMetaData rmd(
      relId, currentOffset, data.size(), multC1, multC2, functional,
      !functional && data.size() > USE_BLOCKS_INDEX_SIZE_TRESHOLD);

  // Write the full pair index.
  out.write(data.data(), data.size() * 2 * sizeof(Id));
  pair<FullRelationMetaData, BlockBasedRelationMetaData> ret;
  ret.first = rmd;

  if (functional) {
    writeFunctionalRelation(data, ret);
  } else {
    writeNonFunctionalRelation(out, data, ret);
  };
  LOG(TRACE) << "Done writing relation.\n";
  return ret;
}

// _____________________________________________________________________________
void Index::writeFunctionalRelation(
    const vector<array<Id, 2>>& data,
    pair<FullRelationMetaData, BlockBasedRelationMetaData>& rmd) {
  // Only has to do something if there are blocks.
  if (rmd.first.hasBlocks()) {
    LOG(TRACE) << "Writing part for functional relation ...\n";
    // Do not write extra LHS and RHS lists.
    rmd.second._startRhs =
        rmd.first._startFullIndex + rmd.first.getNofBytesForFulltextIndex();
    // Since the relation is functional, there are no lhs lists and thus this
    // is trivial.
    rmd.second._offsetAfter = rmd.second._startRhs;
    // Create the block data for the meta data.
    // Blocks are offsets into the full pair index for functional relations.
    size_t nofDistinctLhs = 0;
    Id lastLhs = std::numeric_limits<Id>::max();
    for (size_t i = 0; i < data.size(); ++i) {
      if (data[i][0] != lastLhs) {
        if (nofDistinctLhs % DISTINCT_LHS_PER_BLOCK == 0) {
          rmd.second._blocks.emplace_back(BlockMetaData(
              data[i][0], rmd.first._startFullIndex + i * 2 * sizeof(Id)));
        }
        ++nofDistinctLhs;
      }
    }
  }
}

// _____________________________________________________________________________
void Index::writeNonFunctionalRelation(
    ad_utility::File& out, const vector<array<Id, 2>>& data,
    pair<FullRelationMetaData, BlockBasedRelationMetaData>& rmd) {
  // Only has to do something if there are blocks.
  if (rmd.first.hasBlocks()) {
    LOG(TRACE) << "Writing part for non-functional relation ...\n";
    // Make a pass over the data and extract a RHS list for each LHS.
    // Prepare both in buffers.
    // TODO: add compression - at least to RHS.
    pair<Id, off_t>* bufLhs = new pair<Id, off_t>[data.size()];
    Id* bufRhs = new Id[data.size()];
    size_t nofDistinctLhs = 0;
    Id lastLhs = std::numeric_limits<Id>::max();
    size_t nofRhsDone = 0;
    for (; nofRhsDone < data.size(); ++nofRhsDone) {
      if (data[nofRhsDone][0] != lastLhs) {
        bufLhs[nofDistinctLhs++] =
            pair<Id, off_t>(data[nofRhsDone][0], nofRhsDone * sizeof(Id));
        lastLhs = data[nofRhsDone][0];
      }
      bufRhs[nofRhsDone] = data[nofRhsDone][1];
    }

    // Go over the Lhs data once more and adjust the offsets.
    off_t startRhs = rmd.first.getStartOfLhs() +
                     nofDistinctLhs * (sizeof(Id) + sizeof(off_t));

    for (size_t i = 0; i < nofDistinctLhs; ++i) {
      bufLhs[i].second += startRhs;
    }

    // Write to file.
    out.write(bufLhs, nofDistinctLhs * (sizeof(Id) + sizeof(off_t)));
    out.write(bufRhs, data.size() * sizeof(Id));

    // Update meta data.
    rmd.second._startRhs = startRhs;
    rmd.second._offsetAfter =
        startRhs + rmd.first.getNofElements() * sizeof(Id);

    // Create the block data for the FullRelationMetaData.
    // Block are offsets into the LHS list for non-functional relations.
    for (size_t i = 0; i < nofDistinctLhs; ++i) {
      if (i % DISTINCT_LHS_PER_BLOCK == 0) {
        rmd.second._blocks.emplace_back(BlockMetaData(
            bufLhs[i].first,
            rmd.first.getStartOfLhs() + i * (sizeof(Id) + sizeof(off_t))));
      }
    }
    delete[] bufLhs;
    delete[] bufRhs;
  }
}

// _____________________________________________________________________________
void Index::createFromOnDiskIndex(const string& onDiskBase,
                                  bool allPermutations, bool onDiskLiterals) {
  _onDiskBase = onDiskBase;
  _vocab.readFromFile(onDiskBase + ".vocabulary",
                      onDiskLiterals ? onDiskBase + ".literals-index" : "");
  _psoFile.open(string(_onDiskBase + ".index.pso").c_str(), "r");
  _posFile.open(string(_onDiskBase + ".index.pos").c_str(), "r");
  AD_CHECK(_psoFile.isOpen() && _posFile.isOpen());
  // PSO
  off_t metaFrom;
  off_t metaTo = _psoFile.getLastOffset(&metaFrom);
  unsigned char* buf = new unsigned char[metaTo - metaFrom];
  _psoFile.read(buf, static_cast<size_t>(metaTo - metaFrom), metaFrom);
  _psoMeta.createFromByteBuffer(buf);
  delete[] buf;
  LOG(INFO) << "Registered PSO permutation: " << _psoMeta.statistics()
            << std::endl;
  // POS
  metaTo = _posFile.getLastOffset(&metaFrom);
  buf = new unsigned char[metaTo - metaFrom];
  _posFile.read(buf, static_cast<size_t>(metaTo - metaFrom), metaFrom);
  _posMeta.createFromByteBuffer(buf);
  delete[] buf;
  LOG(INFO) << "Registered POS permutation: " << _posMeta.statistics()
            << std::endl;
  if (allPermutations) {
    _spoFile.open(string(_onDiskBase + ".index.spo").c_str(), "r");
    _sopFile.open(string(_onDiskBase + ".index.sop").c_str(), "r");
    _ospFile.open(string(_onDiskBase + ".index.osp").c_str(), "r");
    _opsFile.open(string(_onDiskBase + ".index.ops").c_str(), "r");
    AD_CHECK(_spoFile.isOpen() && _sopFile.isOpen() && _ospFile.isOpen() &&
             _opsFile.isOpen());
    // SPO
    metaTo = _spoFile.getLastOffset(&metaFrom);
    buf = new unsigned char[metaTo - metaFrom];
    _spoFile.read(buf, static_cast<size_t>(metaTo - metaFrom), metaFrom);
    _spoMeta.createFromByteBuffer(buf);
    delete[] buf;
    LOG(INFO) << "Registered SPO permutation: " << _spoMeta.statistics()
              << std::endl;
    // SOP
    metaTo = _sopFile.getLastOffset(&metaFrom);
    buf = new unsigned char[metaTo - metaFrom];
    _sopFile.read(buf, static_cast<size_t>(metaTo - metaFrom), metaFrom);
    _sopMeta.createFromByteBuffer(buf);
    delete[] buf;
    LOG(INFO) << "Registered SOP permutation: " << _sopMeta.statistics()
              << std::endl;
    // OSP
    metaTo = _ospFile.getLastOffset(&metaFrom);
    buf = new unsigned char[metaTo - metaFrom];
    _ospFile.read(buf, static_cast<size_t>(metaTo - metaFrom), metaFrom);
    _ospMeta.createFromByteBuffer(buf);
    delete[] buf;
    LOG(INFO) << "Registered OSP permutation: " << _ospMeta.statistics()
              << std::endl;
    // OPS
    metaTo = _opsFile.getLastOffset(&metaFrom);
    buf = new unsigned char[metaTo - metaFrom];
    _opsFile.read(buf, static_cast<size_t>(metaTo - metaFrom), metaFrom);
    _opsMeta.createFromByteBuffer(buf);
    delete[] buf;
    LOG(INFO) << "Registered OPS permutation: " << _opsMeta.statistics()
              << std::endl;
  }
  if (_usePatterns) {
    IndexMetaData _patternsMeta;
    ad_utility::File _patternsFile;

    // patterns
    _patternsFile.open(string(_onDiskBase + ".index.patterns").c_str(), "r");
    AD_CHECK(_patternsFile.isOpen());
    metaTo = _patternsFile.getLastOffset(&metaFrom);
    // A second offset is stored at the end of the file
    metaTo -= sizeof(off_t);
    buf = new unsigned char[metaTo - metaFrom];
    _patternsFile.read(buf, static_cast<size_t>(metaTo - metaFrom), metaFrom);
    _patternsMeta.createFromByteBuffer(buf);
    delete[] buf;
    // read the offset of the patterns beginnig
    off_t patternsFrom;
    _patternsFile.read(&patternsFrom, sizeof(off_t), metaTo);
    _patterns.load(_patternsFile, patternsFrom);
    // load the has pattern and has relation relations
    const FullRelationMetaData& rmdP = _patternsMeta.getRmd(0)._rmdPairs;
    const FullRelationMetaData& rmdR = _patternsMeta.getRmd(1)._rmdPairs;
    WidthTwoList buffer;

    size_t numElems = std::max(rmdR.getNofElements(), rmdP.getNofElements());
    buffer.resize(numElems);
    // determine the highest entity id
    size_t maxIdRel = 0, maxIdPat = 0;
    if (rmdR.getNofElements() > 0) {
      _patternsFile.read(&maxIdRel, sizeof(Id),
                         rmdR._startFullIndex +
                             ((rmdR.getNofElements() - 1) * 2 * sizeof(Id)));
    }
    if (rmdP.getNofElements() > 0) {
      _patternsFile.read(&maxIdPat, sizeof(Id),
                         rmdP._startFullIndex +
                             ((rmdP.getNofElements() - 1) * 2 * sizeof(Id)));
    }

    _patternsFile.read(buffer.data(), rmdP.getNofElements() * 2 * sizeof(Id),
                       rmdP._startFullIndex);
    _hasPattern.resize(maxIdPat + 1);
    size_t pos = 0;
    for (size_t i = 0; i < rmdP.getNofElements(); i++) {
      while (buffer[i][0] > pos) {
        _hasPattern[pos] = NO_PATTERN;
        pos++;
      }
      _hasPattern[pos] = buffer[i][1];
      pos++;
    }

    _patternsFile.read(buffer.data(), rmdR.getNofElements() * 2 * sizeof(Id),
                       rmdR._startFullIndex);

    pos = 0;
    vector<vector<Id>> hasRelationTmp;

    if (!(rmdR.getNofElements() == 1 && buffer[0][0] == ID_NO_VALUE)) {
      for (size_t i = 0; i < rmdR.getNofElements(); i++) {
        Id current = buffer[i][0];
        while (current > pos) {
          hasRelationTmp.emplace_back();
          pos++;
        }
        hasRelationTmp.emplace_back();
        while (i < rmdR.getNofElements() && buffer[i][0] == current) {
          hasRelationTmp.back().push_back(buffer[i][1]);
          i++;
        }
        pos++;
      }
    }
    _hasRelation.build(hasRelationTmp);
  }
}

// _____________________________________________________________________________
bool Index::ready() const { return _psoFile.isOpen() && _posFile.isOpen(); }

// _____________________________________________________________________________
void Index::openFileHandles() {
  AD_CHECK(_onDiskBase.size() > 0);
  _psoFile.open((_onDiskBase + ".index.pso").c_str(), "r");
  _posFile.open((_onDiskBase + ".index.pos").c_str(), "r");
  if (ad_utility::File::exists(_onDiskBase + ".index.spo")) {
    _spoFile.open((_onDiskBase + ".index.spo").c_str(), "r");
  }
  if (ad_utility::File::exists(_onDiskBase + ".index.sop")) {
    _sopFile.open((_onDiskBase + ".index.sop").c_str(), "r");
  }
  if (ad_utility::File::exists(_onDiskBase + ".index.osp")) {
    _ospFile.open((_onDiskBase + ".index.osp").c_str(), "r");
  }
  if (ad_utility::File::exists(_onDiskBase + ".index.ops")) {
    _opsFile.open((_onDiskBase + ".index.ops").c_str(), "r");
  }
  AD_CHECK(_psoFile.isOpen());
  AD_CHECK(_posFile.isOpen());
}

// _____________________________________________________________________________
void Index::scanPSO(const string& predicate, WidthTwoList* result) const {
  LOG(DEBUG) << "Performing PSO scan for full relation: " << predicate << "\n";
  Id relId;
  if (_vocab.getId(predicate, &relId)) {
    LOG(TRACE) << "Successfully got key ID.\n";
    scanPSO(relId, result);
  }
  LOG(DEBUG) << "Scan done, got " << result->size() << " elements.\n";
}

// _____________________________________________________________________________
void Index::scanPSO(const string& predicate, const string& subject,
                    WidthOneList* result) const {
  LOG(DEBUG) << "Performing PSO scan of relation " << predicate
             << " with fixed subject: " << subject << "...\n";
  Id relId;
  Id subjId;
  if (_vocab.getId(predicate, &relId) && _vocab.getId(subject, &subjId)) {
    if (_psoMeta.relationExists(relId)) {
      auto rmd = _psoMeta.getRmd(relId);
      if (rmd.hasBlocks()) {
        pair<off_t, size_t> blockOff =
            rmd._rmdBlocks->getBlockStartAndNofBytesForLhs(subjId);
        // Functional relations have blocks point into the pair index,
        // non-functional relations have them point into lhs lists
        if (rmd.isFunctional()) {
          scanFunctionalRelation(blockOff, subjId, _psoFile, result);
        } else {
          pair<off_t, size_t> block2 =
              rmd._rmdBlocks->getFollowBlockForLhs(subjId);
          scanNonFunctionalRelation(blockOff, block2, subjId, _psoFile,
                                    rmd._rmdBlocks->_offsetAfter, result);
        }
      } else {
        // If we don't have blocks, scan the whole relation and filter /
        // restrict.
        WidthTwoList fullRelation;
        fullRelation.resize(rmd.getNofElements());
        _psoFile.read(fullRelation.data(),
                      rmd.getNofElements() * 2 * sizeof(Id),
                      rmd._rmdPairs._startFullIndex);
        getRhsForSingleLhs(fullRelation, subjId, result);
      }
    } else {
      LOG(DEBUG) << "No such relation.\n";
    }
  } else {
    LOG(DEBUG) << "No such subject.\n";
  }
  LOG(DEBUG) << "Scan done, got " << result->size() << " elements.\n";
}

// _____________________________________________________________________________
void Index::scanPOS(const string& predicate, WidthTwoList* result) const {
  LOG(DEBUG) << "Performing POS scan for full relation: " << predicate << "\n";
  Id relId;
  if (_vocab.getId(predicate, &relId)) {
    LOG(TRACE) << "Successfully got key ID.\n";
    scanPOS(relId, result);
  }
  LOG(DEBUG) << "Scan done, got " << result->size() << " elements.\n";
}

// _____________________________________________________________________________
void Index::scanPOS(const string& predicate, const string& object,
                    WidthOneList* result) const {
  LOG(DEBUG) << "Performing POS scan of relation " << predicate
             << " with fixed object: " << object << "...\n";
  Id relId;
  Id objId;
  if (_vocab.getId(predicate, &relId) && _vocab.getId(object, &objId)) {
    if (_posMeta.relationExists(relId)) {
      auto rmd = _posMeta.getRmd(relId);
      if (rmd.hasBlocks()) {
        pair<off_t, size_t> blockOff =
            rmd._rmdBlocks->getBlockStartAndNofBytesForLhs(objId);
        // Functional relations have blocks point into the pair index,
        // non-functional relations have them point into lhs lists
        if (rmd.isFunctional()) {
          scanFunctionalRelation(blockOff, objId, _posFile, result);
        } else {
          pair<off_t, size_t> block2 =
              rmd._rmdBlocks->getFollowBlockForLhs(objId);
          scanNonFunctionalRelation(blockOff, block2, objId, _posFile,
                                    rmd._rmdBlocks->_offsetAfter, result);
        }
      } else {
        // If we don't have blocks, scan the whole relation and filter /
        // restrict.
        WidthTwoList fullRelation;
        fullRelation.resize(rmd.getNofElements());
        _posFile.read(fullRelation.data(),
                      rmd.getNofElements() * 2 * sizeof(Id),
                      rmd._rmdPairs._startFullIndex);
        getRhsForSingleLhs(fullRelation, objId, result);
      }
    } else {
      LOG(DEBUG) << "No such relation.\n";
    }
  } else {
    LOG(DEBUG) << "No such object.\n";
  }
  LOG(DEBUG) << "Scan done, got " << result->size() << " elements.\n";
}

// _____________________________________________________________________________
void Index::scanSOP(const string& subject, const string& object,
                    WidthOneList* result) const {
  if (!_sopFile.isOpen()) {
    AD_THROW(ad_semsearch::Exception::BAD_INPUT,
             "Cannot use predicate variables without the required "
             "index permutations. Build an index with option -a "
             "to use this feature.");
  }
  LOG(DEBUG) << "Performing SOP scan of list for " << subject
             << " with fixed object: " << object << "...\n";
  Id relId;
  Id objId;
  if (_vocab.getId(subject, &relId) && _vocab.getId(object, &objId)) {
    if (_sopMeta.relationExists(relId)) {
      auto rmd = _sopMeta.getRmd(relId);
      if (rmd.hasBlocks()) {
        pair<off_t, size_t> blockOff =
            rmd._rmdBlocks->getBlockStartAndNofBytesForLhs(objId);
        // Functional relations have blocks point into the pair index,
        // non-functional relations have them point into lhs lists
        if (rmd.isFunctional()) {
          scanFunctionalRelation(blockOff, objId, _sopFile, result);
        } else {
          pair<off_t, size_t> block2 =
              rmd._rmdBlocks->getFollowBlockForLhs(objId);
          scanNonFunctionalRelation(blockOff, block2, objId, _sopFile,
                                    rmd._rmdBlocks->_offsetAfter, result);
        }
      } else {
        // If we don't have blocks, scan the whole relation and filter /
        // restrict.
        WidthTwoList fullRelation;
        fullRelation.resize(rmd.getNofElements());
        _sopFile.read(fullRelation.data(),
                      rmd.getNofElements() * 2 * sizeof(Id),
                      rmd._rmdPairs._startFullIndex);
        getRhsForSingleLhs(fullRelation, objId, result);
      }
    } else {
      LOG(DEBUG) << "No such relation.\n";
    }
  } else {
    LOG(DEBUG) << "No such object.\n";
  }
  LOG(DEBUG) << "Scan done, got " << result->size() << " elements.\n";
}

// _____________________________________________________________________________
void Index::scanSPO(const string& subject, WidthTwoList* result) const {
  if (!_spoFile.isOpen()) {
    AD_THROW(ad_semsearch::Exception::BAD_INPUT,
             "Cannot use predicate variables without the required "
             "index permutations. Build an index with option -a "
             "to use this feature.");
  }
  LOG(DEBUG) << "Performing SPO scan for full list for: " << subject << "\n";
  Id relId;
  if (_vocab.getId(subject, &relId)) {
    LOG(TRACE) << "Successfully got key ID.\n";
    scanSPO(relId, result);
  }
  LOG(DEBUG) << "Scan done, got " << result->size() << " elements.\n";
}

// _____________________________________________________________________________
void Index::scanSOP(const string& subject, WidthTwoList* result) const {
  if (!_sopFile.isOpen()) {
    AD_THROW(ad_semsearch::Exception::BAD_INPUT,
             "Cannot use predicate variables without the required "
             "index permutations. Build an index with option -a "
             "to use this feature.");
  }
  LOG(DEBUG) << "Performing SOP scan for full list for: " << subject << "\n";
  Id relId;
  if (_vocab.getId(subject, &relId)) {
    LOG(TRACE) << "Successfully got key ID.\n";
    scanSOP(relId, result);
  }
  LOG(DEBUG) << "Scan done, got " << result->size() << " elements.\n";
}

// _____________________________________________________________________________
void Index::scanOPS(const string& object, WidthTwoList* result) const {
  if (!_opsFile.isOpen()) {
    AD_THROW(ad_semsearch::Exception::BAD_INPUT,
             "Cannot use predicate variables without the required "
             "index permutations. Build an index with option -a "
             "to use this feature.");
  }
  LOG(DEBUG) << "Performing OPS scan for full list for: " << object << "\n";
  Id relId;
  if (_vocab.getId(object, &relId)) {
    LOG(TRACE) << "Successfully got key ID.\n";
    scanOPS(relId, result);
  }
  LOG(DEBUG) << "Scan done, got " << result->size() << " elements.\n";
}

// _____________________________________________________________________________
void Index::scanOSP(const string& object, WidthTwoList* result) const {
  if (!_ospFile.isOpen()) {
    AD_THROW(ad_semsearch::Exception::BAD_INPUT,
             "Cannot use predicate variables without the required "
             "index permutations. Build an index with option -a "
             "to use this feature.");
  }
  LOG(DEBUG) << "Performing OSP scan for full list for: " << object << "\n";
  Id relId;
  if (_vocab.getId(object, &relId)) {
    LOG(TRACE) << "Successfully got key ID.\n";
    scanOSP(relId, result);
  }
  LOG(DEBUG) << "Scan done, got " << result->size() << " elements.\n";
}

// _____________________________________________________________________________
void Index::scanPSO(Id predicate, Index::WidthTwoList* result) const {
  if (_psoMeta.relationExists(predicate)) {
    const FullRelationMetaData& rmd = _psoMeta.getRmd(predicate)._rmdPairs;
    result->reserve(rmd.getNofElements() + 2);
    result->resize(rmd.getNofElements());
    _psoFile.read(result->data(), rmd.getNofElements() * 2 * sizeof(Id),
                  rmd._startFullIndex);
  }
}

// _____________________________________________________________________________
void Index::scanPOS(Id predicate, Index::WidthTwoList* result) const {
  if (_posMeta.relationExists(predicate)) {
    const FullRelationMetaData& rmd = _posMeta.getRmd(predicate)._rmdPairs;
    result->reserve(rmd.getNofElements() + 2);
    result->resize(rmd.getNofElements());
    _posFile.read(result->data(), rmd.getNofElements() * 2 * sizeof(Id),
                  rmd._startFullIndex);
  }
}

// _____________________________________________________________________________
void Index::scanSPO(Id subject, Index::WidthTwoList* result) const {
  if (_spoMeta.relationExists(subject)) {
    const FullRelationMetaData& rmd = _spoMeta.getRmd(subject)._rmdPairs;
    result->reserve(rmd.getNofElements() + 2);
    result->resize(rmd.getNofElements());
    _spoFile.read(result->data(), rmd.getNofElements() * 2 * sizeof(Id),
                  rmd._startFullIndex);
  }
}

// _____________________________________________________________________________
void Index::scanSOP(Id subject, Index::WidthTwoList* result) const {
  if (_sopMeta.relationExists(subject)) {
    const FullRelationMetaData& rmd = _sopMeta.getRmd(subject)._rmdPairs;
    result->reserve(rmd.getNofElements() + 2);
    result->resize(rmd.getNofElements());
    _sopFile.read(result->data(), rmd.getNofElements() * 2 * sizeof(Id),
                  rmd._startFullIndex);
  }
}

// _____________________________________________________________________________
void Index::scanOSP(Id object, Index::WidthTwoList* result) const {
  if (_ospMeta.relationExists(object)) {
    const FullRelationMetaData& rmd = _ospMeta.getRmd(object)._rmdPairs;
    result->reserve(rmd.getNofElements() + 2);
    result->resize(rmd.getNofElements());
    _ospFile.read(result->data(), rmd.getNofElements() * 2 * sizeof(Id),
                  rmd._startFullIndex);
  }
}

// _____________________________________________________________________________
void Index::scanOPS(Id object, Index::WidthTwoList* result) const {
  if (_opsMeta.relationExists(object)) {
    const FullRelationMetaData& rmd = _opsMeta.getRmd(object)._rmdPairs;
    result->reserve(rmd.getNofElements() + 2);
    result->resize(rmd.getNofElements());
    _opsFile.read(result->data(), rmd.getNofElements() * 2 * sizeof(Id),
                  rmd._startFullIndex);
  }
}

// _____________________________________________________________________________
const vector<PatternID>& Index::getHasPattern() const { return _hasPattern; }

// _____________________________________________________________________________
const CompactStringVector<Id, Id>& Index::getHasRelation() const {
  return _hasRelation;
}

// _____________________________________________________________________________
const CompactStringVector<size_t, Id>& Index::getPatterns() const {
  return _patterns;
}

// _____________________________________________________________________________
string Index::idToString(Id id) const {
  if (id < _vocab.size()) {
    return _vocab[id];
  } else if (id == ID_NO_VALUE) {
    return "";
  } else {
    id -= _vocab.size();
    AD_CHECK(id < _vocab.getExternalVocab().size()) {
      return _vocab.getExternalVocab()[id];
    }
  }
}

// _____________________________________________________________________________
void Index::scanFunctionalRelation(const pair<off_t, size_t>& blockOff,
                                   Id lhsId, ad_utility::File& indexFile,
                                   WidthOneList* result) const {
  LOG(TRACE) << "Scanning functional relation ...\n";
  WidthTwoList block;
  block.resize(blockOff.second / (2 * sizeof(Id)));
  indexFile.read(block.data(), blockOff.second, blockOff.first);
  auto it = std::lower_bound(
      block.begin(), block.end(), lhsId,
      [](const array<Id, 2>& elem, Id key) { return elem[0] < key; });
  if ((*it)[0] == lhsId) {
    result->push_back(array<Id, 1>{(*it)[1]});
  }
  LOG(TRACE) << "Read " << result->size() << " RHS.\n";
}

// _____________________________________________________________________________
void Index::scanNonFunctionalRelation(const pair<off_t, size_t>& blockOff,
                                      const pair<off_t, size_t>& followBlock,
                                      Id lhsId, ad_utility::File& indexFile,
                                      off_t upperBound,
                                      Index::WidthOneList* result) const {
  LOG(TRACE) << "Scanning non-functional relation ...\n";
  vector<pair<Id, off_t>> block;
  block.resize(blockOff.second / (sizeof(Id) + sizeof(off_t)));
  indexFile.read(block.data(), blockOff.second, blockOff.first);
  auto it = std::lower_bound(
      block.begin(), block.end(), lhsId,
      [](const pair<Id, off_t>& elem, Id key) { return elem.first < key; });
  if (it->first == lhsId) {
    size_t nofBytes = 0;
    if ((it + 1) != block.end()) {
      LOG(TRACE) << "Obtained upper bound from same block!\n";
      nofBytes = static_cast<size_t>((it + 1)->second - it->second);
    } else {
      // Look at the follow block to determine the upper bound / nofBytes.
      if (followBlock.first == blockOff.first) {
        LOG(TRACE) << "Last block of relation, using rel upper bound!\n";
        nofBytes = static_cast<size_t>(upperBound - it->second);
      } else {
        LOG(TRACE) << "Special case: extra scan of follow block!\n";
        pair<Id, off_t> follower;
        indexFile.read(&follower, sizeof(follower), followBlock.first);
        nofBytes = static_cast<size_t>(follower.second - it->second);
      }
    }
    result->reserve((nofBytes / sizeof(Id)) + 2);
    result->resize(nofBytes / sizeof(Id));
    indexFile.read(result->data(), nofBytes, it->second);
  } else {
    LOG(TRACE) << "Could not find LHS in block. Result will be empty.\n";
  }
}

// _____________________________________________________________________________
size_t Index::relationCardinality(const string& relationName) const {
  if (relationName == INTERNAL_TEXT_MATCH_PREDICATE) {
    return TEXT_PREDICATE_CARDINALITY_ESTIMATE;
  }
  Id relId;
  if (_vocab.getId(relationName, &relId)) {
    if (this->_psoMeta.relationExists(relId)) {
      return this->_psoMeta.getRmd(relId).getNofElements();
    }
  }
  return 0;
}

// _____________________________________________________________________________
size_t Index::subjectCardinality(const string& sub) const {
  Id relId;
  if (_vocab.getId(sub, &relId)) {
    if (this->_spoMeta.relationExists(relId)) {
      return this->_spoMeta.getRmd(relId).getNofElements();
    }
  }
  return 0;
}

// _____________________________________________________________________________
size_t Index::objectCardinality(const string& obj) const {
  Id relId;
  if (_vocab.getId(obj, &relId)) {
    if (this->_ospMeta.relationExists(relId)) {
      return this->_ospMeta.getRmd(relId).getNofElements();
    }
  }
  return 0;
}

// _____________________________________________________________________________
size_t Index::sizeEstimate(const string& sub, const string& pred,
                           const string& obj) const {
  // One or two of the parameters have to be empty strings.
  // This determines the permutations to use.

  // With only one nonempty string, we can get the exact count.
  // With two, we can check if the relation is functional (return 1) or not
  // where we approximate the result size by the block size.
  if (sub.size() > 0 && pred.size() == 0 && obj.size() == 0) {
    return subjectCardinality(sub);
  }
  if (sub.size() == 0 && pred.size() > 0 && obj.size() == 0) {
    return relationCardinality(pred);
  }
  if (sub.size() == 0 && pred.size() == 0 && obj.size() > 0) {
    return objectCardinality(obj);
  }
  if (sub.size() == 0 && pred.size() == 0 && obj.size() == 0) {
    return getNofTriples();
  }
  AD_THROW(ad_semsearch::Exception::CHECK_FAILED,
           "Index::sizeEsimate called with more then one of S/P/O given. "
           "This should never be the case anymore, "
           " since for such SCANs we compute the result "
           "directly and don't need an estimate anymore!");
}

// _____________________________________________________________________________
template <class T>
void Index::writeAsciiListFile(const string& filename, const T& ids) const {
  std::ofstream f(filename.c_str());
  for (size_t i = 0; i < ids.size(); ++i) {
    f << ids[i] << ' ';
  }
  f.close();
}

template void Index::writeAsciiListFile<vector<Id>>(
    const string& filename, const vector<Id>& ids) const;

template void Index::writeAsciiListFile<vector<Score>>(
    const string& filename, const vector<Score>& ids) const;

// _____________________________________________________________________________
bool Index::isLiteral(const string& object) {
  return object.size() > 0 && object[0] == '\"';
}

// _____________________________________________________________________________
bool Index::shouldBeExternalized(const string& object) {
  return Vocabulary::shouldBeExternalized(object);
}

// _____________________________________________________________________________
vector<float> Index::getPSOMultiplicities(const string& key) const {
  Id keyId;
  vector<float> res;
  if (_vocab.getId(key, &keyId) && _psoMeta.relationExists(keyId)) {
    auto rmd = _psoMeta.getRmd(keyId);
    auto logM1 = rmd.getCol1LogMultiplicity();
    res.push_back(static_cast<float>(pow(2, logM1)));
    auto logM2 = rmd.getCol2LogMultiplicity();
    res.push_back(static_cast<float>(pow(2, logM2)));
  } else {
    res.push_back(1);
    res.push_back(1);
  }
  return res;
}

// _____________________________________________________________________________
vector<float> Index::getPOSMultiplicities(const string& key) const {
  Id keyId;
  vector<float> res;
  if (_vocab.getId(key, &keyId) && _posMeta.relationExists(keyId)) {
    auto rmd = _posMeta.getRmd(keyId);
    auto logM1 = rmd.getCol1LogMultiplicity();
    res.push_back(static_cast<float>(pow(2, logM1)));
    auto logM2 = rmd.getCol2LogMultiplicity();
    res.push_back(static_cast<float>(pow(2, logM2)));
  } else {
    res.push_back(1);
    res.push_back(1);
  }
  return res;
}

// _____________________________________________________________________________
vector<float> Index::getSPOMultiplicities(const string& key) const {
  Id keyId;
  vector<float> res;
  if (_vocab.getId(key, &keyId) && _spoMeta.relationExists(keyId)) {
    auto rmd = _spoMeta.getRmd(keyId);
    auto logM1 = rmd.getCol1LogMultiplicity();
    res.push_back(static_cast<float>(pow(2, logM1)));
    auto logM2 = rmd.getCol2LogMultiplicity();
    res.push_back(static_cast<float>(pow(2, logM2)));
  } else {
    res.push_back(1);
    res.push_back(1);
  }
  return res;
}

// _____________________________________________________________________________
vector<float> Index::getSOPMultiplicities(const string& key) const {
  Id keyId;
  vector<float> res;
  if (_vocab.getId(key, &keyId) && _sopMeta.relationExists(keyId)) {
    auto rmd = _sopMeta.getRmd(keyId);
    auto logM1 = rmd.getCol1LogMultiplicity();
    res.push_back(static_cast<float>(pow(2, logM1)));
    auto logM2 = rmd.getCol2LogMultiplicity();
    res.push_back(static_cast<float>(pow(2, logM2)));
  } else {
    res.push_back(1);
    res.push_back(1);
  }
  return res;
}

// _____________________________________________________________________________
vector<float> Index::getOSPMultiplicities(const string& key) const {
  Id keyId;
  vector<float> res;
  if (_vocab.getId(key, &keyId) && _ospMeta.relationExists(keyId)) {
    auto rmd = _ospMeta.getRmd(keyId);
    auto logM1 = rmd.getCol1LogMultiplicity();
    res.push_back(static_cast<float>(pow(2, logM1)));
    auto logM2 = rmd.getCol2LogMultiplicity();
    res.push_back(static_cast<float>(pow(2, logM2)));
  } else {
    res.push_back(1);
    res.push_back(1);
  }
  return res;
}

// _____________________________________________________________________________
vector<float> Index::getOPSMultiplicities(const string& key) const {
  Id keyId;
  vector<float> res;
  if (_vocab.getId(key, &keyId) && _opsMeta.relationExists(keyId)) {
    auto rmd = _opsMeta.getRmd(keyId);
    auto logM1 = rmd.getCol1LogMultiplicity();
    res.push_back(static_cast<float>(pow(2, logM1)));
    auto logM2 = rmd.getCol2LogMultiplicity();
    res.push_back(static_cast<float>(pow(2, logM2)));
  } else {
    res.push_back(1);
    res.push_back(1);
  }
  return res;
}

// _____________________________________________________________________________
vector<float> Index::getSPOMultiplicities() const {
  return vector<float>{
      {static_cast<float>(getNofTriples() / getNofSubjects()),
       static_cast<float>(getNofTriples() / getNofPredicates()),
       static_cast<float>(getNofTriples() / getNofObjects())}};
}

// _____________________________________________________________________________
vector<float> Index::getSOPMultiplicities() const {
  return vector<float>{
      {static_cast<float>(getNofTriples() / getNofSubjects()),
       static_cast<float>(getNofTriples() / getNofObjects()),
       static_cast<float>(getNofTriples() / getNofPredicates())}};
}

// _____________________________________________________________________________
vector<float> Index::getPSOMultiplicities() const {
  return vector<float>{
      {static_cast<float>(getNofTriples() / getNofPredicates()),
       static_cast<float>(getNofTriples() / getNofSubjects()),
       static_cast<float>(getNofTriples() / getNofObjects())}};
}

// _____________________________________________________________________________
vector<float> Index::getPOSMultiplicities() const {
  return vector<float>{
      {static_cast<float>(getNofTriples() / getNofPredicates()),
       static_cast<float>(getNofTriples() / getNofObjects()),
       static_cast<float>(getNofTriples() / getNofSubjects())}};
}

// _____________________________________________________________________________
vector<float> Index::getOSPMultiplicities() const {
  return vector<float>{
      {static_cast<float>(getNofTriples() / getNofObjects()),
       static_cast<float>(getNofTriples() / getNofSubjects()),
       static_cast<float>(getNofTriples() / getNofPredicates())}};
}

// _____________________________________________________________________________
vector<float> Index::getOPSMultiplicities() const {
  return vector<float>{
      {static_cast<float>(getNofTriples() / getNofObjects()),
       static_cast<float>(getNofTriples() / getNofPredicates()),
       static_cast<float>(getNofTriples() / getNofSubjects())}};
}

// _____________________________________________________________________________
void Index::setKbName(const string& name) {
  _psoMeta.setName(name);
  _posMeta.setName(name);
  _spoMeta.setName(name);
  _sopMeta.setName(name);
  _ospMeta.setName(name);
  _opsMeta.setName(name);
}

// _____________________________________________________________________________
void Index::setUsePatterns(bool usePatterns) { _usePatterns = usePatterns; }
