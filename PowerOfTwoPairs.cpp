#include <algorithm>
#include <chrono>
#include <compare>
#include <iostream>
#include <map>
#include <set>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

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

// Measure elapsed time in seconds.
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


// Set of powers of two, for quick check that a sum is such a power.
unordered_set<my_int_t> gen_powers_of_two()
{
	unordered_set<my_int_t> p2;
	for (my_int_t pow = 0; pow < 10; ++pow)
		p2.insert(my_int_t(1) << pow);
	return p2;
}

const unordered_set<my_int_t> powers_of_two = gen_powers_of_two();

bool is_power_of_two(my_int_t number) { return number != 0 && (number & (number - 1)) == 0; }

// Generate triplets of numbers all pair-wise summing to powers of two.
vector<power_triplet_t> generate_power_triplets(const size_t triplet_count)
{
	duration_t duration;

	set<power_triplet_t> triplet_set;

	my_int_t delta = 0;
	while (triplet_set.size() < triplet_count)
	{
		delta += 1;
		for (my_int_t p2 : powers_of_two)
		{
			my_int_t deltas[] = { delta, -delta };
			for (my_int_t delta : deltas)
			{
				const my_int_t i = delta;
				const my_int_t j = p2 - i;
				if (i == j)
					continue;

				for (my_int_t k = -delta; k <= delta; ++k)
				{
					if (k == 0 || k == i || k == j)
						continue;

					if (is_power_of_two(i + k) && is_power_of_two(j + k))
					{
						triplet_set.emplace(i, j, k);
					}
				}
			}
		}
	}

	vector<power_triplet_t> triplets;
	for (const auto& tri : triplet_set)
		triplets.push_back(tri);

	rotate(triplets.begin(), triplets.begin() + triplets.size() * 3 / 5, triplets.end());

	std::cout << triplets.size() << " triplets in " << duration.elapsed() << "." << endl;

	//for (const auto& tri : triplets)
	//	std::cout << tri.a << " " << tri.b << " " << tri.c << endl;

	return triplets;
}

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
	size_t improvement_count = 0;
	unordered_set<my_int_t> numbers;

	number_set_t(size_t size) : desired_size(size) {}

	void reset() { improvement_count = 0; numbers.clear(); }

	bool is_filled() const { return desired_size == numbers.size(); }

	void add(const my_int_t number)
	{
		if (!is_filled())
			numbers.insert(number);
	}
	void add(const power_triplet_t& tri)
	{
		add(tri.a);
		add(tri.b);
		add(tri.c);
	}

	void simplify()
	{
		while (std::all_of(numbers.begin(), numbers.end(), [](my_int_t number) { return (number % 2) == 0; }))
		{
			unordered_set<my_int_t> new_numbers;
			for (const my_int_t number : numbers)
				new_numbers.insert(number / my_int_t(2));
			new_numbers.swap(numbers);
		}
	}

	size_t count_pairs() const
	{
		size_t count = 0;
		const auto numbers_end = numbers.end();
		const auto numbers_end_prev = prev(numbers_end);
		for (auto i1 = numbers.begin(); i1 != numbers_end_prev; ++i1)
		{
			for (auto i2 = next(i1); i2 != numbers_end; ++i2)
			{
				const my_int_t n1 = *i1;
				const my_int_t n2 = *i2;
				if (!is_power_of_two(n1 + n2))
					continue;

				count += 1;
			}
		}
		return count;
	}

	vector<power_pair_t> generate_pairs() const
	{
		vector<power_pair_t> pairs;
		pairs.reserve(desired_size * 3);
		const auto numbers_end = numbers.end();
		const auto numbers_end_prev = prev(numbers_end);
		for (auto i1 = numbers.begin(); i1 != numbers_end_prev; ++ i1)
		{
			for (auto i2 = next(i1); i2 != numbers_end; ++i2)
			{
				const my_int_t n1 = *i1;
				const my_int_t n2 = *i2;
				if (!is_power_of_two(n1 + n2))
					continue;

				pairs.emplace_back(n1, n2);
			}
		}
		return pairs;
	}
};

// Improve a number set, generating other number sets.
// Keep only the best number set.
struct improver_t
{
	number_set_t best_number_set;
	size_t best_pair_count = 0;
	size_t improvement_count = 0;

	improver_t(const size_t set_size) : best_number_set(set_size) {}

