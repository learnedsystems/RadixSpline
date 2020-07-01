#include <unordered_set>
#include <random>

#include "gtest/gtest.h"
#include "include/rs/builder.h"
#include "include/rs/radix_spline.h"

const size_t kNumKeys = 1000;
// Number of iterations (seeds) of random positive and negative test cases.
const size_t kNumIterations = 10;
const size_t kNumRadixBits = 18;
const size_t kMaxError = 32;

namespace {

// *** Helper methods ***

template<class KeyType>
std::vector<KeyType> CreateDenseKeys() {
  std::vector<KeyType> keys;
  keys.reserve(kNumKeys);
  for (size_t i = 0; i < kNumKeys; ++i) keys.push_back(i);
  return keys;
}

template<class KeyType>
std::vector<KeyType> CreateUniqueRandomKeys(size_t seed) {
  std::unordered_set<KeyType> keys;
  keys.reserve(kNumKeys);
  std::mt19937 g(seed);
  std::uniform_int_distribution<KeyType> d(std::numeric_limits<KeyType>::min(), std::numeric_limits<KeyType>::max());
  while (keys.size() < kNumKeys) keys.insert(d(g));
  std::vector<KeyType> sorted_keys(keys.begin(), keys.end());
  std::sort(sorted_keys.begin(), sorted_keys.end());
  return sorted_keys;
}

// Creates lognormal distributed keys, possibly with duplicates.
template<class KeyType>
std::vector<KeyType> CreateSkewedKeys(size_t seed) {
  std::vector<KeyType> keys;
  keys.reserve(kNumKeys);

  // Generate lognormal values.
  std::mt19937 g(seed);
  std::lognormal_distribution<double> d(/*mean*/0, /*stddev=*/2);
  std::vector<double> lognormal_values;
  lognormal_values.reserve(kNumKeys);
  for (size_t i = 0; i < kNumKeys; ++i) lognormal_values.push_back(d(g));
  const auto min_max = std::minmax_element(lognormal_values.begin(), lognormal_values.end());
  const double min = *min_max.first;
  const double max = *min_max.second;
  const double diff = max - min;

  // Scale values to the entire `KeyType` domain.
  const auto domain = std::numeric_limits<KeyType>::max() - std::numeric_limits<KeyType>::min();
  for (size_t i = 0; i < kNumKeys; ++i) {
    const double ratio = (lognormal_values[i] - min) / diff;
    keys.push_back(ratio * domain);
  }

  std::sort(keys.begin(), keys.end());
  return keys;
}

template<class KeyType>
rs::RadixSpline<KeyType> CreateRadixSpline(const std::vector<KeyType>& keys) {
  auto min = std::numeric_limits<KeyType>::min();
  auto max = std::numeric_limits<KeyType>::max();
  if (keys.size() > 0) {
    min = keys.front();
    max = keys.back();
  }
  rs::Builder<KeyType> rsb(min, max, kNumRadixBits, kMaxError);
  for (const auto& key : keys) rsb.AddKey(key);
  return rsb.Finalize();
}

template<class KeyType>
bool BoundContains(const std::vector<KeyType>& keys, rs::SearchBound bound, KeyType key) {
  const auto it = std::lower_bound(keys.begin() + bound.begin, keys.begin() + bound.end, key);
  if (it == keys.end()) return false;
  return *it == key;
}

// *** Tests ***

template<class T>
struct RadixSplineTest : public testing::Test {
  using KeyType = T;
};

using AllKeyTypes = testing::Types<uint32_t, uint64_t>;
TYPED_TEST_SUITE(RadixSplineTest, AllKeyTypes);

TYPED_TEST(RadixSplineTest, AddAndLookupDenseKeys) {
  using KeyType = typename TestFixture::KeyType;
  const auto keys = CreateDenseKeys<KeyType>();
  const auto rs = CreateRadixSpline(keys);
  for (const auto& key : keys)
    EXPECT_TRUE(BoundContains(keys, rs.GetSearchBound(key), key)) << "key: " << key;
}

TYPED_TEST(RadixSplineTest, AddAndLookupRandomKeysPositiveLookups) {
  using KeyType = typename TestFixture::KeyType;
  for (size_t i = 0; i < kNumIterations; ++i) {
    const auto keys = CreateUniqueRandomKeys<KeyType>(/*seed=*/i);
    const auto rs = CreateRadixSpline(keys);
    for (const auto& key : keys)
      EXPECT_TRUE(BoundContains(keys, rs.GetSearchBound(key), key)) << "key: " << key;
  }
}

TYPED_TEST(RadixSplineTest, AddAndLookupRandomIntegersNegativeLookups) {
  using KeyType = typename TestFixture::KeyType;
  for (size_t i = 0; i < kNumIterations; ++i) {
    const auto keys = CreateUniqueRandomKeys<KeyType>(/*seed=*/42 + i);
    const auto lookup_keys = CreateUniqueRandomKeys<KeyType>(/*seed=*/815 + i);
    const auto rs = CreateRadixSpline(keys);
    for (const auto& key : lookup_keys) {
      if (!BoundContains(keys, rs::SearchBound{0, keys.size()}, key))
        EXPECT_FALSE(BoundContains(keys, rs.GetSearchBound(key), key)) << "key: " << key;
    }
  }
}

TYPED_TEST(RadixSplineTest, AddAndLookupRandomIntegersWithDuplicatesPositiveLookups) {
  using KeyType = typename TestFixture::KeyType;

  // Duplicate every key once.
  auto duplicated_keys = CreateUniqueRandomKeys<KeyType>(/*seed=*/42);
  const size_t size = duplicated_keys.size();
  for (size_t i = 0; i < size; ++i) duplicated_keys.push_back(duplicated_keys[i]);
  std::sort(duplicated_keys.begin(), duplicated_keys.end());

  const auto rs = CreateRadixSpline(duplicated_keys);
  for (const auto& key : duplicated_keys)
    EXPECT_TRUE(BoundContains(duplicated_keys, rs.GetSearchBound(key), key)) << "key: " << key;
}

TYPED_TEST(RadixSplineTest, AddAndLookupSkewedKeysPositiveLookups) {
  using KeyType = typename TestFixture::KeyType;
  for (size_t i = 0; i < kNumIterations; ++i) {
    const auto keys = CreateSkewedKeys<KeyType>(/*seed=*/i);
    const auto rs = CreateRadixSpline(keys);
    for (const auto& key : keys)
      EXPECT_TRUE(BoundContains(keys, rs.GetSearchBound(key), key)) << "key: " << key;
  }
}

TYPED_TEST(RadixSplineTest, AddAndLookupSkewedKeysNegativeLookups) {
  using KeyType = typename TestFixture::KeyType;
  for (size_t i = 0; i < kNumIterations; ++i) {
    const auto keys = CreateSkewedKeys<KeyType>(/*seed=*/42 + i);
    const auto lookup_keys = CreateSkewedKeys<KeyType>(/*seed=*/815 + i);
    const auto rs = CreateRadixSpline(keys);
    for (const auto& key : lookup_keys) {
      if (!BoundContains(keys, rs::SearchBound{0, keys.size()}, key))
        EXPECT_FALSE(BoundContains(keys, rs.GetSearchBound(key), key)) << "key: " << key;
    }
  }
}

TYPED_TEST(RadixSplineTest, GetEstimatedPosKeyOutOfRange) {
  using KeyType = typename TestFixture::KeyType;
  const std::vector<KeyType> keys = {1, 2, 3};
  const auto rs = CreateRadixSpline(keys);
  EXPECT_EQ(rs.GetEstimatedPosition(0), 0u);
  EXPECT_EQ(rs.GetEstimatedPosition(4), keys.size() - 1);
}

TYPED_TEST(RadixSplineTest, NoKey) {
  using KeyType = typename TestFixture::KeyType;
  const std::vector<KeyType> keys;
  const auto rs = CreateRadixSpline(keys);
  // We expect the size to be at most the size of rs::RadixSpline and the size of the pre-allocated radix table.
  EXPECT_TRUE(rs.GetSize() <= sizeof(rs::RadixSpline<KeyType>) + ((1ull << kNumRadixBits) + 1) * sizeof(uint32_t));
}

TYPED_TEST(RadixSplineTest, SingleKey) {
  using KeyType = typename TestFixture::KeyType;
  const auto key = std::numeric_limits<KeyType>::min();
  const std::vector<KeyType> keys = {key};
  const auto rs = CreateRadixSpline(keys);
  EXPECT_EQ(rs.GetEstimatedPosition(key), 0u);
  EXPECT_TRUE(BoundContains(keys, rs.GetSearchBound(key), key)) << "key: " << key;
}

TYPED_TEST(RadixSplineTest, TwoKeys) {
  using KeyType = typename TestFixture::KeyType;
  const auto key1 = std::numeric_limits<KeyType>::min();
  const auto key2 = std::numeric_limits<KeyType>::max();
  const std::vector<KeyType> keys = {key1, key2};
  const auto rs = CreateRadixSpline(keys);
  for (const auto& key : keys)
    EXPECT_TRUE(BoundContains(keys, rs.GetSearchBound(key), key)) << "key: " << key;
}

TYPED_TEST(RadixSplineTest, AllMinKeys) {
  using KeyType = typename TestFixture::KeyType;
  const auto key = std::numeric_limits<KeyType>::min();
  const std::vector<KeyType> keys(kNumKeys, key);
  const auto rs = CreateRadixSpline(keys);
  EXPECT_TRUE(BoundContains(keys, rs.GetSearchBound(key), key)) << "key: " << key;
}

TYPED_TEST(RadixSplineTest, AllMaxKeys) {
  using KeyType = typename TestFixture::KeyType;
  const auto key = std::numeric_limits<KeyType>::max();
  const std::vector<KeyType> keys(kNumKeys, key);
  const auto rs = CreateRadixSpline(keys);
  EXPECT_TRUE(BoundContains(keys, rs.GetSearchBound(key), key)) << "key: " << key;
}

}  // namespace