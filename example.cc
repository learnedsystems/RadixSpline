#include <iostream>

#include "include/rs/multi_map.h"

using namespace std;

void RadixSplineExample() {
  // Create random keys.
  vector<uint64_t> keys(1e6);
  generate(keys.begin(), keys.end(), rand);
  keys.push_back(8128);
  sort(keys.begin(), keys.end());

  // Build RadixSpline.
  uint64_t min = keys.front();
  uint64_t max = keys.back();
  rs::Builder<uint64_t> rsb(min, max);
  for (const auto& key : keys) rsb.AddKey(key);
  rs::RadixSpline<uint64_t> rs = rsb.Finalize();

  // Search using RadixSpline.
  rs::SearchBound bound = rs.GetSearchBound(8128);
  cout << "The search key is in the range: ["
       << bound.begin << ", " << bound.end << ")" << endl;
  auto start = begin(keys) + bound.begin, last = begin(keys) + bound.end;
  cout << "The key is at position: " << std::lower_bound(start, last, 8128) - begin(keys) << endl;
}

void MultiMapExample() {
  vector<pair<uint64_t, char>> data = {{1ull, 'a'},
                                       {12ull, 'b'},
                                       {7ull, 'c'}, // Unsorted.
                                       {42ull, 'd'}};
  rs::MultiMap<uint64_t, char> map(begin(data), end(data));

  cout << "find(7): '" << map.find(7)->second << "'" << endl;
  cout << "lower_bound(3): '" << map.lower_bound(3)->second << "'" << endl;
}

int main(int argc, char** argv) {
  RadixSplineExample();
  MultiMapExample();

  return 0;
}