	void improve(const number_set_t& number_set)
	{
		number_sets_to_improve.push_back(number_set);

		while (number_sets_to_improve.size() > 0)
		{
			number_set_t number_set = number_sets_to_improve.back();
			number_sets_to_improve.pop_back();
			update_best_number_set(number_set);
			improve_number_set(number_set);
		}
	}

private:
	vector<my_int_t> better_numbers;
	vector<my_int_t> worst_numbers;
	vector<number_set_t> number_sets_to_improve;
	map<my_int_t, size_t> pair_count_per_numbers;

	void update_best_number_set(const number_set_t& number_set)
	{
		const auto pair_count = number_set.count_pairs();
		if (pair_count > best_pair_count)
		{
			best_number_set = number_set;
			best_pair_count = pair_count;
		}
	}

	void new_improve_number_set(const number_set_t& number_set)
	{
		// Find best numbers to add to the set.
		pair_count_per_numbers.clear();
		for (const my_int_t power : powers_of_two)
		{
			for (const my_int_t number : number_set.numbers)
			{
				const my_int_t maybe_number = power - number;
				pair_count_per_numbers[maybe_number] += 1;
			}
		}

		size_t better_pair_count = 0;
		for (const auto& [number, count] : pair_count_per_numbers)
		{
			if (number_set.numbers.contains(number))
				continue;

			if (count > better_pair_count)
			{
				better_numbers.resize(0);
				better_numbers.push_back(number);
				better_pair_count = count;
			}
			else if (count == better_pair_count)
			{
				better_numbers.push_back(number);
			}
		}

		// Find worst current numbers to replace.
		pair_count_per_numbers.clear();
		for (const power_pair_t& pair : number_set.generate_pairs())
		{
			pair_count_per_numbers[pair.a] += 1;
			pair_count_per_numbers[pair.b] += 1;
		}

		size_t worst_pair_count = 1000000;
		for (const auto& [number, count] : pair_count_per_numbers)
		{
			if (count < worst_pair_count)
			{
				worst_numbers.resize(0);
				worst_numbers.push_back(number);
				worst_pair_count = count;
			}
			else if (count == worst_pair_count)
			{
				worst_numbers.push_back(number);
			}
		}

		// Verify if the best is better than the worst.
		if (better_pair_count <= worst_pair_count)
			return;

		const size_t pair_count = number_set.count_pairs();
		for (const my_int_t better_number : better_numbers)
		{
			for (const my_int_t worst_number : worst_numbers)
			{
				number_set_t improved(number_set);
				improved.numbers.erase(worst_number);
				improved.numbers.insert(better_number);
				if (improved.count_pairs() > pair_count)
				{
					improved.improvement_count += 1;
					improvement_count += 1;
					number_sets_to_improve.emplace_back(move(improved));
				}
			}
		}
	}

	void improve_number_set(const number_set_t& number_set)
	{
		pair_count_per_numbers.clear();

		for (const power_pair_t& pair : number_set.generate_pairs())
		{
			pair_count_per_numbers[pair.a] += 1;
			pair_count_per_numbers[pair.b] += 1;
		}

		size_t worst_pair_count = 1000000;
		for (const auto& [number, count] : pair_count_per_numbers)
		{
			if (count < worst_pair_count)
			{
				worst_numbers.resize(0);
				worst_numbers.push_back(number);
				worst_pair_count = count;
			}
			else if (count == worst_pair_count)
			{
				worst_numbers.push_back(number);
			}
		}

		for (const my_int_t power : powers_of_two)
		{
			for (const my_int_t number : number_set.numbers)
			{
				const my_int_t maybe_number = power - number;
				if (number_set.numbers.contains(maybe_number))
					continue;

				for (const my_int_t worst_number : worst_numbers)
				{
					size_t maybe_pair_count = 0;
					for (const my_int_t number : number_set.numbers)
						if (number != worst_number && is_power_of_two(number + maybe_number))
							maybe_pair_count += 1;

					if (maybe_pair_count > worst_pair_count)
					{
						number_set_t improved(number_set);
						improved.numbers.erase(worst_number);
						improved.numbers.insert(maybe_number);
						improved.improvement_count += 1;
						improvement_count += 1;
						number_sets_to_improve.emplace_back(move(improved));
						return;
					}
				}
			}
		}
	}
};

// Generate a subset all combinations of triplets (i.e N choose K)
// and keep the best resulting combination.
// Hold its own state so that multiple can run in parallel in multiple

struct combiner_t
{
	const vector<power_triplet_t>& triplets;
	const size_t number_set_size;
	vector<size_t> preset_indices;
	improver_t improver;
	size_t combination_count = 0;

