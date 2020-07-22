#include "include/rs/multi_map.h"

#include <iostream>
#include <fstream>
#include <map>
#include <chrono>

using namespace std;

namespace rs_manual_tuning {

// Returns <num_radix_bits, max_error>
pair<uint64_t, uint64_t> GetTuning(const string& data_filename, uint32_t size_scale) {
  string cut = data_filename;

  // Cut the prefix of the filename
  size_t pos = cut.find_last_of('/');
  if (pos != string::npos) {
    cut.erase(cut.begin(), cut.begin() + pos + 1);
  }

  // TODO books32

  using Configs = const vector<pair<size_t, size_t>>;

  // Books (or amazon in the paper)
  if (cut == "books_200M_uint64") {
    Configs configs = {{25, 2},
                       {22, 4},
                       {23, 8},
                       {24, 20},
                       {22, 20},
                       {22, 45},
                       {15, 40},
                       {20, 95},
                       {16, 95},
                       {12, 135}};
    return configs[size_scale - 1];
  }

  if (cut == "books_400M_uint64") {
    Configs configs = {{19, 4},
                       {24, 10},
                       {25, 25},
                       {24, 35},
                       {22, 40},
                       {22, 85},
                       {18, 85},
                       {20, 190},
                       {13, 185},
                       {4, 270}};
    return configs[size_scale - 1];
  }

  if (cut == "books_600M_uint64") {
    Configs configs = {{19, 6},
                       {24, 15},
                       {22, 25},
                       {24, 55},
                       {22, 60},
                       {22, 125},
                       {21, 190},
                       {14, 185},
                       {17, 300},
                       {17, 300}};
    return configs[size_scale - 1];
  }

  if (cut == "books_800M_uint64") {
    Configs configs = {{21, 8},
                       {24, 20},
                       {25, 50},
                       {24, 70},
                       {22, 80},
                       {21, 125},
                       {21, 255},
                       {20, 380},
                       {15, 375},
                       {15, 375}};
    return configs[size_scale - 1];
  }

  // Facebook
  if (cut == "fb_200M_uint64") {
    Configs configs = {{20, 2},
                       {25, 9},
                       {22, 10},
                       {23, 35},
                       {21, 45},
                       {18, 70},
                       {20, 265},
                       {15, 260},
                       {15, 260},
                       {15, 260}};
    return configs[size_scale - 1];
  }

  // OSM
  if (cut == "osm_cellids_200M_uint64") {
    Configs configs = {{27, 7},
                       {24, 4},
                       {25, 25},
                       {24, 50},
                       {23, 95},
                       {22, 185},
                       {21, 365},
                       {15, 165},
                       {13, 325},
                       {13, 325}};
    return configs[size_scale - 1];
  }

  // Wiki
  if (cut == "wiki_ts_200M_uint64") {
    Configs configs = {{27, 8},
                       {26, 15},
                       {25, 20},
                       {24, 25},
                       {23, 40},
                       {22, 70},
                       {21, 125},
                       {20, 250},
                       {11, 45},
                       {17, 135}};
    return configs[size_scale - 1];
  }

  cerr << "No tuning config for this file and size_config" << endl;
  throw;
}
}

namespace util {

// Loads values from binary file into vector.
template<typename T>
static vector<T> load_data(const string& filename,
                           bool print = true) {
  vector<T> data;
  ifstream in(filename, ios::binary);
  if (!in.is_open()) {
    cerr << "unable to open " << filename << endl;
    exit(EXIT_FAILURE);
  }
  // Read size.
  uint64_t size;
  in.read(reinterpret_cast<char*>(&size), sizeof(uint64_t));
  data.resize(size);
  // Read values.
  in.read(reinterpret_cast<char*>(data.data()), size * sizeof(T));

  return data;
}

// Generates deterministic values for keys.
template<class KeyType>
static vector<pair<uint64_t, uint64_t>> add_values(const vector<KeyType>& keys) {
  vector<pair<uint64_t, uint64_t>> result;
  result.reserve(keys.size());

  for (uint64_t i = 0; i < keys.size(); ++i) {
    pair<uint64_t, uint64_t> row;
    row.first = keys[i];
    row.second = i;

    result.push_back(row);
  }
  return result;
}

}

