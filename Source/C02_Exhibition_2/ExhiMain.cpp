#include "Exhi.cpp"

int main()
{
	float arr[4][4] = {
		3, 7, 1, 9,
		5, 1, 9, 9,
		0, 3, 2, 8,
		6, 4, 2, 7,
	};

	Matrix<float>::Determinant(arr);
	float** pl = Matrix<float>::Inverse(arr);

	for (int i = 0; i < 4; ++i)
	{
		delete[] pl[i];
	}
	delete[] pl;
	return 0;
}