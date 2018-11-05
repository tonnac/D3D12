#pragma once
#include <iostream>

template <class X>
using fdMatrix = X(*)[4];


class Matrix
{
public:
	template <typename X = int>
	static void CofactorMatrix(const fdMatrix<X>& matrix, fdMatrix<X> desc);
	template <typename X = int>
	static void InverseMatrix(const fdMatrix<X>& matrix, fdMatrix<float> desc);
	template <typename X = int>
	static X ComputeDeterminant(const fdMatrix<X>& matrix, int order = 4);
};