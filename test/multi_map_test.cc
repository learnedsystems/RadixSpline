#include <unordered_set>
#include <random>

#include "gtest/gtest.h"
#include "include/rs/multi_map.h"

namespace {

TEST(MultiMapTest, SimpleFind) {
  std::vector<std::pair<uint64_t, char>> data = {{1ull, 'a'},
                                                 {12ull, 'c'},
                                                 {7ull, 'b'}, // Unsorted.
                                                 {42ull, 'd'}};
  rs::MultiMap<uint64_t, char> rs_multi_map(data.begin(), data.end());

  // Positive lookups (keys).
  ASSERT_EQ(1u, rs_multi_map.find(1)->first);
  ASSERT_EQ(7u, rs_multi_map.find(7)->first);
  ASSERT_EQ(12u, rs_multi_map.find(12)->first);
  ASSERT_EQ(42u, rs_multi_map.find(42)->first);

  // Positive lookups (values).
  ASSERT_EQ('a', rs_multi_map.find(1)->second);
  ASSERT_EQ('b', rs_multi_map.find(7)->second);
  ASSERT_EQ('c', rs_multi_map.find(12)->second);
  ASSERT_EQ('d', rs_multi_map.find(42)->second);

  // Negative lookups.
  ASSERT_EQ(rs_multi_map.end(), rs_multi_map.find(0));
  ASSERT_EQ(rs_multi_map.end(), rs_multi_map.find(2));
  ASSERT_EQ(rs_multi_map.end(), rs_multi_map.find(6));
  ASSERT_EQ(rs_multi_map.end(), rs_multi_map.find(8));
  ASSERT_EQ(rs_multi_map.end(), rs_multi_map.find(11));
  ASSERT_EQ(rs_multi_map.end(), rs_multi_map.find(13));
  ASSERT_EQ(rs_multi_map.end(), rs_multi_map.find(41));
  ASSERT_EQ(rs_multi_map.end(), rs_multi_map.find(43));
}

TEST(MultiMapTest, LowerBoundFind) {
  std::vector<std::pair<uint64_t, char>> data = {{1ull, 'a'},
                                                 {12ull, 'c'},
                                                 {7ull, 'b'}, // Unsorted.
                                                 {42ull, 'd'}};
  rs::MultiMap<uint64_t, char> rs_multi_map(data.begin(), data.end());

  // Direct-hit lookups (keys).
  ASSERT_EQ(1u, rs_multi_map.lower_bound(1)->first);
  ASSERT_EQ(7u, rs_multi_map.lower_bound(7)->first);
  ASSERT_EQ(12u, rs_multi_map.lower_bound(12)->first);
  ASSERT_EQ(42u, rs_multi_map.lower_bound(42)->first);

  // Direct-hit lookups (values).
  ASSERT_EQ('a', rs_multi_map.lower_bound(1)->second);
  ASSERT_EQ('b', rs_multi_map.lower_bound(7)->second);
  ASSERT_EQ('c', rs_multi_map.lower_bound(12)->second);
  ASSERT_EQ('d', rs_multi_map.lower_bound(42)->second);

  // Negative lookups (keys).
  ASSERT_EQ(1u, rs_multi_map.lower_bound(0)->first);
  ASSERT_EQ(7u, rs_multi_map.lower_bound(2)->first);
  ASSERT_EQ(7u, rs_multi_map.lower_bound(6)->first);
  ASSERT_EQ(12u, rs_multi_map.lower_bound(8)->first);
  ASSERT_EQ(12u, rs_multi_map.lower_bound(11)->first);
  ASSERT_EQ(42u, rs_multi_map.lower_bound(13)->first);
  ASSERT_EQ(42u, rs_multi_map.lower_bound(41)->first);
  ASSERT_EQ(rs_multi_map.end(), rs_multi_map.lower_bound(43));
}

const size_t kNumKeys = 500;
const size_t kNumLookups = 500;

TEST(MultiMapTest, Random) {
  // Create random rs::MultiMap and std::multimap.
  std::vector<std::pair<uint64_t, uint64_t>> entries;
  entries.reserve(kNumKeys);
  std::mt19937 randomness_generator(8128);
  std::uniform_int_distribution<uint64_t> distribution(0, kNumKeys * 10);
  while (entries.size() < kNumKeys) {
    entries.emplace_back(distribution(randomness_generator), entries.size());
  }
  rs::MultiMap<uint64_t, uint64_t> map(entries.begin(), entries.end());
  std::multimap<uint64_t, uint64_t> ref(entries.begin(), entries.end());

  // Look up every key in the generated range
  for (size_t lookup_key = 0; lookup_key < kNumKeys * 10 + 10; ++lookup_key) {
    // Check lower bound
    auto map_iter = map.lower_bound(lookup_key);
    auto ref_iter = ref.lower_bound(lookup_key);

    // Found something -> iterate until the end and remember all values found.
    std::set<uint64_t> found_values_in_map;
    std::set<uint64_t> found_values_in_ref;
    while (map_iter != map.end() && ref_iter != ref.end()) {
      ASSERT_EQ(ref_iter->first, map_iter->first);
      found_values_in_map.insert(map_iter->second);
      found_values_in_ref.insert(ref_iter->second);
      ++ref_iter;
      ++map_iter;
    }

    // Both should be at the end now.
    ASSERT_EQ(map.end(), map_iter);
    ASSERT_EQ(ref.end(), ref_iter);

    // Check that all encountered values are the same.
    ASSERT_EQ(found_values_in_ref.size(), found_values_in_map.size());
    auto val_iter_map = found_values_in_map.begin();
    auto val_iter_ref = found_values_in_ref.begin();
    for (size_t i = 0; i < found_values_in_ref.size(); ++i) {
      ASSERT_EQ(*val_iter_ref, *val_iter_map);
      ++val_iter_map;
      ++val_iter_ref;
    }
  }
}

}  // namespace