RadixSpline: A Single-Pass Learned Index
====

![](https://github.com/learnedsystems/radixspline/workflows/CI/badge.svg)

A read-only learned index structure that can be built in a single pass over sorted data. Can be used as a drop-in replacement for ``std::multimap``. Currently limited to `uint32_t` and `uint64_t` data types.

## Build

```
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
./example
./tester
```

## Examples

Using ``rs::Builder`` to index sorted data in one pass, without copying the data:

```c++
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
```

Using ``rs::MultiMap`` to index unsorted data, which internally creates a sorted copy:

```c++
vector<pair<uint64_t, char>> data = {{1ull, 'a'},
                                     {12ull, 'b'},
                                     {7ull, 'c'}, // Unsorted.
                                     {42ull, 'd'}};
rs::MultiMap<uint64_t, char> map(begin(data), end(data));

cout << "find(7): '" << map.find(7)->second << "'" << endl;
cout << "lower_bound(3): '" << map.lower_bound(3)->second << "'" << endl;
```

## Cite

Please cite our [aiDM@SIGMOD 2020 paper](https://dl.acm.org/doi/10.1145/3401071.3401659) if you use this code in your own work:

```
@inproceedings{radixspline,
  author    = {Andreas Kipf and
               Ryan Marcus and
               Alexander van Renen and
               Mihail Stoian and
               Alfons Kemper and
               Tim Kraska and
               Thomas Neumann},
  title     = {{RadixSpline}: a single-pass learned index},
  booktitle = {Proceedings of the Third International Workshop on Exploiting Artificial
               Intelligence Techniques for Data Management, aiDM@SIGMOD 2020, Portland,
               Oregon, USA, June 19, 2020},
  pages     = {5:1--5:5},
  year      = {2020},
  url       = {https://doi.org/10.1145/3401071.3401659},
  doi       = {10.1145/3401071.3401659},
  timestamp = {Mon, 08 Jun 2020 19:13:59 +0200},
  biburl    = {https://dblp.org/rec/conf/sigmod/KipfMRSKK020.bib},
  bibsource = {dblp computer science bibliography, https://dblp.org}
}
```
