#include <iostream>
#include <compare>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <chrono>
#include <thread>

using namespace std;

// Allow changing integer types in case we ever get to use large powers of two.
using my_int_t = int64_t;

// Pair of numbers summing to a power of two.
// Can be compared and thus used in sets, etc.
struct power_pair_t
{
	my_int_t a, b;

	power_pair_t(my_int_t i, my_int_t j)
	{
		a = std::min({ i, j });
		b = std::max({ i, j });
	}

	my_int_t sum() const { return a + b; }

	auto operator<=>(const power_pair_t&) const = default;
};

// Triplets of numbers that all mutually pair-wise sum to powers of two.
// Can be compared and thus used in sets, etc.
// Can be checked for overlap with another triplet.
struct power_triplet_t
{
	my_int_t a, b, c;

	power_triplet_t(my_int_t i, my_int_t j, my_int_t k)
	{
		a = std::min({ i, j, k });
		c = std::max({ i, j, k });
		b = i + j + k - a - c;
	}

	bool overlaps(const power_triplet_t& other) const
	{
		if (*this == other)
			return false;

		return
			(a == other.a) ||
			(a == other.b) ||
			(a == other.c) ||
			(b == other.a) ||
			(b == other.b) ||
			(b == other.c) ||
			(c == other.a) ||
			(c == other.b) ||
			(c == other.c);
	}

	my_int_t count_overlaps(const power_triplet_t& other) const
	{
		my_int_t count =
			my_int_t(a == other.a) +
			my_int_t(a == other.b) +
			my_int_t(a == other.c) +
			my_int_t(b == other.a) +
			my_int_t(b == other.b) +
			my_int_t(b == other.c) +
			my_int_t(c == other.a) +
			my_int_t(c == other.b) +
			my_int_t(c == other.c);
		
		return count == 3 ? 0 : count;
	}

	auto operator<=>(const power_triplet_t&) const = default;
};

// Set of powers of two, for quick check that a sum is such a power.
set<my_int_t> gen_powers_of_two()
{
	set<my_int_t> p2;
	for (my_int_t pow = 0; pow < 20; ++pow)
		p2.insert(my_int_t(1) << pow);
	return p2;
}

set<my_int_t> powers_of_two = gen_powers_of_two();

// A set of N numbers (N equal to desired_size) that have many
// pair-wise sums equal to powers of two.
//
// Can be progressively filled with triplets until the desired
// size is reached.
//
// Can generates the full list of pair-wise sums of powers of two
// that are produced by the set of numbers.
struct number_set_t
{
	size_t desired_size;
	set<my_int_t> numbers;

	number_set_t(size_t size) : desired_size(size) {}

	bool is_filled() const { return desired_size == numbers.size(); }

	void add(const power_triplet_t& tri)
	{
		if (!is_filled())
			numbers.insert(tri.a);
		if (!is_filled())
			numbers.insert(tri.b);
		if (!is_filled())
			numbers.insert(tri.c);
	}

	set<power_pair_t> pairs() const
	{
		set<power_pair_t> pairs;
		for (const my_int_t n1 : numbers)
		{
			for (const my_int_t n2 : numbers)
			{
				if (n1 == n2)
					continue;

				if (!powers_of_two.contains(n1 + n2))
					continue;

				pairs.emplace(n1, n2);
			}
		}
		return pairs;
	}
};

struct duration_t
{
	const chrono::steady_clock::time_point start_time;

	duration_t() : start_time(chrono::steady_clock::now()) {}

	chrono::seconds elapsed() const
	{
		const auto end_time = chrono::steady_clock::now();
		return chrono::duration_cast<chrono::seconds>(end_time - start_time);
	}
};

// Generate a subset all combinations of triplets (i.e N choose K)
// and keep the best resulting combination.
// Hold its own state so that multiple can run in parallel in multiple

struct combiner_t
{
	const vector<power_triplet_t>& triplets;
	const size_t number_set_size;
	const size_t first_indice;
	number_set_t best_number_set;
	set<power_pair_t> best_number_set_pairs;

	combiner_t(const vector<power_triplet_t>& tris, size_t set_size, size_t first)
		: triplets(tris)
		, number_set_size(set_size)
		, first_indice(first)
		, best_number_set(set_size)
	{}

