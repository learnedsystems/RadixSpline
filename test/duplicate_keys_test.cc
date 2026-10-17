#include "include/rs/multi_map.h"

#include <algorithm>
#include <limits>
#include <random>
#include <utility>
#include <vector>

#include "gtest/gtest.h"
#include "include/rs/builder.h"

namespace {

const size_t kNumRadixBits = 18;

// *** Helper methods ***

// 0, 10, ..., 990, then 1000 repeated 1000 times, then 1010, ..., 1990.
template <class KeyType>
std::vector<KeyType> CreateRunFollowedByGaps() {
  std::vector<KeyType> keys;
  for (KeyType key = 0; key < 1000; key += 10) keys.push_back(key);
  for (size_t i = 0; i < 1000; ++i) keys.push_back(1000);
  for (KeyType key = 1010; key < 2000; key += 10) keys.push_back(key);
  return keys;
}

// 0, 2, ..., 1998, then the maximum key 2000 repeated 50 times.
template <class KeyType>
std::vector<KeyType> CreateDuplicatedMaxKey() {
  std::vector<KeyType> keys;
  for (KeyType key = 0; key < 2000; key += 2) keys.push_back(key);
  for (size_t i = 0; i < 50; ++i) keys.push_back(2000);
  return keys;
}

// 2000 random keys, each repeated 1 to 100 times.
template <class KeyType>
std::vector<KeyType> CreateRandomRuns() {
  std::mt19937_64 g(42);
  std::vector<KeyType> keys;
  for (size_t i = 0; i < 2000; ++i) {
    const KeyType key = static_cast<KeyType>(g());
    const size_t num_copies = g() % 100 + 1;
    for (size_t j = 0; j < num_copies; ++j) keys.push_back(key);
  }
  std::sort(keys.begin(), keys.end());
  return keys;
}

// Every key, key - 1, key + 1, and the smallest and largest possible key.
template <class KeyType>
std::vector<KeyType> CreateLookupKeysNextTo(const std::vector<KeyType>& keys) {
  std::vector<KeyType> lookup_keys = {std::numeric_limits<KeyType>::min(),
                                      std::numeric_limits<KeyType>::max()};
  for (const auto& key : keys) {
    lookup_keys.push_back(key);
    if (key > std::numeric_limits<KeyType>::min())
      lookup_keys.push_back(key - 1);
    if (key < std::numeric_limits<KeyType>::max())
      lookup_keys.push_back(key + 1);
  }
  return lookup_keys;
}

// Checks that the search bound of every lookup key next to `keys` contains the
// lookup key's lower bound, and that MultiMap::lower_bound returns it.
template <class KeyType>
void CheckLowerBounds(const std::vector<KeyType>& keys, size_t max_error) {
  rs::Builder<KeyType> rsb(keys.front(), keys.back(), kNumRadixBits,
                           max_error);
  for (const auto& key : keys) rsb.AddKey(key);
  const auto rs = rsb.Finalize();

  std::vector<std::pair<KeyType, size_t>> data;
  for (size_t i = 0; i < keys.size(); ++i) data.push_back({keys[i], i});
  const rs::MultiMap<KeyType, size_t> map(data.begin(), data.end(),
                                          kNumRadixBits, max_error);

  for (const auto& key : CreateLookupKeysNextTo(keys)) {
    const size_t lower_bound =
        std::lower_bound(keys.begin(), keys.end(), key) - keys.begin();
    const rs::SearchBound bound = rs.GetSearchBound(key);
    ASSERT_LE(bound.begin, lower_bound)
        << "key: " << key << ", max error: " << max_error;
    ASSERT_GE(bound.end, lower_bound)
        << "key: " << key << ", max error: " << max_error;
    ASSERT_EQ(static_cast<size_t>(map.lower_bound(key) - map.begin()),
              lower_bound)
        << "key: " << key << ", max error: " << max_error;
  }
}

// *** Tests ***

template <class T>
struct DuplicateKeysTest : public testing::Test {
  using KeyType = T;
};

using AllKeyTypes = testing::Types<uint32_t, uint64_t>;
TYPED_TEST_SUITE(DuplicateKeysTest, AllKeyTypes);

TYPED_TEST(DuplicateKeysTest, RunFollowedByGaps) {
  using KeyType = typename TestFixture::KeyType;
  for (const size_t max_error : {2, 8, 32})
    CheckLowerBounds(CreateRunFollowedByGaps<KeyType>(), max_error);
}

TYPED_TEST(DuplicateKeysTest, DuplicatedMaxKey) {
  using KeyType = typename TestFixture::KeyType;
  for (const size_t max_error : {2, 8, 32})
    CheckLowerBounds(CreateDuplicatedMaxKey<KeyType>(), max_error);
}

TYPED_TEST(DuplicateKeysTest, RandomRuns) {
  using KeyType = typename TestFixture::KeyType;
  for (const size_t max_error : {2, 8, 32})
    CheckLowerBounds(CreateRandomRuns<KeyType>(), max_error);
}

}  // namespace
