#pragma once
#include <iostream>

template <typename T1 = int>
using fDMatrix = T1(*)[4];

template <class K = int>
class Matrix
{
public:
	static K			Determinant(fDMatrix<K> m);
	static K**			Inverse(fDMatrix<K> m);
private:
	static K			computeDeterminant(K ** ptr, const int& value);
};