	void run()
	{
		// These are the indices of the triplets to combine.
		vector<size_t> indices;
		for (size_t i = 0; i < number_set_size; ++i)
			indices.push_back(i + first_indice);

		bool more_combinations = true;
		while (more_combinations)
		{
			number_set_t number_set(number_set_size);
			for (size_t i : indices)
				number_set.add(triplets[i]);

			const auto number_set_pairs = number_set.pairs();
			if (number_set_pairs.size() > best_number_set_pairs.size())
			{
				best_number_set = number_set;
				best_number_set_pairs = number_set_pairs;
			}

			// Generate the next set of indices of triplets. This is N choose K in maths.
			// This is equal to N! / (K! x (N-K)!). Here N is the number of triplets we found
			// and K is the desired size of the set of numbers.
			more_combinations = false;
			for (size_t which_indice = indices.size() - 1; which_indice > 0; which_indice--)
			{
				if (indices[which_indice] + 1 < triplets.size() - (indices.size() - which_indice - 1))
				{
					indices[which_indice] += 1;
					for (size_t reset_indice = which_indice + 1; reset_indice < indices.size(); reset_indice++)
					{
						indices[reset_indice] = indices[reset_indice - 1] + 1;
					}
					more_combinations = true;
					break;
				}
			}
		}
	}
};

// Run the combiners in multiple threads and return the best result.
number_set_t run_combiners_in_threads(vector<combiner_t>& combiners)
{
	if (combiners.size() <= 0)
		return number_set_t(0);

	atomic<size_t> next_to_do = 0;
	vector<thread*> threads;

	for (size_t i = 0; i < thread::hardware_concurrency(); ++i)
	{
		threads.push_back(new thread([&combiners, &next_to_do]()
			{
				while (true)
				{
					const size_t which = next_to_do.fetch_add(1);
					if (which >= combiners.size())
						break;
					combiner_t& combiner = combiners[which];
					combiner.run();
				}
			}));
	}

	for (size_t i = 0; i < thread::hardware_concurrency(); ++i)
	{
		threads[i]->join();
	}

	number_set_t best_number_set(combiners[0].number_set_size);
	set<power_pair_t> best_number_set_pairs;
	for (const combiner_t& combiner : combiners)
	{
		if (combiner.best_number_set_pairs.size() > best_number_set_pairs.size())
		{
			best_number_set = combiner.best_number_set;
			best_number_set_pairs = combiner.best_number_set_pairs;
		}
	}

	return best_number_set;
}

// Generate triplets of numbers all pair-wise summing to powers of two.
vector<power_triplet_t> generate_power_triplets(const my_int_t delta_bound)
{
	duration_t duration;

	set<power_triplet_t> triplet_set;

	for (my_int_t p2 : powers_of_two)
	{
		for (my_int_t delta = -delta_bound; delta <= delta_bound; ++delta)
		{
			if (delta == 0)
				continue;

			const my_int_t i = delta;
			const my_int_t j = p2 - i;
			if (i == j)
				continue;

			for (my_int_t k = -delta_bound; k < delta_bound; ++k)
			{
				if (k == 0 || k == i || k == j)
					continue;

				if (powers_of_two.count(i + k) && powers_of_two.count(j + k))
				{
					triplet_set.emplace(i, j, k);
				}
			}
		}
	}

	vector<power_triplet_t> triplets;
	for (const auto& tri : triplet_set)
		triplets.push_back(tri);

	std::cout << triplets.size() << " triplets in " << duration.elapsed() << "." << endl;

	//for (const auto& tri : triplets)
	//	std::cout << tri.a << " " << tri.b << " " << tri.c << endl;

	return triplets;
}


// Actual algorithm to find good number sets.
int main()
{
	// Generate triplets of numbers all pair-wise summing to powers of two.
	vector<power_triplet_t> triplets = generate_power_triplets(100);

	// Generate all combinations of 10 triplets and keep the
	// combination that has the most pair-wise sums of powers
	// of two.
	for (size_t number_set_size = 5; number_set_size <= 10; ++number_set_size)
	{
		duration_t duration;

		vector<combiner_t> combiners;
		const size_t first_indice_max = triplets.size() - number_set_size;
		for (size_t first_indice = 0; first_indice <= first_indice_max; ++first_indice)
		{
			combiners.push_back(combiner_t(triplets, number_set_size, first_indice));
		}

		const number_set_t best_number_set = run_combiners_in_threads(combiners);
		const set<power_pair_t> best_number_set_pairs = best_number_set.pairs();

		std::cout << number_set_size << " numbers in " << duration.elapsed() << ":";
		for (const my_int_t number : best_number_set.numbers)
			std::cout << " " << number;
		std::cout << endl;

		std::cout << best_number_set_pairs.size() << " powers pairs:";
		for (const auto& pair : best_number_set_pairs)
			std::cout << " " << pair.a << "+" << pair.b << "=" << pair.sum();
		std::cout << endl;
	}


	return 0;
}