namespace {

template<class KeyType, class ValueType>
class NonOwningMultiMap {
 public:
  using element_type = pair<KeyType, ValueType>;

  NonOwningMultiMap(const vector<element_type>& elements, size_t num_radix_bits = 18, size_t max_error = 32)
      : data_(elements) {
    assert(elements.size() > 0);

    // Create spline builder.
    const auto min_key = data_.front().first;
    const auto max_key = data_.back().first;
    rs::Builder<KeyType> rsb(min_key, max_key, num_radix_bits, max_error);

    // Build the radix spline.
    for (const auto& iter : data_) {
      rsb.AddKey(iter.first);
    }
    rs_ = rsb.Finalize();
  }

  typename vector<element_type>::const_iterator lower_bound(KeyType key) const {
    rs::SearchBound bound = rs_.GetSearchBound(key);
    return ::lower_bound(data_.begin() + bound.begin,
                         data_.begin() + bound.end,
                         key,
                         [](const element_type& lhs, const KeyType& rhs) { return lhs.first < rhs; });
  }

  uint64_t sum_up(KeyType key) const {
    uint64_t result = 0;
    auto iter = lower_bound(key);
    while (iter != data_.end() && iter->first == key) {
      result += iter->second;
      ++iter;
    }
    return result;
  }

  size_t GetSizeInByte() const {
    return rs_.GetSize();
  }

 private:
  const vector<element_type>& data_;
  rs::RadixSpline<KeyType> rs_;
};

template<class KeyType>
struct Lookup {
  KeyType key;
  uint64_t value;
};

}

int main(int argc, char** argv) {
  if (argc != 3) {
    cerr << "usage: " << argv[0] << " <data_file> <lookup_file>" << endl;
    throw;
  }
  const string data_file = argv[1];
  const string lookup_file = argv[2];

  // Load data
  vector<uint64_t> keys = util::load_data<uint64_t>(data_file);
  vector<pair<uint64_t, uint64_t>> elements = util::add_values(keys);
  vector<Lookup<uint64_t>> lookups = util::load_data<Lookup<uint64_t>>(lookup_file);

  // Run benchmark
  for (uint32_t size_config = 1; size_config <= 10; ++size_config) {
    // Build RS
    auto build_begin = chrono::high_resolution_clock::now();
    auto tuning = rs_manual_tuning::GetTuning(data_file, size_config);
    NonOwningMultiMap<uint64_t, uint64_t> map(elements, tuning.first, tuning.second);
    auto build_end = chrono::high_resolution_clock::now();

    // Run queries
    auto lookup_begin = chrono::high_resolution_clock::now();
    for (const Lookup<uint64_t>& lookup_iter : lookups) {
      const uint64_t sum = map.sum_up(lookup_iter.key);
      if (sum != lookup_iter.value) {
        cerr << "wrong result!" << endl;
        throw "error";
      }
    }
    auto lookup_end = chrono::high_resolution_clock::now();

    uint64_t build_ns = chrono::duration_cast<chrono::nanoseconds>(build_end - build_begin).count();
    uint64_t lookup_ns = chrono::duration_cast<chrono::nanoseconds>(lookup_end - lookup_begin).count();
    cout << "RESULT:"
         << " data_file: " << data_file
         << " lookup_file: " << lookup_file
         << " size_config: " << size_config
         << " used_memory[MB]: " << (map.GetSizeInByte() / 1000) / 1000.0
         << " build_time[s]: " << (build_ns / 1000 / 1000) / 1000.0
         << " ns/lookup: " << lookup_ns / lookups.size() << endl;
  }

  return 0;
}
