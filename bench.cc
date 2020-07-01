#include "include/rs/multi_map.h"

#include <iostream>
#include <fstream>
#include <map>
#include <chrono>

using namespace std;

namespace rs_manual_tuning {

// Returns <max_error, #radix_bits>
pair<uint64_t, uint64_t> GetTuning(const string& data_filename, uint32_t size_scale) {
  string cut = data_filename;

  // Cut the prefix of the filename
  size_t pos = cut.find_last_of('/');
  if (pos != string::npos) {
    cut.erase(cut.begin(), cut.begin() + pos + 1);
  }

  // Normal
  if (cut == "normal_200M_uint64"
      || cut == "normal_400M_uint64"
      || cut == "normal_600M_uint64"
      || cut == "normal_800M_uint64"
      || cut == "normal_200M_uint32") {
    if (size_scale == 1) return make_pair(32, 18);
    if (size_scale == 2) return make_pair(16, 18);
    if (size_scale == 3) return make_pair(8, 18);
    if (size_scale == 4) return make_pair(4, 18);
    if (size_scale == 5) return make_pair(1, 18);
    if (size_scale == 6) return make_pair(1, 17);
    if (size_scale == 7) return make_pair(1, 16);
    if (size_scale == 8) return make_pair(1, 15);
    if (size_scale == 9) return make_pair(1, 14);
    if (size_scale == 10) return make_pair(1, 13);
  }

  // Lognormal
  if (cut == "lognormal_200M_uint32") return make_pair(1, 20);
  if (cut == "lognormal_200M_uint64") return make_pair(1, 25);

  // Uniform dense
  if (cut == "uniform_dense_200M_uint32") return make_pair(0, 15);
  if (cut == "uniform_dense_200M_uint64") return make_pair(0, 15);

  // Uniform sparse
  if (cut == "uniform_sparse_200M_uint32") return make_pair(6, 24);
  if (cut == "uniform_sparse_200M_uint64") return make_pair(5, 25);

  // Osm
  if (cut == "osm_cellids_200M_uint64"
      || cut == "osm_cellids_400M_uint64"
      || cut == "osm_cellids_600M_uint64"
      || cut == "osm_cellids_800M_uint64") {
    if (size_scale == 1) return make_pair(13, 25);
    if (size_scale == 2) return make_pair(26, 23);
    if (size_scale == 3) return make_pair(32, 19);
    if (size_scale == 4) return make_pair(64, 18);
    if (size_scale == 5) return make_pair(128, 18);
    if (size_scale == 6) return make_pair(256, 16);
    if (size_scale == 7) return make_pair(512, 15);
    if (size_scale == 8) return make_pair(2 * 1024, 14);
    if (size_scale == 9) return make_pair(2 * 2048, 3);
    if (size_scale == 10) return make_pair(2 * 4096, 3);
  }

  // Wiki
  if (cut == "wiki_ts_200M_uint64") {
    if (size_scale == 1) return make_pair(9, 21);
    if (size_scale == 2) return make_pair(10, 18);
    if (size_scale == 3) return make_pair(32, 18);
    if (size_scale == 4) return make_pair(48, 18);
    if (size_scale == 5) return make_pair(84, 18);
    if (size_scale == 6) return make_pair(256, 16);
    if (size_scale == 7) return make_pair(512, 15);
    if (size_scale == 8) return make_pair(2 * 1024, 14);
    if (size_scale == 9) return make_pair(2 * 2048, 3);
    if (size_scale == 10) return make_pair(2 * 4096, 3);
  }

  // Books (or amazon in the paper)
  if (cut == "books_200M_uint32") return make_pair(14, 20);
  if (cut == "books_200M_uint64"
      || cut == "books_400M_uint64"
      || cut == "books_600M_uint64"
      || cut == "books_800M_uint64"
      || cut == "books_200M_uint32") {

    // TODO this is the original optimal config, but it gives wrong results.
    // if (size_scale == 1) return make_pair(11, 22);
    if (size_scale == 1) return make_pair(64, 18);
    if (size_scale == 2) return make_pair(82, 18);
    if (size_scale == 3) return make_pair(98, 18);
    if (size_scale == 4) return make_pair(256, 18);
    if (size_scale == 5) return make_pair(512, 16);
    if (size_scale == 6) return make_pair(1024, 14);
    if (size_scale == 7) return make_pair(1024, 12);
    if (size_scale == 8) return make_pair(2 * 1024, 10);
    if (size_scale == 9) return make_pair(2 * 2048, 3);
    if (size_scale == 10) return make_pair(2 * 4096, 3);

  }

  // Fb
  if (cut == "fb_200M_uint64" || cut == "fb_200M_uint32") {
    if (size_scale == 1) return make_pair(2, 25);
    if (size_scale == 2) return make_pair(4, 22);
    if (size_scale == 3) return make_pair(10, 20);
    if (size_scale == 4) return make_pair(32, 18);
    if (size_scale == 5) return make_pair(128, 18);
    if (size_scale == 6) return make_pair(512, 15);
    if (size_scale == 7) return make_pair(1024, 14);
    if (size_scale == 8) return make_pair(2 * 1024, 12);
    if (size_scale == 9) return make_pair(2 * 2048, 3);
    if (size_scale == 10) return make_pair(2 * 4096, 3);

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

// A drop-in replacement for multimap. Internally creates a sorted copy of the data.
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
      iter++;
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
  for (uint32_t size_config = 1; size_config <= 10; size_config++) {
    // Build RS
    auto build_begin = chrono::high_resolution_clock::now();
    auto tuning = rs_manual_tuning::GetTuning(data_file, size_config);
    NonOwningMultiMap<uint64_t, uint64_t> map(elements, tuning.second, tuning.first);
    auto build_end = chrono::high_resolution_clock::now();

    // Run queries
    auto lookup_begin = chrono::high_resolution_clock::now();
    uint64_t sum = 0;
    for (const Lookup<uint64_t>& lookup_iter : lookups) {
      uint64_t sum = map.sum_up(lookup_iter.key);
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