	combiner_t(const vector<power_triplet_t>& tris, size_t set_size, vector<size_t> preset)
		: triplets(tris)
		, number_set_size(set_size)
		, preset_indices(preset)
		, improver(set_size)
	{}

	void combine()
	{
		if (number_set_size <= 0)
			return;

		// These are the indices of the triplets to combine.
		vector<size_t> indices;
		if (preset_indices.size() > 0)
			for (size_t preset : preset_indices)
				indices.push_back(preset);
		else
			indices.push_back(0);
		for (size_t i = indices.size(); i < number_set_size; ++i)
			indices.push_back(indices[i-1]+1);

		bool more_combinations = true;
		number_set_t number_set(number_set_size);
		while (more_combinations)
		{
			combination_count++;
			number_set.reset();
			for (size_t i : indices)
				number_set.add(triplets[i]);

			improver.improve(number_set);

			// Generate the next set of indices of triplets. This is N choose K in maths.
			// This is equal to N! / (K! x (N-K)!). Here N is the number of triplets we found
			// and K is the desired size of the set of numbers.
			more_combinations = false;
			for (size_t which_indice = indices.size() - 1; which_indice != preset_indices.size() - 1; which_indice--)
			{
				if (indices[which_indice] + 1 < triplets.size() - (number_set_size - which_indice - 1))
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

vector<combiner_t> generate_combiners(const vector<power_triplet_t>& triplets, const size_t number_set_size, size_t levels)
{
	vector<combiner_t> combiners;

	levels = std::min(levels, number_set_size);

	if (levels <= 0)
	{
		combiners.push_back(combiner_t(triplets, number_set_size, {}));
		return combiners;
	}

	vector<size_t> preset_indices;
	for (size_t i = 0; i < levels; ++i)
		preset_indices.push_back(i);

	bool more_combinations = true;
	while (more_combinations)
	{
		combiners.push_back(combiner_t(triplets, number_set_size, preset_indices));

		more_combinations = false;
		for (size_t which_indice = preset_indices.size() - 1; which_indice != size_t(-1); which_indice--)
		{
			if (preset_indices[which_indice] + 1 < triplets.size() - (number_set_size - which_indice - 1))
			{
				preset_indices[which_indice] += 1;
				for (size_t reset_indice = which_indice + 1; reset_indice < preset_indices.size(); reset_indice++)
				{
					preset_indices[reset_indice] = preset_indices[reset_indice - 1] + 1;
				}
				more_combinations = true;
				break;
			}
		}
	}

	return combiners;
}

// Run the combiners in multiple threads and return the best result.
number_set_t run_combiners_in_threads(vector<combiner_t>& combiners)
{
	if (combiners.size() <= 0)
		return number_set_t(0);

	atomic<size_t> next_to_do = 0;
	vector<thread*> threads;

	// Search number sets.
	for (size_t i = 0; i < thread::hardware_concurrency() - 1; ++i)
	//for (size_t i = 0; i < 1; ++i)
	{
		threads.push_back(new thread([&combiners, &next_to_do]()
			{
				while (true)
				{
					const size_t which = next_to_do.fetch_add(1);
					if (which >= combiners.size())
						break;
					combiner_t& combiner = combiners[which];
					combiner.combine();
				}
			}));
	}

	// Show progression of search.
	threads.push_back(new thread([&combiners, &next_to_do]()
		{
			size_t current_percent = 0;
			const size_t thread_count = thread::hardware_concurrency();
			size_t best_pair_count = 0;
			size_t max_improvement_count = 0;
			duration_t duration;

			auto print_progress = [&combiners, &next_to_do, &thread_count, &duration, &best_pair_count, &max_improvement_count](size_t percent)
			{
				size_t current_combiner_index = next_to_do.load();
				for (size_t i = current_combiner_index - thread_count; i < current_combiner_index; ++i)
				{
					const size_t pair_count = combiners[i].improver.best_pair_count;
					const size_t improvement_count = combiners[i].improver.improvement_count;
					best_pair_count = std::max(best_pair_count, pair_count);
					max_improvement_count = std::max(max_improvement_count, improvement_count);
				}
				std::cout << setw(3) << percent << "% " << setw(5) << duration.elapsed() << " " << best_pair_count << " pairs " << max_improvement_count << " improvements\r";
			};

			while (true)
			{
				size_t skip_count = 0;
				this_thread::sleep_for(chrono::milliseconds(100));
				const size_t which = next_to_do.load();
				if (which >= combiners.size())
				{
					current_percent = 100;
					break;
				}
				const size_t percent = 100 * which / combiners.size();
				if (percent == current_percent)
				{
					skip_count += 1;
					if (skip_count < 20)
						continue;
					skip_count = 0;
				}
				current_percent = percent;
				print_progress(percent);
				std::cout.flush();
			}
			print_progress(100);
			cout << endl;
		}));

	// For for it all to end.
	for (size_t i = 0; i < threads.size(); ++i)
	{
		threads[i]->join();
	}

	number_set_t best_number_set(combiners[0].number_set_size);
	size_t best_pair_count = 0;
	for (const combiner_t& combiner : combiners)
	{
		if (combiner.improver.best_pair_count > best_pair_count)
		{
			best_number_set = combiner.improver.best_number_set;
			best_pair_count = combiner.improver.best_pair_count;
		}
	}

	best_number_set.simplify();
	return best_number_set;
}
number_set_t simple_algo(size_t number_set_size)
{
	number_set_t best_number_set(number_set_size);
	for (my_int_t min_delta_for_negative = 0; min_delta_for_negative < 20; min_delta_for_negative += 2)
	{
		number_set_t number_set(number_set_size);
		for (my_int_t delta = 1; !number_set.is_filled(); delta += 2)
		{
			number_set.add(delta);
			if (delta > min_delta_for_negative)
				number_set.add(-delta + 2);
		}
		if (number_set.count_pairs() > best_number_set.count_pairs())
			best_number_set = number_set;
	}
	return best_number_set;
}

void print_result(const duration_t& duration, const number_set_t& number_set)
{
	std::cout << number_set.desired_size << " numbers in " << duration.elapsed() << ":";
	for (const my_int_t number : set<my_int_t>(number_set.numbers.begin(), number_set.numbers.end()))
		std::cout << " " << number;
	std::cout << endl;

	const vector<power_pair_t> pairs = number_set.generate_pairs();
	std::cout << pairs.size() << " powers pairs:";
	for (const auto& pair : pairs)
		std::cout << " " << pair.a << "+" << pair.b << "=" << pair.sum();
	std::cout << endl;
}

// Actual algorithm to find good number sets.
int main(int argc, char** argv)
{
	if (argc < 2)
	{
		cerr << "Missing arguments." << endl;
		cerr << "Searching  Algo Usage: " << (argc >= 1 ? argv[0] : "PowerOfTwoPairs") << " delta combiner-level min-set-size max-set-size" << endl;
		cerr << "Simplified Algo Usage: " << (argc >= 1 ? argv[0] : "PowerOfTwoPairs") << " set-size" << endl;
		return 1;
	}

	const bool use_simplified_algo = argc <= 3;
	const bool simple_range = (argc == 4 || argc == 2);
	const size_t triplet_count = use_simplified_algo ? 0 : std::atoi(argv[1]);
	const size_t combiner_levels = use_simplified_algo ? 0 : std::atoi(argv[2]);
	const size_t min_set_size = use_simplified_algo ? std::atoi(argv[1]) : std::atoi(argv[3]);
	const size_t max_set_size = simple_range ? min_set_size : use_simplified_algo ? std::atoi(argv[2]) : std::atoi(argv[4]);

	for (size_t number_set_size = min_set_size; number_set_size <= max_set_size; ++number_set_size)
	{
		duration_t duration;

		if (use_simplified_algo)
		{
			number_set_t number_set = simple_algo(number_set_size);
			improver_t improver(number_set_size);
			improver.improve(number_set);
			print_result(duration, improver.best_number_set);
		}
		else
		{
			// Generate triplets of numbers all pair-wise summing to powers of two.
			vector<power_triplet_t> triplets = generate_power_triplets(triplet_count);

			// Generate all combinations of 10 triplets and keep the
			// combination that has the most pair-wise sums of powers
			// of two.

			vector<combiner_t> combiners = generate_combiners(triplets, number_set_size, combiner_levels);
			std::cout << "Using " << combiners.size() << " combiners." << endl;

			const number_set_t number_set = run_combiners_in_threads(combiners);

			size_t total_combination_count = 0;
			for (const auto& combiner : combiners)
				total_combination_count += combiner.combination_count;

			std::cout << "Tried " << total_combination_count << " combinations with " << number_set.improvement_count << " improvements." << endl;

			print_result(duration, number_set);
		}
	}


	return 0;
}
