#include <ppl.h>
#include <array>
#include <sstream>
#include <iostream>

using namespace std;

int wmain()
{
	concurrency::parallel_for(1, 23, [](int value)
	{
		wstringstream ss;
		ss << value << L' ';
		wcout << ss.str();
	});

	cout << endl;

	array<int, 20> values;
	for (auto&x : values)
	{
		x = rand() % 21;
	}

	concurrency::parallel_for_each(begin(values), end(values), [](int value)
	{
		wstringstream ss;
		ss << value << L' ';
		wcout << ss.str();
	});

	return 0;
}